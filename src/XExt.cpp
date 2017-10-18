#include "XExt.h"
#include <stdio.h>
#include <stdlib.h>
#include <WinGDI.h>
#include "XComponent.h"
#include "XmlParser.h"
#include "UIFactory.h"

// AlphaBlend function in here
#pragma   comment(lib,"msimg32.lib")

extern void MyRegisterClass(HINSTANCE ins, const char *className);

//--------------------XExtComponent-------------------------------------
XExtComponent::XExtComponent(XmlNode *node) : XComponent(node) {
	mBgImageForParnet = NULL;
	mEnableFocus = true;
}

bool XExtComponent::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		eraseBackground((HDC)wParam);
		return true;
	}
	return XComponent::wndProc(msg, wParam, lParam, result);
}

void XExtComponent::layout( int x, int y, int width, int height ) {
	XComponent::layout(x, y, width, height);
	if (mBgImageForParnet != NULL) {
		delete mBgImageForParnet;
		mBgImageForParnet = NULL;
	}
}
void XExtComponent::eraseBackground(HDC dc) {
	if (mBgImage == NULL && mBgImageForParnet == NULL && (mAttrFlags & AF_BG_COLOR) == 0) {
		HDC memDc = CreateCompatibleDC(dc);
		HWND parent = getParentWnd();
		mBgImageForParnet = XImage::create(mWidth, mHeight);
		SelectObject(memDc, mBgImageForParnet->getHBitmap());
		HDC dc = GetDC(parent);
		BitBlt(memDc, 0, 0, mWidth, mHeight, dc, mX, mY, SRCCOPY);
		DeleteObject(memDc);
		ReleaseDC(parent, dc);
	}
	if (mAttrFlags & AF_BG_COLOR) {
		HBRUSH brush = CreateSolidBrush(mAttrBgColor);
		RECT rc = {0, 0, mWidth, mHeight};
		FillRect(dc, &rc, brush);
		DeleteObject(brush);
	}
	XImage *bg = mBgImageForParnet != NULL ? mBgImageForParnet : mBgImage;
	if (bg != NULL && bg->getHBitmap() != NULL) {
		HDC memDc = CreateCompatibleDC(dc);
		SelectObject(memDc, bg->getHBitmap());
		BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
		DeleteObject(memDc);
	}
}
XExtComponent::~XExtComponent() {
	if (mBgImageForParnet) {
		delete mBgImageForParnet;
		mBgImageForParnet = NULL;
	}
}
void XExtComponent::setEnableFocus( bool enable ) {
	mEnableFocus = enable;
}

//--------------------XExtLabel-------------------------------------
XExtLabel::XExtLabel( XmlNode *node ) : XExtComponent(node) {
	mText = mNode->getAttrValue("text");
}

bool XExtLabel::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {mAttrPadding[0], mAttrPadding[1], mWidth - mAttrPadding[2], mHeight - mAttrPadding[3]};
		if (mAttrFlags & AF_COLOR)
			SetTextColor(dc, mAttrColor);
		SetBkMode(dc, TRANSPARENT);
		SelectObject(dc, getFont());
		DrawText(dc, mText, -1, &r, 0);
		EndPaint(mWnd, &ps);
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}

char * XExtLabel::getText() {
	return mText;
}

void XExtLabel::setText( char *text ) {
	mText = text;
}
//-------------------XExtButton-----------------------------------
XExtButton::XExtButton( XmlNode *node ) : XExtComponent(node) {
	mIsMouseDown = mIsMouseMoving = mIsMouseLeave = false;
	memset(mImages, 0, sizeof(mImages));
	for (int i = 0; i < mNode->getAttrsCount(); ++i) {
		XmlNode::Attr *attr = mNode->getAttr(i);
		if (strcmp(attr->mName, "normalImage") == 0) {
			mImages[BTN_IMG_NORMAL] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "hoverImage") == 0) {
			mImages[BTN_IMG_HOVER] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "pushImage") == 0) {
			mImages[BTN_IMG_PUSH] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "disableImage") == 0) {
			mImages[BTN_IMG_DISABLE] = XImage::load(attr->mValue);
		}
	}
}

bool XExtButton::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};

		XImage *cur = mImages[getBtnImage()];
		if (cur != NULL && cur->getHBitmap() != NULL) {
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, cur->getHBitmap());
			if (cur->hasAlphaChannel())  {
				BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
				AlphaBlend(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, cur->getWidth(), cur->getHeight(), bf);
			} else {
				StretchBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, cur->getWidth(), cur->getHeight(), SRCCOPY);
			}
			DeleteObject(memDc);
		}

		if (mAttrFlags & AF_COLOR)
			SetTextColor(dc, mAttrColor);
		SetBkMode(dc, TRANSPARENT);
		SelectObject(dc, getFont());
		DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		mIsMouseDown = true;
		mIsMouseMoving = false;
		mIsMouseLeave = false;
		InvalidateRect(mWnd, NULL, TRUE);
		SetCapture(mWnd);
		if (mEnableFocus) SetFocus(mWnd);
		return true;
	} else if (msg == WM_LBUTTONUP) {
		bool md = mIsMouseDown;
		mIsMouseDown = false;
		mIsMouseMoving = false;
		ReleaseCapture();
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (md && PtInRect(&r, pt)) {
			SendMessage(mWnd, WM_COMMAND_SELF, 0, 0);
		}
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		if (! mIsMouseMoving) {
			TRACKMOUSEEVENT a = {0};
			a.cbSize = sizeof(TRACKMOUSEEVENT);
			a.dwFlags = TME_LEAVE;
			a.hwndTrack = mWnd;
			TrackMouseEvent(&a);
		}

		BtnImage bi = getBtnImage();
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (PtInRect(&r, pt)) {
			mIsMouseMoving = true;
			mIsMouseLeave = false;
		} else {
			// mouse leave ( mouse is down now)
			mIsMouseMoving = false;
			mIsMouseLeave = true;
		}
		if (bi != getBtnImage()) {
			InvalidateRect(mWnd, NULL, TRUE);
		}
		return true;
	} else if (msg == WM_MOUSELEAVE) {
		mIsMouseMoving = false;
		mIsMouseLeave = true;
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}

XExtButton::BtnImage XExtButton::getBtnImage() {
	if (GetWindowLong(mWnd, GWL_STYLE) & WS_DISABLED)
		return BTN_IMG_DISABLE;
	if (mIsMouseLeave)
		return BTN_IMG_NORMAL;
	if (mIsMouseDown && ! mIsMouseLeave) {
		return BTN_IMG_PUSH;
	}
	if (!mIsMouseDown && mIsMouseMoving) {
		return BTN_IMG_HOVER;
	}
	return BTN_IMG_NORMAL;
}
//-------------------XExtOption-----------------------------------
XExtOption::XExtOption( XmlNode *node ) : XExtButton(node) {
	mIsSelect = false;
	char *s = mNode->getAttrValue("selectImage");
	if (s != NULL) {
		mImages[BTN_IMG_SELECT] = XImage::load(s);
	}
}

bool XExtOption::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_LBUTTONUP) {
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (! mIsSelect) mIsSelect = mIsMouseDown && PtInRect(&r, pt);
		return XExtButton::wndProc(msg, wParam, lParam, result);
	}
	return XExtButton::wndProc(msg, wParam, lParam, result);
}

bool XExtOption::isSelect() {
	return mIsSelect;
}

void XExtOption::setSelect( bool select ) {
	if (mIsSelect != select) {
		mIsSelect = select;
		InvalidateRect(mWnd, NULL, TRUE);
	}
}

XExtOption::BtnImage XExtOption::getBtnImage() {
	if (mIsSelect)
		return BtnImage(BTN_IMG_SELECT);
	return XExtButton::getBtnImage();
}
//-------------------XExtCheckBox-----------------------------------
XExtCheckBox::XExtCheckBox( XmlNode *node ) : XExtOption(node) {
}

bool XExtCheckBox::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};
		XImage *cur = mImages[getBtnImage()];
		if (cur != NULL && cur->getHBitmap() != NULL) {
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, cur->getHBitmap());
			int y = (mHeight - cur->getHeight()) / 2;
			if (cur->hasAlphaChannel())  {
				BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
				AlphaBlend(dc, 0, y, cur->getWidth(), cur->getHeight(), memDc, 0, 0, cur->getWidth(), cur->getHeight(), bf);
			} else {
				BitBlt(dc, 0, y, cur->getWidth(), cur->getHeight(), memDc, 0, 0, SRCCOPY);
			}
			DeleteObject(memDc);
			r.left = cur->getWidth() + 5 + mAttrPadding[0];
		}

		if (mAttrFlags & AF_COLOR)
			SetTextColor(dc, mAttrColor);
		SetBkMode(dc, TRANSPARENT);
		SelectObject(dc, getFont());
		DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_VCENTER | DT_SINGLELINE);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONUP) {
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mIsMouseDown && PtInRect(&r, pt)) mIsSelect = ! mIsSelect;
		return XExtButton::wndProc(msg, wParam, lParam, result);
	}
	return XExtOption::wndProc(msg, wParam, lParam, result);
}
//-------------------XExtRadio-----------------------------------
XExtRadio::XExtRadio( XmlNode *node ) : XExtCheckBox(node) {
}

bool XExtRadio::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_LBUTTONUP) {
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mIsMouseDown && PtInRect(&r, pt) && mIsSelect)
			return true;
		mIsSelect = true;
		unselectOthers();
		return XExtButton::wndProc(msg, wParam, lParam, result);
	}
	return XExtCheckBox::wndProc(msg, wParam, lParam, result);
}

void XExtRadio::unselectOthers() {
	char *groupName = mNode->getAttrValue("group");
	if (groupName == NULL) return;
	XmlNode *parent = mNode->getParent();
	for (int i = 0; parent != NULL && i < parent->getChildCount(); ++i) {
		XExtRadio *child = dynamic_cast<XExtRadio*>(parent->getComponent()->getChild(i));
		if (child == NULL || child == this) continue;
		if (child->mIsSelect && strcmp(groupName, child->getNode()->getAttrValue("group")) == 0) {
			child->mIsSelect = false;
			InvalidateRect(child->getWnd(), NULL, TRUE);
		}
	}
}
//-------------------XExtScroll-----------------------------------
XScrollBar::XScrollBar( XmlNode *node, bool horizontal ) : XExtComponent(node) {
	mHorizontal = horizontal;
	memset(&mThumbRect, 0, sizeof(mThumbRect));
	mTrack = mThumb = NULL;
	if (horizontal) {
		mThumbRect.bottom = 10;
	} else {
		mThumbRect.right = 10;
	}
	mPos = 0;
	mMax = 0;
	mPage = 0;
	mPressed = false;
	mMouseX = mMouseY = 0;
	mBuffer = NULL;
}
bool XScrollBar::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_SIZE) {
		if (mBuffer) delete mBuffer;
		mBuffer = NULL;
	} else if (msg == WM_ERASEBKGND) {
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		HDC memBufDc = CreateCompatibleDC(dc);
		if (mBuffer == NULL) mBuffer = XImage::create(mWidth, mHeight);
		SelectObject(memBufDc, mBuffer->getHBitmap());
		if (mTrack != NULL) {
			SelectObject(memDc, mTrack->getHBitmap());
			StretchBlt(memBufDc, 0, 0, mWidth, mHeight, memDc, 0, 0, mTrack->getWidth(), mTrack->getHeight(), SRCCOPY);
		}
		if (mThumb != NULL) {
			SelectObject(memDc, mThumb->getHBitmap());
			StretchBlt(memBufDc, mThumbRect.left, mThumbRect.top, mThumbRect.right - mThumbRect.left, 
				mThumbRect.bottom - mThumbRect.top, memDc, 0, 0, mThumb->getWidth(), mThumb->getHeight(), SRCCOPY);
		}
		BitBlt(dc, 0, 0, mWidth, mHeight, memBufDc, 0, 0, SRCCOPY);
		DeleteObject(memDc);
		DeleteObject(memBufDc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		POINT pt = {(short)lParam, (short)(lParam >> 16)};
		if (PtInRect(&mThumbRect, pt)) {
			mPressed = true;
			SetCapture(mWnd);
			mMouseX = short(lParam & 0xffff);
			mMouseY = short((lParam >> 16) & 0xffff);
		}
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		if (! mPressed) return true;
		int x = short(lParam);
		int y = short((lParam >> 16) & 0xffff);
		int dx = x - mMouseX;
		int dy = y - mMouseY;
		if (mHorizontal) {
			if (dx == 0) return true;
			int sz = mPage - (mThumbRect.right - mThumbRect.left);
			if (mThumbRect.left + dx < 0) dx = -mThumbRect.left;
			else if (mThumbRect.right + dx > mWidth) dx = mWidth - mThumbRect.right;
			int newPos = (mThumbRect.left + dx) * (mMax - mPage) / sz;
			if (newPos != mPos) {
				mMouseX = x;
				mMouseY = y;
				mPos = newPos;
				OffsetRect(&mThumbRect, dx, 0);
				InvalidateRect(mWnd, NULL, TRUE);
				SendMessage(getParentWnd(), WM_HSCROLL, newPos, 0);
			}
		} else {
			if (dy == 0) return true;
			int sz = mPage - (mThumbRect.bottom - mThumbRect.top);
			if (mThumbRect.top + dy < 0) dy = -mThumbRect.top;
			else if (mThumbRect.bottom + dy > mHeight) dy = mHeight - mThumbRect.bottom;
			int newPos = (mThumbRect.top + dy) * (mMax - mPage) / sz;
			if (newPos != mPos) {
				mMouseX = x;
				mMouseY = y;
				mPos = newPos;
				OffsetRect(&mThumbRect, 0, dy);
				InvalidateRect(mWnd, NULL, TRUE);
				SendMessage(getParentWnd(), WM_VSCROLL, newPos, 0);
			}
		}
		return true;
	} else if (msg == WM_LBUTTONUP) {
		mPressed = false;
		ReleaseCapture();
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
int XScrollBar::getPos() {
	return mPos;
}
void XScrollBar::setPos( int pos ) {
	if (pos < 0) pos = 0;
	else if (pos > mMax - mPage) pos = mMax - mPage;
	mPos = pos;
	calcThumbInfo();
}
int XScrollBar::getPage() {
	return mPage;
}
int XScrollBar::getMax() {
	return mMax;
}
void XScrollBar::setMaxAndPage( int maxn, int page ) {
	if (maxn < 0) maxn = 0;
	mMax = maxn;
	if (page < 0) page = 0;
	mPage = page;
	if (maxn <= page) mPos = 0;
	else if (mPos > mMax - mPage) mPos = mMax - mPage;
	calcThumbInfo();
}
void XScrollBar::calcThumbInfo() {
	if (mMax <= 0 || mPage <= 0 || mPage >= mMax) {
		if (mHorizontal) {
			mThumbRect.left = mThumbRect.right = 0;
		} else {
			mThumbRect.top = mThumbRect.bottom = 0;
		}
		return;
	}
	int sz = mPage * mPage / mMax;
	float a = mPos * (mPage - sz) / (mMax - mPage);
	if (mHorizontal) {
		mThumbRect.left = a;
		mThumbRect.right = mThumbRect.left + sz;
	} else {
		mThumbRect.top = a;
		mThumbRect.bottom = mThumbRect.top + sz;
	}
}
bool XScrollBar::isNeedShow() {
	return mMax > mPage;
}
int XScrollBar::getThumbSize() {
	if (mHorizontal) return mThumbRect.bottom;
	return mThumbRect.right;
}
void XScrollBar::setImages( XImage *track, XImage *thumb ) {
	mTrack = track;
	mThumb = thumb;
}
XExtScroll::XExtScroll( XmlNode *node ) : XExtComponent(node) {
	mHorNode = new XmlNode(NULL, mNode);
	mVerNode = new XmlNode(NULL, mNode);
	mHorBar = new XScrollBar(mHorNode, true);
	mVerBar = new XScrollBar(mVerNode, false);
	mHorBar->setImages(XImage::load(mNode->getAttrValue("hbarTrack")), XImage::load(mNode->getAttrValue("hbarThumb")));
	mVerBar->setImages(XImage::load(mNode->getAttrValue("vbarTrack")), XImage::load(mNode->getAttrValue("vbarThumb")));
}
#define WND_HIDE(w) SetWindowLong(w, GWL_STYLE, GetWindowLong(w, GWL_STYLE) & ~WS_VISIBLE)
#define WND_SHOW(w) SetWindowLong(w, GWL_STYLE, GetWindowLong(w, GWL_STYLE) | WS_VISIBLE)
void XExtScroll::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	bool hasHorBar = GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getThumbSize() : 0);
	mesureChildren(clientWidth | MS_ATMOST, clientHeight| MS_ATMOST);

	int childRight = 0, childBottom = 0;
	SIZE cs = calcDataSize();
	childRight = cs.cx;
	childBottom = cs.cy;
	mHorBar->setMaxAndPage(childRight, clientWidth);
	mVerBar->setMaxAndPage(childBottom, clientHeight);
	if (mHorBar->isNeedShow()) 
		WND_SHOW(mHorBar->getWnd());
	else
		WND_HIDE(mHorBar->getWnd());
	if (mVerBar->isNeedShow()) 
		WND_SHOW(mVerBar->getWnd());
	else 
		WND_HIDE(mVerBar->getWnd());

	if (mHorBar->isNeedShow() != hasHorBar || mVerBar->isNeedShow() != hasVerBar)
		onMeasure(widthSpec, heightSpec);
}
SIZE XExtScroll::calcDataSize() {
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
	SIZE sz = {childRight, childBottom};
	return sz;
}
void XExtScroll::onLayout( int width, int height ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), width | MS_ATMOST) - mHorBar->getPos();
		int y  = calcSize(child->getAttrY(), height | MS_ATMOST) - mVerBar->getPos();
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
	mHorBar->layout(0, mHeight - mHorBar->getThumbSize(), mWidth, mHorBar->getThumbSize());
	mVerBar->layout(mWidth - mVerBar->getThumbSize(), 0, mVerBar->getThumbSize(), mHeight);
}
bool XExtScroll::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_HSCROLL || msg == WM_VSCROLL) {
		moveChildrenPos(mHorBar->getPos(), mVerBar->getPos());
		return true;
	} else if (msg == WM_MOUSEWHEEL_BUBBLE || msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
		int d = (short)HIWORD(wParam) / WHEEL_DELTA * 100;
		if (GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE) {
			int old = mVerBar->getPos();
			mVerBar->setPos(old - d);
			if (old != mVerBar->getPos()) {
				moveChildrenPos(mHorBar->getPos(), mVerBar->getPos());
				InvalidateRect(mVerBar->getWnd(), NULL, TRUE);
			}
			return true;
		}
		if (GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE) {
			int old = mHorBar->getPos();
			mHorBar->setPos(old - d);
			if (old != mHorBar->getPos()) {
				moveChildrenPos(mHorBar->getPos(), mVerBar->getPos());
				InvalidateRect(mHorBar->getWnd(), NULL, TRUE);
			}
			return true;
		}
		return true;
	} else if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDOWN_BUBBLE) {
		if (mEnableFocus) SetFocus(mWnd);
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
void XExtScroll::moveChildrenPos( int x, int y ) {
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		SetWindowPos(child->getWnd(), 0, -x, -y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		RECT rr = {0};
		GetWindowRect(child->getWnd(), &rr);
	}
	// invalide(this);
}
void XExtScroll::invalide( XComponent *c ) {
	InvalidateRect(c->getWnd(), NULL, TRUE);
	UpdateWindow(c->getWnd());
	for (int i = 0; i < c->getNode()->getChildCount(); ++i) {
		if (GetWindowLong(c->getChild(i)->getWnd(), GWL_STYLE) & WS_VISIBLE)
			invalide(c->getChild(i));
	}
}
void XExtScroll::createWnd() {
	XComponent::createWnd();
	mHorBar->createWnd();
	mVerBar->createWnd();
	WND_HIDE(mHorBar->getWnd());
	WND_HIDE(mVerBar->getWnd());
}
XExtScroll::~XExtScroll() {
	delete mHorNode;
	delete mVerNode;
	delete mHorBar;
	delete mVerBar;
}
XScrollBar* XExtScroll::getHorBar() {
	return mHorBar;
}
XScrollBar* XExtScroll::getVerBar() {
	return mVerBar;
}
//-------------------XExtPopup-------------------------------
XExtPopup::XExtPopup( XmlNode *node ) : XComponent(node) {
	strcpy(mClassName, "XExtPopup");
}

void XExtPopup::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId();  // has no id
	mWnd = CreateWindow(mClassName, NULL, WS_POPUP, 0, 0, 0, 0, getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

bool XExtPopup::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (XComponent::wndProc(msg, wParam, lParam, result))
		return true;
	if (msg == WM_SIZE && lParam > 0) {
		RECT r = {0};
		GetWindowRect(mWnd, &r);
		onMeasure(LOWORD(lParam) | XComponent::MS_ATMOST, HIWORD(lParam) | XComponent::MS_ATMOST);
		onLayout(LOWORD(lParam), HIWORD(lParam));
		return true;
	} else if (msg == WM_DESTROY) {
		// PostQuitMessage(0); // Do not it
		return true;
	} else if (msg == WM_MOUSEACTIVATE) {
		*result = MA_NOACTIVATE; // 鼠标点击时，不活动
		return true;
	}
	return false;
}

void XExtPopup::onLayout( int width, int height ) {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), width | MS_ATMOST);
		int y = calcSize(child->getAttrY(), height | MS_ATMOST);
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
}

int XExtPopup::messageLoop() {
	MSG msg = {0};
	HWND ownerWnd = GetWindow(mWnd, GW_OWNER);
	RECT popupRect;
	GetWindowRect(mWnd, &popupRect);
	while (TRUE) {
		if ((GetWindowLong(mWnd, GWL_STYLE) & WS_VISIBLE) == 0)
			break;
		if (GetForegroundWindow() != mWnd && GetForegroundWindow() != ownerWnd) {
			break;
		}
		if (GetMessage(&msg, NULL, 0, 0)) {
			if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP || msg.message == WM_CHAR || msg.message == WM_IME_CHAR) {
				// transfer the message to menu window
				msg.hwnd = mWnd;
			} else if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP || msg.message == WM_NCLBUTTONDOWN || msg.message == WM_NCLBUTTONUP) {
				POINT pt;
				GetCursorPos(&pt);
				if (! PtInRect(&popupRect, pt)) { // click on other window
					SendMessage(mWnd, WM_EXT_POPUP_CLOSED, 1, 0);
					break;
				}
			} else if (msg.message == WM_QUIT) {
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	ShowWindow(mWnd, SW_HIDE);
	return 0;
}

void XExtPopup::show( int x, int y ) {
	SetWindowPos(mWnd, 0, x, y, getSpecSize(mAttrWidth), getSpecSize(mAttrHeight), 
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
	// ShowWindow(mWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(mWnd);
	messageLoop();
}
void XExtPopup::showNoSize(int screenX, int screenY) {
	SetWindowPos(mWnd, 0, screenX, screenY, 0, 0, SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE);
	UpdateWindow(mWnd);
	messageLoop();
}
void XExtPopup::close() {
	ShowWindow(mWnd, SW_HIDE);
	SendMessage(mWnd, WM_EXT_POPUP_CLOSED, 0, 0);
}
void XExtPopup::disableChildrenFocus() {

}
XExtPopup::~XExtPopup() {
	if (mWnd) DestroyWindow(mWnd);
}
//-------------------XExtTable-------------------------------
XExtTable::XExtTable( XmlNode *node ) : XExtScroll(node) {
	mBuffer = NULL;
	mDataSize.cx = mDataSize.cy = 0;
	mLinePen = CreatePen(PS_SOLID, 1, RGB(110, 120, 250));
	mSelectedRow = -1;
	mModel = NULL;
	mSelectBgColor = RGB(0xE6, 0xE6, 0xFA);
	mSelectBgBrush = NULL;
	mCellRender = NULL;
}
void XExtTable::setModel(XExtTableModel *model) {
	mModel = model;
}
void XExtTable::setCellRender(CellRender *render) {
	mCellRender = render;
}
bool XExtTable::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		// eraseBackground((HDC)wParam);
		return true;
	} if (msg == WM_SIZE) {
		if (mBuffer != NULL) delete mBuffer;
		mBuffer = NULL;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		SIZE sz = getClientSize();
		if (mBuffer == NULL) {
			mBuffer = XImage::create(mWidth, mHeight, 24);
		}
		SelectObject(memDc, mBuffer->getHBitmap());
		if (mModel != NULL) {
			// draw background
			eraseBackground(memDc);
			int hh = mModel->getHeaderHeight();
			drawData(memDc, 0, hh, sz.cx, sz.cy);
			drawHeader(memDc, mWidth, hh);
			// copy header & data
			BitBlt(dc, 0, 0, mWidth, hh, memDc, 0, 0, SRCCOPY);
			BitBlt(dc, 0, hh, sz.cx, sz.cy, memDc, 0, hh, SRCCOPY);
		} else {
			eraseBackground(dc);
		}
		DeleteObject(memDc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		int col = 0;
		int row = findCell((short)LOWORD(lParam), (short)HIWORD(lParam), &col);
		if (mSelectedRow != row) {
			mSelectedRow = row;
			InvalidateRect(mWnd, NULL, TRUE);
		}
		return true;
	}
	return XExtScroll::wndProc(msg, wParam, lParam, result);
}
void XExtTable::drawHeader( HDC dc, int w, int h) {
	if (mModel == NULL) return;
	HDC memDc = CreateCompatibleDC(dc);
	SelectObject(dc, getFont());
	SetBkMode(dc, TRANSPARENT);
	XImage *bg = mModel->getHeaderImage();
	if (bg && bg->getHBitmap()) {
		SelectObject(memDc, bg->getHBitmap());
		StretchBlt(dc, 0, 0, w, h, memDc, 0, 0, bg->getWidth(), bg->getHeight(), SRCCOPY);
	}
	DeleteObject(memDc);
	int x = -mHorBar->getPos();
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		char *txt = mModel->getHeaderText(i);
		if (txt) {
			RECT r = {x, 0, x + mColsWidth[i], h};
			DrawText(dc, txt, strlen(txt), &r, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
		}
		x += mColsWidth[i];
	}
	// draw split line
	x = x = -mHorBar->getPos();
	HGDIOBJ old = SelectObject(dc, mLinePen);
	for (int i = 0; i < mModel->getColumnCount() - 1; ++i) {
		x += mColsWidth[i];
		MoveToEx(dc, x, 2, NULL);
		LineTo(dc, x, h - 4);
	}
	if (mVerBar->isNeedShow()) {
		x = mWidth - mVerBar->getThumbSize();
		MoveToEx(dc, x, 2, NULL);
		LineTo(dc, x, h - 4);
	}
	SelectObject(dc, old);
}
void XExtTable::drawData( HDC dc, int x, int y,  int w, int h ) {
	int from = 0, to = 0;
	getVisibleRows(&from, &to);
	int y2 = -mVerBar->getPos();
	for (int i = 0; i < from; ++i) {
		y2 += mModel->getRowHeight(i);
	}
	SetBkMode(dc, TRANSPARENT);
	int ry = y2;
	for (int i = from; i <= to; ++i) {
		int x2 = -mHorBar->getPos();
		drawRow(dc, i, x + x2, y + y2, w - (x + x2), mModel->getRowHeight(i));
		y2 += mModel->getRowHeight(i);
	}
	drawGridLine(dc, from, to, ry + y);
}
void XExtTable::drawRow(HDC dc, int row, int x, int y, int w, int h ) {
	int rh = mModel->getRowHeight(row);
	if (mSelectedRow == row) {
		if (mSelectBgBrush == NULL) mSelectBgBrush = CreateSolidBrush(mSelectBgColor); 
		RECT r = {x + 1, y + 1, x + w - 1, y + h - 1};
		FillRect(dc, &r, mSelectBgBrush);
	}
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		if (mCellRender == NULL) {
			drawCell(dc, row, i, x + 1, y + 1, mColsWidth[i] - 1, rh - 1);
		} else {
			mCellRender->onDrawCell(dc, row, i, x + 1, y + 1, mColsWidth[i] - 1, rh - 1);
		}
		x += mColsWidth[i];
	}
}
void XExtTable::drawCell(HDC dc, int row, int col, int x, int y, int w, int h ) {
	char *txt = mModel->getCellData(row, col);
	int len = txt == NULL ? 0 : strlen(txt);
	RECT r = {x + 5, y, x + w - 5, y + h};
	DrawText(dc, txt, len, &r, DT_SINGLELINE | DT_VCENTER);
}
void XExtTable::drawGridLine( HDC dc, int from, int to, int y ) {
	SIZE sz = getClientSize();
	HGDIOBJ old = SelectObject(dc, mLinePen);
	int y2 = y;
	for (int i = from; i <= to; ++i) {
		y2 += mModel->getRowHeight(i);
		MoveToEx(dc, 0, y2, NULL);
		LineTo(dc, sz.cx, y2);
	}
	int x = -mHorBar->getPos();
	y = mModel->getHeaderHeight();
	for (int i = 0; i <= mModel->getColumnCount(); ++i) {
		if (i == mModel->getColumnCount()) --x;
		MoveToEx(dc, x, y, NULL);
		LineTo(dc, x, sz.cy + y);
		x += mColsWidth[i];
	}
	SelectObject(dc, old);
}
void XExtTable::getVisibleRows( int *from, int *to ) {
	*from = *to = 0;
	if (mModel == NULL) return;
	int y = -mVerBar->getPos();
	SIZE sz = getClientSize();
	sz.cy -= mModel->getHeaderHeight();
	for (int i = 0; i < mModel->getRowCount(); ++i) {
		y += mModel->getRowHeight(i);
		if (y > 0) {
			*from = i;
			break;
		}
	}
	for (int i = *from + 1; i < mModel->getRowCount(); ++i) {
		*to = i;
		if (y >= sz.cy) {
			break;
		}
	}
	if (*to < *from) *to = *from;
}
void XExtTable::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	if (mModel == NULL) return;
	bool hasHorBar = GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;

	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getThumbSize() : 0);
	mesureColumn(clientWidth, clientHeight);

	mDataSize = calcDataSize();
	mHorBar->setMaxAndPage(mDataSize.cx, clientWidth);
	mVerBar->setMaxAndPage(mDataSize.cy, clientHeight - mModel->getHeaderHeight());
	if (mHorBar->isNeedShow())
		WND_SHOW(mHorBar->getWnd());
	else
		WND_HIDE(mHorBar->getWnd());
	if (mVerBar->isNeedShow())
		WND_SHOW(mVerBar->getWnd());
	else
		WND_HIDE(mVerBar->getWnd());

	if (mHorBar->isNeedShow() != hasHorBar || mVerBar->isNeedShow() != hasVerBar)
		onMeasure(widthSpec, heightSpec);
}
SIZE XExtTable::calcDataSize() {
	SIZE sz = {0};
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		sz.cx += mColsWidth[i];
	}
	int rc = mModel->getRowCount();
	for (int i = 0; i < rc; ++i) {
		sz.cy += mModel->getRowHeight(i);
	}
	return sz;
}
SIZE XExtTable::getClientSize() {
	bool hasHorBar = GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getThumbSize() : 0);
	if (mModel != NULL) clientHeight -= mModel->getHeaderHeight();
	SIZE sz = {clientWidth, clientHeight};
	return sz;
}
void XExtTable::moveChildrenPos( int dx, int dy ) {
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTable::mesureColumn(int width, int height) {
	int widthAll = 0, weightAll = 0;
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		XExtTableModel::ColumnWidth cw = mModel->getColumnWidth(i);
		mColsWidth[i] = calcSize(cw.mWidthSpec, width | MS_ATMOST);
		widthAll += mColsWidth[i];
		weightAll += cw.mWeight;
	}
	int nw = 0;
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		XExtTableModel::ColumnWidth cw = mModel->getColumnWidth(i);
		if (width > widthAll && weightAll > 0) {
			mColsWidth[i] += cw.mWeight * (width - widthAll) / weightAll;
		}
		nw += mColsWidth[i];
	}
	// stretch last column
	if (nw < width && mModel->getColumnCount() > 0) {
		mColsWidth[mModel->getColumnCount() - 1] += width - nw;
	}
}
void XExtTable::onLayout( int width, int height ) {
	if (mModel == NULL) return;
	mHorBar->layout(0, mHeight - mHorBar->getThumbSize(), mHorBar->getPage(), mHorBar->getThumbSize());
	mVerBar->layout(mWidth - mVerBar->getThumbSize(), mModel->getHeaderHeight(), mVerBar->getThumbSize(), mVerBar->getPage());
}
int XExtTable::findCell( int x, int y, int *col ) {
	if (mModel == NULL) return -1;
	int row = -1;
	int y2 = -mVerBar->getPos() + mModel->getHeaderHeight();
	for (int i = 0; i < mModel->getRowCount(); ++i) {
		if (y2 <= y && y2 + mModel->getRowHeight(i) > y) {
			row = i;
			break;
		}
		y2 += mModel->getRowHeight(i);
	}
	if (row != -1 && col != NULL) {
		int x2 = -mHorBar->getPos();
		for (int i = 0; i < mModel->getColumnCount(); ++i) {
			if (x2 <= x && x2 + mColsWidth[i] > x) {
				*col = i;
				break;
			}
			x2 += mColsWidth[i];
		}
	}
	return row;
}
XExtTable::~XExtTable() {
	if (mBuffer) delete mBuffer;
	if (mSelectBgBrush) DeleteObject(mSelectBgBrush);
}
//----------------------------XExtEdit---------------------
XExtEdit::XExtEdit( XmlNode *node ) : XExtComponent(node) {
	mText = NULL;
	mCapacity = 0;
	mLen = 0;
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
	mReadOnly = AttrUtils::parseBool(mNode->getAttrValue("readOnly"));
	insertText(0, mNode->getAttrValue("text"));
	mCaretPen = CreatePen(PS_SOLID, 1, RGB(0xBF, 0x3E, 0xFF));
	mCaretShowing = false;
	mScrollPos = 0;
	mBorderPen = CreatePen(PS_SOLID, 1, RGB(0xAD, 0xAD, 0xAD));
	mFocusBorderPen = CreatePen(PS_SOLID, 1, RGB(0xEE, 0x30, 0xA7));
	mEnableBorder = false;
	mEnableShowCaret = true;
}
bool XExtEdit::wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	if (msg == WM_CHAR || msg == WM_IME_CHAR) {
		onChar(LOWORD(wParam));
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		SetCapture(mWnd);
		if (mEnableFocus) SetFocus(mWnd);
		onLButtonDown(wParam, (short)lParam, (short)(lParam >> 16));
		return true;
	} else if (msg == WM_LBUTTONUP) {
		ReleaseCapture();
		onLButtonUp(wParam, (short)lParam, (short)(lParam >> 16));
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		if (wParam & MK_LBUTTON) {
			onMouseMove((short)lParam, (short)(lParam >> 16));
		}
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		onPaint(dc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_KEYDOWN) {
		onKeyDown(wParam);
		return true;
	} else if (msg == WM_SETFOCUS) {
		if (mEnableShowCaret) SetTimer(mWnd, 0x1000, 500, NULL);
		return true;
	} else if (msg == WM_KILLFOCUS) {
		mCaretShowing = false;
		KillTimer(mWnd, 0x1000);
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	} else if (msg == WM_TIMER) {
		mCaretShowing = !mCaretShowing;
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	}
	return XComponent::wndProc(msg, wParam, lParam, result);
}
void XExtEdit::onChar( wchar_t ch ) {
	static char buf[4];
	buf[0] = (unsigned char)(ch >> 8);
	buf[1] = (unsigned char)ch;
	buf[2] = 0;
	if (mReadOnly) return;
	if (ch == 8) {// back
		back();
	} else if (ch == VK_TAB) { // tab
	} else if (ch == VK_RETURN) { // enter key
	} else if (ch > 31) {
		if (mBeginSelPos != mEndSelPos) {
			back(); // del selected text
		}
		if (buf[0] == 0) insertText(mInsertPos, &ch, 1);
		else insertText(mInsertPos, buf);
		++mInsertPos;
		mBeginSelPos = mEndSelPos = mInsertPos;
	}
	ensureVisible(mInsertPos);
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
}
void XExtEdit::onLButtonDown( int wParam, int x, int y ) {
	mCaretShowing = true;
	mInsertPos = getPosAt(x, y);
	if (wParam & MK_SHIFT) {
		mEndSelPos =  mInsertPos;
	} else {
		mBeginSelPos = mEndSelPos =  mInsertPos;
	}
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
}
void XExtEdit::onLButtonUp( int keyState, int x, int y ) {
	if (mReadOnly) return;
	// mEndSelPos = getPosAt(x, y);
}
void XExtEdit::onMouseMove(int x, int y) {
	mInsertPos = mEndSelPos = getPosAt(x, y);
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
}
void XExtEdit::onPaint( HDC hdc ) {
	// draw select range background color
	drawSelRange(hdc, mBeginSelPos, mEndSelPos);
	HFONT font = getFont();
	SelectObject(hdc, font);
	SetBkMode(hdc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(hdc, mAttrColor);
	RECT r = {mScrollPos, 0, mWidth - mScrollPos, mHeight};
	DrawTextW(hdc, mText, mLen, &r, DT_SINGLELINE | DT_VCENTER);
	POINT pt = {0, 0};
	if (mCaretShowing && getXYAt(mInsertPos, &pt)) {
		SelectObject(hdc, mCaretPen);
		MoveToEx(hdc, pt.x, 2, NULL);
		LineTo(hdc, pt.x, mHeight - 4);
	}
	// draw border
	if (mEnableBorder) {
		bool hasFocus = mWnd == GetFocus();
		SelectObject(hdc, (hasFocus ? mFocusBorderPen : mBorderPen));
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		RoundRect(hdc, 0, 0, mWidth - 1, mHeight - 1, mAttrRoundConerX, mAttrRoundConerX);
	}
}
void XExtEdit::drawSelRange( HDC hdc, int begin, int end ) {
	static HBRUSH bg = 0;
	if (bg == 0) bg = CreateSolidBrush(RGB(0xad, 0xd6, 0xff));
	if(begin == end || begin < 0 || end < 0) return;
	if (end < begin) {int tmp = begin; begin = end; end = tmp;}
	POINT bp, ep;
	getXYAt(begin, &bp);
	getXYAt(end, &ep);
	RECT r = {bp.x, 0, ep.x, mHeight};
	FillRect(hdc, &r, bg);
}
void XExtEdit::onKeyDown( int key ) {
	int ctrl = GetAsyncKeyState(VK_CONTROL) < 0;
	if (key == 'V' && ctrl && !mReadOnly) { // ctrl + v
		paste();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	} else if (key == 46 && !mReadOnly) { // del
		del();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	} else if (key >= VK_END && key <= VK_DOWN) {
		move(key);
	} else if (key == 'A' && ctrl) { // ctrl + A
		mBeginSelPos = 0;
		mEndSelPos = mLen;
		mInsertPos = mLen;
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	} else if (key == 'C' && ctrl) { // ctrl + C
		copy();
	} else if (key == 'X' && ctrl && !mReadOnly) { // ctrl + X
		if (mBeginSelPos == mEndSelPos) return;
		copy();
		back();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	}
}
void XExtEdit::insertText(int pos, char *txt) {
	if (txt == NULL) return;
	int slen = strlen(txt);
	int len = MultiByteToWideChar(CP_ACP, 0, txt, slen, NULL, 0);
	wchar_t *wb = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, txt, slen, wb, len);
	insertText(pos, wb, len);
	delete[] wb;
}
void XExtEdit::insertText(int pos, wchar_t *txt, int len) {
	if (pos < 0 || pos > mLen) pos = mLen;
	if (len <= 0) return;
	if (mText == NULL) {
		mCapacity = 100;
		mText = new wchar_t[mCapacity];
	}
	if (mLen + len >= mCapacity - 10) {
		wchar_t *old = mText;
		mCapacity *= 2;
		mText = new wchar_t[mCapacity];
		memcpy(mText, old, mLen * sizeof(wchar_t));
		delete [] old;
	}
	for (int i = mLen - 1; i >= pos; --i) {
		mText[i + len] = mText[i];
	}
	memcpy(&mText[pos], txt, len * sizeof(wchar_t));
	mLen += len;
}
int XExtEdit::deleteText(int pos, int len) {
	if (pos < 0 || pos >= mLen || len <= 0) return 0;
	if (len + pos > mLen) len = mLen - pos;
	for (; pos < mLen; ++pos) {
		mText[pos] = mText[pos + len];
	}
	mLen -= len;
	return len;
}
int XExtEdit::getPosAt(int x, int y) {
	HDC hdc = GetDC(mWnd);
	HGDIOBJ old = SelectObject(hdc, getFont());
	int k = 0;
	x -= mScrollPos;
	for (int i = 0; i < mLen; ++i, ++k) {
		SIZE sz;
		GetTextExtentPoint32W(hdc, mText, i + 1, &sz);
		if (sz.cx > x) {
			SIZE nsz;
			GetTextExtentPoint32W(hdc, &mText[i], 1, &nsz);
			int nx = sz.cx - nsz.cx / 2;
			if (x >= nx) ++k;
			break;
		}
	}
	SelectObject(hdc, old);
	ReleaseDC(mWnd, hdc);
	return k;
}
BOOL XExtEdit::getXYAt(int pos, POINT *pt) {
	if (pos > mLen || pos < 0) {
		pt->x = pt->y = 0;
		return false;
	}
	HDC hdc = GetDC(mWnd);
	HGDIOBJ old = SelectObject(hdc, getFont());
	SIZE sz;
	GetTextExtentPoint32W(hdc, mText, pos, &sz);
	pt->x = sz.cx + mScrollPos;
	pt->y = 0;
	SelectObject(hdc, old);
	ReleaseDC(mWnd, hdc);
	return true;
}
void XExtEdit::move( int key ) {
	int sh = GetAsyncKeyState(VK_SHIFT) < 0;
	int old = mInsertPos;
	switch (key) {
	case VK_LEFT: 
		if (mInsertPos > 0) --mInsertPos;
		break;
	case VK_RIGHT:
		if (mInsertPos < mLen) ++mInsertPos;
		break;
	case VK_HOME:
		mInsertPos = 0;
		break;
	case VK_END:
		mInsertPos = mLen;
		break;
	}
	if (old == mInsertPos)
		return;
	if (! sh) mBeginSelPos = mInsertPos;
	mEndSelPos = mInsertPos;
	ensureVisible(mInsertPos);
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
}
void XExtEdit::ensureVisible(int pos) {
	SIZE sz;
	HDC dc = GetDC(mWnd);
	HGDIOBJ old = SelectObject(dc, getFont());
	GetTextExtentPoint32W(dc, mText, pos, &sz);
	SelectObject(dc, old);
	ReleaseDC(mWnd, dc);
	sz.cx += mScrollPos;
	if (sz.cx >= 0 && sz.cx < mWidth) return; // it is already visible
	if (sz.cx < 0) {
		mScrollPos -= sz.cx;
	} else {
		mScrollPos -= sz.cx - mWidth + 2;
	}
}
void XExtEdit::back() {
	int len, delLen = 0;
	if (mBeginSelPos != mEndSelPos) {
		int begin = min(mBeginSelPos, mEndSelPos);
		int end = max(mBeginSelPos, mEndSelPos);
		delLen = end - begin;
		len = deleteText(begin, delLen);
		mInsertPos = begin;
		mBeginSelPos = mEndSelPos = begin;
		mCaretShowing = true;
	} else {
		if (mInsertPos > 0) {
			delLen = 1;
			if (mInsertPos > 1 && mText[mInsertPos - 1] == '\n' && mText[mInsertPos - 2] == '\r')
				delLen = 2;
			len = deleteText(mInsertPos - delLen, delLen);
			mInsertPos -= delLen;
			mBeginSelPos = mEndSelPos = mInsertPos;
			mCaretShowing = true;
		}
	}
}
void XExtEdit::del() {
	if (mBeginSelPos != mEndSelPos) {
		int bg = mBeginSelPos < mEndSelPos ? mBeginSelPos : mEndSelPos;
		int ed = mBeginSelPos > mEndSelPos ? mBeginSelPos : mEndSelPos;
		deleteText(bg, ed - bg);
		mInsertPos = bg;
		mBeginSelPos = mEndSelPos = bg;
	} else if (mInsertPos >= 0 && mInsertPos < mLen) {
		deleteText(mInsertPos, 1);
	}
}
void XExtEdit::paste() {
	OpenClipboard(mWnd);
	if (IsClipboardFormatAvailable(CF_TEXT)) {
		if (mBeginSelPos != mEndSelPos) {
			del(); // del select
		}
		HANDLE hdl = GetClipboardData(CF_TEXT);
		char *buf=(char*)GlobalLock(hdl);
		int len = MultiByteToWideChar(CP_ACP, 0, buf, strlen(buf), NULL, 0);
		wchar_t *wb = new wchar_t[len + 1];
		MultiByteToWideChar(CP_ACP, 0, buf, strlen(buf), wb, len);
		GlobalUnlock(hdl);
		insertText(mInsertPos, wb, len);
		delete[] wb;
		mBeginSelPos = mInsertPos;
		mInsertPos += len;
		mEndSelPos = mInsertPos;
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	}
	CloseClipboard();
}
void XExtEdit::copy() {
	if (mBeginSelPos == mEndSelPos) return;
	int bg = mBeginSelPos < mEndSelPos ? mBeginSelPos : mEndSelPos;
	int ed = mBeginSelPos > mEndSelPos ? mBeginSelPos : mEndSelPos;
	int len = WideCharToMultiByte(CP_ACP, 0, mText + bg, ed - bg, NULL, 0, NULL, NULL);
	HANDLE hd = GlobalAlloc(GHND, len + 1);
	char *buf = (char *)GlobalLock(hd);
	WideCharToMultiByte(CP_ACP, 0, mText + bg, ed - bg, buf, len, NULL, NULL);
	buf[len] = 0;
	GlobalUnlock(hd);

	OpenClipboard(mWnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hd);
	CloseClipboard();
}
void XExtEdit::setEnableBorder( bool enable ) {
	mEnableBorder = enable;
}
void XExtEdit::setReadOnly( bool r ) {
	mReadOnly = r;
}
void XExtEdit::setEnableShowCaret( bool enable ) {
	mEnableShowCaret = enable;
}

//----------------------------XExtList---------------------
XExtList::XExtList( XmlNode *node ) : XExtScroll(node) {
	mModel = NULL;
	mItemRender = NULL;
	mBuffer = NULL;
	mDataSize.cx = mDataSize.cy = 0;
	mMouseTrackItem = -1;
	mSelectBgBrush = CreateSolidBrush(RGB(0xA2, 0xB5, 0xCD));
}
XExtList::~XExtList() {
	if (mBuffer) delete mBuffer;
	DeleteObject(mSelectBgBrush);
}
bool XExtList::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		// eraseBackground((HDC)wParam);
		return true;
	} if (msg == WM_SIZE) {
		if (mBuffer != NULL) delete mBuffer;
		mBuffer = NULL;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		SIZE sz = getClientSize();
		if (mBuffer == NULL) {
			mBuffer = XImage::create(mWidth, mHeight, 24);
		}
		SelectObject(memDc, mBuffer->getHBitmap());
		if (mModel != NULL) {
			// draw background
			eraseBackground(memDc);
			drawData(memDc, 0, 0, sz.cx, sz.cy);
			// copy data
			BitBlt(dc, 0, 0, sz.cx, sz.cy, memDc, 0, 0, SRCCOPY);
		} else {
			eraseBackground(dc);
		}
		DeleteObject(memDc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		int idx = findItem((short)LOWORD(lParam), (short)HIWORD(lParam));
		SendMessage(mWnd, WM_EXT_LIST_CLICK_ITEM, idx, 0);
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		updateTrackItem((short)LOWORD(lParam), (short)HIWORD(lParam));
		return true;
	} else if (msg == WM_MOUSEHWHEEL || msg == WM_MOUSEWHEEL || msg == WM_MOUSEWHEEL_BUBBLE) {
		XExtScroll::wndProc(msg, wParam, lParam, result);
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(mWnd, &pt);
		updateTrackItem(pt.x, pt.y);
		return true;
	}
	return XExtScroll::wndProc(msg, wParam, lParam, result);
}
SIZE XExtList::calcDataSize() {
	SIZE cs = getClientSize();
	SIZE sz = {cs.cx, 0};
	int rc = mModel ? mModel->getItemCount() : 0;
	for (int i = 0; i < rc; ++i) {
		sz.cy += mModel->getItemHeight(i);
	}
	return sz;
}
void XExtList::moveChildrenPos( int dx, int dy ) {
	InvalidateRect(mWnd, NULL, TRUE);
}
SIZE XExtList::getClientSize() {
	bool hasHorBar = GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getThumbSize() : 0);
	SIZE sz = {clientWidth, clientHeight};
	return sz;
}
void XExtList::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	if (mModel == NULL) return;
	bool hasHorBar = GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;

	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getThumbSize() : 0);

	mDataSize = calcDataSize();
	mHorBar->setMaxAndPage(mDataSize.cx, clientWidth);
	mVerBar->setMaxAndPage(mDataSize.cy, clientHeight);
	if (mHorBar->isNeedShow())
		WND_SHOW(mHorBar->getWnd());
	else
		WND_HIDE(mHorBar->getWnd());
	if (mVerBar->isNeedShow())
		WND_SHOW(mVerBar->getWnd());
	else
		WND_HIDE(mVerBar->getWnd());

	if (mHorBar->isNeedShow() != hasHorBar || mVerBar->isNeedShow() != hasVerBar)
		onMeasure(widthSpec, heightSpec);
}
void XExtList::onLayout( int width, int height ) {
	if (mModel == NULL) return;
	mHorBar->layout(0, mHeight - mHorBar->getThumbSize(), mHorBar->getPage(), mHorBar->getThumbSize());
	mVerBar->layout(mWidth - mVerBar->getThumbSize(), 0, mVerBar->getThumbSize(), mVerBar->getPage());
}
void XExtList::drawData( HDC memDc, int x, int y, int w, int h ) {
	if (mModel == NULL) return;
	SelectObject(memDc, getFont());
	SetBkMode(memDc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(memDc, mAttrColor);

	int from = 0, to = 0;
	getVisibleRows(&from, &to);
	y += -mVerBar->getPos();
	for (int i = 0; i < from; ++i) {
		y += mModel->getItemHeight(i);
	}
	x += -mHorBar->getPos();
	for (int i = from; i <= to; ++i) {
		int rh = mModel->getItemHeight(i);
		if (mModel->isMouseTrack() && mMouseTrackItem == i) {
			// draw select row background
			RECT r = {x, y, w - x, y + rh};
			FillRect(memDc, &r, mSelectBgBrush);
		}
		if (mItemRender == NULL) {
			drawItem(memDc, i, x, y, w - x, rh);
		} else {
			mItemRender->onDrawItem(memDc, i, x, y, w - x, rh);
		}
		y += rh;
	}
}
void XExtList::drawItem( HDC dc, int item, int x, int y, int w, int h ) {
	XListModel::ItemData *data = mModel->getItemData(item);
	if (data != NULL && data->mText != NULL) {
		RECT r = {x + 10, y, x + w - 10, y + h};
		DrawText(dc, data->mText, strlen(data->mText), &r, DT_SINGLELINE | DT_VCENTER);
	}
}
void XExtList::getVisibleRows( int *from, int *to ) {
	*from = *to = 0;
	if (mModel == NULL) return;
	int y = -mVerBar->getPos();
	SIZE sz = getClientSize();
	for (int i = 0; i < mModel->getItemCount(); ++i) {
		y += mModel->getItemHeight(i);
		if (y > 0) {
			*from = i;
			break;
		}
	}
	for (int i = *from + 1; i < mModel->getItemCount(); ++i) {
		*to = i;
		if (y >= sz.cy) {
			break;
		}
	}
	if (*to < *from) *to = *from;
}
void XExtList::setModel( XListModel *model ) {
	mModel = model;
}
XListModel *XExtList::getModel() {
	return mModel;
}
void XExtList::setItemRender( ItemRender *render ) {
	mItemRender = render;
}
int XExtList::findItem( int x, int y ) {
	if (mModel == NULL) return -1;
	int row = -1;
	int y2 = -mVerBar->getPos();
	for (int i = 0; i < mModel->getItemCount(); ++i) {
		if (y2 <= y && y2 + mModel->getItemHeight(i) > y) {
			row = i;
			break;
		}
		y2 += mModel->getItemHeight(i);
	}
	return row;
}
void XExtList::updateTrackItem( int x, int y ) {
	if (mModel == NULL || !mModel->isMouseTrack())
		return;
	int idx = findItem(x, y);
	XListModel::ItemData *item = mModel->getItemData(idx);
	if (idx != -1 && item != NULL && item->mSelectable) {
		mMouseTrackItem = idx;
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	}
}
//------------------------------XExtComboBox--------------------
XExtComboBox::XExtComboBox( XmlNode *node ) : XExtComponent(node) {
	mEditNode = new XmlNode(NULL, mNode);
	mPopupNode = new XmlNode(NULL, mNode);
	mListNode = new XmlNode(NULL, mPopupNode);
	mEdit = new XExtEdit(mEditNode);
	mPopup = new XExtPopup(mPopupNode);
	// mPopup->setBgColor(RGB(0xF5, 0xFF, 0xFA));
	mPopupNode->setComponent(mPopup);
	mList = new XExtList(mListNode);
	mListNode->setComponent(mList);
	mList->setBgColor(RGB(0xF5, 0xFF, 0xFA));
	mPopup->setListener(this);
	mList->setListener(this);
	mList->setEnableFocus(false);

	mArrowRect.left = mArrowRect.top = 0;
	mArrowRect.right = mArrowRect.bottom = 0;
	mAttrArrowSize.cx = mAttrArrowSize.cy = 0;
	AttrUtils::parseArraySize(mNode->getAttrValue("arrowSize"), (int *)&mAttrArrowSize, 2);
	mAttrPopupSize.cx = mAttrPopupSize.cy = 0;
	AttrUtils::parseArraySize(mNode->getAttrValue("popupSize"), (int *)&mAttrPopupSize, 2);
	mEdit->setReadOnly(AttrUtils::parseBool(mNode->getAttrValue("readOnly")));
	mEnableEditor = false;

	mArrowNormalImage = XImage::load(mNode->getAttrValue("arrowNormal"));
	mArrowDownImage = XImage::load(mNode->getAttrValue("arrowDown"));
	mBoxRender = NULL;
	mPoupShow = false;

	XImage *track = XImage::load(mNode->getAttrValue("hbarTrack"));
	XImage *thumb = XImage::load(mNode->getAttrValue("hbarThumb"));
	if (track != NULL && thumb != NULL) 
		mList->getHorBar()->setImages(track, thumb);
	track = XImage::load(mNode->getAttrValue("vbarTrack"));
	thumb = XImage::load(mNode->getAttrValue("vbarThumb"));
	if (track != NULL && thumb != NULL) 
		mList->getVerBar()->setImages(track, thumb);
	mSelectItem = -1;
}
bool XExtComboBox::onEvent( XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret ) {
	if (msg == WM_EXT_LIST_CLICK_ITEM) {
		mSelectItem = wParam;
		mPopup->close();
		mPoupShow = false;
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
		return true;
	} else if (msg == WM_EXT_POPUP_CLOSED) {
		mPoupShow = false;
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
		return true;
	}
	return false;
}
bool XExtComboBox::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		if (! mEnableEditor) {
			if (mBoxRender != NULL)
				mBoxRender->onDrawBox(dc, 0, 0, mArrowRect.left, mHeight);
			else
				drawBox(dc, 0, 0, mArrowRect.left, mHeight);
		}
		// draw arrow
		XImage *img = mPoupShow ? mArrowDownImage : mArrowNormalImage;
		if (img != NULL && img->getHBitmap() != NULL) {
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, img->getHBitmap());
			if (img->hasAlphaChannel())  {
				BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
				AlphaBlend(dc, mArrowRect.left, mArrowRect.top, (mArrowRect.right - mArrowRect.left), 
					(mArrowRect.bottom - mArrowRect.top), memDc, 0, 0, img->getWidth(), img->getHeight(), bf);
			} else {
				StretchBlt(dc, mArrowRect.left, mArrowRect.top, (mArrowRect.right - mArrowRect.left), 
					(mArrowRect.bottom - mArrowRect.top), memDc, 0, 0, img->getWidth(), img->getHeight(), SRCCOPY);
			}
			DeleteObject(memDc);
		}
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		SetCapture(mWnd);
		return true;
	} else if (msg == WM_LBUTTONUP) {
		ReleaseCapture();
		POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mEnableEditor) r = mArrowRect;
		if (PtInRect(&r, pt)) {
			openPopup();
		}
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
void XExtComboBox::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	int aw = calcSize(mAttrArrowSize.cx, mMesureWidth | MS_ATMOST);
	int ah = calcSize(mAttrArrowSize.cy, mMesureHeight | MS_ATMOST);
	mArrowRect.left = mMesureWidth - aw;
	mArrowRect.right = mMesureWidth;
	mArrowRect.top = (mMesureHeight - ah) / 2;
	mArrowRect.bottom = mArrowRect.top + ah;
	mEdit->onMeasure((mMesureWidth - aw) | MS_FIX, mMesureHeight | MS_FIX);
	int pw = calcSize(mAttrPopupSize.cx, mMesureWidth | MS_ATMOST);
	int ph = calcSize(mAttrPopupSize.cy, mMesureHeight | MS_ATMOST);
	mPopup->onMeasure(pw | MS_FIX, ph | MS_FIX);
	mList->onMeasure(pw | MS_FIX, ph | MS_FIX);
}
void XExtComboBox::onLayout( int width, int height ) {
	mEdit->layout(0, 0, mEdit->getMesureWidth(), mEdit->getMesureHeight());
	mPopup->layout(0, 0, mPopup->getMesureWidth(), mPopup->getMesureHeight());
	mList->layout(0, 0, mList->getMesureWidth(), mList->getMesureHeight());
}
void XExtComboBox::createWnd() {
	XExtComponent::createWnd();
	mEdit->createWnd();
	mPopup->createWnd();
	mList->createWnd();
	WND_HIDE(mEdit->getWnd());
}
void XExtComboBox::setEnableEditor( bool enable ) {
	mEnableEditor = enable;
	if (enable) WND_SHOW(mEdit->getWnd());
	else WND_HIDE(mEdit->getWnd());
}
XExtList * XExtComboBox::getExtList() {
	return mList;
}
void XExtComboBox::setBoxRender( BoxRender *r ) {
	mBoxRender = r;
}
void XExtComboBox::drawBox( HDC dc, int x, int y, int w, int h ) {
	if (mSelectItem == -1 || mList->getModel() == NULL)
		return;
	XListModel::ItemData *data = mList->getModel()->getItemData(mSelectItem);
	if (data == NULL || data->mText == NULL)
		return;
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());
	RECT r = {x + 10, y, x + w, y + h};
	DrawText(dc, data->mText, strlen(data->mText), &r, DT_VCENTER | DT_SINGLELINE);
}
void XExtComboBox::openPopup() {
	POINT pt = {0, mHeight};
	ClientToScreen(mWnd, &pt);
	mPoupShow = true;
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
	mPopup->showNoSize(pt.x, pt.y);
}
int XExtComboBox::getSelectItem() {
	return mSelectItem;
}
XExtComboBox::~XExtComboBox() {
	delete mEdit;
	delete mPopup;
	delete mList;
	delete mEditNode;
	delete mPopupNode;
	delete mListNode;
}
