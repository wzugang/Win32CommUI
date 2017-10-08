#include "XComponent.h"
#include "XmlParser.h"
#include "UIFactory.h"
#include <stdlib.h>
#include <CommCtrl.h>
#include <vector>
#include <map>
#include <string>

#pragma comment(linker,"\"/manifestdependency:type='win32'  name='Microsoft.Windows.Common-Controls' version='6.0.0.0'   processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

HINSTANCE XComponent::mInstance;

void MyRegisterClass(HINSTANCE ins, const char *className) {
	static std::map<std::string, bool> sCache;
	if (sCache.find(className) != sCache.end())
		return;
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= XComponent::__WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= ins;
	//wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HGTUI));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= className;
	wcex.hIconSm		= NULL;
	if (RegisterClassEx(&wcex) != 0) {
		sCache[className] = true;
	}
}

LRESULT CALLBACK XComponent::__WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	XComponent* cc = NULL;
	if (msg == WM_NCCREATE) {
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		cc = static_cast<XComponent*>(lpcs->lpCreateParams);
		cc->mWnd = wnd;
		SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(cc));
	} else {
		cc = (XComponent *)GetWindowLong(wnd, GWL_USERDATA);
	}
	if (cc != NULL) {
		LRESULT ret = 0;
		if (cc->getListener() != NULL && cc->getListener()->onEvent(cc, msg, wParam, lParam, &ret))
			return ret;
		if (cc->wndProc(msg, wParam, lParam, &ret))
			return ret;
		// bubble mouse wheel msg
		if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
			POINT pt;
			GetCursorPos(&pt);
			HWND ptWnd = WindowFromPoint(pt);
			XComponent *pc = (XComponent *)GetWindowLong(ptWnd, GWL_USERDATA);
			XmlNode *node = pc != NULL ? pc->getNode() : NULL;
			while (node != NULL) {
				XComponent *x = node->getComponent();
				if (x == NULL) break;
				if (x->wndProc(WM_MOUSEWHEEL_BUBBLE, wParam, lParam, &ret))
					return ret;
				node = node->getParent();
			}
		}
	}
	_end:
	return DefWindowProc(wnd, msg, wParam, lParam);
}

XComponent::XComponent(XmlNode *node) {
	mID = 0;
	mNode = node;
	mWnd = NULL;
	mParentWnd = NULL;
	mX = mY = mWidth = mHeight = mMesureWidth = mMesureHeight = 0;
	mAttrX = mAttrY = mAttrWidth = mAttrHeight = 0;
	memset(mAttrPadding, 0, sizeof(mAttrPadding));
	memset(mAttrMargin, 0, sizeof(mAttrMargin));
	mListener = NULL;
	mAttrFlags = 0;
	mAttrColor = mAttrBgColor = 0;
	mFont = NULL;
	mRectRgn = NULL;
	mBgColorBrush = NULL;
	mBgImage = NULL;
	mAttrRoundConerX = mAttrRoundConerY = 0;
	mAttrWeight = 0;
	strcpy(mClassName, "XComponent");
	parseAttrs();
}

XComponent::~XComponent() {
	if (mBgColorBrush != NULL)  DeleteObject(mBgColorBrush);
	if (mFont != NULL) DeleteObject(mFont);
	if (mBgImage != NULL) {
		mBgImage->decRef();
		mBgImage = NULL;
	}
}

static bool parseFont(LOGFONT *font, char *str) {
	if (str == NULL || *str == 0) return false;
	bool changed = false;
	char *p = strstr(str, "name:");
	char *p2 = NULL;
	if (p != NULL) {
		p2 = strchr(p, ';');
		if (p2 && p2 - p - 5 <= sizeof(font->lfFaceName - 1)) {
			memcpy(font->lfFaceName, p + 5, p2 - p - 5);
			font->lfFaceName[p2 - p - 5] = 0;
			changed = true;
		} else {
			if (strlen(p + 5) < sizeof(font->lfFaceName - 1)) {
				strcpy(font->lfFaceName, p + 5);
				changed = true;
			}
		}
	}

	p = strstr(str, "size:");
	if (p != NULL) {
		int s = atoi(p + 5);
		if (s != 0) {
			font->lfHeight = s;
			changed = true;
		}
	}
	p = strstr(str, "weight:");
	if (p != NULL) {
		int s = atoi(p + 7);
		if (s > 0) {
			font->lfWeight = s;
			changed = true;
		}
	}
	
	if (strstr(str, "italic") != NULL) {
		font->lfItalic = TRUE;
		changed = true;
	}
	if (strstr(str, "underline") != NULL) {
		font->lfUnderline = TRUE;
		changed = true;
	}
	if (strstr(str, "strikeout") != NULL) {
		font->lfStrikeOut = TRUE;
		changed = true;
	}
	return changed;
}

void XComponent::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	mID = generateWndId();
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_CHILDWINDOW | WS_VISIBLE,
		mX, mY, mWidth, mHeight,
		getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

void XComponent::createWndTree( HWND parent ) {
	mParentWnd = parent;
	createWnd();
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *cc = mNode->getChild(i)->getComponent();
		cc->createWndTree(NULL);
	}
}

void XComponent::onMeasure(int widthSpec, int heightSpec) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	mesureChildren((mMesureWidth - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST, 
		(mMesureHeight - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
}

void XComponent::onLayout( int width, int height ) {
	// layout children
}

void XComponent::layout( int x, int y, int width, int height ) {
	mX = x;
	mY = y;
	mWidth = width;
	mHeight = height;
	MoveWindow(mWnd, mX, mY, mWidth, mHeight, TRUE);
	if (mAttrRoundConerX != 0 && mAttrRoundConerY != 0) {
		if (mRectRgn != NULL) {
			DeleteObject(mRectRgn);
			mRectRgn = NULL;
		}
		mRectRgn = CreateRoundRectRgn(0, 0, mWidth, mHeight, mAttrRoundConerX, mAttrRoundConerY);
		SetWindowRgn(mWnd, mRectRgn, TRUE);
	}
	onLayout(width, height);
}

static int parseSize(const char *str) {
	char *ep = NULL;
	int v = (int)strtod(str, &ep);
	if (str == ep && strstr(str, "auto") != NULL) {
		return XComponent::MS_AUTO;
	}
	if (ep != NULL && *ep == '%') {
		if (v < 0) v = -v;
		v = v | XComponent::MS_PERCENT;
	} else {
		v = v | XComponent::MS_FIX;
	}
	return v;
}
static void parseArraySize(const char *str, int *arr) {
	for (int i = 0; i < 4; ++i) {
		while (*str == ' ') ++str;
		arr[i] = parseSize(str);
		while (*str != ' ' && *str) ++str;
	}
}
static int parseInt(const char *str) {
	return (int)strtod(str, NULL);
}
static void parseArrayInt(const char *str, int *arr) {
	for (int i = 0; i < 4; ++i) {
		while (*str == ' ') ++str;
		arr[i] = parseInt(str);
		while (*str != ' ' && *str) ++str;
	}
}

static COLORREF parseColor(const char *color, bool *valid) {
	static const char *HEX = "0123456789ABCDEF";
	if (valid) *valid = false;
	if (color == NULL) {
		if (valid) *valid = false;
		return 0;
	}
	while (*color == ' ') ++color;
	if (*color != '#') {
		if (valid) *valid = false;
		return 0;
	}
	++color;
	COLORREF cc = 0;
	for (int i = 0; i < 3; ++i) {
		char *p0 = strchr((char *)HEX, toupper(color[i * 2]));
		char *p1 = strchr((char *)HEX, toupper(color[i * 2 + 1]));
		if (p0 == NULL || p1 == NULL) {
			if (valid) *valid = false;
			return 0;
		}
		cc |= ((p0 - HEX) * 16 + (p1 - HEX)) << i * 8;
	}
	if (valid) *valid = true;
	return cc;
}

void XComponent::parseAttrs() {
	bool ok = false;
	for (int i = 0; i < mNode->getAttrsCount(); ++i) {
		XmlNode::Attr *attr = mNode->getAttr(i);
		if (strcmp(attr->mName, "rect") == 0) {
			int rect[4] = {0};
			parseArraySize(attr->mValue, rect);
			mAttrX = rect[0];
			mAttrY = rect[1];
			mAttrWidth = rect[2];
			mAttrHeight = rect[3];
		} else if (strcmp(attr->mName, "padding") == 0) {
			parseArrayInt(attr->mValue, mAttrPadding);
		} else if (strcmp(attr->mName, "margin") == 0) {
			parseArrayInt(attr->mValue, mAttrMargin);
		} else if (strcmp(attr->mName, "color") == 0) {
			COLORREF cc = parseColor(attr->mValue, &ok);
			if (ok) {
				mAttrColor = cc;
				mAttrFlags |= AF_COLOR;
			}
		} else if (strcmp(attr->mName, "bgcolor") == 0) {
			COLORREF cc = parseColor(attr->mValue, &ok);
			if (ok) {
				mAttrBgColor = cc;
				mAttrFlags |= AF_BG_COLOR;
			}
		} else if (strcmp(attr->mName, "bgimage") == 0) {
			mBgImage = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "roundConer") == 0) {
			char *p = NULL;
			int v1 = (int)strtod(attr->mValue, &p);
			int v2 = 0;
			if (p) {
				v2 = (int)strtod(p + 1, NULL);
				mAttrRoundConerX = v1;
				mAttrRoundConerY = v2;
			}
		} else if (strcmp(attr->mName, "weight") == 0) {
			int v1 = (int)strtod(attr->mValue, NULL);
			if (v1 > 0) mAttrWeight = v1;
		} 
	}
}

void XComponent::applyAttrs() {
	HFONT font = getFont();
	SendMessage(mWnd, WM_SETFONT, (WPARAM)font, 0);
	char *visible = mNode->getAttrValue("visible");
	if (visible != NULL && strcmp("false", visible) == 0) {
		DWORD st = GetWindowLong(mWnd, GWL_STYLE);
		st = st & ~WS_VISIBLE;
		SetWindowLong(mWnd,GWL_STYLE, st);
	}
}

int XComponent::getSpecSize( int sizeSpec ) {
	return sizeSpec & ~(MS_ATMOST | MS_AUTO | MS_FIX | MS_PERCENT);
}

int XComponent::calcSize( int selfSizeSpec, int parentSizeSpec ) {
	if (selfSizeSpec & MS_FIX) {
		return getSpecSize(selfSizeSpec);
	}
	if (selfSizeSpec & MS_PERCENT) {
		if ((parentSizeSpec & MS_FIX) || (parentSizeSpec & MS_ATMOST)) {
			return getSpecSize(selfSizeSpec) * getSpecSize(parentSizeSpec) / 100;
		}
	}
	/*if ((parentSizeSpec & MS_FIX) || (parentSizeSpec & MS_ATMOST)) {
		return getSpecSize(parentSizeSpec);
	}*/
	return 0;
}

HWND XComponent::getParentWnd() {
	if (mParentWnd != NULL)
		return mParentWnd;
	if (mNode->getParent() == NULL)
		return NULL;
	return mNode->getParent()->getComponent()->getWnd();
}

void XComponent::mesureChildren( int widthSpec, int heightSpec ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		child->onMeasure(widthSpec, heightSpec);
	}
}

HWND XComponent::getWnd() {
	return mWnd;
}

bool XComponent::wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res) {
	if (msg == WM_ERASEBKGND) {
		bool hasBgImg = (mBgImage != NULL && mBgImage->getHBitmap() != NULL);
		if ((mAttrFlags & AF_BG_COLOR) /*&& !hasBgImg*/) {
			HDC dc = (HDC)wParam;
			HBRUSH brush = CreateSolidBrush(mAttrBgColor);
			RECT rc = {0};
			GetClientRect(mWnd, &rc);
			FillRect(dc, &rc, brush);
			DeleteObject(brush);
		}
		if (hasBgImg) {
			HDC dc = (HDC)wParam;
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, mBgImage->getHBitmap());
			BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
			DeleteObject(memDc);
		}
		if ((mAttrFlags & AF_BG_COLOR) || hasBgImg)
			return true;
	}
	return false;
}

XComponent* XComponent::findById( const char *id ) {
	XmlNode *n = mNode->findById(id);
	if (n != NULL) {
		return n->getComponent();
	}
	return NULL;
}

XComponent* XComponent::getChildById( DWORD id ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		if (child->mID == id) return child;
	}
	return NULL;
}

XComponent* XComponent::getChildById( const char *id ) {
	XmlNode *child = mNode->getChildById(id);
	if (child != NULL)
		return child->getComponent();
	return NULL;
}
XComponent* XComponent::getChild(int idx) {
	return mNode->getChild(idx)->getComponent();
}

void XComponent::setListener( XListener *v ) {
	mListener = v;
}

XListener* XComponent::getListener() {
	return mListener;
}

DWORD XComponent::generateWndId() {
	static DWORD s_id = 1000;
	return ++s_id;
}

bool XComponent::onCtrlColor( HDC dc, LRESULT *result ) {
	if (mAttrFlags & AF_COLOR)
		SetTextColor(dc, mAttrColor);
	if (mAttrFlags & AF_BG_COLOR) {
		SetBkColor(dc, mAttrBgColor); // only文字背景色
		if (mBgColorBrush == NULL)
			mBgColorBrush = CreateSolidBrush(mAttrBgColor); // 控件背景
		*result = (LRESULT)mBgColorBrush;
	}
	SetBkMode(dc, TRANSPARENT);
	return (mAttrFlags & AF_COLOR)  || (mAttrFlags &  AF_BG_COLOR);
}

HFONT XComponent::getFont() {
	if (mFont != NULL)
		return mFont;
	if (mNode->getParent() == NULL || mNode->getAttrValue("font") != NULL) {
		LOGFONT ff = {0};
		HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		GetObject(font, sizeof(LOGFONT), &ff);
		if (parseFont(&ff, mNode->getAttrValue("font"))) {
			font = mFont = CreateFontIndirect(&ff);
		}
		return font;
	}
	return mNode->getParent()->getComponent()->getFont();
}

void XComponent::init() {
	INITCOMMONCONTROLSEX cc = {0};
	cc.dwSize = sizeof(cc);
	InitCommonControlsEx(&cc);
	mInstance = GetModuleHandle(NULL);
}

HINSTANCE XComponent::getInstance() {
	return mInstance;
}

int XComponent::getMesureWidth() {
	return mMesureWidth;
}

int XComponent::getMesureHeight() {
	return mMesureHeight;
}

int XComponent::getAttrX() {
	return mAttrX;
}

int XComponent::getAttrY() {
	return mAttrY;
}

int XComponent::getAttrWeight() {
	return mAttrWeight;
}

int * XComponent::getAttrMargin() {
	return mAttrMargin;
}

//--------------------------XContainer-----------------------------
XContainer::XContainer( XmlNode *node ) : XComponent(node) {
}

bool XContainer::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	if (XComponent::wndProc(msg, wParam, lParam, result)) {
		return true;
	}
	if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN) {
		DWORD id = GetWindowLong((HWND)lParam, GWL_ID);
		XComponent *c = getChildById(id);
		if (c != NULL) {
			return c->onCtrlColor((HDC)wParam, result);
		}
	} else if (msg == WM_COMMAND && lParam != 0) {
		DWORD id = GetWindowLong((HWND)lParam, GWL_ID);
		XComponent *c = getChildById(id);
		if (c != NULL) {
			*result = SendMessage((HWND)lParam, WM_COMMAND_SELF, 0, 0);
			return true;
		}
	} else if (msg == WM_NOTIFY && lParam != 0) {
		NMHDR *nmh = (NMHDR*)lParam;
		XComponent *c = getChildById(nmh->idFrom);
		if (c != NULL) {
			*result = SendMessage(nmh->hwndFrom, WM_NOTIFY_SELF, wParam, lParam);
			return true;
		}
	}
	return false;
}

//--------------------------XAbsLayout-----------------------------
XAbsLayout::XAbsLayout( XmlNode *node ) : XContainer(node) {
}

void XAbsLayout::onLayout( int width, int height ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), width | MS_ATMOST);
		int y  = calcSize(child->getAttrY(), height | MS_ATMOST);
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
}

//--------------------------XHLineLayout--------------------------
XHLineLayout::XHLineLayout(XmlNode *node) : XContainer(node) {
}

void XHLineLayout::onLayout( int width, int height ) {
	int x = mAttrPadding[0], weightAll = 0, childWidths = 0, lessWidth = width - mAttrPadding[0] - mAttrPadding[2];
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		weightAll += child->getAttrWeight();
		childWidths += child->getMesureWidth() + child->getAttrMargin()[0] + child->getAttrMargin()[2];
	}
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int y  = calcSize(child->getAttrY(), (height - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
		x += child->getAttrMargin()[0];
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
		x += child->getMesureWidth() + child->getAttrMargin()[2];
		if (childWidths < lessWidth && weightAll > 0) {
			x += 1.0 * child->getAttrWeight() / weightAll * (lessWidth - childWidths);
		}
	}
}
//--------------------------XVLineLayout--------------------------
XVLineLayout::XVLineLayout(XmlNode *node) : XContainer(node) {
}

void XVLineLayout::onLayout( int width, int height ) {
	int y = mAttrPadding[1], weightAll = 0, childHeights = 0, lessHeight = height - mAttrPadding[1] - mAttrPadding[3];
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		weightAll += child->getAttrWeight();
		childHeights += child->getMesureHeight() + child->getAttrMargin()[1] + child->getAttrMargin()[3];
	}
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), (width - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST);
		y += child->getAttrMargin()[1];
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
		y += child->getMesureHeight() + child->getAttrMargin()[3];
		if (childHeights < lessHeight && weightAll > 0) {
			y += 1.0 * child->getAttrWeight() / weightAll * (lessHeight - childHeights);
		}
	}
}
//--------------------------XBasicWnd-----------------------------
XBasicWnd::XBasicWnd( XmlNode *node ) : XComponent(node) {
	mOldWndProc = NULL;
}

bool XBasicWnd::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res ) {
	*res = CallWindowProc(mOldWndProc, mWnd, msg, wParam, lParam);
	return true;
}

void XBasicWnd::createWnd() {
	mOldWndProc = (WNDPROC)SetWindowLongPtr(mWnd, GWL_WNDPROC, (LONG_PTR)__WndProc);
	applyAttrs();
}

//--------------------------XButton-----------------------------
XButton::XButton( XmlNode *node ) : XBasicWnd(node) {
}

void XButton::createWnd() {
	char *txt = mNode->getAttrValue("text");
	mID = generateWndId();
	mWnd = CreateWindow("BUTTON", txt, WS_VISIBLE | WS_CHILD, mX, mY, mWidth, mHeight, 
		getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	XBasicWnd::createWnd();
}

//--------------------------XLabel-----------------------------
XLabel::XLabel( XmlNode *node ) : XBasicWnd(node) {
}

void XLabel::createWnd() {
	char *txt = mNode->getAttrValue("text");
	mID = generateWndId();
	mWnd = CreateWindow("STATIC", txt, WS_VISIBLE | WS_CHILD, mX, mY, mWidth, mHeight, 
		getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	XBasicWnd::createWnd();
}

//--------------------------XCheckBox----------------------------
XCheckBox::XCheckBox( XmlNode *node ) : XButton(node) {
}
void XCheckBox::createWnd() {
	XButton::createWnd();
	SetWindowLong(mWnd, GWL_STYLE, GetWindowLong(mWnd, GWL_STYLE) | BS_AUTOCHECKBOX);
}
//--------------------------XRadio----------------------------
XRadio::XRadio( XmlNode *node ) : XButton(node) {
}
void XRadio::createWnd() {
	XButton::createWnd();
	DWORD style = GetWindowLong(mWnd, GWL_STYLE) | BS_AUTORADIOBUTTON;
	if (mNode->getAttrValue("group") != NULL) {
		style |= WS_GROUP;
	}
	SetWindowLong(mWnd, GWL_STYLE, style);
}
XGroupBox::XGroupBox( XmlNode *node ) : XButton(node) {
}
void XGroupBox::createWnd() {
	XButton::createWnd();
	DWORD style = GetWindowLong(mWnd, GWL_STYLE) | BS_GROUPBOX;
	SetWindowLong(mWnd, GWL_STYLE, style);
}

XEdit::XEdit( XmlNode *node ) : XBasicWnd(node) {
}

void XEdit::createWnd() {
	char *txt = mNode->getAttrValue("text");
	mID = generateWndId();
	DWORD style = WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL| ES_AUTOVSCROLL;
	if (mNode->getAttrValue("multiline") != NULL) {
		style |= ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL;
	}
	if (mNode->getAttrValue("readonly") != NULL) {
		style |= ES_READONLY;
	}
	mID = generateWndId();
	mWnd = CreateWindow("EDIT", txt, style, mX, mY, mWidth, mHeight, 
		getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	XBasicWnd::createWnd();
}

XComboBox::XComboBox( XmlNode *node ) : XBasicWnd(node) {
	mDropHeight = 0;
}
static std::vector<char*> splitBy( char *data, char splitChar) {
	std::vector<char*> arr;
	if (data == NULL || *data == 0)
		return arr;
	char *ps = data;
	char *end = data + strlen(data);
	while (ps <= end && *ps != 0) {
		char *p = strchr(ps, splitChar);
		if (p != NULL) {
			*p = 0; 
		}
		arr.push_back(ps);
		ps = p + 1;
		if (p == NULL) break;
	}
	return arr;
}

void XComboBox::createWnd() {
	mID = generateWndId();
	mWnd = CreateWindow("ComboBox", NULL,  WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL | CBS_NOINTEGRALHEIGHT, 
		mX, mY, mWidth, mHeight, getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);

	std::vector<char*> arr = splitBy(mNode->getAttrValue("data"), ';');
	for (int i = 0; i < arr.size(); ++i) {
		SendMessage(mWnd, CB_ADDSTRING, 0, (LPARAM)arr.at(i));
	}
	XBasicWnd::createWnd();
}

XTable::XTable( XmlNode *node ) : XBasicWnd(node) {
}

void XTable::createWnd() {
	mID = generateWndId();
	mWnd = CreateWindow("SysListView32", NULL,  WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SINGLESEL,
		mX, mY, mWidth, mHeight, getParentWnd(), (HMENU)mID, mInstance, this);
	ListView_SetExtendedListViewStyle(mWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES); // LVS_EX_SUBITEMIMAGES
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	XBasicWnd::createWnd();
}

XTree::XTree( XmlNode *node ) : XBasicWnd(node) {
}
void XTree::createWnd() {
	mID = generateWndId();
	mWnd = CreateWindow("SysTreeView32", NULL,  WS_VISIBLE | WS_CHILD | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT,
		mX, mY, mWidth, mHeight, getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	XBasicWnd::createWnd();
}

XTab::XTab(XmlNode *node) : XContainer(node) {
	mOldWndProc = NULL;
}
void XTab::createWnd() {
	mID = generateWndId();
	mWnd = CreateWindow("SysTabControl32", NULL,  WS_VISIBLE | WS_CHILD | TCS_FIXEDWIDTH,
		mX, mY, mWidth, mHeight, getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	std::vector<char*> titles = splitBy(mNode->getAttrValue("data"), ';');

	for (int i = 0; i < titles.size(); ++i) {
		TC_ITEM it = {0};
		it.mask = TCIF_TEXT;
		it.pszText = titles.at(i);
		TabCtrl_InsertItem(mWnd, i, &it);
	}
	mOldWndProc = (WNDPROC)SetWindowLongPtr(mWnd, GWL_WNDPROC, (LONG_PTR)__WndProc);
	applyAttrs();
}
bool XTab::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res ) {
	if (XContainer::wndProc(msg, wParam, lParam, res)) {
		return true;
	}
	if (msg == WM_NOTIFY_SELF) {
		NMHDR *hd = (NMHDR *)lParam;
		if (hd->idFrom == mID) {
			if (hd->code == TCN_SELCHANGING) { //Tab before change
				int sel = TabCtrl_GetCurSel(mWnd);
				ShowWindow(getChild(sel)->getWnd(), SW_HIDE);
				return true;
			} else if (hd->code == TCN_SELCHANGE) { //Tab after change
				int sel = TabCtrl_GetCurSel(mWnd);
				ShowWindow(getChild(sel)->getWnd(), SW_SHOW);
				return true;
			}
		}
		return true;
	}
	*res = CallWindowProc(mOldWndProc, mWnd, msg, wParam, lParam);
	return true;
}

void XTab::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	RECT rect = {0 , 0, mMesureWidth, mMesureHeight};
	TabCtrl_AdjustRect(mWnd, FALSE, &rect);
	mesureChildren((rect.right - rect.left) | MS_ATMOST, (rect.bottom - rect.top) | MS_ATMOST);
}

void XTab::onLayout(int width, int height) {
	RECT rect = {0 , 0, width, height};
	TabCtrl_AdjustRect(mWnd, FALSE, &rect);
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), (rect.right - rect.left) | MS_ATMOST) + rect.left;
		int y  = calcSize(child->getAttrY(), (rect.bottom - rect.top) | MS_ATMOST) + rect.top;
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
}

XListBox::XListBox( XmlNode *node ) : XBasicWnd(node) {
}
void XListBox::createWnd() {
	mID = generateWndId();
	mWnd = CreateWindow("ListBox", NULL,  WS_VISIBLE | WS_CHILD | LBS_HASSTRINGS | LBS_MULTIPLESEL | WS_VSCROLL,
		mX, mY, mWidth, mHeight, getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	std::vector<char*> items = splitBy(mNode->getAttrValue("data"), ';');

	for (int i = 0; i < items.size(); ++i) {
		SendMessage(mWnd, LB_ADDSTRING, 0, (LPARAM)items.at(i));
	}
	SendMessage(mWnd, LB_SETTOPINDEX, items.size(), 0); // auto scroll
	XBasicWnd::createWnd();
}
XDateTimePicker::XDateTimePicker( XmlNode *node ) : XBasicWnd(node) {
}
void XDateTimePicker::createWnd() {
	mID = generateWndId();
	mWnd = CreateWindow("SysDateTimePick32", NULL,  WS_VISIBLE | WS_CHILD,
		mX, mY, mWidth, mHeight, getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	DateTime_SetFormat(mWnd, "yyyy-MM-dd");
	XBasicWnd::createWnd();
}

XWindow::XWindow( XmlNode *node ) : XContainer(node) {
	strcpy(mClassName, "XWindow");
}

void XWindow::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId();  // has no id
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_OVERLAPPEDWINDOW,
		getSpecSize(mAttrX), getSpecSize(mAttrY), getSpecSize(mAttrWidth), getSpecSize(mAttrHeight),
		getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

bool XWindow::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (XContainer::wndProc(msg, wParam, lParam, result))
		return true;
	if (msg == WM_SIZE && lParam > 0) {
		RECT r = {0};
		GetWindowRect(mWnd, &r);
		onMeasure(LOWORD(lParam) | XComponent::MS_FIX, HIWORD(lParam) | XComponent::MS_FIX);
		onLayout(LOWORD(lParam), HIWORD(lParam));
		return true;
	} else if (msg == WM_DESTROY) {
		PostQuitMessage(0);
		return true;
	}
	return false;
}

int XWindow::messageLoop() {
	MSG msg;
	HACCEL hAccelTable = NULL;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (! TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}

void XWindow::show(int nCmdShow) {
	HWND desk = GetDesktopWindow();
	RECT rect = {0};
	GetClientRect(desk, &rect);
	if (getSpecSize(mAttrX) == 0 && getSpecSize(mAttrY) == 0) {
		int x = (rect.right - getSpecSize(mAttrWidth)) / 2;
		int y = (rect.bottom - getSpecSize(mAttrHeight)) / 2 - 30;
		SetWindowPos(mWnd, 0, x, y, getSpecSize(mAttrWidth), getSpecSize(mAttrHeight), SWP_NOSIZE | SWP_NOZORDER);
	}
	ShowWindow(mWnd, nCmdShow);
	UpdateWindow(mWnd);
}

void XWindow::onMeasure(int widthSpec, int heightSpec) {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	mesureChildren((r.right - r.left) | MS_FIX, (r.bottom - r.top) | MS_FIX);
}

void XWindow::onLayout(int width, int height) {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		child->layout(0, 0, r.right, r.bottom);
	}
}

XDialog::XDialog( XmlNode *node ) : XContainer(node) {
	strcpy(mClassName, "XDialog");
}
void XDialog::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId(); // has no id
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_DLGFRAME,
		getSpecSize(mAttrX), getSpecSize(mAttrY), getSpecSize(mAttrWidth), getSpecSize(mAttrHeight),
		getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

bool XDialog::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	int err = 0;
	if (XContainer::wndProc(msg, wParam, lParam, result))
		return true;
	if (msg == WM_SIZE && lParam > 0) {
		RECT r = {0};
		GetWindowRect(mWnd, &r);
		onMeasure(LOWORD(lParam) | XComponent::MS_FIX, HIWORD(lParam) | XComponent::MS_FIX);
		onLayout(LOWORD(lParam), HIWORD(lParam));
		return true;
	} else if (msg == WM_DESTROY) {
		PostQuitMessage(wParam);
		return true;
	} else if (msg == WM_CLOSE) {
		// default system close button is 0
		HWND parent = getParentWnd();
		EnableWindow(parent, TRUE);
		// SetForegroundWindow(parent);
		SetFocus(parent);
		// go through, deal by default
	}
	return false;
}

int XDialog::showModal() {
	RECT rect = {0};
	HWND parent = getParentWnd();
	GetWindowRect(parent, &rect);
	if (getSpecSize(mAttrX) == 0 && getSpecSize(mAttrY) == 0 && rect.right > 0 && rect.bottom > 0) {
		int x = (rect.right - rect.left - getSpecSize(mAttrWidth)) / 2 + rect.left;
		int y = (rect.bottom - rect.top - getSpecSize(mAttrHeight)) / 2 + rect.top;
		SetWindowPos(mWnd, 0, x, y, getSpecSize(mAttrWidth), getSpecSize(mAttrHeight), SWP_NOSIZE|SWP_NOZORDER);
	}
	EnableWindow(parent, FALSE);
	ShowWindow(mWnd, SW_SHOWNORMAL);

	int err = GetLastError();

	MSG msg;
	int nRet = 0;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if( msg.message == WM_CLOSE && msg.hwnd == mWnd ) {
			nRet = msg.wParam;
		}
		if (! TranslateAccelerator(msg.hwnd, NULL, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return nRet;
}

void XDialog::onMeasure(int widthSpec, int heightSpec) {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	mesureChildren((r.right - r.left) | MS_FIX, (r.bottom - r.top) | MS_FIX);
}

void XDialog::onLayout(int width, int height) {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		child->layout(0, 0, r.right, r.bottom);
	}
}

void XDialog::close( int nRet ) {
	PostMessage(mWnd, WM_CLOSE, (WPARAM)nRet, 0L);
}

// --------------------------XScroller-----------------------------------
XScroll::XScroll( XmlNode *node ) : XContainer(node) {
}

bool XScroll::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_HSCROLL) {
		SCROLLINFO si = {0};
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		GetScrollInfo(mWnd, SB_HORZ, &si);
		int oldPos = si.nPos;
		switch(LOWORD (wParam)) {
			case SB_LEFT: si.nPos = si.nMin; break;
			case SB_RIGHT: si.nPos = si.nMax; break;
			case SB_LINELEFT: si.nPos -= 20; break;
			case SB_LINERIGHT: si.nPos += 20; break;
			case SB_PAGELEFT: si.nPos -= si.nPage;  break;
			case SB_PAGERIGHT: si.nPos += si.nPage; break;
			case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
		}
		if (si.nPos < si.nMin) si.nPos = si.nMin;
		else if (si.nPos > si.nMax - si.nPage) si.nPos = si.nMax - si.nPage;
		SetScrollInfo(mWnd, SB_HORZ, &si, TRUE);
		moveChildrenPos(-si.nPos + oldPos, 0);
		return true;
	} else if (msg == WM_VSCROLL) {
		SCROLLINFO si = {0};
		si.cbSize = sizeof(SCROLLINFO);
		si.fMask = SIF_ALL;
		GetScrollInfo(mWnd, SB_VERT, &si);
		int oldPos = si.nPos;
		switch (LOWORD(wParam)) {
			case SB_TOP: si.nPos = si.nMin; break;
			case SB_BOTTOM: si.nPos = si.nMax; break;
			case SB_LINEUP: si.nPos -= 20; break;
			case SB_LINEDOWN: si.nPos += 20; break;
			case SB_PAGEUP: si.nPos -= si.nPage;  break;
			case SB_PAGEDOWN: si.nPos += si.nPage; break;
			case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
		}
		if (si.nPos < si.nMin) si.nPos = si.nMin;
		else if (si.nPos > si.nMax - si.nPage) si.nPos = si.nMax - si.nPage;
		SetScrollInfo(mWnd, SB_VERT, &si, TRUE);
		moveChildrenPos(0, -si.nPos + oldPos);
		return true;
	} else if (msg == WM_MOUSEWHEEL_BUBBLE) {
		int d = (short)HIWORD(wParam) / WHEEL_DELTA * 100;
		if (GetWindowLong(mWnd, GWL_STYLE) & WS_VSCROLL) {
			int oldPos = GetScrollPos(mWnd, SB_VERT);
			SetScrollPos(mWnd, SB_VERT, oldPos - d, TRUE);
			moveChildrenPos(0, oldPos - GetScrollPos(mWnd, SB_VERT));
			return true;
		}
		if (GetWindowLong(mWnd, GWL_STYLE) & WS_HSCROLL) {
			int oldPos = GetScrollPos(mWnd, SB_HORZ);
			SetScrollPos(mWnd, SB_HORZ, oldPos - d, TRUE);
			moveChildrenPos(0, oldPos - GetScrollPos(mWnd, SB_HORZ));
			return true;
		}
		return true;
	}
	return XContainer::wndProc(msg, wParam, lParam, result);
}

void XScroll::onMeasure( int widthSpec, int heightSpec ) {
	bool hasHorBar = GetWindowLong(mWnd, GWL_STYLE) & WS_HSCROLL;
	bool hasVerBar = GetWindowLong(mWnd, GWL_STYLE) & WS_VSCROLL;
	int thumbSize = GetSystemMetrics(SM_CXHTHUMB);

	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	int clientWidth = mMesureWidth - (hasVerBar ? thumbSize : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? thumbSize : 0);
	mesureChildren(clientWidth | MS_FIX, clientHeight| MS_FIX);
	
	int childRight = 0, childBottom = 0;
	for (int i = mNode->getChildCount() - 1; i >= 0; --i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		if ((GetWindowLong(child->getWnd(), GWL_STYLE) & WS_VISIBLE) == 0)
			continue;
		int x = calcSize(child->getAttrX(), mMesureWidth | MS_ATMOST);
		int y  = calcSize(child->getAttrY(), mMesureHeight | MS_ATMOST);
		childRight = x + child->getMesureWidth();
		childBottom = y = child->getMesureHeight();
		break;
	}
	SCROLLINFO si = {0};
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_RANGE | SIF_PAGE;
	si.nMax = childRight - 1; // nMax 等于 nPage 时也会显示scroll bar
	si.nPage = clientWidth;
	SetScrollInfo(mWnd, SB_HORZ, &si, FALSE);

	si.nMax = childBottom - 1;
	si.nPage = clientHeight;
	SetScrollInfo(mWnd, SB_VERT, &si, FALSE);

	bool curHasHorBar = GetWindowLong(mWnd, GWL_STYLE) & WS_HSCROLL;
	bool curHasVerBar = GetWindowLong(mWnd, GWL_STYLE) & WS_VSCROLL;
	if (curHasHorBar != hasHorBar || curHasVerBar != hasVerBar)
		onMeasure(widthSpec, heightSpec);
}

void XScroll::onLayout( int width, int height ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), width | MS_ATMOST) - GetScrollPos(mWnd, SB_HORZ);
		int y  = calcSize(child->getAttrY(), height | MS_ATMOST) - GetScrollPos(mWnd, SB_VERT);
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
}

void XScroll::moveChildrenPos( int dx, int dy ) {
	if (dx == 0 && dy == 0)
		return;
	RECT pa, ch;
	GetWindowRect(mWnd, &pa);
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		HWND wnd = child->getWnd();
		GetWindowRect(wnd, &ch);
		SetWindowPos(wnd, HWND_NOTOPMOST, ch.left - pa.left + dx, ch.top - pa.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
}
