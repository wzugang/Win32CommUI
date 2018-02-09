#include "XComponent.h"
#include "XmlParser.h"
#include "UIFactory.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>

#pragma comment(linker,"\"/manifestdependency:type='win32'  name='Microsoft.Windows.Common-Controls' version='6.0.0.0'   processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
// #pragma comment(lib, "comctl32.lib")

HINSTANCE XComponent::mInstance;

void MyRegisterClass(HINSTANCE ins, const char *className) {
	static std::map<std::string, bool> sCache;
	if (sCache.find(className) != sCache.end())
		return;
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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
	if (cc == NULL) goto _end;

	LRESULT ret = 0;
	if (cc->getListener() != NULL && cc->getListener()->onEvent(cc, msg, wParam, lParam, &ret))
		return ret;
	if (cc->wndProc(msg, wParam, lParam, &ret))
		return ret;
	// bubble mouse wheel, mouse down msg
	if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL || msg == WM_LBUTTONDOWN) {
		UINT bmsg = 0;
		switch (msg) {
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			bmsg = MSG_MOUSEWHEEL_BUBBLE;break;
		case WM_LBUTTONDOWN:
			bmsg = MSG_LBUTTONDOWN_BUBBLE;break;
		}
		bubbleMsg(bmsg, wParam, lParam, &ret);
		return ret;
	}
	_end:
	return DefWindowProc(wnd, msg, wParam, lParam);
}

bool XComponent::bubbleMsg(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	POINT pt;
	GetCursorPos(&pt);
	HWND ptWnd = WindowFromPoint(pt);
	XComponent *pc = (XComponent *)GetWindowLong(ptWnd, GWL_USERDATA);
	XmlNode *node = pc != NULL ? pc->getNode() : NULL;
	
	while (node != NULL) {
		XComponent *x = node->getComponent();
		if (x == NULL) break;
		if (x->wndProc(msg, wParam, lParam, result))
			return true;
		node = node->getParent();
	}
	return false;
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
		// mBgImage->decRef();
		mBgImage = NULL;
	}
}

void XComponent::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	mID = generateWndId();
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0, 0, 0, getParentWnd(), (HMENU)mID, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

void XComponent::createWndTree() {
	createWnd();
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *cc = mNode->getChild(i)->getComponent();
		cc->createWndTree();
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

void XComponent::parseAttrs() {
	bool ok = false;
	for (int i = 0; i < mNode->getAttrsCount(); ++i) {
		XmlNode::Attr *attr = mNode->getAttr(i);
		if (strcmp(attr->mName, "rect") == 0) {
			int rect[4] = {0};
			AttrUtils::parseArraySize(attr->mValue, rect, 4);
			mAttrX = rect[0];
			mAttrY = rect[1];
			mAttrWidth = rect[2];
			mAttrHeight = rect[3];
		} else if (strcmp(attr->mName, "padding") == 0) {
			AttrUtils::parseArrayInt(attr->mValue, mAttrPadding, 4);
		} else if (strcmp(attr->mName, "margin") == 0) {
			AttrUtils::parseArrayInt(attr->mValue, mAttrMargin, 4);
		} else if (strcmp(attr->mName, "color") == 0) {
			if (AttrUtils::parseColor(attr->mValue, &mAttrColor)) {
				mAttrFlags |= AF_COLOR;
			}
		} else if (strcmp(attr->mName, "bgcolor") == 0) {
			if (AttrUtils::parseColor(attr->mValue, &mAttrBgColor)) {
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
	if (parentSizeSpec & MS_FIX) {
		return getSpecSize(parentSizeSpec);
	}
	if (selfSizeSpec & MS_FIX) {
		return getSpecSize(selfSizeSpec);
	}
	if (selfSizeSpec & MS_PERCENT) {
		if (parentSizeSpec & MS_ATMOST) {
			return getSpecSize(selfSizeSpec) * getSpecSize(parentSizeSpec) / 100;
		}
	}
	return 0;
}

HWND XComponent::getParentWnd() {
	if (mParentWnd != NULL)
		return mParentWnd;
	if (mNode->getParent() == NULL)
		return NULL;
	XComponent *pp = mNode->getParent()->getComponent();
	return pp->getWnd();
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
			mBgImage->draw(dc, 0, 0, mWidth, mHeight);
		}
		if ((mAttrFlags & AF_BG_COLOR) || hasBgImg)
			return true;
	} else if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN) {
		DWORD id = GetWindowLong((HWND)lParam, GWL_ID);
		XComponent *c = getChildById(id);
		if (c != NULL) {
			return c->onCtrlColor((HDC)wParam, res);
		}
	} else if (msg == WM_COMMAND && lParam != 0) {
		DWORD id = GetWindowLong((HWND)lParam, GWL_ID);
		XComponent *c = getChildById(id);
		if (c != NULL) {
			*res = SendMessage((HWND)lParam, MSG_COMMAND, 0, 0);
			return true;
		}
	} else if (msg == WM_NOTIFY && lParam != 0) {
		NMHDR *nmh = (NMHDR*)lParam;
		XComponent *c = getChildById(nmh->idFrom);
		if (c != NULL) {
			*res = SendMessage(nmh->hwndFrom, MSG_NOTIFY, wParam, lParam);
			return true;
		}
	} else if (msg == WM_SETCURSOR) {
		// every time mouse move will go here
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		*res = FALSE;
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

XComponent* XComponent::getParent() {
	XmlNode *parent = mNode->getParent();
	if (parent != NULL) {
		return parent->getComponent();
	}
	return NULL;
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
		SetBkColor(dc, mAttrBgColor); // onlyÎÄ×Ö±³¾°É«
		if (mBgColorBrush == NULL)
			mBgColorBrush = CreateSolidBrush(mAttrBgColor); // ¿Ø¼þ±³¾°
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
		if (AttrUtils::parseFont(&ff, mNode->getAttrValue("font"))) {
			font = mFont = CreateFontIndirect(&ff);
		}
		return font;
	}
	return mNode->getParent()->getComponent()->getFont();
}

HINSTANCE XComponent::getInstance() {
	if (mInstance == NULL) {
		mInstance = GetModuleHandle(NULL);
	}
	return mInstance;
}

int XComponent::getWidth() {
	return mWidth;
}

int XComponent::getHeight() {
	return mHeight;
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

void XComponent::setAttrX(int attrX) {
	mAttrX = attrX;
}

void XComponent::setAttrY(int attrY) {
	mAttrY = attrY;
}

int XComponent::getAttrWeight() {
	return mAttrWeight;
}

void XComponent::setAttrWeight(int weight) {
	mAttrWeight = weight;
}

int * XComponent::getAttrMargin() {
	return mAttrMargin;
}

void XComponent::setAttrMargin(int left, int top, int right, int bottom) {
	mAttrMargin[0] = left;
	mAttrMargin[1] = top;
	mAttrMargin[2] = right;
	mAttrMargin[3] = bottom;
}

int *XComponent::getAttrPadding() {
	return mAttrPadding;
}

void XComponent::setAttrPadding(int left, int top, int right, int bottom) {
	mAttrPadding[0] = left;
	mAttrPadding[1] = top;
	mAttrPadding[2] = right;
	mAttrPadding[3] = bottom;
}

COLORREF XComponent::getAttrColor() {
	return mAttrColor;
}

void XComponent::setAttrColor(COLORREF c) {
	mAttrFlags |= AF_COLOR;
	mAttrColor = c;
}

COLORREF XComponent::getAttrBgColor() {
	return mAttrBgColor;
}

void XComponent::setAttrBgColor(COLORREF c) {
	mAttrFlags |= AF_BG_COLOR;
	mAttrBgColor = c;
}

XWindow::XWindow( XmlNode *node ) : XComponent(node) {
	strcpy(mClassName, "XWindow");
}

void XWindow::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId();  // has no id
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_OVERLAPPEDWINDOW,
		0, 0, 0, 0, getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
	applyIcon();
}

bool XWindow::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (XComponent::wndProc(msg, wParam, lParam, result))
		return true;
	if (msg == WM_SIZE && lParam > 0) {
		int w = LOWORD(lParam) , h = HIWORD(lParam);
		onMeasure(w | XComponent::MS_FIX, h | XComponent::MS_FIX);
		onLayout(w, h);
		
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
		if (msg.message == WM_MOUSEHWHEEL || msg.message == WM_MOUSEWHEEL) {
			POINT pt = {0};
			GetCursorPos(&pt);
			msg.hwnd = WindowFromPoint(pt);
		}
		if (! TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}

void XWindow::show(int nCmdShow) {
	RECT rect = {0};
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	int w = calcSize(mAttrWidth, (rect.right - rect.left) | MS_ATMOST);
	int h = calcSize(mAttrHeight, (rect.bottom - rect.top) | MS_ATMOST);
	int x = (rect.right - w) / 2;
	int y = (rect.bottom - h) / 2;
	SetWindowPos(mWnd, 0, x, y, w, h, SWP_NOZORDER | SWP_SHOWWINDOW);
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
		int x = calcSize(child->getAttrX(), width | MS_ATMOST);
		int y  = calcSize(child->getAttrY(), height | MS_ATMOST);
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
}

void XWindow::applyIcon() {
	HICON ico = XImage::loadIcon(mNode->getAttrValue("icon"));
	if (ico != NULL) {
		SendMessage(mWnd, WM_SETICON, ICON_BIG, (LPARAM)ico);
		SendMessage(mWnd, WM_SETICON, ICON_SMALL, (LPARAM)ico);
	}
}

XDialog::XDialog( XmlNode *node ) : XComponent(node) {
	strcpy(mClassName, "XDialog");
}
void XDialog::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId(); // has no id
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_POPUP /*| WS_SYSMENU*/ | WS_CAPTION | WS_DLGFRAME,
		0, 0, 0, 0, getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

bool XDialog::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	int err = 0;
	if (XComponent::wndProc(msg, wParam, lParam, result))
		return true;
	if (msg == WM_SIZE && lParam > 0) {
		onMeasure(0, 0);
		onLayout(0, 0);
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
	if (parent == NULL) parent = GetDesktopWindow();
	GetWindowRect(parent, &rect);
	int x = getSpecSize(mAttrX);
	int y = getSpecSize(mAttrY);
	if (x == 0 && y == 0 && rect.right > 0 && rect.bottom > 0) {
		x = (rect.right - rect.left - getSpecSize(mAttrWidth)) / 2 + rect.left;
		y = (rect.bottom - rect.top - getSpecSize(mAttrHeight)) / 2 + rect.top;
	}
	SetWindowPos(mWnd, 0, x, y, getSpecSize(mAttrWidth), getSpecSize(mAttrHeight), SWP_NOZORDER | SWP_SHOWWINDOW);
	EnableWindow(parent, FALSE);
	// ShowWindow(mWnd, SW_SHOWNORMAL);

	MSG msg;
	int nRet = 0;
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (msg.message == WM_MOUSEHWHEEL || msg.message == WM_MOUSEWHEEL) {
			POINT pt = {0};
			GetCursorPos(&pt);
			msg.hwnd = WindowFromPoint(pt);
		}
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
