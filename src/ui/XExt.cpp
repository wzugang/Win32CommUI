#include "XExt.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include "XComponent.h"
#include "XmlParser.h"
#include "UIFactory.h"

// AlphaBlend function in here
#pragma   comment(lib,"msimg32.lib")

extern void MyRegisterClass(HINSTANCE ins, const char *className);

//--------------------XExtComponent-------------------------------------
XExtComponent::XExtComponent(XmlNode *node) : XComponent(node) {
	mBgImageForParnet = NULL;
	mEnableFocus = AttrUtils::parseBool(mNode->getAttrValue("enableFocus"), false);
	mMemBuffer = NULL;
	mEnableMemBuffer = false;
	_mMouseTrack = false;
}

bool XExtComponent::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		if (! mEnableMemBuffer) {
			eraseBackground((HDC)wParam);
		}
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		if (mEnableMemBuffer) {
			HDC memDc = CreateCompatibleDC(dc);
			if (mMemBuffer == NULL) {
				mMemBuffer = XImage::create(mWidth, mHeight);
			}
			SelectObject(memDc, mMemBuffer->getHBitmap());
			onPaint(memDc, &ps.rcPaint);
			RECT *r = &ps.rcPaint;
			BitBlt(dc, r->left, r->top, r->right-r->left, r->bottom-r->top, memDc, r->left, r->top, SRCCOPY);
			DeleteObject(memDc);
		} else {
			onPaint(dc, &ps.rcPaint);
		}
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_KEYDOWN) {
		onKeyDown((int)wParam);
		return true;
	} else if (msg == WM_KEYUP) {
		onKeyUp((int)wParam);
		return true;
	} else if (msg == WM_CHAR || msg == WM_IME_CHAR) {
		onChar(wchar_t(wParam));
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		onMouseMove(x, y, vkeys);
		if (! _mMouseTrack) {
			_mMouseTrack = false;
			TRACKMOUSEEVENT tme = {0};
			tme.cbSize = sizeof(tme);
			tme.dwFlags = TME_LEAVE | TME_HOVER;
			tme.hwndTrack = mWnd;
			tme.dwHoverTime = 10;
			TrackMouseEvent(&tme);
		}
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		if (mEnableFocus) {
			SetCapture(mWnd);
		}
		onLButtonDown(x, y, vkeys);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		if (mEnableFocus) {
			ReleaseCapture();
		}
		onLButtonUp(x, y, vkeys);
		return true;
	} else if (msg == WM_LBUTTONDBLCLK) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		onLButtonDbClick(x, y, vkeys);
		return true;
	} else if (msg == WM_RBUTTONDOWN) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		onRButtonDown(x, y, vkeys);
		return true;
	} else if (msg == WM_RBUTTONUP) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		onRButtonUp(x, y, vkeys);
		return true;
	} else if (msg == WM_RBUTTONDBLCLK) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		onRButtonDbClick(x, y, vkeys);
		return true;
	} else if (msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		onWheel(x, y, HIWORD(wParam), LOWORD(wParam));
		return true;
	} else if (msg == WM_MOUSEHOVER) {
		short x =  (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		int vkeys = (int)wParam;
		return true;
	} else if (msg == WM_MOUSELEAVE) {
		_mMouseTrack = false;
		onMouseLeave();
		return true;
	}
	// WM_CAPTURECHANGED
	return XComponent::wndProc(msg, wParam, lParam, result);
}

void XExtComponent::layout( int x, int y, int width, int height ) {
	XComponent::layout(x, y, width, height);
	if (mBgImageForParnet != NULL) {
		delete mBgImageForParnet;
	}
	mBgImageForParnet = NULL;
	if (mMemBuffer != NULL) {
		delete mMemBuffer;
	}
	mMemBuffer = NULL;
}
void XExtComponent::eraseBackground(HDC dc) {
	if (mBgImage == NULL && mBgImageForParnet == NULL && (mAttrFlags & AF_BG_COLOR) == 0) {
		HDC memDc = CreateCompatibleDC(dc);
		HWND parent = getParentWnd();
		mBgImageForParnet = XImage::create(mWidth, mHeight, 24);
		SelectObject(memDc, mBgImageForParnet->getHBitmap());
		HDC dc = GetDC(parent);
		BitBlt(memDc, 0, 0, mWidth, mHeight, dc, mX, mY, SRCCOPY);
		DeleteObject(memDc);
		ReleaseDC(parent, dc);
	}
	if (mAttrFlags & AF_BG_COLOR) {
		if (mBgColorBrush == NULL) {
			mBgColorBrush = CreateSolidBrush(mAttrBgColor & 0xffffff);
		}
		RECT rc = {0, 0, mWidth, mHeight};
		FillRect(dc, &rc, mBgColorBrush);
	}
	XImage *bg = mBgImageForParnet != NULL ? mBgImageForParnet : mBgImage;
	if (bg != NULL) {
		bg->draw(dc, 0, 0, mWidth, mHeight);
	}
}
XExtComponent::~XExtComponent() {
	if (mBgImageForParnet) {
		delete mBgImageForParnet;
		mBgImageForParnet = NULL;
	}
	if (mMemBuffer != NULL) {
		delete mMemBuffer;
		mMemBuffer = NULL;
	}
}
void XExtComponent::setEnableFocus( bool enable ) {
	mEnableFocus = enable;
}

//------------------XExtEmptyComponent---------------------------------
XExtEmptyComponent::XExtEmptyComponent(XmlNode *node) : XExtComponent(node) {
}
bool XExtEmptyComponent::wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		BeginPaint(mWnd, &ps);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_ERASEBKGND) {
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
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
	memset(mStateImages, 0, sizeof(mStateImages));
	for (int i = 0; i < mNode->getAttrsCount(); ++i) {
		XmlNode::Attr *attr = mNode->getAttr(i);
		if (strcmp(attr->mName, "normalImage") == 0) {
			mStateImages[STATE_IMG_NORMAL] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "hoverImage") == 0) {
			mStateImages[STATE_IMG_HOVER] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "pushImage") == 0) {
			mStateImages[STATE_IMG_PUSH] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "disableImage") == 0) {
			mStateImages[STATE_IMG_DISABLE] = XImage::load(attr->mValue);
		}
	}
}

bool XExtButton::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};

		XImage *cur = mStateImages[getStateImage()];
		if (cur != NULL)
			cur->draw(dc, 0, 0, mWidth, mHeight);
		
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
			SendMessage(mWnd, MSG_COMMAND, 0, 0);
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

		StateImage bi = getStateImage();
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
		if (bi != getStateImage()) {
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

XExtButton::StateImage XExtButton::getStateImage() {
	if (GetWindowLong(mWnd, GWL_STYLE) & WS_DISABLED)
		return STATE_IMG_DISABLE;
	if (mIsMouseDown && ! mIsMouseLeave) {
		return STATE_IMG_PUSH;
	}
	if (!mIsMouseDown && mIsMouseMoving) {
		return STATE_IMG_HOVER;
	}
	if (mIsMouseLeave) {
		return STATE_IMG_NORMAL;
	}

	return STATE_IMG_NORMAL;
}
//-------------------XExtOption-----------------------------------
XExtOption::XExtOption( XmlNode *node ) : XExtButton(node) {
	mAutoSelect = true;
	mIsSelect = false;
	char *s = mNode->getAttrValue("selectImage");
	if (s != NULL) {
		mStateImages[BTN_IMG_SELECT] = XImage::load(s);
	}
	s = mNode->getAttrValue("autoSelect");
	if (s != NULL) mAutoSelect = AttrUtils::parseBool(s);
}

bool XExtOption::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_LBUTTONUP) {
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mAutoSelect && mIsMouseDown && PtInRect(&r, pt)) {
			mIsSelect = ! mIsSelect;
		}
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

void XExtOption::setAutoSelect(bool autoSelect) {
	mAutoSelect = autoSelect;
}

XExtOption::StateImage XExtOption::getStateImage() {
	if (GetWindowLong(mWnd, GWL_STYLE) & WS_DISABLED)
		return STATE_IMG_DISABLE;
	if (mIsMouseDown && ! mIsMouseLeave) {
		return STATE_IMG_PUSH;
	}
	if (!mIsMouseDown && mIsMouseMoving) {
		return STATE_IMG_HOVER;
	}
	if (mIsSelect) {
		return StateImage(BTN_IMG_SELECT);
	}
	if (mIsMouseLeave) {
		return STATE_IMG_NORMAL;
	}

	return STATE_IMG_NORMAL;
}

//-------------------XExtCheckBox-----------------------------------
XExtCheckBox::XExtCheckBox( XmlNode *node ) : XExtOption(node) {
}

bool XExtCheckBox::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};
		XImage *cur = mStateImages[getStateImage()];
		if (cur != NULL) {
			int y = (mHeight - cur->getHeight()) / 2;
			cur->draw(dc, 0, y, cur->getWidth(), cur->getHeight());
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
//-----------------------XExtIconButton----------------------------
XExtIconButton::XExtIconButton(XmlNode *node) : XExtOption(node) {
	memset(mAttrIconRect, 0, sizeof(mAttrIconRect));
	memset(mAttrTextRect, 0, sizeof(mAttrTextRect));
	AttrUtils::parseArraySize(mNode->getAttrValue("iconRect"), mAttrIconRect, 4);
	AttrUtils::parseArraySize(mNode->getAttrValue("textRect"), mAttrTextRect, 4);
	mIcon = XImage::load(mNode->getAttrValue("icon"));
}
bool XExtIconButton::wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		XImage *cur = mStateImages[getStateImage()];
		if (cur != NULL) {
			cur->draw(dc, 0, 0, mWidth, mHeight);
		}
		if (mAttrFlags & AF_COLOR) {
			SetTextColor(dc, mAttrColor);
		}
		SetBkMode(dc, TRANSPARENT);
		SelectObject(dc, getFont());
		RECT r = getRectBy(mAttrTextRect);
		DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		if (mIcon != NULL) {
			r = getRectBy(mAttrIconRect);
			mIcon->draw(dc, r.left, r.top, r.right - r.left, r.bottom - r.top);
		}
		EndPaint(mWnd, &ps);
		return true;
	}
	return XExtOption::wndProc(msg, wParam, lParam, result);
}

RECT XExtIconButton::getRectBy(int *attr) {
	RECT r = {0};
	r.left = calcSize(attr[0], (mWidth - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST);
	r.top = calcSize(attr[1], (mHeight - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
	r.right = calcSize(attr[2], (mWidth - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST);
	r.bottom = calcSize(attr[3], (mHeight - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
	r.left += mAttrPadding[0];
	r.top += mAttrPadding[1];
	r.right += r.left;
	r.bottom += r.top;
	return r;
}

//-------------------XExtScroll-----------------------------------
XExtScrollBar::XExtScrollBar( XmlNode *node, bool horizontal ) : XExtComponent(node) {
	mHorizontal = horizontal;
	memset(&mThumbRect, 0, sizeof(mThumbRect));
	mTrack = XImage::load(mNode->getAttrValue("track"));
	mThumb = XImage::load(mNode->getAttrValue("thumb"));
	mPos = 0;
	mMax = 0;
	mPage = 0;
	mPressed = false;
	mMouseX = mMouseY = 0;
}
bool XExtScrollBar::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		if (mMemBuffer == NULL) mMemBuffer = XImage::create(mWidth, mHeight);
		SelectObject(memDc, mMemBuffer->getHBitmap());
		if (mTrack != NULL) {
			mTrack->draw(memDc, 0, 0, mWidth, mHeight);
		}
		if (mThumb != NULL) {
			mThumb->draw(memDc, mThumbRect.left, mThumbRect.top, mThumbRect.right - mThumbRect.left, 
				mThumbRect.bottom - mThumbRect.top);
		}
		BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
		DeleteObject(memDc);
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
int XExtScrollBar::getPos() {
	return mPos;
}
void XExtScrollBar::setPos( int pos ) {
	if (pos < 0) pos = 0;
	else if (pos > mMax - mPage) pos = mMax - mPage;
	mPos = pos;
	calcThumbInfo();
}
int XExtScrollBar::getPage() {
	return mPage;
}
int XExtScrollBar::getMax() {
	return mMax;
}
void XExtScrollBar::setMaxAndPage( int maxn, int page ) {
	if (maxn < 0) maxn = 0;
	mMax = maxn;
	if (page < 0) page = 0;
	mPage = page;
	if (maxn <= page) mPos = 0;
	else if (mPos > mMax - mPage) mPos = mMax - mPage;
	calcThumbInfo();
}
void XExtScrollBar::calcThumbInfo() {
	if (mMax <= 0 || mPage <= 0 || mPage >= mMax) {
		if (mHorizontal) {
			mThumbRect.left = mThumbRect.right = 0;
		} else {
			mThumbRect.top = mThumbRect.bottom = 0;
		}
		return;
	}
	int sz = mPage * mPage / mMax;
	int a = mPos * (mPage - sz) / (mMax - mPage);
	if (mHorizontal) {
		mThumbRect.left = a;
		mThumbRect.right = mThumbRect.left + sz;
	} else {
		mThumbRect.top = a;
		mThumbRect.bottom = mThumbRect.top + sz;
	}
}
bool XExtScrollBar::isNeedShow() {
	return mMax > mPage;
}
int XExtScrollBar::getThumbSize() {
	if (mHorizontal) {
		return mThumbRect.bottom;
	}
	return mThumbRect.right;
}
void XExtScrollBar::setThumbSize(int sz) {
	if (mHorizontal) {
		mThumbRect.bottom = sz;
	} else {
		mThumbRect.right = sz;
	}
}
void XExtScrollBar::onMeasure(int widthSpec, int heightSpec) {
	XExtComponent::onMeasure(widthSpec, heightSpec);
	if (mHorizontal) {
		mThumbRect.bottom = mMesureHeight;
	} else {
		mThumbRect.right = mMesureWidth;
	}
}

//----------XExtScroll-----------------------------
XExtScroll::XExtScroll( XmlNode *node ) : XExtComponent(node) {
	mHorNode = new XmlNode("ExtHorScrollBar", mNode);
	mVerNode = new XmlNode("ExtVerScrollBar", mNode);
	mHorBar = new XExtScrollBar(mHorNode, true);
	mHorBar->setThumbSize(10);
	mVerBar = new XExtScrollBar(mVerNode, false);
	mVerBar->setThumbSize(10);
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
	} else if (msg == MSG_MOUSEWHEEL_BUBBLE || msg == WM_MOUSEWHEEL || msg == WM_MOUSEHWHEEL) {
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
	} else if (msg == WM_LBUTTONDOWN || msg == MSG_LBUTTONDOWN_BUBBLE) {
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
XExtScrollBar* XExtScroll::getHorBar() {
	return mHorBar;
}
XExtScrollBar* XExtScroll::getVerBar() {
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
		GetMessage(&msg, NULL, 0, 0);
		if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP || msg.message == WM_CHAR || msg.message == WM_IME_CHAR) {
			// transfer the message to menu window
			msg.hwnd = mWnd;
		} else if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP || msg.message == WM_NCLBUTTONDOWN || msg.message == WM_NCLBUTTONUP) {
			POINT pt;
			GetCursorPos(&pt);
			if (! PtInRect(&popupRect, pt)) { // click on other window
				SendMessage(mWnd, MSG_POPUP_CLOSED, 1, 0);
				break;
			}
		} else if (msg.message == WM_MOUSEHWHEEL || msg.message == WM_MOUSEWHEEL) {
			POINT pt = {0};
			GetCursorPos(&pt);
			msg.hwnd = WindowFromPoint(pt);
		} else if (msg.message == WM_QUIT) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ShowWindow(mWnd, SW_HIDE);
	return 0;
}

void XExtPopup::show( int screenX, int screenY ) {
	if (mAttrWidth == 0 || mAttrHeight == 0) {
		SetWindowPos(mWnd, 0, screenX, screenY, 0, 0, SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE);
	} else {
		SetWindowPos(mWnd, 0, screenX, screenY, getSpecSize(mAttrWidth), getSpecSize(mAttrHeight), 
		SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
	}
	UpdateWindow(mWnd);
	messageLoop();
}
void XExtPopup::close() {
	ShowWindow(mWnd, SW_HIDE);
	SendMessage(mWnd, MSG_POPUP_CLOSED, 0, 0);
}
static void DisableFocus(XmlNode *n) {
	XExtComponent *ext = dynamic_cast<XExtComponent*>(n->getComponent());
	if (ext == NULL) return;
	ext->setEnableFocus(false);
	for (int i = 0; i < n->getChildCount(); ++i) {
		XmlNode *child = n->getChild(i);
		DisableFocus(child);
	}
}
void XExtPopup::disableChildrenFocus() {
	DisableFocus(mNode);
}
XExtPopup::~XExtPopup() {
	if (mWnd) DestroyWindow(mWnd);
}
//-------------------XExtTable-------------------------------
XExtTable::XExtTable( XmlNode *node ) : XExtScroll(node) {
	mDataSize.cx = mDataSize.cy = 0;
	mSelectedRow = -1;
	mModel = NULL;
	mCellRender = NULL;
	COLORREF color = 0xE6E0B0;
	AttrUtils::parseColor(mNode->getAttrValue("selRowBgColor"), &color);
	mSelectBgBrush = CreateSolidBrush(color & 0xffffff);
	color = RGB(110, 120, 250);
	AttrUtils::parseColor(mNode->getAttrValue("lineColor"), &color);
	mLinePen = CreatePen(PS_SOLID, 1, color & 0xffffff);
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
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		SIZE sz = getClientSize();
		if (mMemBuffer == NULL) {
			mMemBuffer = XImage::create(mWidth, mHeight, 24);
		}
		SelectObject(memDc, mMemBuffer->getHBitmap());
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
	if (mSelectBgBrush) DeleteObject(mSelectBgBrush);
}
//----------------------------XExtTextArea---------------------
XExtTextArea::XExtTextArea( XmlNode *node ) : XExtComponent(node) {
	mEnableFocus = AttrUtils::parseBool(mNode->getAttrValue("enableFocus"), true);
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
	mReadOnly = AttrUtils::parseBool(mNode->getAttrValue("readOnly"));
	insertText(0, mNode->getAttrValue("text"));
	mCaretShowing = false;
	mCaretPen = CreatePen(PS_SOLID, 1, RGB(0xff, 0x14, 0x93));
	mEnableShowCaret = true;
	mVerBarNode = NULL;
	mVerBar = NULL;
	mEnableScrollBars = AttrUtils::parseBool(mNode->getAttrValue("enableScrollBars"), false);
	if (mAttrPadding[0] == 0 && mAttrPadding[2] == 0) {
		mAttrPadding[0] = mAttrPadding[2] = 2;
	}
	mAutoNewLine = true;
}
bool XExtTextArea::wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	if (msg == WM_CHAR || msg == WM_IME_CHAR) {
		onChar(LOWORD(wParam));
		return true;
	} else if (msg == WM_VSCROLL) {
		InvalidateRect(mWnd, NULL, TRUE);
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
	} else if (msg == WM_MOUSEHWHEEL || msg == WM_MOUSEWHEEL) {
		int d = (short)HIWORD(wParam) / WHEEL_DELTA * 100;
		int ad = d < 0 ? -d : d;
		ad = min(ad, mHeight);
		ad = d < 0 ? -ad : ad;
		if (mVerBar != NULL) {
			int old = mVerBar->getPos();
			mVerBar->setPos(old - ad);
			InvalidateRect(mWnd, NULL, TRUE);
		}
		return true;
	} else if (msg == WM_ERASEBKGND) {
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		if (mMemBuffer == NULL) {
			mMemBuffer = XImage::create(mWidth, mHeight, 24);
		}
		HDC memDc = CreateCompatibleDC(dc);
		SelectObject(memDc, mMemBuffer->getHBitmap());
		eraseBackground(memDc);
		onPaint(memDc);
		// SIZE sz = getClientSize();
		// BitBlt(dc, 0, 0, sz.cx, mHeight, memDc, 0, 0, SRCCOPY);
		BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
		DeleteObject(memDc);
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
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
void XExtTextArea::onChar( wchar_t ch ) {
	static char buf[4];
	buf[0] = (unsigned char)(ch >> 8);
	buf[1] = (unsigned char)ch;
	buf[2] = 0;
	if (mReadOnly) {
		return;
	}
	bool changed = false;
	if (ch == VK_BACK) {// back
		back();
		changed = true;
	} else if (ch == VK_TAB) { // tab
		wchar_t chs[4] = {' ', ' ', ' ', ' '};
		insertText(mInsertPos, chs, 4);
		mInsertPos += 4;
		changed = true;
	} else if (ch == VK_RETURN) { // enter key
		wchar_t wch = '\n';
		insertText(mInsertPos, &wch, 1);
		++mInsertPos;
		changed = true;
	} else if (ch > 31) {
		if (mBeginSelPos != mEndSelPos) {
			back(); // del selected text
		}
		if (buf[0] == 0) {
			insertText(mInsertPos, &ch, 1);
		} else {
			insertText(mInsertPos, buf);
		}
		++mInsertPos;
		mBeginSelPos = mEndSelPos = mInsertPos;
		changed = true;
	}
	if (changed) {
		notifyChanged();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
	}
}
void XExtTextArea::onLButtonDown( int wParam, int x, int y ) {
	mCaretShowing = true;
	mInsertPos = getPosAt(x, y);
	if (wParam & MK_SHIFT) {
		mEndSelPos =  mInsertPos;
	} else {
		mBeginSelPos = mEndSelPos =  mInsertPos;
	}
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTextArea::onLButtonUp( int keyState, int x, int y ) {
	if (mReadOnly) return;
	// mEndSelPos = getPosAt(x, y);
}
void XExtTextArea::onMouseMove(int x, int y) {
	mInsertPos = mEndSelPos = getPosAt(x, y);
	ensureVisible(mInsertPos);
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
}
void XExtTextArea::onPaint( HDC hdc ) {
	int from = 0, to = 0;
	buildLines();
	SIZE clientSize = getClientSize();
	SelectObject(hdc, getTextFont());
	getVisibleRows(&from, &to);
	// draw select range background color
	drawSelRange(hdc, mBeginSelPos, mEndSelPos);
	if (mAttrFlags & AF_COLOR) {
		SetTextColor(hdc, mAttrColor);
	}
	SetBkMode(hdc, TRANSPARENT);
	int y = -getScrollY() + from * mLineHeight;
	for (int i = from; i < to; ++i) {
		int bg = mLines[i].mBeginPos;
		int ln = mLines[i].mLen;
		RECT rr = {mAttrPadding[0], y, mAttrPadding[0] + clientSize.cx, y + mLineHeight};
		DrawTextW(hdc, &mWideText[bg], ln, &rr, DT_SINGLELINE|DT_VCENTER);
		// TextOutW(hdc, 0, y, &mWideText[bg], ln);
		y += mLineHeight;
	}

	POINT pt = {0, 0};
	if (mCaretShowing && getPointAt(mInsertPos, &pt)) {
		pt.x -= getScrollX() - mAttrPadding[0];
		pt.y -= getScrollY() - mAttrPadding[1];
		SelectObject(hdc, mCaretPen);
		MoveToEx(hdc, pt.x, pt.y, NULL);
		LineTo(hdc, pt.x, pt.y + mLineHeight);
	}
}
void XExtTextArea::drawSelRange( HDC hdc, int begin, int end ) {
	static HBRUSH bg = 0;
	if (bg == 0) bg = CreateSolidBrush(RGB(0xad, 0xd6, 0xff));
	RECT r;
	if(begin == end || begin < 0 || end < 0) {
		return;
	}

	if (end < begin) {int tmp = begin; begin = end; end = tmp;}
	SIZE client = getClientSize();
	POINT bp, ep;
	getPointAt(begin, &bp);
	getPointAt(end, &ep);
	int brow = getLineNoByY(bp.y);
	int erow = getLineNoByY(ep.y);
	for (int i = brow; i <= erow && i >= 0; ++i) {
		r.left = getRealX(i == brow ? bp.x : 0);
		r.top = getRealY(bp.y + mLineHeight * (i - brow));
		r.right = getRealX(i == erow ? ep.x : client.cx);
		r.bottom = getRealY(r.top + mLineHeight);
		FillRect(hdc, &r, bg);
	}
}
void XExtTextArea::onKeyDown( int key ) {
	int ctrl = GetAsyncKeyState(VK_CONTROL) < 0;
	if (key == 'V' && ctrl && !mReadOnly) { // ctrl + v
		paste();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
	} else if (key == 46 && !mReadOnly) { // del
		del();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
	} else if (key >= VK_END && key <= VK_DOWN) {
		move(key);
	} else if (key == 'A' && ctrl) { // ctrl + A
		mBeginSelPos = 0;
		mEndSelPos = mWideTextLen;
		mInsertPos = mWideTextLen;
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
	} else if (key == 'C' && ctrl) { // ctrl + C
		copy();
	} else if (key == 'X' && ctrl && !mReadOnly) { // ctrl + X
		if (mBeginSelPos == mEndSelPos) return;
		copy();
		back();
		ensureVisible(mInsertPos);
		InvalidateRect(mWnd, NULL, TRUE);
	}
}
void XExtTextArea::move( int key ) {
	int sh = GetAsyncKeyState(VK_SHIFT) < 0;
	int old = mInsertPos;
	switch (key) {
	case VK_LEFT: 
		if (mInsertPos > 0) --mInsertPos;
		break;
	case VK_RIGHT:
		if (mInsertPos < mWideTextLen) ++mInsertPos;
		break;
	case VK_HOME:
		mInsertPos = 0;
		break;
	case VK_END:
		mInsertPos = mWideTextLen;
		break;
	}
	if (old == mInsertPos)
		return;
	if (! sh) mBeginSelPos = mInsertPos;
	mEndSelPos = mInsertPos;
	ensureVisible(mInsertPos);
	InvalidateRect(mWnd, NULL, TRUE);

}
void XExtTextArea::back() {
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
			if (mInsertPos > 1 && mWideText[mInsertPos - 1] == '\n' && mWideText[mInsertPos - 2] == '\r')
				delLen = 2;
			len = deleteText(mInsertPos - delLen, delLen);
			mInsertPos -= delLen;
			mBeginSelPos = mEndSelPos = mInsertPos;
			mCaretShowing = true;
		}
	}
	// buildLines();
	notifyChanged();
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTextArea::del() {
	if (mBeginSelPos != mEndSelPos) {
		int bg = mBeginSelPos < mEndSelPos ? mBeginSelPos : mEndSelPos;
		int ed = mBeginSelPos > mEndSelPos ? mBeginSelPos : mEndSelPos;
		deleteText(bg, ed - bg);
		mInsertPos = bg;
		mBeginSelPos = mEndSelPos = bg;
	} else if (mInsertPos >= 0 && mInsertPos < mWideTextLen) {
		deleteText(mInsertPos, 1);
	}
	// buildLines();
	notifyChanged();
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTextArea::paste() {
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
		// InvalidateRect(mWnd, NULL, TRUE);
		// UpdateWindow(mWnd);
	}
	CloseClipboard();
	// buildLines();
	notifyChanged();
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTextArea::copy() {
	if (mBeginSelPos == mEndSelPos) return;
	int bg = mBeginSelPos < mEndSelPos ? mBeginSelPos : mEndSelPos;
	int ed = mBeginSelPos > mEndSelPos ? mBeginSelPos : mEndSelPos;
	int len = WideCharToMultiByte(CP_ACP, 0, mWideText + bg, ed - bg, NULL, 0, NULL, NULL);
	HANDLE hd = GlobalAlloc(GHND, len + 1);
	char *buf = (char *)GlobalLock(hd);
	WideCharToMultiByte(CP_ACP, 0, mWideText + bg, ed - bg, buf, len, NULL, NULL);
	buf[len] = 0;
	GlobalUnlock(hd);

	OpenClipboard(mWnd);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hd);
	CloseClipboard();
}
void XExtTextArea::setReadOnly( bool r ) {
	mReadOnly = r;
}
void XExtTextArea::setEnableShowCaret( bool enable ) {
	mEnableShowCaret = enable;
}
void XExtTextArea::setText( const char *txt ) {
	XAreaText::setText(txt);
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
}
void XExtTextArea::setWideText( const wchar_t *txt ) {
	XAreaText::setWideText(txt);
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
}
void XExtTextArea::createWnd() {
	XExtComponent::createWnd();
	if (mEnableScrollBars) {
		mVerBarNode = new XmlNode("ExtVerScrollBar", mNode);
		mVerBar = new XExtScrollBar(mVerBarNode, false);
		mVerBarNode->setComponent(mVerBar);
		mVerBar->createWnd();
		int dd = GetWindowLong(mVerBar->getWnd(), GWL_STYLE);
		SetWindowLong(mVerBar->getWnd(), GWL_STYLE, dd & ~WS_VISIBLE);
	}
}
void XExtTextArea::notifyChanged() {
	buildLines();
	if (mVerBar == NULL) {
		InvalidateRect(mWnd, NULL, TRUE);
		return;
	}
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	mVerBar->setMaxAndPage(mTextHeight, mMesureHeight);
	if (mVerBar->isNeedShow())
		WND_SHOW(mVerBar->getWnd());
	else
		WND_HIDE(mVerBar->getWnd());

	if (mVerBar->isNeedShow() != hasVerBar)
		notifyChanged();
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTextArea::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	bool hasVerBar = false;
	if (mVerBar != NULL) {
		hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	}
	buildLines();
	if (mVerBar != NULL) {
		mVerBar->setMaxAndPage(mTextHeight, mMesureHeight);
		if (mVerBar->isNeedShow())
			WND_SHOW(mVerBar->getWnd());
		else
			WND_HIDE(mVerBar->getWnd());

		if (mVerBar->isNeedShow() != hasVerBar)
			onMeasure(widthSpec, heightSpec);
	}
}
void XExtTextArea::onLayout( int width, int height ) {
	if (mVerBar != NULL) {
		mVerBar->layout(mWidth - mVerBar->getThumbSize(), 0,
			mVerBar->getThumbSize(), mVerBar->getPage());
	}
}
XExtTextArea::~XExtTextArea() {
	if (mVerBar) delete mVerBar;
	if (mVerBarNode) delete mVerBarNode;
}

int XExtTextArea::getScrollX() {
	// TODO:
	return 0;
}
int XExtTextArea::getScrollY() {
	// TODO:
	return 0;
}
void XExtTextArea::setScrollX( int x ) {
	// TODO:
}
void XExtTextArea::setScrollY( int y ) {
	// TODO: 
}
void XExtTextArea::getVisibleRows( int *from, int *to ) {
	if (mLineHeight <= 0 || mLinesNum == 0) {
		*from = *to = 0;
		return;
	}
	SIZE sz = getClientSize();
	int y = -getScrollY();
	*from = getScrollY() / mLineHeight;
	*to = *from;
	int mh = min(sz.cy, mLineHeight * mLinesNum);
	for (int r = 0; r <= mLinesNum; ++r) {
		if (y + mLineHeight * r >= mh) {
			*to = r;
			break;
		}
	}
}
void XExtTextArea::ensureVisible( int pos ) {
	POINT pt = {0};
	if (! getPointAt(pos, &pt)) return;
	SIZE sz = getClientSize();
	if (pt.y < getScrollY()) {
		setScrollY(pt.y);
		//InvalidateRect(mWnd, NULL, TRUE);
	} else if (pt.y > getScrollY() + sz.cy) {
		setScrollY(pt.y + mLineHeight - sz.cy);
		// mVerBar->setPos(pt.y + mLineHeight - mHeight);
		//InvalidateRect(mWnd, NULL, TRUE);
		//UpdateWindow(mWnd);
	}
	if (pt.x < getScrollX()) {
		setScrollX(pt.x);
	} else if (pt.x > getScrollX() + sz.cx) {
		setScrollX(pt.x + mCharWidth - sz.cx);
	}
}
SIZE XExtTextArea::getClientSize() {
	bool hasVerBar = false;
	if (mVerBar != NULL) {
		hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	}
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	SIZE sz = {clientWidth - mAttrPadding[0] - mAttrPadding[2], 
		mMesureHeight - mAttrPadding[1] - mAttrPadding[3]};
	return sz;
}
HWND XExtTextArea::getBindWnd() {
	return mWnd;
}

HFONT XExtTextArea::getTextFont() {
	return getFont();
}
int XExtTextArea::getRealX( int x ) {
	return x - getScrollX() + mAttrPadding[0];
}
int XExtTextArea::getRealY( int y ) {
	return y - getScrollY() + mAttrPadding[1];
}

//------------------------XExtLineEdit--------------------
XExtLineEdit::XExtLineEdit(XmlNode *node) : XExtTextArea(node) {
	mAutoNewLine = false;
}
void XExtLineEdit::onChar( wchar_t ch ) {
	if (ch == VK_RETURN || ch == VK_TAB) {
		return;
	}
	XExtTextArea::onChar(ch);
}

void XExtLineEdit::createWnd() {
	XExtComponent::createWnd();
}
void XExtLineEdit::insertText( int pos, wchar_t *txt, int len ) {
	if (len <= 0 || txt == NULL) {
		return;
	}
	if (pos < 0 || pos > mWideTextLen) {
		pos = mWideTextLen;
	}
	if (mWideTextLen + len >= mWideTextCapacity - 10) {
		mWideTextCapacity = max(mWideTextLen + len + 50, mWideTextCapacity * 2);
		mWideTextCapacity = (mWideTextCapacity & (~63)) + 64;
		mWideText = (wchar_t *)realloc(mWideText, sizeof(wchar_t) * mWideTextCapacity);
	}
	int nlen = 0;
	wchar_t *pt = txt;
	for (int i = 0; i < len; ++i) {
		if (pt[i] != '\r' && pt[i] != '\n') 
			++nlen;
	}

	for (int i = mWideTextLen - 1; i >= pos; --i) {
		mWideText[i + nlen] = mWideText[i];
	}
	for (int i = 0, j = 0; i < len; ++i) {
		if (pt[i] != '\r' && pt[i] != '\n') {
			mWideText[pos + j++] = txt[i];
		}
	}
	mWideTextLen += nlen;
}
XExtLineEdit::~XExtLineEdit() {
}

//----------------------------XExtList---------------------
XExtList::XExtList( XmlNode *node ) : XExtScroll(node) {
	mModel = NULL;
	mItemRender = NULL;
	mDataSize.cx = mDataSize.cy = 0;
	mMouseTrackItem = -1;
	COLORREF color = RGB(0xA2, 0xB5, 0xCD);
	AttrUtils::parseColor(mNode->getAttrValue("selBgColor"), &color);
	mSelectBgBrush = CreateSolidBrush(color & 0xffffff);
}
XExtList::~XExtList() {
	DeleteObject(mSelectBgBrush);
}
bool XExtList::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		// eraseBackground((HDC)wParam);
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		SIZE sz = getClientSize();
		if (mMemBuffer == NULL) {
			mMemBuffer = XImage::create(mWidth, mHeight, 24);
		}
		SelectObject(memDc, mMemBuffer->getHBitmap());
		if (mModel != NULL) {
			// draw background
			eraseBackground(memDc);
			drawData(memDc, 0, 0, sz.cx, sz.cy);
			// copy data
			RECT dst = {0};
			RECT pr = {0, 0, sz.cx, sz.cy};
			IntersectRect(&dst, &ps.rcPaint, &pr);
			BitBlt(dc, dst.left, dst.top, dst.right-dst.left, dst.bottom-dst.top, memDc, dst.left, dst.top, SRCCOPY);
		} else {
			eraseBackground(dc);
		}
		DeleteObject(memDc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
		int idx = findItem((short)LOWORD(lParam), (short)HIWORD(lParam));
		pt.x = pt.x + mHorBar->getPos();
		pt.y = pt.y - getItemY(idx);
		SendMessage(mWnd, MSG_LIST_CLICK_ITEM, idx, (LPARAM)&pt);
		return true;
	} else if (msg == WM_LBUTTONDBLCLK) {
		POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
		int idx = findItem((short)LOWORD(lParam), (short)HIWORD(lParam));
		pt.x = pt.x + mHorBar->getPos();
		pt.y = pt.y - getItemY(idx);
		SendMessage(mWnd, MSG_LIST_DBCLICK_ITEM, idx, (LPARAM)&pt);
		return true;
	} 
	else if (msg == WM_MOUSEMOVE) {
		updateTrackItem((short)LOWORD(lParam), (short)HIWORD(lParam));
		return XExtScroll::wndProc(msg, wParam, lParam, result);;
	} else if (msg == WM_MOUSEHWHEEL || msg == WM_MOUSEWHEEL || msg == MSG_MOUSEWHEEL_BUBBLE) {
		XExtScroll::wndProc(msg, wParam, lParam, result);
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(mWnd, &pt);
		updateTrackItem(pt.x, pt.y);
		return true;
	} else if (msg == WM_MOUSELEAVE) {
		updateTrackItem(-1, -1);
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
	if (mModel == NULL) {
		return;
	}
	mHorBar->layout(0, mHeight - mHorBar->getThumbSize(), mHorBar->getPage(), mHorBar->getThumbSize());
	mVerBar->layout(mWidth - mVerBar->getThumbSize(), 0, mVerBar->getThumbSize(), mVerBar->getPage());
}

void XExtList::notifyModelChanged() {
	if (mMesureWidth == 0 || mMesureHeight == 0) {
		return;
	}
	onMeasure(mMesureWidth | MS_FIX, mMesureHeight | MS_FIX);
	onLayout(mWidth, mHeight);
	InvalidateRect(mWnd, NULL, TRUE);
}

void XExtList::drawData( HDC memDc, int x, int y, int w, int h ) {
	if (mModel == NULL) return;
	SelectObject(memDc, getFont());
	SetBkMode(memDc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(memDc, mAttrColor);

	int from = 0, num = 0;
	getVisibleRows(&from, &num);
	y += -mVerBar->getPos();
	for (int i = 0; i < from; ++i) {
		y += mModel->getItemHeight(i);
	}
	x += -mHorBar->getPos();
	for (int i = from; i < from + num && i >= 0; ++i) {
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
void XExtList::getVisibleRows( int *from, int *num ) {
	*from = *num = 0;
	if (mModel == NULL) {
		return;
	}
	int y = -mVerBar->getPos();
	SIZE sz = getClientSize();
	for (int i = 0; i < mModel->getItemCount(); ++i) {
		y += mModel->getItemHeight(i);
		if (y > 0) {
			*from = i;
			break;
		}
	}
	for (int i = *from; i < mModel->getItemCount(); ++i) {
		*num = *num + 1;
		if (y >= sz.cy) {
			break;
		}
	}
}
void XExtList::setModel( XListModel *model ) {
	if (mModel != model) {
		mModel = model;
		notifyModelChanged();
	}
}
XListModel *XExtList::getModel() {
	return mModel;
}
void XExtList::setItemRender( ItemRender *render ) {
	mItemRender = render;
}
int XExtList::findItem( int x, int y ) {
	if (mModel == NULL || x < 0 || y < 0) {
		return -1;
	}
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

int XExtList::getItemY(int item) {
	if (mModel == NULL) {
		return 0;
	}
	int cc = mModel->getItemCount();
	int y = -mVerBar->getPos();
	for (int i = 0; i < item && i < cc; ++i) {
		y += mModel->getItemHeight(i);
	}
	return y;
}

void XExtList::updateTrackItem( int x, int y ) {
	if (mModel == NULL || !mModel->isMouseTrack())
		return;
	int old = mMouseTrackItem;
	int idx = findItem(x, y);
	if (idx < 0) {
		mMouseTrackItem = -1;
	} else {
		XListModel::ItemData *item = mModel->getItemData(idx);
		if (item != NULL && item->mSelectable) {
			mMouseTrackItem = idx;
		}
	}
	if (old != mMouseTrackItem) {
		int sy = getItemY(old);
		int ey = getItemY(mMouseTrackItem);
		int sh = old >= 0 ? mModel->getItemHeight(old) : 0;
		int eh = mMouseTrackItem >= 0 ? mModel->getItemHeight(mMouseTrackItem) : 0;
		RECT sr = {0, sy, mWidth, sy + sh};
		RECT er = {0, ey, mWidth, ey + eh};
		SIZE sz = getClientSize();
		RECT dest = {0}, client = {0, 0, sz.cx, sz.cy}, rr = {0};
		UnionRect(&dest, &sr, &er);
		IntersectRect(&rr, &dest, &client);
		InvalidateRect(mWnd, &rr, TRUE);
		UpdateWindow(mWnd);
	}
}

int XExtList::getMouseTrackItem() {
	return mMouseTrackItem;
}

//------------------------------XArrowButton--------------------
XExtArrowButton::XExtArrowButton(XmlNode *node) : XExtButton(node) {
	mMouseAtArrow = false;
	mArrowWidth = 0;
	for (int i = 0; i < mNode->getAttrsCount(); ++i) {
		XmlNode::Attr *attr = mNode->getAttr(i);
		if (strcmp(attr->mName, "arrowPushImage") == 0) {
			mStateImages[ARROW_IMG_PUSH] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "arrowHoverImage") == 0) {
			mStateImages[ARROW_IMG_HOVER] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "arrowWidth") == 0) {
			mArrowWidth = AttrUtils::parseInt(attr->mValue);
		}
	}
}

bool XExtArrowButton::wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};

		XImage *cur = mStateImages[getStateImage()];
		if (cur != NULL) {
			cur->draw(dc, 0, 0, mWidth, mHeight);
		}
		if (mAttrFlags & AF_COLOR) {
			SetTextColor(dc, mAttrColor);
		}
		SetBkMode(dc, TRANSPARENT);
		SelectObject(dc, getFont());
		DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		mIsMouseDown = true;
		mIsMouseMoving = false;
		mIsMouseLeave = false;
		mMouseAtArrow = isPointInArrow((short)LOWORD(lParam), (short)HIWORD(lParam));
		InvalidateRect(mWnd, NULL, TRUE);
		SetCapture(mWnd);
		if (mEnableFocus) SetFocus(mWnd);
		return true;
	} else if (msg == WM_LBUTTONUP) {
		bool md = mIsMouseDown;
		mIsMouseDown = false;
		mIsMouseMoving = false;
		mMouseAtArrow = isPointInArrow((short)LOWORD(lParam), (short)HIWORD(lParam));
		ReleaseCapture();
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (md && PtInRect(&r, pt)) {
			SendMessage(mWnd, MSG_COMMAND, mMouseAtArrow, 0);
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

		StateImage bi = getStateImage();
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
		mMouseAtArrow = isPointInArrow((short)LOWORD(lParam), (short)HIWORD(lParam));
		if (bi != getStateImage()) {
			InvalidateRect(mWnd, NULL, TRUE);
		}
		return true;
	} else if (msg == WM_MOUSELEAVE) {
		mIsMouseMoving = false;
		mIsMouseLeave = true;
		mMouseAtArrow = isPointInArrow((short)LOWORD(lParam), (short)HIWORD(lParam));
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}

bool XExtArrowButton::isPointInArrow(int x, int y) {
	return y >= 0 && y < mHeight &&
		x >= mWidth - mArrowWidth && x < mWidth;
}

XExtArrowButton::StateImage XExtArrowButton::getStateImage() {
	if (GetWindowLong(mWnd, GWL_STYLE) & WS_DISABLED)
		return STATE_IMG_DISABLE;
	if (mIsMouseDown && ! mIsMouseLeave) {
		if (mMouseAtArrow)
			return StateImage(ARROW_IMG_PUSH);
		return STATE_IMG_PUSH;
	}
	if (!mIsMouseDown && mIsMouseMoving) {
		if (mMouseAtArrow)
			return StateImage(ARROW_IMG_HOVER);
		return STATE_IMG_HOVER;
	}
	if (mIsMouseLeave) {
		return STATE_IMG_NORMAL;
	}

	return STATE_IMG_NORMAL;
}

//------------------------------XExtComboBox--------------------
XExtComboBox::XExtComboBox( XmlNode *node ) : XExtComponent(node) {
	memset(mStateImages, 0, sizeof(mStateImages));
	mIsMouseDown = false;
	mIsMouseMoving = false;
	mIsMouseLeave = false;
	mEditNode = new XmlNode("ExtLineEdit", mNode);
	mPopupNode = new XmlNode("ExtPopup", mNode);
	mListNode = new XmlNode("ExtList", mPopupNode);
	mEdit = new XExtLineEdit(mEditNode);
	mPopup = new XExtPopup(mPopupNode);
	// mPopup->setBgColor(RGB(0xF5, 0xFF, 0xFA));
	mPopupNode->setComponent(mPopup);
	mList = new XExtList(mListNode);
	mListNode->setComponent(mList);
	mList->setAttrBgColor(RGB(0xF5, 0xFF, 0xFA));
	mPopup->setListener(this);
	mList->setListener(this);
	mList->setEnableFocus(false);

	mArrowWidth = AttrUtils::parseInt(mNode->getAttrValue("arrowWidth"));
	AttrUtils::parseArraySize(mNode->getAttrValue("popupSize"), (int *)&mAttrPopupSize, 2);
	mEdit->setReadOnly(AttrUtils::parseBool(mNode->getAttrValue("readOnly")));
	mEnableEditor = false;

	mStateImages[STATE_IMG_NORMAL] = XImage::load(mNode->getAttrValue("normalImage"));
	mStateImages[STATE_IMG_HOVER] = XImage::load(mNode->getAttrValue("hoverImage"));
	mStateImages[STATE_IMG_PUSH] = XImage::load(mNode->getAttrValue("pushImage"));
	mBoxRender = NULL;
	mPoupShow = false;
	mSelectItem = -1;
}
bool XExtComboBox::onEvent( XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret ) {
	if (msg == MSG_LIST_CLICK_ITEM) {
		mSelectItem = wParam;
		mPopup->close();
		mPoupShow = false;
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
		SendMessage(mWnd, MSG_COMBOBOX_CLICK_ITEM, wParam, lParam);
		return true;
	} else if (msg == MSG_POPUP_CLOSED) {
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
		XImage *img = mStateImages[getStateImage()];
		if (img != NULL) {
			img->draw(dc, 0, 0, mWidth, mHeight);
		}
		if (! mEnableEditor) {
			if (mBoxRender != NULL) {
				mBoxRender->onDrawBox(dc, 0, 0, mWidth - mArrowWidth, mHeight);
			} else {
				drawBox(dc, 0, 0, mWidth - mArrowWidth, mHeight);
			}
		}
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		mIsMouseDown = true;
		mIsMouseMoving = false;
		mIsMouseLeave = false;
		InvalidateRect(mWnd, NULL, TRUE);
		if (mEnableFocus) SetFocus(mWnd);
		UpdateWindow(mWnd);
		SetCapture(mWnd);
		return true;
	} else if (msg == WM_LBUTTONUP) {
		mIsMouseDown = false;
		mIsMouseMoving = false;
		ReleaseCapture();
		POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mEnableEditor) {
			r.left = mWidth - mArrowWidth;
		}
		if (PtInRect(&r, pt)) {
			openPopup();
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

		StateImage bi = getStateImage();
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mEnableEditor) {
			r.left = mWidth - mArrowWidth;
		}
		if (PtInRect(&r, pt)) {
			mIsMouseMoving = true;
			mIsMouseLeave = false;
		} else {
			// mouse leave
			mIsMouseMoving = false;
			mIsMouseLeave = true;
		}
		if (bi != getStateImage()) {
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
void XExtComboBox::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	mEdit->onMeasure((mMesureWidth - mArrowWidth) | MS_FIX, mMesureHeight | MS_FIX);
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
	XComponent *cc = mEdit;
	cc->createWnd();
	cc = mPopup;
	cc->createWnd();
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
	mPopup->show(pt.x, pt.y);
}
XExtComboBox::StateImage XExtComboBox::getStateImage() {
	if (GetWindowLong(mWnd, GWL_STYLE) & WS_DISABLED)
		return STATE_IMG_DISABLE;
	if (mPoupShow) {
		return STATE_IMG_PUSH;
	}
	if (mIsMouseDown && ! mIsMouseLeave) {
		return STATE_IMG_PUSH;
	}
	if (!mIsMouseDown && mIsMouseMoving) {
		return STATE_IMG_HOVER;
	}
	if (mIsMouseLeave) {
		return STATE_IMG_NORMAL;
	}
	return STATE_IMG_NORMAL;
}
int XExtComboBox::getSelectItem() {
	return mSelectItem;
}
void XExtComboBox::setSelectItem(int idx) {
	mSelectItem = idx;
	InvalidateRect(mWnd, NULL, TRUE);
}
XExtComboBox::~XExtComboBox() {
	delete mEdit;
	delete mPopup;
	delete mList;
	delete mEditNode;
	delete mPopupNode;
	delete mListNode;
}
//--------------------MenuItem--------------------
XExtMenuItem::XExtMenuItem(const char *name, char *text) {
	mName[0] = 0;
	mText = text;
	mActive = true;
	mChild = NULL;
	mSeparator = false;
	mVisible = true;
	mCheckable = false;
	mChecked = false;
	if (name) strcpy(mName, name);
}
XExtMenuModel::XExtMenuModel() {
	mCount = 0;
	memset(mItems, 0, sizeof(mItems));
	mWidth = 150;
}
void XExtMenuModel::add( XExtMenuItem *item ) {
	insert(mCount, item);
}
void XExtMenuModel::insert( int pos, XExtMenuItem *item ) {
	if (pos < 0 || pos > mCount || item == NULL)
		return;
	for (int i = mCount - 1; i >= pos; --i) {
		mItems[i + 1] = mItems[i];
	}
	mItems[pos] = item;
	++mCount;
}
int XExtMenuModel::getCount() {
	return mCount;
}
XExtMenuItem * XExtMenuModel::get( int idx ) {
	if (idx >= 0 && idx < mCount)
		return mItems[idx];
	return NULL;
}
XExtMenuModel::~XExtMenuModel() {
	for (int i = 0; i < mCount; ++i) {
		delete mItems[i];
	}
	free(mItems);
}

XExtMenuItem * XExtMenuModel::findByName( const char *name ) {
	if (name == NULL)
		return NULL;
	for (int i = 0; i < mCount; ++i) {
		XExtMenuItem *item = mItems[i];
		if (strcmp(name, item->mName) == 0) {
			return item;
		}
		if (item->mChild != NULL) {
			item = item->mChild->findByName(name);
			if (item != NULL) return item;
		}
	}
	return NULL;
}

int XExtMenuModel::getItemHeight(int pos) {
	if (pos < 0 || pos >= mCount) {
		return 0;
	}
	if (mItems[pos]->mSeparator) {
		return 5;
	}
	return 25;
}

int XExtMenuModel::getWidth() {
	return mWidth;
}

int XExtMenuModel::getHeight() {
	int h = 0;
	for (int i = 0; i < mCount; ++i) {
		XExtMenuItem *item = mItems[i];
		if (! item->mVisible) continue;
		h += getItemHeight(i);
	}
	return h;
}

XExtMenu::XExtMenu( XmlNode *node, XExtMenuManager *mgr) : XExtComponent(node) {
	strcpy(mClassName, "XExtMenu");
	mManager = mgr;
	mSelectItem = -1;
	mModel = NULL;
	mAttrFlags |= AF_BG_COLOR;
	mAttrBgColor = RGB(0xfa, 0xfa, 0xfa);
	mSeparatorPen = CreatePen(PS_SOLID, 1, RGB(0xcc, 0xcc, 0xcc));
	mCheckedPen = CreatePen(PS_SOLID, 1, RGB(0x64, 0x95, 0xED));
	mSelectBrush = CreateSolidBrush(RGB(0xB2, 0xDF, 0xEE));
	createWnd();
}
void XExtMenu::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId();  // has no id
	HWND owner = mNode->getRoot()->getComponent()->getWnd();
	mWnd = CreateWindow(mClassName, NULL, WS_POPUP, 0, 0, 0, 0, owner, NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}
bool XExtMenu::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_SIZE) {
		return true;
	} else if (msg == WM_LBUTTONUP) {
		int x = (short)(LOWORD(lParam)), y = (short)(HIWORD(lParam));
		POINT pt = {x, y};
		RECT r = {0};
		GetClientRect(mWnd, &r);
		if (! PtInRect(&r, pt)) {
			return true;
		}
		int idx = getItemIndexAt(x, y);
		if (idx < 0) return true;
		XExtMenuItem *item = mModel->get(idx);
		if (item == NULL) return true;
		if (item->mSeparator) return true;
		if (! item->mActive) return true;
		if (item->mChild && item->mChild->getCount() > 0) return true;
		if (item->mCheckable) item->mChecked = !item->mChecked;
		// send click menu item msg
		mManager->notifyItemClicked(item);
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		drawItems(dc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_MOUSEACTIVATE) {
		*result = MA_NOACTIVATE; // 鼠标点击时，不活动
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		int x = (short)(LOWORD(lParam)), y = (short)(HIWORD(lParam));
		int old = mSelectItem;
		mSelectItem = getItemIndexAt(x, y);
		if (old == mSelectItem || mModel == NULL)
			return true;
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
		XExtMenuItem *oldItem = mModel->get(old);
		XExtMenuItem *item = mModel->get(mSelectItem);
		if (oldItem != NULL && oldItem->mChild != NULL && oldItem->mChild->getCount() > 0) {
			mManager->closeMenu(oldItem->mChild);
		}
		if (item && item->mChild != NULL && item->mChild->getCount() > 0) {
			// notify to open sub menu
			RECT r = getItemRect(mSelectItem);
			POINT pt = {r.right, r.top};
			ClientToScreen(mWnd, &pt);
			mManager->openMenu(item->mChild, pt.x, pt.y);
		}
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
void XExtMenu::setModel( XExtMenuModel *model ) {
	mModel = model;
}
int XExtMenu::getItemIndexAt( int x, int y ) {
	int h = 0;
	for (int i = 0; i < mModel->getCount(); ++i) {
		XExtMenuItem *item = mModel->get(i);
		if (! item->mVisible) continue;
		h += mModel->getItemHeight(i);
		if (h >= y) return i;
	}
	return -1;
}
void XExtMenu::calcSize() {
	mMesureWidth = mWidth = mModel->getWidth();
	mMesureHeight = mHeight = mModel->getHeight();
}
void XExtMenu::show( int screenX, int screenY ) {
	mSelectItem = -1;
	calcSize();
	MoveWindow(mWnd, 0, 0, mWidth, mHeight, TRUE);
	SetWindowPos(mWnd, 0, screenX, screenY, 0, 0, SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOSIZE);
	UpdateWindow(mWnd);
}
void XExtMenu::drawItems( HDC dc ) {
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());
	for (int i = 0, h = 0; i < mModel->getCount(); ++i) {
		XExtMenuItem *item = mModel->get(i);
		if (! item->mVisible) continue;
		int k = mModel->getItemHeight(i);
		if (item->mSeparator) {
			SelectObject(dc, mSeparatorPen);
			MoveToEx(dc, 30, h + k / 2 - 1, NULL);
			LineTo(dc, mWidth - 10, h + k / 2 - 1);
		} else {
			RECT r = {30, h, mWidth - 20, h + k};
			int len = item->mText == NULL ? 0 : strlen(item->mText);
			if (item->mActive)
				SetTextColor(dc, RGB(0x20, 0x20, 0x20));
			else 
				SetTextColor(dc, RGB(0x8E, 0x8E, 0x8E));
			if (mSelectItem == i) {
				RECT br = {0, h, mWidth, h + k};
				FillRect(dc, &br, mSelectBrush);
			}
			if (item->mChild != NULL && item->mChild->getCount() > 0) {
				// draw arrow
				static HBRUSH arrowBrush = CreateSolidBrush(RGB(0x55, 0x55, 0x55));
				int SJ = 5, LW = 15;
				POINT pts[3] = {{mWidth-LW, h+k/2-SJ}, {mWidth-LW, h+k/2+SJ}, {mWidth-LW+(int)(SJ/0.57735), h+k/2}};
				HRGN rgn = CreatePolygonRgn(pts, 3, ALTERNATE);
				FillRgn(dc, rgn, arrowBrush);
				DeleteObject(rgn);
			}
			if (item->mCheckable && item->mChecked) {
				int LH = 3, x = 5, y = h + k/2-3;
				SelectObject(dc, mCheckedPen);
				for (int i = 0; i < 4; ++i) {
					MoveToEx(dc, x + i, y + i, NULL);
					LineTo(dc, x + i, y + LH + i);
				}
				x += 4;
				y += 4;
				for (int i = 0; i < 8; ++i) {
					MoveToEx(dc, x + i, y - i, NULL);
					LineTo(dc, x + i, y + LH - i);
				}
			}
			DrawText(dc, item->mText, len, &r, DT_SINGLELINE | DT_VCENTER);
		}
		h += k;
	}
}
RECT XExtMenu::getItemRect( int idx ) {
	RECT r = {0, 0, mWidth, 0};
	for (int i = 0, h = 0; i < mModel->getCount(); ++i) {
		XExtMenuItem *item = mModel->get(i);
		if (! item->mVisible) continue;
		int k = mModel->getItemHeight(i);
		if (i == idx) {
			r.top = h;
			r.bottom = h + k;
			break;
		}
		h += k;
	}
	return r;
}
XExtMenu::~XExtMenu() {
	DeleteObject(mSeparatorPen);
	DeleteObject(mSelectBrush);
	DeleteObject(mCheckedPen);
	DestroyWindow(mWnd);
}
XExtMenuManager::XExtMenuManager( XExtMenuModel *mlist, XComponent *owner, ItemListener *listener ) {
	mMenuList = mlist;
	mOwner = owner;
	mLevel = -1;
	mListener = listener;
	memset(mMenus, 0, sizeof(mMenus));
	mLooping = false;
}
void XExtMenuManager::show( int screenX, int screenY ) {
	if (mMenus[++mLevel] == NULL) {
		mMenus[mLevel] = new XExtMenu(new XmlNode("ExtMenu", mOwner->getNode()), this);
	}
	mMenus[mLevel]->setModel(mMenuList);
	mMenus[mLevel]->show(screenX, screenY);
	messageLoop();
}
void XExtMenuManager::messageLoop() {
	MSG msg = {0};
	HWND ownerWnd = mOwner->getNode()->getRoot()->getComponent()->getWnd();
	mLooping = true;
	while (mLooping) {
		if (GetForegroundWindow() != ownerWnd || mLevel < 0) {
			break;
		}
		GetMessage(&msg, NULL, 0, 0);
		if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP || msg.message == WM_CHAR || msg.message == WM_IME_CHAR) {
			// transfer the message to menu window
			POINT pt = {0};
			GetCursorPos(&pt);
			int idx = whereIs(pt.x, pt.y);
			if (idx >= 0) {
				msg.hwnd = mMenus[idx]->getWnd();
			}
		} else if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP 
			|| msg.message == WM_RBUTTONDOWN || msg.message == WM_RBUTTONUP 
			|| msg.message == WM_NCLBUTTONDOWN || msg.message == WM_NCLBUTTONUP) {
			POINT pt;
			GetCursorPos(&pt);
			int idx = whereIs(pt.x, pt.y);
			if (idx < 0) { // click on other window
				break;
			}
			msg.hwnd = mMenus[idx]->getWnd();
		} else if (msg.message == WM_MOUSEHWHEEL || msg.message == WM_MOUSEWHEEL 
			|| msg.message == WM_MOUSEMOVE) {
			POINT pt = {0};
			GetCursorPos(&pt);
			int idx = whereIs(pt.x, pt.y);
			if (idx < 0) { // in other window
				continue;
			}
			msg.hwnd = mMenus[idx]->getWnd();
			// msg.hwnd = WindowFromPoint(pt);
		} else if (msg.message == XComponent::MSG_QUIT) {
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	closeMenuTo(-1);
}
int XExtMenuManager::whereIs( int x, int y ) {
	POINT pt = {x, y};
	for (int i = mLevel; i >= 0; --i) {
		RECT r;
		GetWindowRect(mMenus[i]->getWnd(), &r);
		if (PtInRect(&r, pt)) return i;
	}
	return -1;
}
void XExtMenuManager::closeMenuTo( int idx ) {
	for (; mLevel > idx; --mLevel) {
		ShowWindow(mMenus[mLevel]->getWnd(), SW_HIDE);
	}
}
void XExtMenuManager::notifyItemClicked( XExtMenuItem *item ) {
	HWND ownerWnd = mOwner->getNode()->getRoot()->getComponent()->getWnd();
	closeMenuTo(-1);
	if (mListener != NULL) {
		mListener->onClickItem(item);
	}
	mLooping = false;
	PostMessage(ownerWnd, XComponent::MSG_QUIT, 0, 0);
}
void XExtMenuManager::closeMenu( XExtMenuModel *mlist ) {
	for (int i = 0; i <= mLevel; ++i) {
		if (mMenus[i]->getMenuList() == mlist) {
			closeMenuTo(i - 1);
			break;
		}
	}
}
void XExtMenuManager::openMenu( XExtMenuModel *mlist, int x, int y ) {
	if (mMenus[++mLevel] == NULL) {
		mMenus[mLevel] = new XExtMenu(new XmlNode("ExtMenu", mOwner->getNode()), this);
	}
	mMenus[mLevel]->setModel(mlist);
	mMenus[mLevel]->show(x, y);
}
XExtMenuManager::~XExtMenuManager() {
	for (int i = mLevel; i >= 0; --i) {
		delete mMenus[i];
	}
}
//-------------------------XExtTreeNode--------------------
XExtTreeNode::XExtTreeNode( const char *text ) {
	mText = (char *)text;
	mParent = NULL;
	mChildren = NULL;
	mUserData = NULL;
	mAttrFlags = 0;
	mExpand = false;
	mContentWidth = -1;// -1 表示宽度未定
	mCheckable = false;
	mChecked = false;
}
void XExtTreeNode::insert( int pos, XExtTreeNode *child ) {
	if (child == NULL || pos > getChildCount()) 
		return;
	if (mChildren == NULL) mChildren = new std::vector<XExtTreeNode*>();
	if (pos < 0) pos = getChildCount();
	mChildren->insert(mChildren->begin() + pos, child);
	child->mParent = this;
}
void XExtTreeNode::remove( int pos ) {
	if (pos >= 0 && pos < getChildCount()) {
		mChildren->erase(mChildren->begin() + pos);
	}
}
void XExtTreeNode::remove(XExtTreeNode *child) {
	remove(indexOf(child));
}
int XExtTreeNode::indexOf( XExtTreeNode *child ) {
	if (child == NULL || mChildren == NULL) 
		return -1;
	for (int i = 0; i < mChildren->size(); ++i) {
		if (mChildren->at(i) == child) 
			return i;
	}
	return -1;
}
int XExtTreeNode::getChildCount() {
	if (mChildren == NULL) return 0;
	return mChildren->size();
}
XExtTreeNode * XExtTreeNode::getChild( int idx ) {
	if (idx >= 0 && idx < getChildCount())
		return mChildren->at(idx);
	return NULL;
}
void * XExtTreeNode::getUserData() {
	return mUserData;
}
void XExtTreeNode::setUserData( void *userData ) {
	mUserData = userData;
}
int XExtTreeNode::getContentWidth() {
	return mContentWidth;
}
void XExtTreeNode::setContentWidth( int w ) {
	mContentWidth = w;
}
bool XExtTreeNode::isExpand() {
	return mExpand;
}
void XExtTreeNode::setExpand( bool expand ) {
	mExpand = expand;
}
char * XExtTreeNode::getText() {
	return mText;
}
void XExtTreeNode::setText( char *text ) {
	mText = text;
	mContentWidth = -1;
}
XExtTreeNode::PosInfo XExtTreeNode::getPosInfo() {
	if (mParent == NULL) return PI_FIRST;
	int v = 0;
	int idx = mParent->indexOf(this);
	if (idx == 0) v |= PI_FIRST;
	if (idx == mParent->getChildCount() - 1) v |= PI_LAST;
	if (idx > 0 && idx < mParent->getChildCount() - 1) v |= PI_CENTER;
	return PosInfo(v);
}
int XExtTreeNode::getLevel() {
	XExtTreeNode *p = mParent;
	int level = -1;
	for (; p != NULL; p = p->mParent) ++level;
	return level;
}
XExtTreeNode * XExtTreeNode::getParent() {
	return mParent;
}
bool XExtTreeNode::isCheckable() {
	return mCheckable;
}
void XExtTreeNode::setCheckable( bool cb ) {
	mCheckable = cb;
}
bool XExtTreeNode::isChecked() {
	return mChecked;
}
void XExtTreeNode::setChecked( bool cb ) {
	mChecked = cb;
}

static const int TREE_NODE_HEIGHT = 30;
static const int TREE_NODE_HEADER_WIDTH = 40;
static const int TREE_NODE_BOX = 16;
static const int TREE_NODE_BOX_LEFT = 5;

XExtTree::XExtTree( XmlNode *node ) : XExtScroll(node) {
	mDataSize.cx = mDataSize.cy = 0;
	mModel = NULL;
	COLORREF color = RGB(0x3A, 0x9D, 0xF9);
	AttrUtils::parseColor(mNode->getAttrValue("lineColor"), &color);
	mLinePen = CreatePen(PS_SOLID, 1, color & 0xffffff);
	color = RGB(0x64, 0x95, 0xED);
	AttrUtils::parseColor(mNode->getAttrValue("checkBoxColor"), &color);
	mCheckPen = CreatePen(PS_SOLID, 1, color & 0xffffff);
	color = 0xE6E0B0;
	AttrUtils::parseColor(mNode->getAttrValue("selBgColor"), &color);
	mSelectBgBrush = CreateSolidBrush(color & 0xffffff);
	mSelectNode = NULL;
	mWidthSpec = mHeightSpec = 0;
	mNodeRender = NULL;
	mWhenSelect = WHEN_CLICK;
	char *ws = mNode->getAttrValue("whenSelect");
	if (ws != NULL && strcmp(ws, "click") == 0) {
		mWhenSelect = WHEN_CLICK;
	} else if (ws != NULL && strcmp(ws, "dbclick") == 0) {
		mWhenSelect = WHEN_DBCLICK;
	}
}
void XExtTree::setModel( XExtTreeNode *root ) {
	mModel = root;
}
XExtTreeNode* XExtTree::getModel() {
	return mModel;
}
XExtTree::~XExtTree() {
	DeleteObject(mLinePen);
	DeleteObject(mCheckPen);
	DeleteObject(mSelectBgBrush);
}
bool XExtTree::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		SIZE sz = getClientSize();
		if (mMemBuffer == NULL) {
			mMemBuffer = XImage::create(mWidth, mHeight, 24);
		}
		SelectObject(memDc, mMemBuffer->getHBitmap());
		// draw background
		eraseBackground(memDc);
		drawData(memDc, sz.cx, sz.cy);
		BitBlt(dc, 0, 0, sz.cx, sz.cy, memDc, 0, 0, SRCCOPY);
		DeleteObject(memDc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		onLBtnDown(x, y);
		return true;
	} else if (msg == WM_LBUTTONDBLCLK) {
		int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		onLBtnDbClick(x, y);
		return true;
	}
	return XExtScroll::wndProc(msg, wParam, lParam, result);
}
void XExtTree::onMeasure( int widthSpec, int heightSpec ) {
	mWidthSpec = widthSpec;
	mHeightSpec = heightSpec;
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
void XExtTree::onLayout( int width, int height ) {
	if (mModel == NULL) return;
	mHorBar->layout(0, mHeight - mHorBar->getThumbSize(), mHorBar->getPage(), mHorBar->getThumbSize());
	mVerBar->layout(mWidth - mVerBar->getThumbSize(), 0, mVerBar->getThumbSize(), mVerBar->getPage());
}
static void CalcDataSize(HDC dc, XExtTreeNode *n, int *nodeNum, int *maxWidth, int level) {
	*nodeNum = *nodeNum + 1;
	if (n->getContentWidth() < 0) {
		SIZE sztw = {0};
		if (n->getText() != NULL) {
			GetTextExtentPoint32(dc, n->getText(), strlen(n->getText()), &sztw);
		} else {
			sztw.cx = 30;
		}
		if (n->isCheckable()) sztw.cx += TREE_NODE_BOX + 3;
		sztw.cx += 6; // 6 is left & right padding 
		n->setContentWidth(sztw.cx);
	}
	int mw = n->getContentWidth() + level * TREE_NODE_HEADER_WIDTH;
	if (*maxWidth < mw) *maxWidth = mw;
	if (n->isExpand()) {
		for (int i = 0; i < n->getChildCount(); ++i) {
			CalcDataSize(dc, n->getChild(i), nodeNum, maxWidth, level + 1);
		}
	}
}
SIZE XExtTree::calcDataSize() {
	SIZE sz = {0};
	if (mModel == NULL) return sz;
	int nn = 0, mw = 0;
	HDC dc = GetDC(mWnd);
	SelectObject(dc, getFont());
	mModel->setExpand(true); // always expand root node
	CalcDataSize(dc, mModel, &nn, &mw, 0);
	ReleaseDC(mWnd, dc);
	sz.cy = (nn - 1) * TREE_NODE_HEIGHT;
	sz.cx = mw + 5;
	return sz;
}
SIZE XExtTree::getClientSize() {
	bool hasHorBar = GetWindowLong(mHorBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	bool hasVerBar = GetWindowLong(mVerBar->getWnd(), GWL_STYLE) & WS_VISIBLE;
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getThumbSize() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getThumbSize() : 0);
	SIZE sz = {clientWidth, clientHeight};
	return sz;
}
void XExtTree::moveChildrenPos( int dx, int dy ) {
	InvalidateRect(mWnd, NULL, TRUE);
}
void XExtTree::drawData( HDC dc, int w, int h) {
	if (mModel == NULL) return;
	int y = -mVerBar->getPos();
	SelectObject(dc, mLinePen);
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());

	for (int i = 0; i < mModel->getChildCount(); ++i) {
		XExtTreeNode *child = mModel->getChild(i);
		drawNode(dc, child, 0, w, h, &y);
	}
}
void XExtTree::drawNode( HDC dc, XExtTreeNode *n, int level, int clientWidth, int clientHeight, int *py ) {
	if (*py >= clientHeight) return;
	if (*py + TREE_NODE_HEIGHT <= 0) goto _drawChild;
	int y = *py;
	// draw level ver line
	XExtTreeNode *pa = n->getParent();
	for (int i = 0, j = level - 1; i < level; ++i, --j, pa = pa->getParent()) {
		if (pa->getPosInfo() & XExtTreeNode::PI_LAST)
			continue;
		int lx = -mHorBar->getPos() + j * TREE_NODE_HEADER_WIDTH + TREE_NODE_BOX_LEFT + TREE_NODE_BOX / 2;
		MoveToEx(dc, lx, y, NULL);
		LineTo(dc, lx, y + TREE_NODE_HEIGHT);
	}
	// draw hor line
	int x = -mHorBar->getPos() + level * TREE_NODE_HEADER_WIDTH;
	MoveToEx(dc, x+TREE_NODE_BOX_LEFT+TREE_NODE_BOX/2, y+TREE_NODE_HEIGHT/2, NULL);
	LineTo(dc, x+TREE_NODE_HEADER_WIDTH-2, y+TREE_NODE_HEIGHT/2);
	// draw ver line
	XExtTreeNode::PosInfo pi = n->getPosInfo();
	int ly = y + TREE_NODE_HEIGHT, fy = y;
	if (pi & XExtTreeNode::PI_LAST) {
		ly = y + TREE_NODE_HEIGHT / 2;
	}
	if ((pi & XExtTreeNode::PI_FIRST) && level == 0) {
		fy = y + TREE_NODE_HEIGHT / 2;
	}
	MoveToEx(dc, x+TREE_NODE_BOX_LEFT + TREE_NODE_BOX/2, fy, NULL);
	LineTo(dc, x+TREE_NODE_BOX_LEFT + TREE_NODE_BOX/2, ly);
	// draw box
	if (n->getChildCount() > 0) {
		int lx = x + TREE_NODE_BOX_LEFT;
		int ly = y + (TREE_NODE_HEIGHT - TREE_NODE_BOX) / 2;
		Rectangle(dc, lx, ly, lx + TREE_NODE_BOX, ly + TREE_NODE_BOX);
		ly += TREE_NODE_BOX / 2;
		MoveToEx(dc, lx + 4, ly, NULL);
		LineTo(dc, lx + TREE_NODE_BOX - 4, ly);
		if (! n->isExpand()) {
			lx += TREE_NODE_BOX / 2 - 1;
			ly -= TREE_NODE_BOX / 2;
			MoveToEx(dc, lx, ly + 5, NULL);
			LineTo(dc, lx, ly + TREE_NODE_BOX - 4);
		}
	}

	// draw node content
	x += TREE_NODE_HEADER_WIDTH;
	RECT r = {x, y, x+n->getContentWidth(), y+TREE_NODE_HEIGHT};
	if (mSelectNode == n) FillRect(dc, &r, mSelectBgBrush);
	if (n->isCheckable()) {
		// draw check box
		HGDIOBJ old = SelectObject(dc, mCheckPen);
		Rectangle(dc, x, y + (TREE_NODE_HEIGHT - TREE_NODE_BOX) / 2, 
			x + TREE_NODE_BOX, y + (TREE_NODE_HEIGHT + TREE_NODE_BOX) / 2);
		if (n->isChecked()) {
			int LH = 2, BX = 4 + x, BY = 7 + y + (TREE_NODE_HEIGHT - TREE_NODE_BOX) / 2, N = 3;
			for (int i = 0; i < N; ++i) {
				MoveToEx(dc, BX + i, BY + i, NULL);
				LineTo(dc, BX + i, BY + i + LH);
			}
			for (int i = 0; i < 5; ++i) {
				MoveToEx(dc, BX + N + i, BY + N - 1 - i, NULL);
				LineTo(dc, BX + N + i, BY + N - 1 - i + LH);
			}
		}
		SelectObject(dc, old);
		r.left += TREE_NODE_BOX + 2;
	}

	if (mNodeRender != NULL) {
		mNodeRender->onDrawNode(dc, n, r.left, r.top, r.right-r.left, r.bottom-r.top);
	} else {
		if (n->getText()) {
			if (mAttrFlags & AF_COLOR) SetTextColor(dc, mAttrColor);
			DrawText(dc, n->getText(), strlen(n->getText()), &r, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		}
	}
	
	_drawChild:
	*py = *py + TREE_NODE_HEIGHT;
	if (n->getChildCount() > 0 && n->isExpand()) {
		for (int i = 0; i < n->getChildCount(); ++i) {
			XExtTreeNode *ss = n->getChild(i);
			drawNode(dc, ss, level + 1, clientWidth, clientHeight, py);
		}
	}
}
void XExtTree::notifyChanged() {
	if (mWidthSpec != 0 && mHeightSpec != 0) {
		onMeasure(mWidthSpec, mHeightSpec);
		InvalidateRect(mWnd, NULL, TRUE);
		UpdateWindow(mWnd);
	}
}
void XExtTree::onLBtnDown( int x, int y ) {
	POINT pt = {x, y};
	int y2 = 0;
	XExtTreeNode * node = getNodeAtY(y, &y2);
	if (node == NULL) return;
	int x2 = (node->getLevel() + 1) * TREE_NODE_HEADER_WIDTH;
	RECT cntRect = {x2, y2, x2 + node->getContentWidth(), y2 + TREE_NODE_HEIGHT};
	if (PtInRect(&cntRect, pt)) { // click in node content
		if (mWhenSelect == WHEN_CLICK) {
			setSelectNode(node);
		}
		return;
	}
	if (node->getChildCount() == 0) // has no child
		return;
	x2 = node->getLevel() * TREE_NODE_HEADER_WIDTH + TREE_NODE_BOX_LEFT;
	int yy = y2 + (TREE_NODE_HEIGHT - TREE_NODE_BOX) / 2;
	RECT r = {x2, yy, x2 + TREE_NODE_BOX, yy + TREE_NODE_BOX};
	if (PtInRect(&r, pt)) { // click in expand box
		node->setExpand(! node->isExpand());
		notifyChanged();
	}
}
static XExtTreeNode * GetNodeAtY(XExtTreeNode *n, int y, int *py) {
	if (y >= *py && y < *py + TREE_NODE_HEIGHT) {
		return n;
	}
	*py = *py + TREE_NODE_HEIGHT;
	if (n->isExpand()) {
		for (int i = 0; i < n->getChildCount(); ++i) {
			XExtTreeNode *cc = GetNodeAtY(n->getChild(i), y, py);
			if (cc != NULL) return cc;
		}
	}
	return NULL;
}
XExtTreeNode * XExtTree::getNodeAtY( int y, int *py ) {
	if (mModel == NULL) return NULL;
	int y2 = -mVerBar->getPos();
	XExtTreeNode *n = NULL;
	for (int i = 0; i < mModel->getChildCount(); ++i) {
		n = GetNodeAtY(mModel->getChild(i), y, &y2);
		if (n != NULL) break;
	}
	*py = y2;
	return n;
}
void XExtTree::onLBtnDbClick( int x, int y ) {
	POINT pt = {x, y};
	int y2 = 0;
	XExtTreeNode * node = getNodeAtY(y, &y2);
	if (node == NULL) {
		return;
	}
	int x2 = (node->getLevel() + 1) * TREE_NODE_HEADER_WIDTH;
	RECT cntRect = {x2, y2, x2 + node->getContentWidth(), y2 + TREE_NODE_HEIGHT};
	// db-click not in node content
	if (! PtInRect(&cntRect, pt)) {
		return;
	}
	if (node->getChildCount() != 0) {
		// node->setExpand(! node->isExpand());
		// notifyChanged();
	}
	if (mWhenSelect == WHEN_DBCLICK) {
		setSelectNode(node);
	}
}
void XExtTree::setNodeRender( NodeRender *render ) {
	mNodeRender = render;
}
static bool GetNodeRect(XExtTreeNode *n, XExtTreeNode *target, int *py) {
	if (n == target) {
		return true;
	}
	*py = *py + TREE_NODE_HEIGHT;
	if (n->isExpand()) {
		for (int i = 0; i < n->getChildCount(); ++i) {
			bool fd = GetNodeRect(n->getChild(i), target, py);
			if (fd) {
				return true;
			}
		}
	}
	return false;
}
bool XExtTree::getNodeRect( XExtTreeNode *node, RECT *r ) {
	if (r == NULL) return false;
	r->left = r->right = r->top = r->bottom = 0;
	if (mModel == NULL || node == NULL) {
		return false;
	}
	int y = -mVerBar->getPos();
	bool finded = false;
	for (int i = 0; i < mModel->getChildCount(); ++i) {
		finded = GetNodeRect(mModel->getChild(i), node, &y);
		if (finded) break;
	}
	if (finded) {
		r->left = -mHorBar->getPos() + (node->getLevel() + 1) * TREE_NODE_HEADER_WIDTH;
		r->right = r->left + node->getContentWidth();
		r->top = y;
		r->bottom = r->top + TREE_NODE_HEIGHT;
	}
	return finded;
}

XExtTreeNode * XExtTree::getAtPoint(int x, int y) {
	int y2 = 0;
	XExtTreeNode * node = getNodeAtY(y, &y2);
	if (node == NULL) {
		return NULL;
	}
	int x2 = (node->getLevel() + 1) * TREE_NODE_HEADER_WIDTH;
	if (x >= x2 && x < x2 + node->getContentWidth()) {
		return node;
	}
	return NULL;
}

XExtTree::WhenSelect XExtTree::getWhenSelect() {
	return mWhenSelect;
}

void XExtTree::setWhenSelect(WhenSelect when) {
	mWhenSelect = when;
}

XExtTreeNode * XExtTree::getSelectNode() {
	return mSelectNode;
}

void XExtTree::setSelectNode(XExtTreeNode *node) {
	XExtTreeNode *old = mSelectNode;
	if (mSelectNode == node) {
		return;
	}
	mSelectNode = node;
	InvalidateRect(mWnd, NULL, TRUE);
	SendMessage(mWnd, MSG_TREE_SEL_CHANGED, (WPARAM)node, (LPARAM)old);
	
	if (node != NULL && node->isCheckable()) {
		node->setChecked(! node->isChecked());
		InvalidateRect(mWnd, NULL, TRUE);
		SendMessage(mWnd, MSG_TREE_CHECK_CHANGED, (WPARAM)node, 0);
	}
}

//---------------------------XExtCalender--------------
static const int CALENDER_HEAD_HEIGHT = 30;
XExtCalendar::Date::Date() {
	mYear = mMonth = mDay = 0;
}
bool XExtCalendar::Date::isValid() {
	if (mYear <= 0 || mMonth <= 0 || mDay <= 0 || mMonth > 12 || mDay > 31)
		return false;
	int d = XExtCalendar::getDaysNum(mYear, mMonth);
	return d >= mDay;
}
bool XExtCalendar::Date::equals( const Date &d ) {
	return mYear == d.mYear && mMonth == d.mMonth && mDay == d.mDay;
}

XExtCalendar::XExtCalendar( XmlNode *node ) : XExtComponent(node) {
	mViewMode = VM_SEL_DAY;
	memset(&mLeftArrowRect, 0, sizeof(mLeftArrowRect));
	memset(&mRightArowRect, 0, sizeof(mRightArowRect));
	memset(&mHeadTitleRect, 0, sizeof(mHeadTitleRect));
	mSelectLeftArrow = false;
	mSelectRightArrow = false;
	mSelectHeadTitle = false;
	mTrackSelectIdx = -1;
	mArrowNormalBrush = CreateSolidBrush(RGB(0x5c, 0x5c, 0x6c));
	mArrowSelBrush = CreateSolidBrush(RGB(0x64, 0x95, 0xED));
	mSelectBgBrush = CreateSolidBrush(RGB(0x64, 0x95, 0xED));
	mTrackBgBrush = CreateSolidBrush(RGB(0xA4, 0xD3, 0xEE));
	mLinePen = CreatePen(PS_SOLID, 1, RGB(0x5c, 0x5c, 0x6c));
	time_t cur = time(NULL);
	struct tm *st = localtime(&cur);
	mYearInDayMode = 1900 + st->tm_year;
	mMonthInDayMode = st->tm_mon + 1;
	mYearInMonthMode = mYearInDayMode;
	mBeginYearInYearMode = (st->tm_year + 1900) / 10 * 10;
	mEndYearInYearMode = mBeginYearInYearMode + 9;
	fillViewDates(mYearInDayMode, mMonthInDayMode);
	mNormalColor = RGB(0x35, 0x35, 0x35);
	mGreyColor = RGB(0xcc, 0xcc, 0xcc);
	mTrackMouseLeave = false;
}
void XExtCalendar::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	mLeftArrowRect.right = CALENDER_HEAD_HEIGHT;
	mLeftArrowRect.bottom = CALENDER_HEAD_HEIGHT;
	mRightArowRect.left = mMesureWidth - CALENDER_HEAD_HEIGHT;
	mRightArowRect.right = mMesureWidth;
	mRightArowRect.bottom = CALENDER_HEAD_HEIGHT;
	mHeadTitleRect.left = CALENDER_HEAD_HEIGHT;
	mHeadTitleRect.right = mMesureWidth - CALENDER_HEAD_HEIGHT;
	mHeadTitleRect.bottom = CALENDER_HEAD_HEIGHT;
}
bool XExtCalendar::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		HDC memDc = CreateCompatibleDC(dc);
		if (mMemBuffer == NULL) mMemBuffer = XImage::create(mWidth, mHeight, 24);
		SelectObject(memDc, mMemBuffer->getHBitmap());
		eraseBackground(memDc);
		SelectObject(memDc, getFont());
		drawHeader(memDc);
		if (mViewMode == VM_SEL_DAY) drawSelDay(memDc);
		else if (mViewMode == VM_SEL_MONTH) drawSelMonth(memDc);
		else drawSelYear(memDc);
		BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
		DeleteObject(memDc);
		EndPaint(mWnd, &ps);
		return true;
	} else if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		POINT pt = {x, y};
		if (PtInRect(&mHeadTitleRect, pt)) {
			mViewMode = ViewMode((mViewMode + 1) % VM_NUM);
			if (mViewMode == VM_SEL_DAY) {
				mYearInMonthMode = mYearInDayMode;
				mBeginYearInYearMode = mYearInDayMode / 10 * 10;
				mEndYearInYearMode = mBeginYearInYearMode + 9;
			}
			InvalidateRect(mWnd, NULL, TRUE);
			return true;
		}
		switch (mViewMode) {
		case VM_SEL_DAY:
			onLButtonDownInDayMode(x, y);
			break;
		case VM_SEL_MONTH:
			onLButtonDownInMonthMode(x, y);
			break;
		case VM_SEL_YEAR:
			onLButtonDownInYearMode(x, y);
			break;
		}
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam);
		POINT pt = {x, y};
		if (! mTrackMouseLeave) {
			mTrackMouseLeave = true;
			TRACKMOUSEEVENT a = {0};
			a.cbSize = sizeof(TRACKMOUSEEVENT);
			a.dwFlags = TME_LEAVE;
			a.hwndTrack = mWnd;
			TrackMouseEvent(&a);
		}
		resetSelect();
		if (PtInRect(&mLeftArrowRect, pt)) {
			mSelectLeftArrow = true;
		}
		if (PtInRect(&mRightArowRect, pt)) {
			mSelectRightArrow = true;
		}
		if (PtInRect(&mHeadTitleRect, pt)) {
			mSelectHeadTitle = true;
		}
		if (mViewMode == VM_SEL_DAY) onMouseMoveInDayMode(x, y);
		else if (mViewMode == VM_SEL_MONTH) onMouseMoveInMonthMode(x, y);
		else if (mViewMode == VM_SEL_YEAR) onMouseMoveInYearMode(x, y);
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	} else if (msg == WM_MOUSELEAVE) {
		mTrackMouseLeave = false;
		resetSelect();
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
void XExtCalendar::drawSelDay( HDC dc ) {
	static const char *hd[7] = {"一", "二", "三", "四", "五", "六", "日"};
	int W = mMesureWidth / 7, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 7;
	SetTextColor(dc, mNormalColor);
	RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
	for (int i = 0; i < 7; ++i) {
		DrawText(dc, hd[i], 2, &r, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
		OffsetRect(&r, W, 0);
	}

	RECT rc = {0, CALENDER_HEAD_HEIGHT + H, W, CALENDER_HEAD_HEIGHT + H * 2};
	char buf[4];
	for (int i = 0; i < 42; ++i) {
		if (mTrackSelectIdx == i) {
			FillRect(dc, &rc, mTrackBgBrush);
		}
		if (mSelectDate.equals(mViewDates[i])) {
			FillRect(dc, &rc, mSelectBgBrush);
		}
		sprintf(buf, "%d", mViewDates[i].mDay);
		SetTextColor(dc, (mViewDates[i].mMonth == mMonthInDayMode ? mNormalColor : mGreyColor));
		DrawText(dc, buf, strlen(buf), &rc, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
		if ((i + 1) % 7 != 0) OffsetRect(&rc, W, 0);
		else OffsetRect(&rc, -6 * W, H);
	}
}
void XExtCalendar::drawSelMonth( HDC dc ) {
	static const char *hd[12] = {"一月", "二月", "三月", "四月", "五月", "六月", "七月", 
		"八月", "九月", "十月", "十一月", "十二月"};
	int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
	RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
	SetTextColor(dc, mNormalColor);
	for (int i = 0; i < 12; ++i) {
		if (mTrackSelectIdx == i) {
			FillRect(dc, &r, mTrackBgBrush);
		}
		DrawText(dc, hd[i], strlen(hd[i]), &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		if ((i + 1) % 4) OffsetRect(&r, W, 0);
		else OffsetRect(&r, -3 * W, H);
	}
}
void XExtCalendar::drawSelYear( HDC dc ) {
	int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
	RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
	SetTextColor(dc, mNormalColor);
	char buf[8];
	for (int i = 0; i < 10; ++i) {
		if (mTrackSelectIdx == i) {
			FillRect(dc, &r, mTrackBgBrush);
		}
		sprintf(buf, "%d", mBeginYearInYearMode + i);
		DrawText(dc, buf, strlen(buf), &r, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		if ((i + 1) % 4) OffsetRect(&r, W, 0);
		else OffsetRect(&r, -3 * W, H);
	}
}
void XExtCalendar::drawHeader( HDC dc ) {
	// draw left arrow
	int X = 6, H = 16, MH = CALENDER_HEAD_HEIGHT;
	POINT pt[3] = {{X, MH/2}, {X+H/(2*0.57735), (MH-H)/2}, {X+H/(2*0.57735), (MH+H)/2}};
	HRGN rgn = CreatePolygonRgn(pt, 3, ALTERNATE);
	FillRgn(dc, rgn, (mSelectLeftArrow ? mArrowSelBrush : mArrowNormalBrush));
	DeleteObject(rgn);
	// draw right arrow
	X = mRightArowRect.left + 10;
	POINT pt2[3] = {{X, (MH-H)/2}, {X, (MH+H)/2}, {X+H/(2*0.57735), MH/2}};
	rgn = CreatePolygonRgn(pt2, 3, ALTERNATE);
	FillRgn(dc, rgn, (mSelectRightArrow ? mArrowSelBrush : mArrowNormalBrush));
	DeleteObject(rgn);
	// draw header title
	SetBkMode(dc, TRANSPARENT);
	SetTextColor(dc, (mSelectHeadTitle ? RGB(0x64, 0x95, 0xED) : mNormalColor));
	char buf[30];
	if (mViewMode == VM_SEL_DAY) {
		sprintf(buf, "%d年%d月", mYearInDayMode, mMonthInDayMode);
	} else if (mViewMode == VM_SEL_MONTH) {
		sprintf(buf, "%d年",mYearInMonthMode);
	} else {
		sprintf(buf, "%d - %d",mBeginYearInYearMode, mEndYearInYearMode);
	}
	DrawText(dc, buf, strlen(buf), &mHeadTitleRect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
	// draw header line
	SelectObject(dc, mLinePen);
	MoveToEx(dc, 0, MH - 1, NULL);
	LineTo(dc, mMesureWidth, MH - 1);
}
XExtCalendar::Date XExtCalendar::getSelectDate() {
	return mSelectDate;
}
void XExtCalendar::setSelectDate( Date d ) {
	if (! d.isValid()) {
		time_t ct = time(NULL);
		struct tm *cur = localtime(&ct);
		d.mYear = cur->tm_year + 1900;
		d.mMonth = cur->tm_mon + 1;
		mSelectDate.mYear = mSelectDate.mMonth = mSelectDate.mDay = 0;
	} else {
		mSelectDate = d;
	}
	mYearInDayMode = d.mYear;
	mMonthInDayMode = d.mMonth;
	mYearInMonthMode = mYearInDayMode;
	mBeginYearInYearMode = d.mYear / 10 * 10;
	mEndYearInYearMode = mBeginYearInYearMode + 9;
	fillViewDates(mYearInDayMode, mMonthInDayMode);
}
void XExtCalendar::fillViewDates( int year, int month ) {
	int mm = month, yy = year, dd = 1;
	if (mm == 1 || mm == 2) {
		mm += 12;
		yy--;
	}
	int week = (dd + 2 * mm + 3 * (mm + 1 ) / 5 + yy + yy / 4 - yy / 100 + yy / 400) % 7; // week = 0 ~ 6
	
	int daysNum = getDaysNum(year, month);
	int skipRow = 0;
	if (daysNum + week < 42 - 14) skipRow = 1;

	int lastYear = month == 1 ? year - 1 : year;
	int lastMonth = month == 1 ? 12 : month - 1;
	int lastMonthDaysNum = getDaysNum(lastYear, lastMonth);
	int lastIn = skipRow * 7 + week;
	for (int i = lastMonthDaysNum - lastIn + 1, j = 0; j < lastIn; ++j, ++i) {
		mViewDates[j].mYear = lastYear;
		mViewDates[j].mMonth = lastMonth;
		mViewDates[j].mDay = i;
	}
	int i = lastIn;
	for (int j = 0; j < daysNum; ++j, ++i) {
		mViewDates[i].mYear = year;
		mViewDates[i].mMonth = month;
		mViewDates[i].mDay = j + 1;
	}
	int nextYear = month == 12 ? year + 1 : year;
	int nextMonth = month == 12 ? 1 : month + 1;
	for (int j = 0; i < 42; ++i, ++j) {
		mViewDates[i].mYear = nextYear;
		mViewDates[i].mMonth = nextMonth;
		mViewDates[i].mDay = j + 1;
	}
}
int XExtCalendar::getDaysNum( int year, int month ) {
	static int DAYS_NUM[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month != 2) return DAYS_NUM[month - 1];
	if (year % 100 != 0 && year % 4 == 0)
		return 29;
	return 28;
}
void XExtCalendar::onLButtonDownInDayMode( int x, int y ) {
	POINT pt = {x, y};
	if (PtInRect(&mLeftArrowRect, pt)) {
		if (mMonthInDayMode == 1) {
			mMonthInDayMode = 12;
			--mYearInDayMode;
		} else {
			--mMonthInDayMode;
		}
		fillViewDates(mYearInDayMode, mMonthInDayMode);
		InvalidateRect(mWnd, NULL, TRUE);
	}  else if (PtInRect(&mRightArowRect, pt)) {
		if (mMonthInDayMode == 12) {
			mMonthInDayMode = 1;
			++mYearInDayMode;
		} else {
			++mMonthInDayMode;
		}
		fillViewDates(mYearInDayMode, mMonthInDayMode);
		InvalidateRect(mWnd, NULL, TRUE);
	} else {
		int W = mMesureWidth / 7, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 7;
		pt.y -= CALENDER_HEAD_HEIGHT + H;
		RECT rc = {0, 0, W, H};
		for (int i = 0; i < 42; ++i) {
			if (PtInRect(&rc, pt)) {
				mSelectDate = mViewDates[i];
				InvalidateRect(mWnd, NULL, TRUE);
				SendMessage(mWnd, MSG_CALENDAR_SEL_DATE, (WPARAM)&mSelectDate, 0);
				break;
			}
			if ((i + 1) % 7 != 0) OffsetRect(&rc, W, 0);
			else OffsetRect(&rc, -6 * W, H);
		}
	}
	mYearInMonthMode = mYearInDayMode;
	mBeginYearInYearMode = mYearInDayMode / 10 * 10;
	mEndYearInYearMode = mBeginYearInYearMode + 9;
}
void XExtCalendar::onLButtonDownInMonthMode( int x, int y ) {
	POINT pt = {x, y};
	if (PtInRect(&mLeftArrowRect, pt)) {
		--mYearInMonthMode;
		InvalidateRect(mWnd, NULL, TRUE);
	} else if (PtInRect(&mRightArowRect, pt)) {
		++mYearInMonthMode;
		InvalidateRect(mWnd, NULL, TRUE);
	} else {
		int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
		RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
		for (int i = 0; i < 12; ++i) {
			if (PtInRect(&r, pt)) {
				mViewMode = VM_SEL_DAY;
				mYearInDayMode = mYearInMonthMode;
				mMonthInDayMode = i + 1;
				fillViewDates(mYearInDayMode, mMonthInDayMode);
				InvalidateRect(mWnd, NULL, TRUE);
				break;
			}
			if ((i + 1) % 4) OffsetRect(&r, W, 0);
			else OffsetRect(&r, -3 * W, H);
		}
	}
	mBeginYearInYearMode = mYearInMonthMode / 10 * 10;
	mEndYearInYearMode = mBeginYearInYearMode + 9;
}
void XExtCalendar::onLButtonDownInYearMode( int x, int y ) {
	POINT pt = {x, y};
	if (PtInRect(&mLeftArrowRect, pt)) {
		mBeginYearInYearMode -= 10;
		mEndYearInYearMode -= 10;
		InvalidateRect(mWnd, NULL, TRUE);
	} else if (PtInRect(&mRightArowRect, pt)) {
		mBeginYearInYearMode += 10;
		mEndYearInYearMode += 10;
		InvalidateRect(mWnd, NULL, TRUE);
	} else {
		int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
		RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
		for (int i = 0; i < 10; ++i) {
			if (PtInRect(&r, pt)) {
				mViewMode = VM_SEL_MONTH;
				mYearInMonthMode = mBeginYearInYearMode + i;
				InvalidateRect(mWnd, NULL, TRUE);
				break;
			}
			if ((i + 1) % 4) OffsetRect(&r, W, 0);
			else OffsetRect(&r, -3 * W, H);
		}
	}
}
XExtCalendar::~XExtCalendar() {
	DeleteObject(mArrowNormalBrush);
	DeleteObject(mArrowSelBrush);
	DeleteObject(mSelectBgBrush);
	DeleteObject(mTrackBgBrush);
	DeleteObject(mLinePen);
}
void XExtCalendar::resetSelect() {
	mSelectLeftArrow = false;
	mSelectRightArrow = false;
	mSelectHeadTitle = false;
	mTrackSelectIdx = -1;
}
void XExtCalendar::onMouseMoveInDayMode( int x, int y ) {
	POINT pt = {x, y};
	int W = mMesureWidth / 7, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 7;
	pt.y -= CALENDER_HEAD_HEIGHT + H;
	RECT rc = {0, 0, W, H};
	for (int i = 0; i < 42; ++i) {
		if (PtInRect(&rc, pt)) {
			mTrackSelectIdx = i;
			break;
		}
		if ((i + 1) % 7 != 0) OffsetRect(&rc, W, 0);
		else OffsetRect(&rc, -6 * W, H);
	}
}
void XExtCalendar::onMouseMoveInMonthMode( int x, int y ) {
	POINT pt = {x, y};
	int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
	RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
	for (int i = 0; i < 12; ++i) {
		if (PtInRect(&r, pt)) {
			mTrackSelectIdx = i;
			break;
		}
		if ((i + 1) % 4) OffsetRect(&r, W, 0);
		else OffsetRect(&r, -3 * W, H);
	}
}
void XExtCalendar::onMouseMoveInYearMode( int x, int y ) {
	POINT pt = {x, y};
	int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
	RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
	for (int i = 0; i < 10; ++i) {
		if (PtInRect(&r, pt)) {
			mTrackSelectIdx = i;
			break;
		}
		if ((i + 1) % 4) OffsetRect(&r, W, 0);
		else OffsetRect(&r, -3 * W, H);
	}
}
//------------------------XExtMaskEditor-----------------------------------
XExtMaskEdit::XExtMaskEdit( XmlNode *node ) : XExtTextArea(node) {
	mCase = C_NONE;
	mCaretBrush = CreateSolidBrush(RGB(0x48, 0x76, 0xFF));
	char *str = mNode->getAttrValue("placeHolder");
	if (str == NULL) mPlaceHolder = ' ';
	else mPlaceHolder = str[0];
	setMask(mNode->getAttrValue("mask"));
	str = mNode->getAttrValue("case");
	if (str != NULL && strcmp(str, "upper") == 0) mCase = C_UPPER;
	else if (str != NULL && strcmp(str, "lower") == 0) mCase = C_LOWER;
	mValidate = NULL;
}
bool XExtMaskEdit::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	 if (msg == WM_LBUTTONDOWN) {
		if (mEnableFocus) SetFocus(mWnd);
		int x = (short)lParam, y = (short)(lParam >> 16);
		int pos = getPosAt(x, y);
		if (pos >= 0) {
			mInsertPos = pos;
			mCaretShowing = true;
			InvalidateRect(mWnd, NULL, TRUE);
			UpdateWindow(mWnd);
		}
		return true;
	} else if (msg == WM_LBUTTONUP) {
		return true;
	} else if (msg == WM_MOUSEMOVE) {
		return true;
	}
	return XExtTextArea::wndProc(msg, wParam, lParam, result);
}
void XExtMaskEdit::onChar( wchar_t ch ) {
	if (mReadOnly) return;
	if (ch < 32 || ch > 126) return;
	if (mCase == C_LOWER) {
		ch = tolower(ch);
	} else if (mCase == C_UPPER) {
		ch = toupper(ch);
	}
	if (mValidate) {
		if (mValidate(mInsertPos, ch)) {
			mWideText[mInsertPos] = ch;
			onKeyDown(VK_RIGHT);
		}
	} else {
		if (acceptChar(ch, mInsertPos)) {
			mWideText[mInsertPos] = ch;
			onKeyDown(VK_RIGHT);
		}
	}
	ensureVisible(mInsertPos);
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
}

bool XExtMaskEdit::acceptChar( char ch, int pos ) {
	if (mMask == NULL) return false;
	if (pos >= strlen(mMask)) return false;
	char m = mMask[pos];
	if (m == '0') {
		return ch >= '0' && ch <= '9';
	}
	if (m == '9') {
		return ch >= '1' && ch <= '9';
	}
	if (m == 'A') {
		return ch >= 'A' && ch <= 'Z';
	}
	if (m == 'a') {
		return ch >= 'a' && ch <= 'a';
	}
	if (m == 'C') {
		return (ch >= 'a' && ch <= 'a') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
	}
	if (m == 'H') {
		return (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') || (ch >= '0' && ch <= '9');
	}
	if (m == 'B') {
		return ch == '0' || ch == '1';
	}
	return false;
}
void XExtMaskEdit::onPaint( HDC hdc ) {
	HFONT font = getFont();
	SelectObject(hdc, font);
	SetBkMode(hdc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(hdc, mAttrColor);
	RECT r = {getScrollX(), 0, mWidth - getScrollX(), mHeight};
	POINT pt = {0, 0};
	if (mInsertPos >= 0 && mCaretShowing && getPointAt(mInsertPos, &pt)) {
		SelectObject(hdc, mCaretPen);
		SIZE sz;
		GetTextExtentPoint32W(hdc, mWideText + mInsertPos, 1, &sz);
		RECT rc = {pt.x, (mHeight-sz.cy)/2-2, pt.x + sz.cx, (mHeight+sz.cy)/2+2};
		FillRect(hdc, &rc, mCaretBrush);
	}
	DrawTextW(hdc, mWideText, mWideTextLen, &r, DT_SINGLELINE | DT_VCENTER);
}
void XExtMaskEdit::onKeyDown( int key ) {
	if (mWideTextLen == 0) return;
	if (key == VK_BACK || key == VK_DELETE) {// back
		if (mInsertPos >= 0) {
			mWideText[mInsertPos] = mPlaceHolder;
			onKeyDown(key == VK_BACK ? VK_LEFT : VK_RIGHT);
		}
		return;
	}
	if (key < VK_END || key > VK_DOWN) return;
	if (key == VK_END) {
		for (int i = mWideTextLen - 1; i >= 0; --i) {
			if (isMaskChar(mMask[i])) {
				mInsertPos = i;
				break;
			}
		}
	} else if (key == VK_HOME) {
		for (int i = 0; i < mWideTextLen; ++i) {
			if (isMaskChar(mMask[i])) {
				mInsertPos = i;
				break;
			}
		}
	} else if (key == VK_LEFT) {
		for (int i = mInsertPos - 1; i >= 0; --i) {
			if (isMaskChar(mMask[i])) {
				mInsertPos = i;
				break;
			}
		}
	} else if (key == VK_RIGHT) {
		for (int i = mInsertPos + 1; i < mWideTextLen; ++i) {
			if (isMaskChar(mMask[i])) {
				mInsertPos = i;
				break;
			}
		}
	}
	ensureVisible(mInsertPos);
	InvalidateRect(mWnd, NULL, TRUE);
}
int XExtMaskEdit::getPosAt( int x, int y ) {
	HDC hdc = GetDC(mWnd);
	HGDIOBJ old = SelectObject(hdc, getFont());
	int k = -1;
	x -= getScrollX();
	for (int i = 0; i < mWideTextLen; ++i) {
		SIZE sz;
		GetTextExtentPoint32W(hdc, mWideText, i + 1, &sz);
		if (sz.cx >= x) {
			k = i;
			break;
		}
	}
	SelectObject(hdc, old);
	ReleaseDC(mWnd, hdc);
	if (k < 0 || ! isMaskChar(mMask[k])) return -1;
	return k;
}
void XExtMaskEdit::setMask( const char *mask ) {
	mMask = (char *)mask;
	mWideTextLen = 0;
	if (mMask == NULL) {
		return;
	}
	int len = strlen(mMask);
	wchar_t v;
	for (int i = 0; i < len; ++i) {
		v = mMask[i];
		if (isMaskChar(mMask[i]))
			insertText(i, &mPlaceHolder, 1);
		else 
			insertText(i, &v, 1);
	}
	mInsertPos = -1;
	// find first mask char
	for (int i = 0; i < len; ++i) {
		if (isMaskChar(mMask[i])) {
			mInsertPos = i;
			break;
		}
	}
}
bool XExtMaskEdit::isMaskChar( char ch ) {
	static char MC[] = {'0', '9', 'A', 'a', 'C', 'H', 'B'};
	for (int i = 0; i < sizeof(MC); ++i) {
		if (MC[i] == ch) return true;
	}
	return false;
}
void XExtMaskEdit::setPlaceHolder( char ch ) {
	if (ch >= 32 && ch <= 127) mPlaceHolder = ch;
}
void XExtMaskEdit::setInputValidate( InputValidate iv ) {
	mValidate = iv;
}
// ----------------------XExtPassword--------------------
XExtPassword::XExtPassword( XmlNode *node ) : XExtTextArea(node) {
}
void XExtPassword::onChar( wchar_t ch ) {
	if (ch > 126) return;
	if (ch > 31) {
		if (mWideTextLen < 63) XExtTextArea::onChar(ch);
	} else {
		XExtTextArea::onChar(ch);
	}
}
void XExtPassword::paste() {
	// ignore it
}
void XExtPassword::onPaint( HDC hdc ) {
	// draw select range background color
	drawSelRange(hdc, mBeginSelPos, mEndSelPos);
	HFONT font = getFont();
	SelectObject(hdc, font);
	SetBkMode(hdc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(hdc, mAttrColor);
	RECT r = {getScrollX(), 0, mWidth - getScrollX(), mHeight};
	char echo[64];
	memset(echo, '*', mWideTextLen);
	echo[mWideTextLen] = 0;
	DrawText(hdc, echo, mWideTextLen, &r, DT_SINGLELINE | DT_VCENTER);
	POINT pt = {0, 0};
	if (mCaretShowing && getPointAt(mInsertPos, &pt)) {
		SelectObject(hdc, mCaretPen);
		MoveToEx(hdc, pt.x, 2, NULL);
		LineTo(hdc, pt.x, mHeight - 4);
	}
}
// --------------------XExtDatePicker-------------------
XExtDatePicker::XExtDatePicker( XmlNode *node ) : XExtComponent(node) {
	mEditNode = new XmlNode("ExtMaskEdit", mNode);
	mPopupNode = new XmlNode("ExtPopup", mNode);
	mCalendarNode = new XmlNode("ExtCalendar", mPopupNode);
	mEdit = new XExtMaskEdit(mEditNode);
	mPopup = new XExtPopup(mPopupNode);
	mCalendar = new XExtCalendar(mCalendarNode);
	mEditNode->setComponent(mEdit);
	mPopupNode->setComponent(mPopup);
	mCalendarNode->setComponent(mCalendar);
	mCalendar->setAttrBgColor(RGB(0xF5, 0xFF, 0xFA));
	mPopup->setListener(this);
	mCalendar->setListener(this);
	mCalendar->setEnableFocus(false);
	mEdit->setMask("0000-00-00");

	mArrowRect.left = mArrowRect.top = 0;
	mArrowRect.right = mArrowRect.bottom = 0;
	mAttrArrowSize.cx = mAttrArrowSize.cy = 0;
	AttrUtils::parseArraySize(mNode->getAttrValue("arrowSize"), (int *)&mAttrArrowSize, 2);
	mAttrPopupSize.cx = mAttrPopupSize.cy = 0;
	AttrUtils::parseArraySize(mNode->getAttrValue("popupSize"), (int *)&mAttrPopupSize, 2);
	mEdit->setReadOnly(AttrUtils::parseBool(mNode->getAttrValue("readOnly")));

	mArrowNormalImage = XImage::load(mNode->getAttrValue("arrowNormal"));
	mArrowDownImage = XImage::load(mNode->getAttrValue("arrowDown"));
	mPoupShow = false;
}
XExtDatePicker::~XExtDatePicker() {
	delete mEditNode;
	delete mPopupNode;
	delete mCalendarNode;
}
void XExtDatePicker::createWnd() {
	XExtComponent::createWnd();
	XComponent *cc = mEdit;
	cc->createWnd();
	cc = mPopup;
	cc->createWnd();
	mCalendar->createWnd();
}
bool XExtDatePicker::onEvent( XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret ) {
	if (msg == MSG_CALENDAR_SEL_DATE) {
		XExtCalendar::Date *val = (XExtCalendar::Date *)wParam;
		char buf[20];
		sprintf(buf, "%d-%02d-%02d", val->mYear, val->mMonth, val->mDay);
		mEdit->setText(buf);
		mPopup->close();
		mPoupShow = false;
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	} else if (msg == MSG_POPUP_CLOSED) {
		mPoupShow = false;
		InvalidateRect(mWnd, NULL, TRUE);
		return true;
	}
	return false;
}
bool XExtDatePicker::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		// draw arrow
		XImage *img = mPoupShow ? mArrowDownImage : mArrowNormalImage;
		if (img != NULL && img->getHBitmap() != NULL) {
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, img->getHBitmap());
			int mw = min(mArrowRect.right - mArrowRect.left, img->getWidth());
			int mh = min(mArrowRect.bottom - mArrowRect.top, img->getHeight());
			if (img->hasAlphaChannel())  {
				BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
				AlphaBlend(dc, mArrowRect.left, mArrowRect.top, mw, 
					mh, memDc, 0, 0, img->getWidth(), img->getHeight(), bf);
			} else {
				BitBlt(dc, mArrowRect.left, mArrowRect.top, mw, mh, memDc, 0, 0, SRCCOPY);
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
		if (PtInRect(&mArrowRect, pt)) {
			openPopup();
		}
		return true;
	}
	return XExtComponent::wndProc(msg, wParam, lParam, result);
}
void XExtDatePicker::onMeasure( int widthSpec, int heightSpec ) {
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
	XComponent *cc = mPopup;
	cc->onMeasure(pw | MS_FIX, ph | MS_FIX);
	cc = mCalendar;
	cc->onMeasure(pw | MS_FIX, ph | MS_FIX);
}
void XExtDatePicker::onLayout( int width, int height ) {
	mEdit->layout(0, 0, mEdit->getMesureWidth(), mEdit->getMesureHeight());
	mPopup->layout(0, 0, mPopup->getMesureWidth(), mPopup->getMesureHeight());
	mCalendar->layout(0, 0, mCalendar->getMesureWidth(), mCalendar->getMesureHeight());
}
void XExtDatePicker::openPopup() {
	POINT pt = {0, mHeight};
	ClientToScreen(mWnd, &pt);
	mPoupShow = true;
	InvalidateRect(mWnd, NULL, TRUE);
	UpdateWindow(mWnd);
	char str[24];
	strcpy(str, mEdit->getText());
	XExtCalendar::Date cur;
	cur.mYear = atoi(str);
	cur.mMonth = atoi(str + 5);
	cur.mDay = atoi(str + 8);
	mCalendar->setSelectDate(cur);
	mPopup->show(pt.x, pt.y);
}
char * XExtDatePicker::getText() {
	return mEdit->getText();
}


//--------------------------XAbsLayout-----------------------------
XAbsLayout::XAbsLayout( XmlNode *node ) : XExtComponent(node) {
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
XHLineLayout::XHLineLayout(XmlNode *node) : XExtComponent(node) {
}

void XHLineLayout::onLayout( int width, int height ) {
	int x = mAttrPadding[0], weightAll = 0, childWidths = 0, lessWidth = width - mAttrPadding[0] - mAttrPadding[2];
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		weightAll += child->getAttrWeight();
		childWidths += child->getMesureWidth() + child->getAttrMargin()[0] + child->getAttrMargin()[2];
	}
	lessWidth -= childWidths;
	double perWeight = 0;
	if (weightAll > 0 && lessWidth > 0) perWeight = lessWidth / weightAll;
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int y  = calcSize(child->getAttrY(), (height - mAttrPadding[1] - mAttrPadding[3]) | MS_ATMOST);
		y += mAttrPadding[1];
		x += child->getAttrMargin()[0];
		if (child->getAttrWeight() > 0 && perWeight > 0) {
			int nw = child->getMesureWidth() + child->getAttrWeight() * perWeight;
			child->onMeasure(nw | MS_FIX , child->getMesureHeight() | MS_FIX);
		}
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
		x += child->getMesureWidth() + child->getAttrMargin()[2];
	}
}
//--------------------------XVLineLayout--------------------------
XVLineLayout::XVLineLayout(XmlNode *node) : XExtComponent(node) {
}

void XVLineLayout::onLayout( int width, int height ) {
	int y = mAttrPadding[1], weightAll = 0, childHeights = 0, lessHeight = height - mAttrPadding[1] - mAttrPadding[3];
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		weightAll += child->getAttrWeight();
		childHeights += child->getMesureHeight() + child->getAttrMargin()[1] + child->getAttrMargin()[3];
	}
	lessHeight -= childHeights;
	double perWeight = 0;
	if (weightAll > 0 && lessHeight > 0) perWeight = lessHeight / weightAll;
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		XComponent *child = mNode->getChild(i)->getComponent();
		int x = calcSize(child->getAttrX(), (width - mAttrPadding[0] - mAttrPadding[2]) | MS_ATMOST);
		x += mAttrPadding[0];
		y += child->getAttrMargin()[1];
		if (child->getAttrWeight() > 0 && perWeight > 0) {
			int nh = child->getMesureHeight() + child->getAttrWeight() * perWeight;
			child->onMeasure(child->getMesureWidth() | MS_FIX , nh | MS_FIX);
		}
		child->layout(x, y, child->getMesureWidth(), child->getMesureHeight());
		y += child->getMesureHeight() + child->getAttrMargin()[3];
	}
}
//---------------XExtWindow------------------------------
XExtWindow::XExtWindow( XmlNode *node ) : XWindow(node) {
	strcpy(mClassName, "XExtWindow");
	memset(mBorders, 0, sizeof(mBorders));
	AttrUtils::parseArrayInt(mNode->getAttrValue("border"), mBorders, 4);
	mSizable = AttrUtils::parseBool(mNode->getAttrValue("sizable"));
}
void XExtWindow::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId();  // has no id
	mWnd = CreateWindow(mClassName, mNode->getAttrValue("text"), WS_POPUP | WS_MINIMIZEBOX | WS_SYSMENU,
		0, 0, 0, 0, getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
	applyIcon();
}

RECT XExtWindow::getClientRect() {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	r.left += mBorders[0];
	r.top += mBorders[1];
	r.right -= mBorders[2];
	r.bottom -= mBorders[3];
	return r;
}

XExtDialog::XExtDialog(XmlNode *node) : XDialog(node) {
	memset(mBorders, 0, sizeof(mBorders));
	AttrUtils::parseArrayInt(mNode->getAttrValue("border"), mBorders, 4);
}

void XExtDialog::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId(); // has no id
	mWnd = CreateWindow(mClassName, NULL, WS_POPUP,
		0, 0, 0, 0, getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

RECT XExtDialog::getClientRect() {
	RECT r = {0};
	GetClientRect(mWnd, &r);
	r.left += mBorders[0];
	r.top += mBorders[1];
	r.right -= mBorders[2];
	r.bottom -= mBorders[3];
	return r;
}
