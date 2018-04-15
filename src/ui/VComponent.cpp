#include "VComponent.h"
#include "../ui/XmlParser.h"
#include "../ui/UIFactory.h"
#include <stdlib.h>
#include <vector>
#include <map>
#include <string>

// #pragma comment(linker,"\"/manifestdependency:type='win32'  name='Microsoft.Windows.Common-Controls' version='6.0.0.0'   processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
// #pragma comment(lib, "comctl32.lib")
static LRESULT CALLBACK __WndProc(HWND wnd, UINT msgId, WPARAM wParam, LPARAM lParam);

HINSTANCE VComponent::mInstance;


XRect::XRect() {
	mX = mY = 0;
	mWidth = mHeight = 0;
}

XRect::XRect(int x, int y, int w, int h) {
	mX = x;
	mY = y;
	mWidth = w;
	mHeight = h;
}

void XRect::offset(int dx, int dy) {
	mX += dx;
	mY += dy;
}

XRect XRect::intersect(XRect &r) {
	XRect v;
	if (!isValid() || !r.isValid()) {
		return v;
	}
	v.mX = mX >= r.mX ? mX : r.mX;
	v.mY = mY >= r.mY ? mY : r.mY;
	v.mWidth = (mX + mWidth >= r.mX + r.mWidth ? r.mX + r.mWidth : mX + mWidth) - v.mX;
	v.mHeight = (mY + mHeight >= r.mY + r.mHeight ? r.mY + r.mHeight : mY + mHeight) - v.mY;
	if (v.mWidth < 0 || v.mHeight < 0) {
		v.mWidth = v.mHeight = 0;
	}
	return v;
}

XRect XRect::join(XRect &r) {
	XRect v;
	if (! isValid()) {
		return r;
	}
	if (! r.isValid()) {
		return *this;
	}
	v.mX = mX < r.mX ? mX : r.mX;
	v.mY = mY < r.mY ? mY : r.mY;
	v.mWidth = (mX + mWidth < r.mX + r.mWidth ? r.mX + r.mWidth : mX + mWidth) - v.mX;
	v.mHeight = (mY + mHeight < r.mY + r.mHeight ? r.mY + r.mHeight : mY + mHeight) - v.mY;
	return v;
}

bool XRect::isValid() const {
	return mWidth > 0 && mHeight > 0;
}

void XRect::reset() {
	mX = mY = 0;
	mWidth = mHeight = 0;
}

void XRect::from(RECT &r) {
	mX = r.left;
	mY = r.top;
	mWidth = r.right - r.left;
	mHeight = r.bottom - r.top;
}

RECT XRect::to() {
	RECT r;
	r.left = mX;
	r.top = mY;
	r.right = r.left + mWidth;
	r.bottom = r.top + mHeight;
	return r;
}

void XRect::set(int x, int y, int w, int h) {
	mX = x;
	mY = y;
	mWidth = w;
	mHeight = h;
}

VComponent::VComponent(XmlNode *node) {
	static int s_id = 1000;

	mID = (s_id++) * 10;
	mNode = node;
	mX = mY = mWidth = mHeight = mMesureWidth = mMesureHeight = 0;
	mAttrX = mAttrY = mAttrWidth = mAttrHeight = 0;
	memset(mAttrPadding, 0, sizeof(mAttrPadding));
	memset(mAttrMargin, 0, sizeof(mAttrMargin));
	mListener = NULL;
	mAttrFlags = 0;
	mAttrColor = mAttrBgColor = 0;
	mVisibility = VISIBLE;
	mTranslateX = mTranslateY = 0;
	mEnableFocus = false;
	mHasFocus = false;

	mDirty = true;
	mHasDirtyChild = true;
	mCache = NULL;

	mFont = NULL;
	mRectRgn = NULL;
	mBgImage = NULL;
	mAttrRoundConerX = mAttrRoundConerY = 0;
	mAttrWeight = 0;
	parseAttrs();
}

VComponent::~VComponent() {
	if (mFont != NULL) {
		DeleteObject(mFont);
	}
	if (mBgImage != NULL) {
		// mBgImage->decRef();
		mBgImage = NULL;
	}
	if (mCache != NULL) {
		delete mCache;
	}
}

void VComponent::onMeasure(int widthSpec, int heightSpec) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	onMesureChildren((mMesureWidth - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST, 
		(mMesureHeight - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
}

void VComponent::onLayoutChildren( int width, int height ) {
	// layout children
}

void VComponent::onLayout( int x, int y, int width, int height ) {
	bool changed = mWidth != width || mHeight != height;
	mX = x;
	mY = y;
	mWidth = width;
	mHeight = height;
	/*MoveWindow(mWnd, mX, mY, mWidth, mHeight, TRUE);
	if (mAttrRoundConerX != 0 && mAttrRoundConerY != 0) {
		if (mRectRgn != NULL) {
			DeleteObject(mRectRgn);
			mRectRgn = NULL;
		}
		mRectRgn = CreateRoundRectRgn(0, 0, mWidth, mHeight, mAttrRoundConerX, mAttrRoundConerY);
		SetWindowRgn(mWnd, mRectRgn, TRUE);
	}*/
	onLayoutChildren(width, height);
	if (changed && mCache != NULL) {
		delete mCache;
		mCache = NULL;
	}
}

void VComponent::parseAttrs() {
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
		} else if (strcmp(attr->mName, "visible") == 0) {
			if (strcmp(attr->mValue, "false") == 0) {
				mVisibility = INVISIBLE;
			} else if (strcmp(attr->mValue, "gone") == 0) {
				mVisibility = VISIBLE_GONE;
			}
		}
	}
}

int VComponent::getSpecSize( int sizeSpec ) {
	return sizeSpec & ~(MS_ATMOST | MS_AUTO | MS_FIX | MS_PERCENT);
}

int VComponent::calcSize( int selfSizeSpec, int parentSizeSpec ) {
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

void VComponent::onMesureChildren( int widthSpec, int heightSpec ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		child->onMeasure(widthSpec, heightSpec);
	}
}

VComponent* VComponent::findById( const char *id ) {
	XmlNode *n = mNode->findById(id);
	if (n != NULL) {
		return n->getComponentV();
	}
	return NULL;
}

VComponent* VComponent::getChildById( DWORD id ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		if (child->mID == id) return child;
	}
	return NULL;
}

VComponent* VComponent::getChildById( const char *id ) {
	XmlNode *child = mNode->getChildById(id);
	if (child != NULL)
		return child->getComponentV();
	return NULL;
}
VComponent* VComponent::getChild(int idx) {
	return mNode->getChild(idx)->getComponentV();
}

VComponent* VComponent::getParent() {
	XmlNode *parent = mNode->getParent();
	if (parent != NULL) {
		return parent->getComponentV();
	}
	return NULL;
}

void VComponent::setListener( VListener *v ) {
	mListener = v;
}

VListener* VComponent::getListener() {
	return mListener;
}

HFONT VComponent::getFont() {
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
	return mNode->getParent()->getComponentV()->getFont();
}

HINSTANCE VComponent::getInstance() {
	if (mInstance == NULL) {
		mInstance = GetModuleHandle(NULL);
	}
	return mInstance;
}

int VComponent::getWidth() {
	return mWidth;
}

int VComponent::getHeight() {
	return mHeight;
}

int VComponent::getMesureWidth() {
	return mMesureWidth;
}

int VComponent::getMesureHeight() {
	return mMesureHeight;
}

int VComponent::getAttrX() {
	return mAttrX;
}

int VComponent::getAttrY() {
	return mAttrY;
}

void VComponent::setAttrX(int attrX) {
	mAttrX = attrX;
}

void VComponent::setAttrY(int attrY) {
	mAttrY = attrY;
}

int VComponent::getAttrWeight() {
	return mAttrWeight;
}

void VComponent::setAttrWeight(int weight) {
	mAttrWeight = weight;
}

int * VComponent::getAttrMargin() {
	return mAttrMargin;
}

void VComponent::setAttrMargin(int left, int top, int right, int bottom) {
	mAttrMargin[0] = left;
	mAttrMargin[1] = top;
	mAttrMargin[2] = right;
	mAttrMargin[3] = bottom;
}

int *VComponent::getAttrPadding() {
	return mAttrPadding;
}

void VComponent::setAttrPadding(int left, int top, int right, int bottom) {
	mAttrPadding[0] = left;
	mAttrPadding[1] = top;
	mAttrPadding[2] = right;
	mAttrPadding[3] = bottom;
}

COLORREF VComponent::getAttrColor() {
	return mAttrColor;
}

void VComponent::setAttrColor(COLORREF c) {
	mAttrFlags |= AF_COLOR;
	mAttrColor = c;
}

COLORREF VComponent::getAttrBgColor() {
	return mAttrBgColor;
}

void VComponent::setAttrBgColor(COLORREF c) {
	mAttrFlags |= AF_BG_COLOR;
	mAttrBgColor = c;
}

bool VComponent::dispatchMessage(Msg *msg) {
	// is mouse event, but not mouse_leave
	if (msg->mId >= Msg::LBUTTONDOWN && msg->mId < Msg::MOUSE_LEAVE) {
		VComponent *target = NULL;
		for (int i = mNode->getChildCount() - 1; i >= 0; --i) {
			VComponent *child = getChild(i);
			int x = msg->mouse.x - mTranslateX, y = msg->mouse.y - mTranslateY;
			if (x >= child->mX && y >= child->mY && x < child->mX + child->mWidth && y < child->mY + child->mHeight) {
				Msg n = *msg;
				n.mouse.x -= child->mX + mTranslateX;
				n.mouse.y -= child->mY + mTranslateY;
				if (child->dispatchMessage(&n)) {
					msg->mResult = n.mResult;
					return true;
				}
				break;
			}
		}
	}
	// go here, means mouse msg is not deal, or is mouse_leave
	if (msg->mId >= Msg::LBUTTONDOWN && msg->mId <= Msg::MOUSE_LEAVE) {
		if (mListener != NULL) {
			if (mListener->onEvent(this, msg)) {
				return true;
			}
		}
		return onMouseEvent(msg);
	}
	if (msg->mId >= Msg::KEY_DOWN && msg->mId <= Msg::CHAR) {
		return onKeyEvent(msg);
	}
	if (msg->mId == Msg::GAIN_FOCUS || msg->mId == Msg::LOST_FOCUS) {
		return onFocusEvent(msg->mId == Msg::GAIN_FOCUS);
	}

	/*if (msg->mId == Msg::SET_CURSOR) {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		return true;
	}*/
	return false;
}

bool VComponent::onMouseEvent(Msg *m) {
	return false;
}

bool VComponent::onKeyEvent(Msg *m) {
	return false;
}

bool VComponent::onFocusEvent(bool gainFocus) {
	return false;
}

bool VComponent::onPaint(HDC dc) {
	eraseBackground(dc);
	return true;
}

void VComponent::dispatchPaintEvent(Msg *msg) {
	if (mDirty) {
		drawCache(msg->paint.dc);
		mDirty = false;
		mDirtyRect.reset();
	}
	if (! mHasDirtyChild) {
		return;
	}
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = getChild(i);
		child->dispatchPaintEvent(msg);
	}
	mHasDirtyChild = false;
}

void VComponent::dispatchPaintMerge(XImage *dst, XRect &clip, int x, int y) {
	XRect self(x, y, mWidth, mHeight);
	XRect self2 = self.intersect(clip);
	if (! self2.isValid()) {
		return;
	}
	// int sid = SaveDC(dstDc);
	// IntersectClipRect(dstDc, self2.mX, self2.mY, self2.mX + self2.mWidth, self2.mY + self2.mHeight);
	// RECT rr = {0};
	// int vv = GetClipBox(dstDc, &rr);
	// mCache->draw(dstDc, x, y, mWidth, mHeight);
	dst->drawCopy(mCache, self2.mX, self2.mY, self2.mWidth, self2.mHeight, self2.mX - x, self2.mY - y);
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *cc = getChild(i);
		cc->dispatchPaintMerge(dst, self2, x + cc->mX - mTranslateX, y + cc->mY - mTranslateY);
	}
	// RestoreDC(dstDc, sid);
}

void VComponent::drawCache(HDC dc) {
	HDC memDc = CreateCompatibleDC(dc);
	if (mCache == NULL) {
		mCache = XImage::create(mWidth, mHeight, 32);
	}
	SelectObject(memDc, mCache->getHBitmap());
	/*IntersectClipRect(memDc, mDirtyRect.mX, mDirtyRect.mY,
		mDirtyRect.mX + mDirtyRect.mWidth, mDirtyRect.mY + mDirtyRect.mHeight);*/
	onPaint(memDc);
	DeleteObject(memDc);
}

POINT VComponent::getDrawPoint() {
	POINT pt = {mX, mY};
	for (VComponent *cc = getParent(); cc != NULL; cc = cc->getParent()) {
		pt.x += cc->mX - cc->mTranslateX;
		pt.y += cc->mY - cc->mTranslateY;
	}
	return pt;
}

RECT VComponent::getDrawRect() {
	static VComponent *path[30];
	int num = 0;
	int x = 0, y = 0, w = 0, h = 0;
	VComponent *cc = this;
	while (cc != NULL) {
		path[num++] = cc;
		XmlNode *node = cc->mNode->getParent();
		if (node == NULL) {
			break;
		}
		cc = node->getComponentV();
	}
	RECT r1, r2, rs;
	r1.left = r1.right = 0;
	r1.right = path[num - 1]->mWidth;
	r1.bottom = path[num - 1]->mHeight;
	rs = r1;
	for (int i = num - 2; i >= 0; --i) {
		r2.left = path[i]->mX - path[i]->mTranslateX;
		r2.top = path[i]->mY - path[i]->mTranslateY;
		r2.right = r2.left + path[i]->mWidth;
		r2.bottom = r2.top + path[i]->mHeight;
		if (! IntersectRect(&rs, &r1, &r2)) {
			return rs;
		}
		r1 = rs;
	}
	return rs;
}

void VComponent::eraseBackground(HDC dc) {
	bool hasBg = false;
	if (mAttrFlags & AF_BG_COLOR) {
		mCache->fillColor(mAttrBgColor);
		hasBg = true;
	}
	if (mBgImage != NULL) {
		/*if (! hasBg) {
			mCache->fillAlpha(0xff);
		}*/
		// mBgImage->draw(dc, 0, 0, mWidth, mHeight);
		mCache->draw(mBgImage, 0, 0, mWidth, mHeight, XImage::DA_COPY);
		hasBg = true;
	}
	if (! hasBg) {
		// has no background, draw transparent color
		mCache->fillColor(0x00000000);
	}
}

void VComponent::repaint(XRect *dirtyRect) {
	XRect rect;
	if (dirtyRect == NULL) {
		mDirtyRect.mX = mDirtyRect.mY = 0;
		mDirtyRect.mWidth = mWidth;
		mDirtyRect.mHeight = mHeight;
		rect = mDirtyRect;
	} else {
		mDirtyRect = mDirtyRect.join(*dirtyRect);
		rect = *dirtyRect;
	}
	mDirty = true;
	VComponent *parent = getParent();
	while (parent != NULL) {
		parent->mHasDirtyChild = true;
		parent = parent->getParent();
	}
	POINT pt = getDrawPoint();
	rect.offset(pt.x, pt.y);
	RECT rr = rect.to();
	InvalidateRect(getWnd(), &rr, FALSE);
}

HWND VComponent::getWnd() {
	if (getParent() != NULL) {
		return getParent()->getWnd();
	}
	return NULL;
}

VBaseWindow * VComponent::getRoot() {
	VComponent *v = getParent();
	while (v != NULL) {
		VComponent *vv = v->getParent();
		if (vv != NULL) {
			v = vv;
		} else {
			break;
		}
	}
	return dynamic_cast<VBaseWindow*>(v);
}

void VComponent::setCapture() {
	VBaseWindow *w = getRoot();
	if (w) {
		w->mCapture = this;
	}
}

void VComponent::releaseCapture() {
	VBaseWindow *w = getRoot();
	if (w) {
		w->mCapture = NULL;
	}
}

bool VComponent::hasBackground() {
	return (mAttrFlags & AF_BG_COLOR) || mBgImage != NULL;
}

void VComponent::setFocus() {
	VBaseWindow *w = getRoot();
	if (w) {
		w->mFocus = this;
	}
}

void VComponent::releaseFocus() {
	VBaseWindow *w = getRoot();
	if (w) {
		w->mFocus = NULL;
	}
}

void VComponent::updateWindow() {
	HWND wnd = getWnd();
	UpdateWindow(wnd);
}

//----------------------------------------------------------
void MyRegisterClassV(HINSTANCE ins, const char *className) {
	static std::map<std::string, bool> sCache;
	if (sCache.find(className) != sCache.end())
		return;
	WNDCLASSEX wcex = {0};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc	= __WndProc;
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

static LRESULT CALLBACK __WndProc(HWND wnd, UINT msgId, WPARAM wParam, LPARAM lParam) {
	VComponent* cc = NULL;
	LRESULT ret = 0;
	Msg msg;
	// printf("WndProc msgId: %x \n", msgId);

	if (msgId == WM_NCCREATE) {
		LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
		cc = static_cast<VComponent*>(lpcs->lpCreateParams);
		VBaseWindow *win = static_cast<VBaseWindow*>(cc);
		win->setWnd(wnd);
		SetWindowLongPtr(wnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(cc));
	} else {
		cc = (VComponent *)GetWindowLong(wnd, GWL_USERDATA);
	}
	if (cc == NULL) goto _end;

	if (msgId >= WM_MOUSEFIRST && msgId <= WM_MOUSELAST) {
		msg.mouse.x = (short)LOWORD(lParam);
		msg.mouse.y = (short)HIWORD(lParam);
		msg.mouse.vkey.ctrl = (int)wParam & MK_CONTROL;
		msg.mouse.vkey.shift = (int)wParam & MK_SHIFT;
		msg.mouse.vkey.lbutton = (int)wParam & MK_LBUTTON;
		msg.mouse.vkey.rbutton = (int)wParam & MK_RBUTTON;
		msg.mouse.vkey.mbutton = (int)wParam & MK_MBUTTON;
	}

	switch (msgId) {
	case WM_LBUTTONDOWN:
		SetCapture(wnd);
		msg.mId = Msg::LBUTTONDOWN;
		cc->dispatchMessage(&msg);
		return 0;
	case WM_LBUTTONUP:
		msg.mId = Msg::LBUTTONUP;
		ReleaseCapture();
		cc->dispatchMessage(&msg);
		// TODO: send a CLICK msg here
		return 0;
	case WM_RBUTTONDOWN:
		msg.mId = Msg::RBUTTONDOWN;
		SetCapture(wnd);
		cc->dispatchMessage(&msg);
		return 0;
	case WM_RBUTTONUP:
		msg.mId = Msg::RBUTTONUP;
		ReleaseCapture();
		cc->dispatchMessage(&msg);
		return 0;
	case WM_LBUTTONDBLCLK:
		msg.mId = Msg::DBCLICK;
		cc->dispatchMessage(&msg);
		return 0;
	case WM_MOUSEMOVE:
		msg.mId = Msg::MOUSE_MOVE;
		cc->dispatchMessage(&msg);
		goto _end;
		return 0;
		// TODO: mouse leave msg
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL: {
		POINT pt = {0};
		GetCursorPos(&pt);
		msg.mId = Msg::MOUSE_WHEEL;
		msg.mouse.deta = HIWORD(wParam) / WHEEL_DELTA;
		msg.mouse.vkey.ctrl = LOWORD(wParam) & MK_CONTROL;
		msg.mouse.vkey.shift = LOWORD(wParam) & MK_SHIFT;
		msg.mouse.vkey.lbutton = LOWORD(wParam) & MK_LBUTTON;
		msg.mouse.vkey.rbutton = LOWORD(wParam) & MK_RBUTTON;
		msg.mouse.vkey.mbutton = LOWORD(wParam) & MK_MBUTTON;
		msg.mouse.x = pt.x;
		msg.mouse.y = pt.y;
		cc->dispatchMessage(&msg);
		return 0;}
	case WM_KEYDOWN:
		msg.mId = Msg::KEY_DOWN;
		msg.key.vkey.ctrl = (int)wParam & MK_CONTROL;
		msg.key.vkey.shift = (int)wParam & MK_SHIFT;
		msg.key.vkey.lbutton = (int)wParam & MK_LBUTTON;
		msg.key.vkey.rbutton = (int)wParam & MK_RBUTTON;
		msg.key.vkey.mbutton = (int)wParam & MK_MBUTTON;
		cc->dispatchMessage(&msg);
		return 0;
	case WM_KEYUP:
		msg.mId = Msg::KEY_UP;
		msg.key.vkey.ctrl = (int)wParam & MK_CONTROL;
		msg.key.vkey.shift = (int)wParam & MK_SHIFT;
		msg.key.vkey.lbutton = (int)wParam & MK_LBUTTON;
		msg.key.vkey.rbutton = (int)wParam & MK_RBUTTON;
		msg.key.vkey.mbutton = (int)wParam & MK_MBUTTON;
		cc->dispatchMessage(&msg);
		return 0;
	case WM_CHAR:
	case WM_IME_CHAR:
		msg.mId = Msg::CHAR;
		msg.key.code = LOWORD(wParam);
		cc->dispatchMessage(&msg);
		return 0;
	// TODO: gain/lost focus 
	case WM_PAINT: {
		msg.mId = Msg::PAINT;
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(wnd, &ps);
		msg.paint.dc = dc;
		msg.paint.rect.from(ps.rcPaint);
		cc->dispatchPaintEvent(&msg);

		XRect clip(0, 0, cc->getWidth(), cc->getHeight());
		clip = clip.intersect(msg.paint.rect);
		IntersectClipRect(dc, 0, 0, cc->getWidth(), cc->getHeight());
		VBaseWindow *win = static_cast<VBaseWindow*>(cc);
		XRect clp = clip;
		XImage *canvas = win->dispatchPaintMerge(clp);

		HDC mdc = CreateCompatibleDC(dc);
		SelectObject(mdc, canvas->getHBitmap());
		BitBlt(dc, clip.mX, clip.mY, clip.mWidth, clip.mHeight, mdc, clip.mX, clip.mY, SRCCOPY);
		DeleteObject(mdc);

		EndPaint(wnd, &ps);
		return 0;}
	case WM_SETCURSOR:
		msg.mId = Msg::SET_CURSOR;
		// every time mouse move will go here
		// SetCursor(LoadCursor(NULL, IDC_ARROW));
		if (cc->dispatchMessage(&msg)) {
			return FALSE;
		}
		goto _end;
	case WM_ERASEBKGND:
		return 0;
	}

	msg.mId = (Msg::ID)msgId;
	msg.def.wParam = wParam;
	msg.def.lParam = lParam;
	if (cc->dispatchMessage(&msg)) {
		return msg.mResult;
	}

	/*if (cc->getListener() != NULL && cc->getListener()->onEvent(cc, msg, wParam, lParam, &ret))
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
	}*/
	_end:
	return DefWindowProc(wnd, msgId, wParam, lParam);
}

//-------------VBaseWindow---------------------------------
VBaseWindow::VBaseWindow(XmlNode *node) : VComponent(node) {
	mWnd = NULL;
	mCapture = NULL;
	mFocus = NULL;
	mCanvas = NULL;
}

void VBaseWindow::onLayoutChildren(int width, int height) {
	RECT r = getClientRect();
	width = r.right - r.left;
	height = r.bottom - r.top;
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		int x = calcSize(child->getAttrX(), width | MS_ATMOST);
		int y  = calcSize(child->getAttrY(), height | MS_ATMOST);
		child->onLayout(r.left + x, r.top + y, child->getMesureWidth(), child->getMesureHeight());
	}
}

void VBaseWindow::applyAttrs() {
	HFONT font = getFont();
	SendMessage(mWnd, WM_SETFONT, (WPARAM)font, 0);
	if (mVisibility != VISIBLE) {
		DWORD st = GetWindowLong(mWnd, GWL_STYLE);
		st = st & ~WS_VISIBLE;
		SetWindowLong(mWnd, GWL_STYLE, st);
	}
}

void VBaseWindow::applyIcon() {
	HICON ico = XImage::loadIcon(mNode->getAttrValue("icon"));
	if (ico != NULL) {
		SendMessage(mWnd, WM_SETICON, ICON_BIG, (LPARAM)ico);
		SendMessage(mWnd, WM_SETICON, ICON_SMALL, (LPARAM)ico);
	}
}

bool VBaseWindow::dispatchMessage(Msg *msg) {
	if (msg->mId == WM_ACTIVATE) {
		if (WA_INACTIVE == LOWORD(msg->def.wParam)) {
			if (mCapture != NULL) {
				Msg m = *msg;
				m.mId = Msg::MOUSE_CANCEL;
				mCapture->onMouseEvent(&m);
			}
		}
		// go through
	} else if (msg->mId >= Msg::LBUTTONDOWN && msg->mId <= Msg::MOUSE_LEAVE) {
		if (mCapture != NULL) {
			Msg m = *msg;
			POINT pt = mCapture->getDrawPoint();
			m.mouse.x -= pt.x;
			m.mouse.y -= pt.y;
			return mCapture->dispatchMessage(&m);
		}
		// go through
	} else if (msg->mId == WM_SIZE && msg->def.lParam > 0) {
		// int w = LOWORD(msg->def.lParam) , h = HIWORD(msg->def.lParam);
		notifyLayout();
		return true;
	} else if (msg->mId == WM_DESTROY) {
		PostQuitMessage(msg->def.wParam);
		return true;
	}
	return VComponent::dispatchMessage(msg);
}

RECT VBaseWindow::getClientRect() {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	return r;
}

void VBaseWindow::onMeasure(int widthSpec, int heightSpec) {
	mMesureWidth = getSpecSize(widthSpec);
	mMesureHeight = getSpecSize(heightSpec);
	onMesureChildren((mMesureWidth - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST, 
		(mMesureHeight - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
}

void VBaseWindow::notifyLayout() {
	RECT rr = getClientRect();
	int w = rr.right - rr.left, h = rr.bottom - rr.top;
	onMeasure(w | VComponent::MS_ATMOST, h | VComponent::MS_ATMOST);
	onLayout(0, 0, mMesureWidth, mMesureHeight);
	mDirty = true;
	mHasDirtyChild = true;
}

DWORD VBaseWindow::getStyle(DWORD def) {
	DWORD style = 0;
	const char *ss = mNode->getAttrValue("style");
	if (ss == NULL) {
		return def;
	}
	if (strstr(ss, "caption") != NULL) {
		style |= WS_CAPTION;
	}
	if (strstr(ss, "sys-menu") != NULL) {
		style |= WS_SYSMENU;
	}
	if (strstr(ss, "thick-frame") != NULL) {
		style |= WS_THICKFRAME;
	}
	if (strstr(ss, "dlg-frame") != NULL) {
		style |= WS_DLGFRAME;
	}
	if (strstr(ss, "border") != NULL) {
		style |= WS_BORDER;
	}
	if (strstr(ss, "min-box") != NULL) {
		style |= WS_MINIMIZEBOX;
	}
	if (strstr(ss, "max-box") != NULL) {
		style |= WS_MAXIMIZEBOX;
	}
	if (strstr(ss, "popup") != NULL) {
		style |= WS_POPUP;
	}
	return style;
}

void VBaseWindow::setWnd(HWND wnd) {
	mWnd = wnd;
}

HWND VBaseWindow::getWnd() {
	return mWnd;
}

XImage* VBaseWindow::dispatchPaintMerge(XRect &clip) {
	if (mCanvas == NULL || mCanvas->getWidth() != mWidth || mCanvas->getHeight() != mHeight) {
		delete mCanvas;
		mCanvas = XImage::create(mWidth, mHeight, 24);
	}
	VComponent::dispatchPaintMerge(mCanvas, clip, 0, 0);
	return mCanvas;
}

//--------------------------------------------------------
VWindow::VWindow(XmlNode *node) : VBaseWindow(node) {
	mWnd = NULL;
}

void VWindow::createWnd() {
	MyRegisterClassV(mInstance, "VWindow");
	mWnd = CreateWindow("VWindow", mNode->getAttrValue("text"), 
		getStyle(WS_OVERLAPPEDWINDOW),
		0, 0, 0, 0, NULL, NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
	applyIcon();
}

int VWindow::msgLoop() {
	MSG msg;
	HACCEL hAccelTable = NULL;
	while (GetMessage(&msg, NULL, 0, 0)) {
		/*if (msg.message == WM_MOUSEHWHEEL || msg.message == WM_MOUSEWHEEL) {
			POINT pt = {0};
			GetCursorPos(&pt);
			msg.hwnd = WindowFromPoint(pt);
		}*/
		if (! TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}

void VWindow::show() {
	RECT rect = {0};
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	int w = calcSize(mAttrWidth, (rect.right - rect.left) | MS_ATMOST);
	int h = calcSize(mAttrHeight, (rect.bottom - rect.top) | MS_ATMOST);
	int x = (rect.right - w) / 2;
	int y = (rect.bottom - h) / 2;
	SetWindowPos(mWnd, 0, x, y, w, h, SWP_NOZORDER | SWP_SHOWWINDOW);
	UpdateWindow(mWnd);
}

//------------VDialog--------------------------------
VDialog::VDialog(XmlNode *node) : VBaseWindow(node) {
	mShowModal = false;
	mParentWnd = NULL;
}

void VDialog::createWnd(HWND parent) {
	mParentWnd = parent;
	MyRegisterClassV(mInstance, "VDialog");
	mWnd = CreateWindow("VDialog", mNode->getAttrValue("text"),
		getStyle(WS_POPUP | WS_SYSMENU | WS_CAPTION | WS_DLGFRAME),
		0, 0, 0, 0, parent, NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
	applyIcon();
}

bool VDialog::dispatchMessage(Msg *msg) {
	if (msg->mId == WM_CLOSE) {
		// default system close button is 0
		EnableWindow(mParentWnd, TRUE);
		// SetForegroundWindow(mParentWnd);
		SetFocus(mParentWnd);
		// deal by default
		return false;
	}
	return VBaseWindow::dispatchMessage(msg);
}

void VDialog::showNormal() {
	mShowModal = false;
	showCenter();
}

int VDialog::showModal() {
	mShowModal = true;
	showCenter();
	EnableWindow(mParentWnd, FALSE);
	// ShowWindow(mWnd, SW_SHOWNORMAL);

	MSG msg;
	int nRet = 0;
	while (GetMessage(&msg, NULL, 0, 0)) {
		/*if (msg.message == WM_MOUSEHWHEEL || msg.message == WM_MOUSEWHEEL) {
			POINT pt = {0};
			GetCursorPos(&pt);
			msg.hwnd = WindowFromPoint(pt);
		}*/
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

void VDialog::showCenter() {
	RECT rect = {0};
	HWND parent = mParentWnd;
	if (parent == NULL) {
		parent = GetDesktopWindow();
	}
	GetWindowRect(parent, &rect);
	int x = getSpecSize(mAttrX);
	int y = getSpecSize(mAttrY);
	if (x == 0 && y == 0 && rect.right > 0 && rect.bottom > 0) {
		x = (rect.right - rect.left - getSpecSize(mAttrWidth)) / 2 + rect.left;
		y = (rect.bottom - rect.top - getSpecSize(mAttrHeight)) / 2 + rect.top;
	}
	SetWindowPos(mWnd, 0, x, y, getSpecSize(mAttrWidth), getSpecSize(mAttrHeight), SWP_NOZORDER | SWP_SHOWWINDOW);
}

void VDialog::close( int nRet ) {
	if (mShowModal) {
		PostMessage(mWnd, WM_CLOSE, (WPARAM)nRet, 0L);
	} else {
		SetWindowPos(mWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);
	}
}

