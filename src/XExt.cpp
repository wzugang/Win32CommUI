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

void XTableModel::apply(HWND tableWnd) {
	if (tableWnd == NULL)
		return;
	LVCOLUMN c = {0};

	for (int i = 0; i < getColumnCount(); ++i) {
		getColumn(i, &c);
		SendMessage(tableWnd, LVM_INSERTCOLUMN, 0, (LPARAM)&c);
	}

	LVITEM item = {0};
	for (int i = 0; i < getRowCount(); ++i) {
		for (int j = 0; j < getColumnCount(); ++j) {
			getItem(i, j, &item);
			if (j == 0)
				ListView_InsertItem(tableWnd, &item);
			else 
				ListView_SetItem(tableWnd, &item);
		}
	}
}

void XTableModel::getColumn( int col, LVCOLUMN *lv ) {
	memset(lv, 0, sizeof(LVCOLUMN));
	lv->mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lv->fmt = LVCFMT_LEFT;
	lv->cx = getColumnWidth(col);
	lv->pszText = getColumnTitle(col);
}

void XTableModel::getItem( int row, int col, LVITEM *item ) {
	memset(item, 0, sizeof(LVITEM));
	item->mask = LVIF_TEXT;
	item->iItem = row;
	item->iSubItem = col;
	item->pszText = NULL;
}
//--------------------XExtLabel-------------------------------------
XExtLabel::XExtLabel( XmlNode *node ) : XComponent(node) {
	mBgImageForParnet = NULL;
	mText = mNode->getAttrValue("text");
}

void XExtLabel::layout(int x, int y, int width, int height) {
	XComponent::layout(x, y, width, height);
	if (mBgImageForParnet != NULL) {
		mBgImageForParnet->decRef();
		mBgImageForParnet = NULL;
	}
}

bool XExtLabel::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		if (mBgImage == NULL && mBgImageForParnet == NULL && (mAttrFlags & AF_BG_COLOR) == 0) {
			HDC memDc = CreateCompatibleDC((HDC)wParam);
			HWND parent = getParentWnd();
			mBgImageForParnet = XImage::create(mWidth, mHeight);
			SelectObject(memDc, mBgImageForParnet->getHBitmap());
			HDC dc = GetDC(parent);
			BitBlt(memDc, 0, 0, mWidth, mHeight, dc, mX, mY, SRCCOPY);
			DeleteObject(memDc);
			ReleaseDC(parent, dc);
		}
		if (mAttrFlags & AF_BG_COLOR) {
			HDC dc = (HDC)wParam;
			HBRUSH brush = CreateSolidBrush(mAttrBgColor);
			RECT rc = {0, 0, mWidth, mHeight};
			FillRect(dc, &rc, brush);
			DeleteObject(brush);
		}
		XImage *bg = mBgImageForParnet != NULL ? mBgImageForParnet : mBgImage;
		if (bg != NULL && bg->getHBitmap() != NULL) {
			HDC dc = (HDC)wParam;
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, bg->getHBitmap());
			BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
			DeleteObject(memDc);
		}
		return true;
	} else if (msg == WM_PAINT) {
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
	return XComponent::wndProc(msg, wParam, lParam, result);
}

char * XExtLabel::getText() {
	return mText;
}

void XExtLabel::setText( char *text ) {
	mText = text;
}

//-------------------XExtButton-----------------------------------
XExtButton::XExtButton( XmlNode *node ) : XComponent(node) {
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

void XExtButton::layout(int x, int y, int width, int height) {
	XComponent::layout(x, y, width, height);
	if (mBgImage != NULL) {
		mBgImage->decRef();
		mBgImage = NULL;
	}
}

bool XExtButton::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		if (mBgImage == NULL) {
			HDC memDc = CreateCompatibleDC((HDC)wParam);
			HWND parent = getParentWnd();
			mBgImage = XImage::create(mWidth, mHeight);
			SelectObject(memDc, mBgImage->getHBitmap());
			HDC dc = GetDC(parent);
			BitBlt(memDc, 0, 0, mWidth, mHeight, dc, mX, mY, SRCCOPY);
			DeleteObject(memDc);
			ReleaseDC(parent, dc);
		}
		if (mBgImage != NULL && mBgImage->getHBitmap() != NULL) {
			HDC dc = (HDC)wParam;
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, mBgImage->getHBitmap());
			BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
			DeleteObject(memDc);
		}
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};

		XImage *cur = mImages[getBtnImage()];
		if (cur != NULL && cur->getHBitmap() != NULL) {
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, cur->getHBitmap());
			StretchBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, cur->getWidth(), cur->getHeight(), SRCCOPY);
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
	return XComponent::wndProc(msg, wParam, lParam, result);
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

XExtButton::~XExtButton() {
	for (int i = 0; i < sizeof(mImages)/sizeof(XImage*); ++i) {
		if (mImages[i] != NULL)
			mImages[i]->decRef();
	}
}

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
		mIsSelect = mIsMouseDown && PtInRect(&r, pt);
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

XExtCheckBox::XExtCheckBox( XmlNode *node ) : XExtOption(node) {
	mBgImageForParnet = NULL;
}

bool XExtCheckBox::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_ERASEBKGND) {
		if (mBgImage == NULL && mBgImageForParnet == NULL && (mAttrFlags & AF_BG_COLOR) == 0) {
			HDC memDc = CreateCompatibleDC((HDC)wParam);
			HWND parent = getParentWnd();
			mBgImageForParnet = XImage::create(mWidth, mHeight);
			SelectObject(memDc, mBgImageForParnet->getHBitmap());
			HDC dc = GetDC(parent);
			BitBlt(memDc, 0, 0, mWidth, mHeight, dc, mX, mY, SRCCOPY);
			DeleteObject(memDc);
			ReleaseDC(parent, dc);
		}
		if (mAttrFlags & AF_BG_COLOR) {
			HDC dc = (HDC)wParam;
			HBRUSH brush = CreateSolidBrush(mAttrBgColor);
			RECT rc = {0, 0, mWidth, mHeight};
			FillRect(dc, &rc, brush);
			DeleteObject(brush);
		}
		XImage *bg = mBgImageForParnet != NULL ? mBgImageForParnet : mBgImage;
		if (bg != NULL && bg->getHBitmap() != NULL) {
			HDC dc = (HDC)wParam;
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, bg->getHBitmap());
			BitBlt(dc, 0, 0, mWidth, mHeight, memDc, 0, 0, SRCCOPY);
			DeleteObject(memDc);
		}
		return true;
	} else if (msg == WM_PAINT) {
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(mWnd, &ps);
		RECT r = {0, 0, mWidth, mHeight};
		XImage *cur = mImages[getBtnImage()];
		if (cur != NULL && cur->getHBitmap() != NULL) {
			HDC memDc = CreateCompatibleDC(dc);
			SelectObject(memDc, cur->getHBitmap());
			int y = (mHeight - cur->getHeight()) / 2;
			BitBlt(dc, 0, y, cur->getWidth(), cur->getHeight(), memDc, 0, 0, SRCCOPY);
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

XExtRadio::XExtRadio( XmlNode *node ) : XExtCheckBox(node) {
}

bool XExtRadio::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_LBUTTONUP) {
		POINT pt = {(LONG)(short)LOWORD(lParam), (LONG)(short)HIWORD(lParam)};
		RECT r = {0, 0, mWidth, mHeight};
		if (mIsMouseDown && PtInRect(&r, pt) && mIsSelect) 
			return true;
		mIsSelect = true;
		return XExtButton::wndProc(msg, wParam, lParam, result);
	}
	return XExtCheckBox::wndProc(msg, wParam, lParam, result);
}

XExtScroll::XExtScroll( XmlNode *node ) : XScroll(node) {
	mHorBar = new XScrollBar(this, true);
	mVerBar = new XScrollBar(this, false);
}

bool XExtScroll::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (msg == WM_NCPAINT) {
		return true;
	}
	return XScroll::wndProc(msg, wParam, lParam, result);
}

XExtScroll::~XExtScroll() {
	delete mHorBar;
	delete mVerBar;
}

XScrollBar::XScrollBar( XComponent *owner, bool horizontal ) {
	mOwner = owner;
	mHorizontal = horizontal;
	mX = mY = mWidth = mHeight = 0;
	mRange = 0;
	mVisible = false;
}

void XScrollBar::onPaint( HDC hdc ) {
	if (! mVisible) return;
	int barSize = 0;
	if (mRange > 0) barSize = mHorizontal ? (mWidth * mWidth / mRange) : (mHeight * mHeight / mRange);
	RECT r = getRect();
	HBRUSH br = CreateSolidBrush(RGB(250, 20, 20));
	FillRect(hdc, &r, br);
	RECT tmp = {20, 20, 100, 100};
	FillRect(hdc, &r, br);
}

void XScrollBar::onEvent( UINT msg, WPARAM wParam, LPARAM lParam ) {
	if (msg == WM_SIZE) {
		if (mHorizontal) {
			mX = 0;
			mWidth = LOWORD(lParam);
			mHeight = 15;
			mY = HIWORD(lParam) - mHeight;
		} else {
			mY = 0;
			mHeight = HIWORD(lParam);
			mWidth = 15;
			mX = LOWORD(lParam) - mWidth;
		}
	}
}

RECT XScrollBar::getRect() {
	RECT r = {mX, mY, mX + mWidth, mY + mHeight};
	return r;
}

void XScrollBar::setRange( int range ) {
	mRange = range;
}

void XScrollBar::setVisible(bool visible) {
	mVisible = visible;
}

XExtPopup::XExtPopup( XmlNode *node ) : XContainer(node) {
	strcpy(mClassName, "XExtPopup");
}

void XExtPopup::createWnd() {
	MyRegisterClass(mInstance, mClassName);
	// mID = generateWndId();  // has no id
	mWnd = CreateWindow(mClassName, NULL, WS_POPUP,
		getSpecSize(mAttrX), getSpecSize(mAttrY), getSpecSize(mAttrWidth), getSpecSize(mAttrHeight),
		getParentWnd(), NULL, mInstance, this);
	SetWindowLong(mWnd, GWL_USERDATA, (LONG)this);
	applyAttrs();
}

bool XExtPopup::wndProc( UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result ) {
	if (XContainer::wndProc(msg, wParam, lParam, result))
		return true;
	if (msg == WM_SIZE && lParam > 0) {
		RECT r = {0};
		GetWindowRect(mWnd, &r);
		onMeasure(LOWORD(lParam) | XComponent::MS_FIX, HIWORD(lParam) | XComponent::MS_FIX);
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
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP || msg.message == WM_CHAR || msg.message == WM_IME_CHAR) {
				// transfer the message to menu window
				msg.hwnd = mWnd;
			} else if (msg.message == WM_LBUTTONDOWN || msg.message == WM_LBUTTONUP || msg.message == WM_NCLBUTTONDOWN || msg.message == WM_NCLBUTTONUP) {
				POINT pt;
				GetCursorPos(&pt);
				if (! PtInRect(&popupRect, pt)) { // click on other window
					break;
				}
			} else if (msg.message == WM_QUIT) {
				break;
			}
		} else {
			MsgWaitForMultipleObjects(0, 0, 0, 10, QS_ALLINPUT);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ShowWindow(mWnd, SW_HIDE);
	return 0;
}

void XExtPopup::show( int x, int y ) {
	SetWindowPos(mWnd, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOZORDER);
	// ShowWindow(mWnd, SW_SHOWNOACTIVATE);
	UpdateWindow(mWnd);
	messageLoop();
}

void XExtPopup::close() {
	ShowWindow(mWnd, SW_HIDE);
}

XExtPopup::~XExtPopup() {
	if (mWnd) DestroyWindow(mWnd);
}
