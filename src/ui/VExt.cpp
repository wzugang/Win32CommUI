#include "VExt.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include "VComponent.h"
#include "XmlParser.h"
#include "UIFactory.h"

// AlphaBlend function in here
#pragma   comment(lib, "msimg32.lib")

extern void MyRegisterClassV(HINSTANCE ins, const char *className);

//--------------------VExtComponent-------------------------------------
VExtComponent::VExtComponent(XmlNode *node) : VComponent(node) {
	mEnableState = false;
	memset(mStateImages, 0, sizeof(mStateImages));
	mMouseDown = mMouseMoving = mMouseLeave = false;

	for (int i = 0; i < mNode->getAttrsCount(); ++i) {
		XmlNode::Attr *attr = mNode->getAttr(i);
		if (strcmp(attr->mName, "normalImage") == 0) {
			mStateImages[STATE_IMG_NORMAL] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "hoverImage") == 0) {
			mStateImages[STATE_IMG_HOVER] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "pushImage") == 0) {
			mStateImages[STATE_IMG_PUSH] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "focusImage") == 0) {
			mStateImages[STATE_IMG_FOCUS] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "disableImage") == 0) {
			mStateImages[STATE_IMG_DISABLE] = XImage::load(attr->mValue);
		} else if (strcmp(attr->mName, "enableState") == 0) {
			mEnableState = AttrUtils::parseBool(attr->mValue, false);
		}
	}
}

VExtComponent::~VExtComponent() {
}

VExtComponent::StateImage VExtComponent::getStateImage(void *param1, void *param2) {
	if (! mEnable) {
		return STATE_IMG_DISABLE;
	}
	if (mHasFocus) {
		return STATE_IMG_FOCUS;
	}
	if (mMouseDown && ! mMouseLeave) {
		return STATE_IMG_PUSH;
	}
	if (!mMouseDown && mMouseMoving) {
		return STATE_IMG_HOVER;
	}
	if (mMouseLeave) {
		return STATE_IMG_NORMAL;
	}

	return STATE_IMG_NORMAL;
}

bool VExtComponent::doStateImage(Msg *m) {
	switch (m->mId) {
	case Msg::LBUTTONDOWN: {
		mMouseDown = true;
		mMouseMoving = false;
		mMouseLeave = false;
		setCapture();
		repaint(NULL);
		return true;}
	case Msg::LBUTTONUP: {
		RECT r = {0, 0, mWidth, mHeight};
		POINT pt = {m->mouse.x, m->mouse.y};
		bool md = mMouseDown;
		releaseCapture();
		mMouseDown = false;
		if (PtInRect(&r, pt)) {
			mMouseMoving = true;
		} else {
			mMouseMoving = false;
		}
		repaint(NULL);
		if (md && PtInRect(&r, pt) && mListener != NULL) {
			m->mId = Msg::CLICK;
			mListener->onEvent(this, m);
		}
		return true;}
	case Msg::MOUSE_MOVE: {
		StateImage bi = getStateImage(NULL, NULL);
		POINT pt = {m->mouse.x, m->mouse.y};
		RECT r = {0, 0, mWidth, mHeight};
		if (PtInRect(&r, pt)) {
			mMouseMoving = true;
			mMouseLeave = false;
		} else {
			// mouse leave ( mouse is down now)
			mMouseMoving = false;
			mMouseLeave = true;
		}
		StateImage bi2 = getStateImage(NULL, NULL);
		if (bi != bi2) {
			repaint(NULL);
		}
		return true;}
	case Msg::MOUSE_LEAVE: {
		mMouseMoving = false;
		mMouseLeave = true;
		repaint(NULL);
		return true;}
	case Msg::MOUSE_CANCEL: {
		mMouseDown = false;
		mMouseMoving = false;
		mMouseLeave = false;
		repaint(NULL);
		return true;}
	}
	return false;
}

bool VExtComponent::onMouseEvent(Msg *m) {
	if (mEnableState) {
		return doStateImage(m);
	}
	return false;
}

void VExtComponent::onPaint(Msg *m) {
	eraseBackground(m);
	if (mEnableState) {
		StateImage si = getStateImage(NULL, NULL);
		XImage *cur = mStateImages[si];
		if (cur != NULL) {
			cur->draw(m->paint.dc, mAttrPadding[0], mAttrPadding[1], 
				mWidth - mAttrPadding[0] - mAttrPadding[2], 
				mHeight - mAttrPadding[1] - mAttrPadding[3]);
		}
	}
}

//--------------------VLabel-------------------------------------
VLabel::VLabel( XmlNode *node ) : VExtComponent(node) {
	mText = mNode->getAttrValue("text");
	mTextAlign = 0;

	char *align = mNode->getAttrValue("align");
	if (align != NULL && strstr(align, "center") != NULL) {
		mTextAlign |= DT_CENTER;
	}
	if (align != NULL && strstr(align, "ver-center") != NULL) {
		mTextAlign |= DT_VCENTER;
	}
	if (align != NULL && strstr(align, "right") != NULL) {
		mTextAlign |= DT_RIGHT;
	}
	if (align != NULL && strstr(align, "bottom") != NULL) {
		mTextAlign |= DT_BOTTOM;
	}
	if (align != NULL && strstr(align, "single-line") != NULL) {
		mTextAlign |= DT_SINGLELINE;
	}
}

void VLabel::onPaint(Msg *m) {
	HDC dc = m->paint.dc;
	RECT r = {mAttrPadding[0], mAttrPadding[1], mWidth - mAttrPadding[2], mHeight - mAttrPadding[3]};
	eraseBackground(m);

	if (mText == NULL) {
		return;
	}
	if (mAttrFlags & AF_COLOR) {
		SetTextColor(dc, mAttrColor & 0xffffff);
	} else {
		SetTextColor(dc, 0);
	}
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());
	DrawText(dc, mText, -1, &r, mTextAlign);
}

char * VLabel::getText() {
	return mText;
}

void VLabel::setText( char *text ) {
	mText = text;
}


//-------------------VButton-----------------------------------
VButton::VButton( XmlNode *node ) : VExtComponent(node) {
	mEnableState = true;
}

bool VButton::onMouseEvent(Msg *m) {
	if (mEnableState) {
		return doStateImage(m);
	}
	return false;
}

void VButton::onPaint(Msg *m) {
	HDC dc = m->paint.dc;
	RECT r = {0,0, mWidth, mHeight};
	StateImage si = getStateImage(NULL, NULL);
	XImage *cur = mStateImages[si];
	eraseBackground(m);
	if (cur != NULL) {
		cur->draw(dc, 0, 0, mWidth, mHeight);
	}

	if (mAttrFlags & AF_COLOR) {
		SetTextColor(dc, mAttrColor);
	} else {
		SetTextColor(dc, 0);
	}
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());
	DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}


//-------------------VExtOption-----------------------------------
VOption::VOption( XmlNode *node ) : VButton(node) {
	mAutoSelect = true;
	mSelected = false;
	char *s = mNode->getAttrValue("selectImage");
	if (s != NULL) {
		mStateImages[BTN_IMG_SELECT] = XImage::load(s);
	}
	s = mNode->getAttrValue("autoSelect");
	if (s != NULL) mAutoSelect = AttrUtils::parseBool(s);
}

bool VOption::isSelect() {
	return mSelected;
}

void VOption::setSelect( bool select ) {
	if (mSelected != select) {
		mSelected = select;
		repaint(NULL);
	}
}

void VOption::setAutoSelect(bool autoSelect) {
	mAutoSelect = autoSelect;
}

VOption::StateImage VOption::getStateImage(void *param1, void *param2) {
	if (mSelected) {
		return StateImage(BTN_IMG_SELECT);
	}
	return VButton::getStateImage(param1, param2);
}

bool VOption::doStateImage(Msg *m) {
	if (m->mId == Msg::LBUTTONUP) {
		XRect r (0, 0, mWidth, mHeight);
		if (mAutoSelect && mMouseDown && r.contains(m->mouse.x, m->mouse.y)) {
			setSelect(! mSelected);
		}
		// go through
	}
	return VButton::doStateImage(m);
}

//-------------------VExtCheckBox-----------------------------------
VCheckBox::VCheckBox( XmlNode *node ) : VOption(node) {
}

void VCheckBox::onPaint(Msg *m) {
	HDC dc = m->paint.dc;
	RECT r = {0,0, mWidth, mHeight};
	StateImage si = getStateImage(NULL, NULL);
	XImage *cur = mStateImages[si];

	eraseBackground(m);
	if (cur != NULL) {
		int y = (mHeight - cur->getHeight()) / 2;
		cur->draw(dc, 0, y, cur->getWidth(), cur->getHeight());
		r.left = cur->getWidth() + 5 + mAttrPadding[0];
	}

	if (mAttrFlags & AF_COLOR) {
		SetTextColor(dc, mAttrColor);
	} else {
		SetTextColor(dc, 0);
	}
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());
	DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_VCENTER | DT_SINGLELINE);
}

//-------------------VExtRadio-----------------------------------
VRadio::VRadio( XmlNode *node ) : VCheckBox(node) {
}

void VRadio::unselectOthers() {
	char *groupName = mNode->getAttrValue("group");
	if (groupName == NULL) return;
	XmlNode *parent = mNode->getParent();
	for (int i = 0; parent != NULL && i < parent->getChildCount(); ++i) {
		VRadio *child = dynamic_cast<VRadio*>(parent->getComponentV()->getChild(i));
		if (child == NULL || child == this) continue;
		if (child->mSelected && strcmp(groupName, child->getNode()->getAttrValue("group")) == 0) {
			child->setSelect(false);
		}
	}
}

void VRadio::setSelect(bool select) {
	if (mSelected == select) {
		return;
	}
	mSelected = select;
	if (mSelected) {
		unselectOthers();
	}
	repaint(NULL);
}

//-----------------------VExtIconButton----------------------------
VIconButton::VIconButton(XmlNode *node) : VOption(node) {
	memset(mAttrIconRect, 0, sizeof(mAttrIconRect));
	memset(mAttrTextRect, 0, sizeof(mAttrTextRect));
	AttrUtils::parseArraySize(mNode->getAttrValue("iconRect"), mAttrIconRect, 4);
	AttrUtils::parseArraySize(mNode->getAttrValue("textRect"), mAttrTextRect, 4);
	mIcon = XImage::load(mNode->getAttrValue("icon"));
}

RECT VIconButton::getRectBy(int *attr) {
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

void VIconButton::onPaint(Msg *m) {
	HDC dc = m->paint.dc;
	StateImage si = getStateImage(NULL, NULL);
	XImage *cur = mStateImages[si];

	eraseBackground(m);
	if (cur != NULL) {
		cur->draw(dc, 0, 0, mWidth, mHeight);
	}
	if (mAttrFlags & AF_COLOR) {
		SetTextColor(dc, mAttrColor);
	} else {
		SetTextColor(dc, 0);
	}
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());
	RECT r = getRectBy(mAttrTextRect);
	DrawText(dc, mNode->getAttrValue("text"), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	if (mIcon != NULL) {
		r = getRectBy(mAttrIconRect);
		mIcon->draw(dc, r.left, r.top, r.right - r.left, r.bottom - r.top);
	}
}

//-------------------VScrollBar-----------------------------------
VScrollBar::VScrollBar( XmlNode *node) : VExtComponent(node) {
	mTrack = XImage::load(mNode->getAttrValue("track"));
	mThumb = XImage::load(mNode->getAttrValue("thumb"));
	mHorizontal = false;
	char *org = mNode->getAttrValue("orientation");
	if (org != NULL && strcmp(org, "hor") == 0) {
		mHorizontal = true;
	}
	mPos = 0;
	mMax = 0;
	mPage = 0;
	mPressed = false;
	mMoving = false;
	mMouseX = mMouseY = 0;
}

int VScrollBar::getPos() {
	return mPos;
}

void VScrollBar::setPos( int pos ) {
	int old = mPos;
	if (pos > mMax - mPage) {
		pos = mMax - mPage;
	}
	if (pos < 0) {
		pos = 0;
	}
	mPos = pos;
	if (mPos == old) {
		return;
	}
	if (! mMoving) {
		mThumbRect = calcThumbRect();
	}
	if (mListener != NULL) {
		Msg m;
		m.mId = mHorizontal ? Msg::HSCROLL : Msg::VSCROLL;
		m.def.wParam = mPos;
		mListener->onEvent(this, &m);
	}
	repaint(NULL);
}

int VScrollBar::getPage() {
	return mPage;
}

int VScrollBar::getMax() {
	return mMax;
}

void VScrollBar::setMaxAndPage( int maxn, int page ) {
	if (maxn < 0) maxn = 0;
	mMax = maxn;
	if (page < 0) page = 0;
	mPage = page;
	if (maxn <= page) {
		mPos = 0;
	} else if (mPos > mMax - mPage) {
		mPos = mMax - mPage;
	}
	mThumbRect = calcThumbRect();
}

bool VScrollBar::onMouseEvent(Msg *m) {
	if (m->mId == Msg::LBUTTONDOWN) {
		setCapture();
		if (! mThumbRect.contains(m->mouse.x, m->mouse.y)) {
			mPressed = false;
			return true;
		}
		mPressed = true;
		mMoving = false;
		mMouseX = m->mouse.x;
		mMouseY = m->mouse.y;
		return true;
	} else if (m->mId == Msg::LBUTTONUP) {
		mPressed = false;
		mMoving = false;
		releaseCapture();
	} else if (m->mId == Msg::MOUSE_CANCEL) {
		mPressed = false;
		mMoving = false;
	} else if (m->mId == Msg::MOUSE_MOVE) {
		if (! mPressed) return true;
		int newPos = mPos;
		int dx = 0, dy = 0;
		if (mHorizontal) {
			dx = m->mouse.x - mMouseX;
			if (dx == 0) return true;
			newPos = getPosBy(mThumbRect.mX + dx);
		} else {
			dy = m->mouse.y - mMouseY;
			if (dy == 0) return true;
			newPos = getPosBy(mThumbRect.mY + dy);
		}
		if (newPos != mPos) {
			mThumbRect.offset(dx, dy);
			mMouseX = m->mouse.x;
			mMouseY = m->mouse.y;
			mMoving = true;
			setPos(newPos);
		}
		return true;
	}
	return VExtComponent::onMouseEvent(m);
}

void VScrollBar::onPaint(Msg *m) {
	HDC dc = m->paint.dc;
	if (mTrack != NULL) {
		mTrack->draw(dc, 0, 0, mWidth, mHeight);
	}
	if (mThumb != NULL) {
		XRect &rr = mThumbRect;
		if (mHorizontal && mPage < mMax && mWidth > 0 && rr.mWidth == 0) {
			mThumbRect = calcThumbRect();
		}else if (!mHorizontal && mPage < mMax && mHeight > 0 && rr.mHeight == 0) {
			mThumbRect = calcThumbRect();
		}
		mThumb->draw(dc, rr.mX, rr.mY, rr.mWidth, rr.mHeight);
	}
}

XRect VScrollBar::calcThumbRect() {
	XRect rr;
	int range = getScrollRange();
	if (mMax <= 0 || mPage <= 0 || mPage >= mMax) {
		return rr;
	}
	int sz = (int)((float)range * mPage / mMax);
	int start = (int)((float)mPos * (range - sz) / (mMax - mPage));

	if (mHorizontal) {
		rr.set(start, mAttrPadding[1], sz, mHeight-mAttrPadding[1]-mAttrPadding[3]);
	} else {
		rr.set(mAttrPadding[0], start, mWidth-mAttrPadding[0]-mAttrPadding[2], sz);
	}
	return rr;
}

int VScrollBar::getPosBy(int start) {
	if (start <= 0) {
		return 0;
	}
	if (mPage >= mMax) {
		return 0;
	}
	int range = getScrollRange();
	int sz = (int)((float)range * mPage / mMax);
	if (start >= range - sz) {
		return mMax - mPage;
	}
	int pos = (int)((float)start * (mMax - mPage) / (range - sz));
	return pos;
}

int VScrollBar::getStart() {
	int range = getScrollRange();
	int sz = (int)((float)range * mPage / mMax);
	int start = (int)((float)mPos * (range - sz) / (mMax - mPage));
	return start;
}

void VScrollBar::setStart(int start) {
	int pos = getPosBy(start);
	setPos(pos);
}

void VScrollBar::setOrientation(bool hor) {
	mHorizontal = hor;
}

int VScrollBar::getScrollRange() {
	if (mHorizontal) {
		return mWidth;
	}
	return mHeight;
}

POINT VScrollBar::getDrawPoint() {
	POINT pt = {mX, mY};
	int i = 0;
	for (VComponent *cc = getParent(); cc != NULL; cc = cc->getParent(), ++i) {
		if (i == 0) {
			pt.x += cc->getX();
			pt.y += cc->getY();
		} else {
			pt.x += cc->getX() - cc->getTranslateX();
			pt.y += cc->getY() - cc->getTranslateY();
		}
	}
	return pt;
}

//----------------------------VTextArea---------------------
class TextAreaLisener : public VListener {
public:
	TextAreaLisener(VTextArea *a) {
		mArea = a;
	}
	bool onEvent(VComponent *evtSource, Msg *msg) {
		if (msg->mId == Msg::VSCROLL) {
			mArea->setScrollY(msg->def.wParam);
			mArea->repaint();
			mArea->updateWindow();
			return true;
		}
		return false;
	}
private:
	VTextArea *mArea;
};

VTextArea::VTextArea( XmlNode *node ) : VExtComponent(node) {
	mEnableFocus = true;
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
	mReadOnly = AttrUtils::parseBool(mNode->getAttrValue("readOnly"));
	insertText(0, mNode->getAttrValue("text"));
	mCaretShowing = false;
	mCaretPen = CreatePen(PS_SOLID, 1, RGB(0xff, 0x14, 0x93));
	mEnableShowCaret = AttrUtils::parseBool(mNode->getAttrValue("showCaret"), true);
	mVerBar = NULL;
	mEnableScrollBars = true;
	/*if (mAttrPadding[0] == 0 && mAttrPadding[2] == 0) {
		mAttrPadding[0] = mAttrPadding[2] = 2;
	}*/
	mAutoNewLine = true;
	mScrollX = mScrollY = 0;
}

bool VTextArea::dispatchMessage(Msg *m) {
	if (m->mId == Msg::TIMER) {
		mCaretShowing = !mCaretShowing;
		repaint();
		return true;
	} else if (m->mId == Msg::GAIN_FOCUS) {
		if (mEnableShowCaret) {
			getRoot()->startTimer(this, mID, 500);
		}
		return true;
	} else if (m->mId == Msg::LOST_FOCUS) {
		mCaretShowing = false;
		getRoot()->killTimer(this, mID);
		repaint();
		return true;
	}
	return VExtComponent::dispatchMessage(m);
}


bool VTextArea::onMouseEvent(Msg *m) {
	if (m->mId == Msg::LBUTTONDOWN) {
		setCapture();
		if (mEnableFocus) setFocus();
		onLButtonDown(m);
		return true;
	} else if (m->mId == Msg::LBUTTONUP) {
		releaseCapture();
		onLButtonUp(m);
	} else if (m->mId == Msg::MOUSE_CANCEL) {
		releaseCapture();
	} else if (m->mId == Msg::MOUSE_MOVE) {
		if (m->mouse.vkey.lbutton) {
			onMouseMove(m->mouse.x, m->mouse.y);
		}
		return true;
	} else if (m->mId == Msg::MOUSE_WHEEL) {
		int d = m->mouse.deta * 100;
		int ad = d < 0 ? -d : d;
		ad = min(ad, mHeight);
		ad = d < 0 ? -ad : ad;
		if (mVerBar != NULL) {
			int old = mVerBar->getPos();
			mVerBar->setPos(old - ad);
			repaint(NULL);
		}
		return true;
	}
	return false;
}

bool VTextArea::onKeyEvent(Msg *m) {
	if (m->mId == Msg::KEY_DOWN) {
		onKeyDown(m->key.code);
		return true;
	} else if (m->mId == Msg::CHAR) {
		onChar(m->key.code);
		return true;
	}
	return false;
}

void VTextArea::onChar( wchar_t ch ) {
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
		repaint(NULL);
	}
}

void VTextArea::onLButtonDown(Msg *m) {
	mCaretShowing = true;
	int x = m->mouse.x + getScrollX() - mAttrPadding[0];
	int y = m->mouse.y + getScrollY() - mAttrPadding[1];
	mInsertPos = getPosAt(x, y);
	if (m->mouse.vkey.shift) {
		mEndSelPos =  mInsertPos;
	} else {
		mBeginSelPos = mEndSelPos =  mInsertPos;
	}
	repaint(NULL);
}

void VTextArea::onLButtonUp(Msg *m) {
	if (mReadOnly) return;
	// mEndSelPos = getPosAt(x, y);
}

void VTextArea::onMouseMove(int x, int y) {
	x += getScrollX();
	y += getScrollY();
	mInsertPos = mEndSelPos = getPosAt(x, y);
	ensureVisible(mInsertPos);
	repaint();
	updateWindow();
}

void VTextArea::onPaint(Msg *m) {
	HDC hdc = m->paint.dc;
	int from = 0, to = 0;

	eraseBackground(m);
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
	int y = -getScrollY() + from * mLineHeight + mAttrPadding[1];
	for (int i = from; i < to; ++i) {
		int bg = mLines[i].mBeginPos;
		int ln = mLines[i].mLen;
		RECT rr = {mAttrPadding[0] - getScrollX(), y, mAttrPadding[0] + clientSize.cx, y + mLineHeight};
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

void VTextArea::drawSelRange( HDC hdc, int begin, int end ) {
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
		int ry = bp.y + mLineHeight * (i - brow);
		r.top = getRealY(ry);
		r.right = getRealX(i == erow ? ep.x : client.cx);
		r.bottom = getRealY(ry + mLineHeight);
		FillRect(hdc, &r, bg);
	}
}

void VTextArea::onKeyDown( int key ) {
	int ctrl = GetAsyncKeyState(VK_CONTROL) < 0;
	if (key == 'V' && ctrl && !mReadOnly) { // ctrl + v
		paste();
		ensureVisible(mInsertPos);
		repaint();
	} else if (key == 46 && !mReadOnly) { // del
		del();
		ensureVisible(mInsertPos);
		repaint();
	} else if (key >= VK_END && key <= VK_DOWN) {
		move(key);
	} else if (key == 'A' && ctrl) { // ctrl + A
		mBeginSelPos = 0;
		mEndSelPos = mWideTextLen;
		mInsertPos = mWideTextLen;
		ensureVisible(mInsertPos);
		repaint();
	} else if (key == 'C' && ctrl) { // ctrl + C
		copy();
	} else if (key == 'X' && ctrl && !mReadOnly) { // ctrl + X
		if (mBeginSelPos == mEndSelPos) return;
		copy();
		back();
		ensureVisible(mInsertPos);
		repaint();
	}
}

void VTextArea::move( int key ) {
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
	repaint();

}

void VTextArea::back() {
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
	repaint();
}

void VTextArea::del() {
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
	repaint();
}

void VTextArea::paste() {
	OpenClipboard(getWnd());
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
	repaint();
}

void VTextArea::copy() {
	if (mBeginSelPos == mEndSelPos) return;
	int bg = mBeginSelPos < mEndSelPos ? mBeginSelPos : mEndSelPos;
	int ed = mBeginSelPos > mEndSelPos ? mBeginSelPos : mEndSelPos;
	int len = WideCharToMultiByte(CP_ACP, 0, mWideText + bg, ed - bg, NULL, 0, NULL, NULL);
	HANDLE hd = GlobalAlloc(GHND, len + 1);
	char *buf = (char *)GlobalLock(hd);
	WideCharToMultiByte(CP_ACP, 0, mWideText + bg, ed - bg, buf, len, NULL, NULL);
	buf[len] = 0;
	GlobalUnlock(hd);

	OpenClipboard(getWnd());
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hd);
	CloseClipboard();
}

void VTextArea::setReadOnly( bool r ) {
	mReadOnly = r;
}

void VTextArea::setEnableShowCaret( bool enable ) {
	mEnableShowCaret = enable;
}

void VTextArea::setText( const char *txt ) {
	XAreaText::setText(txt);
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
}

void VTextArea::setWideText( const wchar_t *txt ) {
	XAreaText::setWideText(txt);
	mInsertPos = 0;
	mBeginSelPos = mEndSelPos = 0;
}

void VTextArea::notifyChanged() {
	buildLines();
	if (mVerBar == NULL) {
		repaint();
		return;
	}
	bool hasVerBar = mVerBar->isVisible();
	int mh = mMesureHeight - mAttrPadding[1] - mAttrPadding[3];
	mVerBar->setMaxAndPage(mTextHeight, mh);
	bool needShow = mTextHeight > mh;
	mVerBar->setVisible(needShow);
	if (! mVerBar->isVisible()) {
		setScrollY(0);
		mVerBar->setPos(0);
	}
	if (needShow != hasVerBar) {
		notifyChanged();
	}
	repaint();
}

void VTextArea::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);

	if (mEnableScrollBars && mVerBar == NULL) {
		XmlNode *node = new XmlNode("VerScrollBar", mNode);
		mVerBar = new VScrollBar(node);
		mVerBar->setOrientation(false);
		node->setComponentV(mVerBar);
		mVerBar->setVisible(false);
		mNode->addChild(node);
		mVerBar->setListener(new TextAreaLisener(this));
	}
	
	buildLines();

	if (mVerBar != NULL) {
		bool hasVerBar = mVerBar->isVisible();
		int mh = mMesureHeight - mAttrPadding[1] - mAttrPadding[3];
		mVerBar->setMaxAndPage(mTextHeight, mh);
		bool newHas = mTextHeight > mh;
		mVerBar->setVisible(newHas);
		if (newHas != hasVerBar) {
			onMeasure(widthSpec, heightSpec);
		}
	}
}

void VTextArea::onLayoutChildren( int width, int height ) {
	if (mVerBar != NULL) {
		mVerBar->onMeasure(mWidth | MS_ATMOST, mHeight | MS_ATMOST);
		int w = mVerBar->getMesureWidth();
		mVerBar->onLayout(mWidth - w, 0, w, mVerBar->getMesureHeight());
	}
}

int VTextArea::getScrollX() {
	return mScrollX;
}

int VTextArea::getScrollY() {
	return mScrollY;
}

void VTextArea::setScrollX( int x ) {
	mScrollX = x;
}

void VTextArea::setScrollY( int y ) {
	int mm = mTextHeight - (mMesureHeight - mAttrPadding[1] - mAttrPadding[3]);
	if (mm < 0) {
		mm = 0;
	}
	int old = mScrollY;
	mScrollY = min(y, mm);
	if (old == mScrollY) {
		return;
	}
	if (mVerBar != NULL && mVerBar->isVisible()) {
		mVerBar->setPos(y);
	}
}

void VTextArea::getVisibleRows( int *from, int *to ) {
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

void VTextArea::ensureVisible( int pos ) {
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
	} else if (pt.y + mLineHeight > getScrollY() + sz.cy) {
		setScrollY(pt.y + mLineHeight - sz.cy);
	}
	if (pt.x < getScrollX()) {
		setScrollX(pt.x);
	} else if (pt.x > getScrollX() + sz.cx) {
		setScrollX(pt.x - sz.cx);
	}
}

SIZE VTextArea::getClientSize() {
	bool hasVerBar = mVerBar != NULL && mVerBar->isVisible();
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getWidth() : 0);
	SIZE sz = {clientWidth - mAttrPadding[0] - mAttrPadding[2],
		mMesureHeight - mAttrPadding[1] - mAttrPadding[3]};
	return sz;
}

HWND VTextArea::getBindWnd() {
	return getWnd();
}

HFONT VTextArea::getTextFont() {
	return getFont();
}

int VTextArea::getRealX( int x ) {
	return x - getScrollX() + mAttrPadding[0];
}

int VTextArea::getRealY( int y ) {
	return y - getScrollY() + mAttrPadding[1];
}

//------------------------VLineEdit--------------------
VLineEdit::VLineEdit(XmlNode *node) : VTextArea(node) {
	mAutoNewLine = false;
	mEnableScrollBars = false;
}

void VLineEdit::onChar( wchar_t ch ) {
	if (ch == VK_RETURN || ch == VK_TAB) {
		return;
	}
	VTextArea::onChar(ch);
}

void VLineEdit::insertText( int pos, wchar_t *txt, int len ) {
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
	if (mWideTextLen > 0) {
		mWideText[mWideTextLen] = 0;
	}
	mNeedRebuildLines = true;
}


//------------------------VMaskEditor-----------------------------------
VMaskEdit::VMaskEdit( XmlNode *node ) : VLineEdit(node) {
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

bool VMaskEdit::onMouseEvent( Msg *m ) {
	if (m->mId == Msg::LBUTTONDOWN) {
		if (mEnableFocus) setFocus();
		int pos = getPosAt(m->mouse.x, m->mouse.y);
		if (pos >= 0) {
			mInsertPos = pos;
			mCaretShowing = true;
			repaint();
			updateWindow();
		}
		return true;
	} else if (m->mId == Msg::MOUSE_MOVE) {
		return true;
	}
	return VLineEdit::onMouseEvent(m);
}


void VMaskEdit::onChar( wchar_t ch ) {
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
	repaint();
	updateWindow();
}

bool VMaskEdit::acceptChar( wchar_t ch, int pos ) {
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

void VMaskEdit::onPaint(Msg *m) {
	HDC hdc = m->paint.dc;
	HFONT font = getFont();
	SelectObject(hdc, font);
	SetBkMode(hdc, TRANSPARENT);
	eraseBackground(m);
	if (mAttrFlags & AF_COLOR) SetTextColor(hdc, mAttrColor);
	RECT r = {getScrollX() + mAttrPadding[0], 0, mWidth + getScrollX() - mAttrPadding[2], mHeight};
	POINT pt = {0, 0};
	if (mInsertPos >= 0 && mCaretShowing && getPointAt(mInsertPos, &pt)) {
		SelectObject(hdc, mCaretPen);
		SIZE sz;
		GetTextExtentPoint32W(hdc, mWideText + mInsertPos, 1, &sz);
		pt.x += mAttrPadding[0];
		RECT rc = {pt.x, (mHeight-sz.cy)/2-2, pt.x + sz.cx, (mHeight+sz.cy)/2+2};
		FillRect(hdc, &rc, mCaretBrush);
	}
	DrawTextW(hdc, mWideText, mWideTextLen, &r, DT_SINGLELINE | DT_VCENTER);
}

void VMaskEdit::onKeyDown( int key ) {
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
	repaint();
}

int VMaskEdit::getPosAt( int x, int y ) {
	HWND wnd = getWnd();
	HDC hdc = GetDC(wnd);
	HGDIOBJ old = SelectObject(hdc, getFont());
	int k = -1;
	x = x - getScrollX() - mAttrPadding[0];
	for (int i = 0; i < mWideTextLen; ++i) {
		SIZE sz;
		GetTextExtentPoint32W(hdc, mWideText, i + 1, &sz);
		if (sz.cx >= x) {
			k = i;
			break;
		}
	}
	SelectObject(hdc, old);
	ReleaseDC(wnd, hdc);
	if (k < 0 || ! isMaskChar(mMask[k])) return -1;
	return k;
}

void VMaskEdit::setMask( const char *mask ) {
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

bool VMaskEdit::isMaskChar( char ch ) {
	static char MC[] = {'0', '9', 'A', 'a', 'C', 'H', 'B'};
	for (int i = 0; i < sizeof(MC); ++i) {
		if (MC[i] == ch) return true;
	}
	return false;
}

void VMaskEdit::setPlaceHolder( char ch ) {
	if (ch >= 32 && ch <= 127) mPlaceHolder = ch;
}

void VMaskEdit::setInputValidate( InputValidate iv ) {
	mValidate = iv;
}

// ----------------------VExtPassword--------------------
VPassword::VPassword( XmlNode *node ) : VLineEdit(node) {
}

void VPassword::onChar( wchar_t ch ) {
	if (ch > 126) return;
	if (ch > 31) {
		if (mWideTextLen < 63) VLineEdit::onChar(ch);
	} else {
		VLineEdit::onChar(ch);
	}
}

void VPassword::paste() {
	// ignore it
}

void VPassword::onPaint( Msg *m ) {
	HDC hdc = m->paint.dc;
	eraseBackground(m);
	// draw select range background color
	drawSelRange(hdc, mBeginSelPos, mEndSelPos);
	HFONT font = getFont();
	SelectObject(hdc, font);
	SetBkMode(hdc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(hdc, mAttrColor);

	RECT r = {getScrollX() + mAttrPadding[0], 0, mWidth + getScrollX() - mAttrPadding[2], mHeight};
	char echo[64];
	memset(echo, '*', mWideTextLen);
	echo[mWideTextLen] = 0;
	DrawText(hdc, echo, mWideTextLen, &r, DT_SINGLELINE | DT_VCENTER);
	POINT pt = {0, 0};
	if (mInsertPos >= 0 && mCaretShowing && getPointAt(mInsertPos, &pt)) {
		SelectObject(hdc, mCaretPen);
		pt.x += mAttrPadding[0];
		int sy = (mHeight - mLineHeight) / 2;
		int ey = (mHeight + mLineHeight) / 2;
		MoveToEx(hdc, pt.x, sy, NULL);
		LineTo(hdc, pt.x, ey);
	}
}

int VPassword::getRealY(int y) {
	int ny = VLineEdit::getRealY(y);
	return ny + (mHeight - mLineHeight) / 2;
}

//----------VScroll-----------------------------
class VScrollListener : public VListener {
public:
	VScrollListener(VScroll *s) {
		mScroll = s;
	}
	bool onEvent(VComponent *evtSource, Msg *msg) {
		if (msg->mId == Msg::HSCROLL) {
			mScroll->setTranslateX(msg->def.wParam);
			mScroll->repaint();
			return true;
		} else if (msg->mId == Msg::VSCROLL) {
			mScroll->setTranslateY(msg->def.wParam);
			mScroll->repaint();
			return true;
		}
		return false;
	}
private:
	VScroll *mScroll;
};


VScroll::VScroll( XmlNode *node ) : VExtComponent(node) {
	mDataSize.cx = mDataSize.cy = 0;
	XmlNode *horNode = new XmlNode("HorScrollBar", mNode);
	XmlNode *verNode = new XmlNode("VerScrollBar", mNode);
	mHorBar = new VScrollBar(horNode);
	mVerBar = new VScrollBar(verNode);
	mHorBar->setOrientation(true);
	mVerBar->setOrientation(false);
	horNode->setComponentV(mHorBar);
	verNode->setComponentV(mVerBar);
	mHorBar->setVisible(false);
	mVerBar->setVisible(false);
	VScrollListener *vl = new VScrollListener(this);
	mHorBar->setListener(vl);
	mVerBar->setListener(vl);
}

void VScroll::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);
	bool hasHorBar = mHorBar->isVisible();
	bool hasVerBar = mVerBar->isVisible();

	mHorBar->onMeasure((mMesureWidth - (hasVerBar ? mVerBar->getMesureWidth() : 0)) | MS_ATMOST, mMesureHeight | MS_ATMOST);
	mVerBar->onMeasure(mMesureWidth | MS_ATMOST, (mMesureHeight - (hasHorBar ? mHorBar->getMesureHeight() : 0)) | MS_ATMOST);

	SIZE client = getClientSize();
	onMeasureChildren(client.cx | MS_ATMOST, client.cy| MS_ATMOST);

	SIZE cs = mDataSize = calcDataSize();
	mHorBar->setMaxAndPage(cs.cx, client.cx);
	mVerBar->setMaxAndPage(cs.cy, client.cy);

	mHorBar->setVisible(cs.cx > client.cx);
	mVerBar->setVisible(cs.cy > client.cy);

	if (mHorBar->isVisible() != hasHorBar || mVerBar->isVisible() != hasVerBar) {
		onMeasure(widthSpec, heightSpec);
	}
}

SIZE VScroll::calcDataSize() {
	int childRight = 0, childBottom = 0;
	int mw = mMesureWidth - mAttrPadding[0] - mAttrPadding[2];
	int mh = mMesureHeight - mAttrPadding[1] - mAttrPadding[3];
	for (int i = mNode->getChildCount() - 1; i >= 0; --i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		if (! child->isVisible()) {
			continue;
		}
		int x = calcSize(child->getAttrX(), mw | MS_ATMOST);
		int y  = calcSize(child->getAttrY(), mh | MS_ATMOST);
		int cr = x + child->getMesureWidth();
		int cb = y + child->getMesureHeight();
		if (cr > childRight) childRight = cr;
		if (cb > childBottom) childBottom = cb;
	}
	SIZE sz = {childRight, childBottom};
	return sz;
}

void VScroll::onLayoutChildren( int width, int height ) {
	int mw = width - mAttrPadding[0] - mAttrPadding[2];
	int mh = height - mAttrPadding[1] - mAttrPadding[3];
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		int x = calcSize(child->getAttrX(), mw | MS_ATMOST);
		int y  = calcSize(child->getAttrY(), mh | MS_ATMOST);
		child->onLayout(x, y, child->getMesureWidth(), child->getMesureHeight());
	}
	mHorBar->onLayout(0, mHeight - mHorBar->getMesureHeight(), mWidth, mHorBar->getMesureHeight());
	mVerBar->onLayout(mWidth - mVerBar->getMesureWidth(), 0, mVerBar->getMesureWidth(), mHeight);
}

VScrollBar* VScroll::getHorBar() {
	return mHorBar;
}

VScrollBar* VScroll::getVerBar() {
	return mVerBar;
}

void VScroll::onMouseWheel(Msg *msg) {
	if (mVerBar->isVisible()) {
		int old = mVerBar->getPos();
		mVerBar->setPos(old - msg->mouse.deta * 100);
		if (old != mVerBar->getPos()) {
			mTranslateY = mVerBar->getPos();
			repaint();
		}
	} else if (mHorBar->isVisible()) {
		int old = mHorBar->getPos();
		mHorBar->setPos(old - msg->mouse.deta * 100);
		if (old != mHorBar->getPos()) {
			mTranslateX = mHorBar->getPos();
			repaint();
		}
	}
}

bool VScroll::onMouseEvent(Msg *msg) {
	if (msg->mId == Msg::MOUSE_WHEEL) {
		onMouseWheel(msg);
		return true;
	}
	return VExtComponent::onMouseEvent(msg);
}

SIZE VScroll::getClientSize() {
	bool hasHorBar = mHorBar->isVisible();
	bool hasVerBar = mVerBar->isVisible();
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getMesureWidth() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getMesureHeight() : 0);
	SIZE sz = {clientWidth, clientHeight};
	return sz;
}

int VScroll::getChildCountForDispatch(DispatchAction da) {
	return mNode->getChildCount() + 2;
}

VComponent* VScroll::getChildForDispatch(DispatchAction da, int idx) {
	int cc = mNode->getChildCount();
	if (idx < cc) {
		return getChild(idx);
	}
	idx -= cc;
	if (idx == 0) {
		return mVerBar;
	}
	return mHorBar;
}

POINT VScroll::getChildPointForDispatch(DispatchAction da, int idx, VComponent *child) {
	POINT pt = {child->getX(), child->getY()};
	if (child == mVerBar || child == mHorBar) {
		pt.x += mTranslateX;
		pt.y += mTranslateY;
	}
	return pt;
}


//---------------------------VCalender--------------
static const int CALENDER_HEAD_HEIGHT = 30;
VCalendar::Date::Date() {
	mYear = mMonth = mDay = 0;
}

bool VCalendar::Date::isValid() {
	if (mYear <= 0 || mMonth <= 0 || mDay <= 0 || mMonth > 12 || mDay > 31)
		return false;
	int d = VCalendar::getDaysNum(mYear, mMonth);
	return d >= mDay;
}

bool VCalendar::Date::equals( const Date &d ) {
	return mYear == d.mYear && mMonth == d.mMonth && mDay == d.mDay;
}

VCalendar::VCalendar( XmlNode *node ) : VExtComponent(node) {
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

void VCalendar::onMeasure( int widthSpec, int heightSpec ) {
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

void VCalendar::onPaint( Msg *m ) {
	HDC dc = m->paint.dc;
	eraseBackground(m);
	SelectObject(dc, getFont());
	drawHeader(dc);
	if (mViewMode == VM_SEL_DAY) drawSelDay(dc);
	else if (mViewMode == VM_SEL_MONTH) drawSelMonth(dc);
	else drawSelYear(dc);
}

bool VCalendar::onMouseEvent( Msg *m ) {
	if (m->mId == Msg::LBUTTONDOWN) {
		if (mEnableFocus) setFocus();
		POINT pt = {m->mouse.x, m->mouse.y};
		if (PtInRect(&mHeadTitleRect, pt)) {
			mViewMode = ViewMode((mViewMode + 1) % VM_NUM);
			if (mViewMode == VM_SEL_DAY) {
				mYearInMonthMode = mYearInDayMode;
				mBeginYearInYearMode = mYearInDayMode / 10 * 10;
				mEndYearInYearMode = mBeginYearInYearMode + 9;
			}
			repaint();
			return true;
		}
		switch (mViewMode) {
		case VM_SEL_DAY:
			onLButtonDownInDayMode(m->mouse.x, m->mouse.y);
			break;
		case VM_SEL_MONTH:
			onLButtonDownInMonthMode(m->mouse.x, m->mouse.y);
			break;
		case VM_SEL_YEAR:
			onLButtonDownInYearMode(m->mouse.x, m->mouse.y);
			break;
		}
		return true;
	} else if (m->mId == Msg::MOUSE_MOVE) {
		POINT pt = {m->mouse.x, m->mouse.y};
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
		if (mViewMode == VM_SEL_DAY) onMouseMoveInDayMode(m->mouse.x, m->mouse.y);
		else if (mViewMode == VM_SEL_MONTH) onMouseMoveInMonthMode(m->mouse.x, m->mouse.y);
		else if (mViewMode == VM_SEL_YEAR) onMouseMoveInYearMode(m->mouse.x, m->mouse.y);
		repaint();
		return true;
	} else if (m->mId == Msg::MOUSE_LEAVE) {
		resetSelect();
		repaint();
		return true;
	}
	return true;
}

void VCalendar::drawSelDay( HDC dc ) {
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

void VCalendar::drawSelMonth( HDC dc ) {
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

void VCalendar::drawSelYear( HDC dc ) {
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

void VCalendar::drawHeader( HDC dc ) {
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

VCalendar::Date VCalendar::getSelectDate() {
	return mSelectDate;
}

void VCalendar::setSelectDate( Date d ) {
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

void VCalendar::fillViewDates( int year, int month ) {
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

int VCalendar::getDaysNum( int year, int month ) {
	static int DAYS_NUM[12] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month != 2) return DAYS_NUM[month - 1];
	if (year % 100 != 0 && year % 4 == 0)
		return 29;
	return 28;
}

void VCalendar::onLButtonDownInDayMode( int x, int y ) {
	POINT pt = {x, y};
	if (PtInRect(&mLeftArrowRect, pt)) {
		if (mMonthInDayMode == 1) {
			mMonthInDayMode = 12;
			--mYearInDayMode;
		} else {
			--mMonthInDayMode;
		}
		fillViewDates(mYearInDayMode, mMonthInDayMode);
		repaint();
	}  else if (PtInRect(&mRightArowRect, pt)) {
		if (mMonthInDayMode == 12) {
			mMonthInDayMode = 1;
			++mYearInDayMode;
		} else {
			++mMonthInDayMode;
		}
		fillViewDates(mYearInDayMode, mMonthInDayMode);
		repaint();
	} else {
		int W = mMesureWidth / 7, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 7;
		pt.y -= CALENDER_HEAD_HEIGHT + H;
		RECT rc = {0, 0, W, H};
		for (int i = 0; i < 42; ++i) {
			if (PtInRect(&rc, pt)) {
				mSelectDate = mViewDates[i];
				repaint();
				if (mListener != NULL) {
					Msg msg;
					msg.mId = Msg::SELECT_ITEM;
					msg.def.wParam = (WPARAM)&mSelectDate;
					mListener->onEvent(this, &msg);
				}
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

void VCalendar::onLButtonDownInMonthMode( int x, int y ) {
	POINT pt = {x, y};
	if (PtInRect(&mLeftArrowRect, pt)) {
		--mYearInMonthMode;
		repaint();
	} else if (PtInRect(&mRightArowRect, pt)) {
		++mYearInMonthMode;
		repaint();
	} else {
		int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
		RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
		for (int i = 0; i < 12; ++i) {
			if (PtInRect(&r, pt)) {
				mViewMode = VM_SEL_DAY;
				mYearInDayMode = mYearInMonthMode;
				mMonthInDayMode = i + 1;
				fillViewDates(mYearInDayMode, mMonthInDayMode);
				repaint();
				break;
			}
			if ((i + 1) % 4) OffsetRect(&r, W, 0);
			else OffsetRect(&r, -3 * W, H);
		}
	}
	mBeginYearInYearMode = mYearInMonthMode / 10 * 10;
	mEndYearInYearMode = mBeginYearInYearMode + 9;
}

void VCalendar::onLButtonDownInYearMode( int x, int y ) {
	POINT pt = {x, y};
	if (PtInRect(&mLeftArrowRect, pt)) {
		mBeginYearInYearMode -= 10;
		mEndYearInYearMode -= 10;
		repaint();
	} else if (PtInRect(&mRightArowRect, pt)) {
		mBeginYearInYearMode += 10;
		mEndYearInYearMode += 10;
		repaint();
	} else {
		int W = mMesureWidth / 4, H = (mMesureHeight - CALENDER_HEAD_HEIGHT) / 3;
		RECT r = {0, CALENDER_HEAD_HEIGHT, W, CALENDER_HEAD_HEIGHT + H};
		for (int i = 0; i < 10; ++i) {
			if (PtInRect(&r, pt)) {
				mViewMode = VM_SEL_MONTH;
				mYearInMonthMode = mBeginYearInYearMode + i;
				repaint();
				break;
			}
			if ((i + 1) % 4) OffsetRect(&r, W, 0);
			else OffsetRect(&r, -3 * W, H);
		}
	}
}

VCalendar::~VCalendar() {
	DeleteObject(mArrowNormalBrush);
	DeleteObject(mArrowSelBrush);
	DeleteObject(mSelectBgBrush);
	DeleteObject(mTrackBgBrush);
	DeleteObject(mLinePen);
}

void VCalendar::resetSelect() {
	mSelectLeftArrow = false;
	mSelectRightArrow = false;
	mSelectHeadTitle = false;
	mTrackSelectIdx = -1;
}

void VCalendar::onMouseMoveInDayMode( int x, int y ) {
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

void VCalendar::onMouseMoveInMonthMode( int x, int y ) {
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

void VCalendar::onMouseMoveInYearMode( int x, int y ) {
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

//-------------------VTable-------------------------------
VTable::VTable( XmlNode *node ) : VScroll(node) {
	mSelectedRow = -1;
	mModel = NULL;
	mRender = NULL;
	mSelectBgImage = XImage::load(mNode->getAttrValue("selRowBgImage"));
	COLORREF color = RGB(110, 120, 250);
	AttrUtils::parseColor(mNode->getAttrValue("horLineColor"), &color);
	mHorLinePen = CreatePen(PS_SOLID, 1, color);
	color = RGB(110, 120, 250);
	AttrUtils::parseColor(mNode->getAttrValue("verLineColor"), &color);
	mVerLinePen = CreatePen(PS_SOLID, 1, color);
}

void VTable::setModel(VTableModel *model) {
	mModel = model;
}

void VTable::setRender(Render *render) {
	mRender = render;
}

void VTable::onPaint( Msg *m ) {
	HDC dc = m->paint.dc;
	SIZE sz = getClientSize();
	eraseBackground(m);
	if (mModel == NULL) {
		return;
	}
	int hh = mModel->getHeaderHeight();
	drawHeader(dc, sz.cx, hh);
	IntersectClipRect(dc, 0, hh, sz.cx, sz.cy + hh);
	drawData(dc, 0, hh, sz.cx, sz.cy);
}

bool VTable::onMouseEvent( Msg *msg ) {
	if (msg->mId == Msg::LBUTTONDOWN) {
		if (mEnableFocus) setFocus();
		int col = 0;
		int row = findCell(msg->mouse.x, msg->mouse.y, &col);
		if (mSelectedRow != row && row >= 0) {
			mSelectedRow = row;
			repaint();
		}
		return true;
	}
	return VScroll::onMouseEvent(msg);
}

void VTable::drawHeader( HDC dc, int w, int h) {
	if (mModel == NULL || h <= 0) {
		return;
	}

	XImage *bg = mModel->getHeaderBgImage();
	if (bg != NULL) {
		bg->draw(dc, 0, 0, mWidth, h);
	}

	int sid = SaveDC(dc);
	IntersectClipRect(dc, 0, 0, w + 1, h);
	SelectObject(dc, getFont());
	SetBkMode(dc, TRANSPARENT);

	int x = -mHorBar->getPos();
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		int cw = mModel->getColumnWidth(i, w);
		if (mRender == NULL || !mRender->onDrawColumn(dc, i, x, 0, cw, h)) {
			drawColumn(dc, i, x, 0, cw, h);
		}
		x += cw;
	}

	// draw split line
	x = -mHorBar->getPos();
	HGDIOBJ old = SelectObject(dc, mVerLinePen);
	for (int i = 0; i < mModel->getColumnCount() - 1; ++i) {
		int cw = mModel->getColumnWidth(i, w);
		x += cw;
		MoveToEx(dc, x, 2, NULL);
		LineTo(dc, x, h - 4);
	}
	if (mVerBar->getMax() > mVerBar->getPage()) {
		x = mWidth - mVerBar->getMesureWidth();
		MoveToEx(dc, x, 2, NULL);
		LineTo(dc, x, h - 4);
	}
	SelectObject(dc, old);
	RestoreDC(dc, sid);
}

void VTable::drawColumn(HDC dc, int col, int x, int y, int w, int h) {
	VTableModel::HeaderData *hd = mModel->getHeaderData(col);
	if (hd == NULL) {
		return;
	}
	RECT r = {x, y, x + w, y + h};
	if (hd->mBgImage != NULL) {
		hd->mBgImage->draw(dc, x, y, w, h);
	}
	if (hd->mText != NULL) {
		DrawText(dc, hd->mText, strlen(hd->mText), &r, DT_VCENTER|DT_CENTER|DT_SINGLELINE);
	}
}

void VTable::drawData( HDC dc, int x, int y,  int w, int h ) {
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

void VTable::drawRow(HDC dc, int row, int x, int y, int w, int h ) {
	SIZE client = getClientSize();
	int cheight = mModel->getRowHeight(row);
	if (mSelectedRow == row && mSelectBgImage != NULL) {
		mSelectBgImage->draw(dc, x + 1, y + 1, w - 1, h - 1);
	}
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		int cwidth = mModel->getColumnWidth(i, client.cx);
		if (mRender == NULL || !mRender->onDrawCell(dc, row, i, x + 1, y + 1, cwidth - 1, cheight - 1)) {
			drawCell(dc, row, i, x + 1, y + 1, cwidth - 1, cheight - 1);
		}
		x += cwidth;
	}
}

void VTable::drawCell(HDC dc, int row, int col, int x, int y, int w, int h ) {
	VTableModel::CellData *data = mModel->getCellData(row, col);
	if (data == NULL || data->mText == NULL) {
		return;
	}
	char *txt = data->mText;
	RECT r = {x + 5, y, x + w - 5, y + h};
	DrawText(dc, txt, strlen(txt), &r, DT_SINGLELINE | DT_VCENTER);
}

void VTable::drawGridLine( HDC dc, int from, int to, int y ) {
	if (mModel == NULL) {
		return;
	}
	SIZE sz = getClientSize();
	HGDIOBJ old = SelectObject(dc, mHorLinePen);
	// draw hor-line
	int y2 = y;
	for (int i = from; i <= to; ++i) {
		y2 += mModel->getRowHeight(i);
		MoveToEx(dc, 0, y2, NULL);
		LineTo(dc, sz.cx, y2);
	}
	// draw ver-line
	SelectObject(dc, mVerLinePen);
	int x = -mHorBar->getPos();
	y = mModel->getHeaderHeight();
	for (int i = 0; i <= mModel->getColumnCount(); ++i) {
		if (i == mModel->getColumnCount()) --x;
		MoveToEx(dc, x, y, NULL);
		LineTo(dc, x, sz.cy + y);
		x += mModel->getColumnWidth(i, sz.cx);
	}
	SelectObject(dc, old);
}

void VTable::getVisibleRows( int *from, int *to ) {
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

/*
void VTable::onMeasure( int widthSpec, int heightSpec ) {
	mMesureWidth = calcSize(mAttrWidth, widthSpec);
	mMesureHeight = calcSize(mAttrHeight, heightSpec);

	bool hasHorBar = mHorBar->isVisible();
	bool hasVerBar = mVerBar->isVisible();
	mHorBar->onMeasure((mMesureWidth - (hasVerBar ? mVerBar->getMesureWidth() : 0)) | MS_ATMOST, mMesureHeight | MS_ATMOST);
	mVerBar->onMeasure(mMesureWidth | MS_ATMOST, (mMesureHeight - (hasHorBar ? mHorBar->getMesureHeight() : 0)) | MS_ATMOST);
	
	SIZE clientSize = getClientSize();
	SIZE cs = mDataSize = calcDataSize();
	mHorBar->setMaxAndPage(cs.cx, clientSize.cx);
	mVerBar->setMaxAndPage(cs.cy, clientSize.cy);

	mHorBar->setVisible(cs.cx > clientSize.cx);
	mVerBar->setVisible(cs.cy > clientSize.cy);

	if (mHorBar->isVisible() != hasHorBar || mVerBar->isVisible() != hasVerBar) {
		onMeasure(widthSpec, heightSpec);
	}
}*/

SIZE VTable::calcDataSize() {
	SIZE sz = {0};
	if (mModel == NULL) {
		return sz;
	}
	SIZE client = getClientSize();
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		sz.cx += mModel->getColumnWidth(i, client.cx);
	}
	int rc = mModel->getRowCount();
	for (int i = 0; i < rc; ++i) {
		sz.cy += mModel->getRowHeight(i);
	}
	return sz;
}

SIZE VTable::getClientSize() {
	bool hasHorBar = mHorBar->isVisible();
	bool hasVerBar = mVerBar->isVisible();
	int clientWidth = mMesureWidth - (hasVerBar ? mVerBar->getMesureWidth() : 0);
	int clientHeight = mMesureHeight - (hasHorBar ? mHorBar->getMesureHeight() : 0);
	if (mModel != NULL) {
		clientHeight -= mModel->getHeaderHeight();
	}
	SIZE sz = {clientWidth, clientHeight};
	return sz;
}

void VTable::mesureColumn(int width, int height) {
#if 0
	int widthAll = 0, weightAll = 0;
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		VTableModel::ColumnWidth cw = mModel->getColumnWidth(i);
		mColsWidth[i] = calcSize(cw.mWidthSpec, width | MS_ATMOST);
		widthAll += mColsWidth[i];
		weightAll += cw.mWeight;
	}
	int nw = 0;
	for (int i = 0; i < mModel->getColumnCount(); ++i) {
		VTableModel::ColumnWidth cw = mModel->getColumnWidth(i);
		if (width > widthAll && weightAll > 0) {
			mColsWidth[i] += cw.mWeight * (width - widthAll) / weightAll;
		}
		nw += mColsWidth[i];
	}
	// stretch last column
	if (nw < width && mModel->getColumnCount() > 0) {
		mColsWidth[mModel->getColumnCount() - 1] += width - nw;
	}
#endif
}

void VTable::onLayoutChildren( int width, int height ) {
	if (mModel == NULL) {
		return;
	}
	mHorBar->onLayout(0, mHeight - mHorBar->getMesureHeight(),
		mHorBar->getMesureWidth(), mHorBar->getMesureHeight());

	int hd = mModel->getHeaderHeight();
	mVerBar->onMeasure(mVerBar->getMesureWidth() | MS_FIX, (mVerBar->getMesureHeight() - hd) | MS_FIX);
	mVerBar->onLayout(mWidth - mVerBar->getMesureWidth(), hd,
		mVerBar->getMesureWidth(), mVerBar->getMesureHeight());
}

int VTable::findCell( int x, int y, int *col ) {
	if (mModel == NULL) {
		return -1;
	}
	SIZE sz = getClientSize();
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
			if (x2 <= x && x2 + mModel->getColumnWidth(i, sz.cx) > x) {
				*col = i;
				break;
			}
			x2 += mModel->getColumnWidth(i, sz.cx);
		}
	}
	return row;
}

VTable::~VTable() {
}


//----------------------------VList---------------------
VList::VList( XmlNode *node ) : VScroll(node) {
	mModel = NULL;
	mItemRender = NULL;
	mTrackItem = -1;
	mSelectItem = -1;
	mSelBgImage = XImage::load(mNode->getAttrValue("selBgImage"));
	if (mSelBgImage == NULL) {
		mSelBgImage = XImage::create(1, 1);
		mSelBgImage->mStretch = true;
		mSelBgImage->fillColor(0xFFA2B5CD);
	}
	mTrackBgImage = XImage::load(mNode->getAttrValue("trackBgImage"));
	if (mTrackBgImage == NULL) {
		mTrackBgImage = XImage::create(1, 1);
		mTrackBgImage->mStretch = true;
		mTrackBgImage->fillColor(0xFFA2B5BD);
	}
	mEnableTrack = true;
}

VList::~VList() {
}


void VList::onPaint(Msg *m) {
	eraseBackground(m);
	if (mModel != NULL) {
		SIZE sz = getClientSize();
		drawData(m->paint.dc, 0, 0, sz.cx, sz.cy);
	}
}

bool VList::onMouseEvent(Msg *msg) {
	if (mModel == NULL) {
		return false;
	}
	if (msg->mId == Msg::LBUTTONDOWN) {
		if (mEnableFocus) setFocus();
		int oldSelItem = mSelectItem;
		int idx = findItem(msg->mouse.x, msg->mouse.y);
		if (idx >= 0) {
			VListModel::ItemData *data = mModel->getItemData(idx);
			if (data == NULL) {
				return true;
			}
			if (data->mSelectable) {
				mSelectItem = idx;
			}
		}
		if (mSelectItem != oldSelItem) {
			repaint();
			updateWindow();
			if (mListener != NULL) {
				Msg m = *msg;
				m.mId = Msg::SELECT_ITEM;
				m.def.wParam = msg->mouse.x;
				m.def.lParam = msg->mouse.y;
				mListener->onEvent(this, &m);
			}
		}
		return true;
	} else if (msg->mId == Msg::MOUSE_WHEEL) {
		VScroll::onMouseEvent(msg);
		updateTrackItem(msg->mouse.x, msg->mouse.y);
		return true;
	} else if (msg->mId == Msg::MOUSE_MOVE) {
		updateTrackItem(msg->mouse.x, msg->mouse.y);
		return true;
	} else if (msg->mId == Msg::MOUSE_LEAVE || msg->mId == Msg::MOUSE_CANCEL) {
		updateTrackItem(-1, -1);
		return true;
	}
	return VScroll::onMouseEvent(msg);
}

SIZE VList::calcDataSize() {
	SIZE cs = getClientSize();
	SIZE sz = {cs.cx, 0};
	int rc = mModel ? mModel->getItemCount() : 0;
	for (int i = 0; i < rc; ++i) {
		sz.cy += mModel->getItemHeight(i);
	}
	return sz;
}

SIZE VList::getClientSize() {
	int clientWidth = mMesureWidth - (mVerBar->isVisible() ? mVerBar->getMesureWidth() : 0);
	SIZE sz = {clientWidth, mMesureHeight};
	return sz;
}

void VList::notifyModelChanged() {
	if (mMesureWidth == 0 || mMesureHeight == 0) {
		return;
	}
	onMeasure(mMesureWidth | MS_FIX, mMesureHeight | MS_FIX);
	onLayout(mX, mY, mWidth, mHeight);
	repaint();
}

void VList::drawData( HDC dc, int x, int y, int w, int h ) {
	if (mModel == NULL) {
		return;
	}
	SelectObject(dc, getFont());
	SetBkMode(dc, TRANSPARENT);
	if (mAttrFlags & AF_COLOR) SetTextColor(dc, mAttrColor);

	int from = 0, num = 0;
	getVisibleRows(&from, &num);
	y += -mVerBar->getPos();
	for (int i = 0; i < from; ++i) {
		y += mModel->getItemHeight(i);
	}
	x += -mHorBar->getPos();
	for (int i = from; i < from + num && i >= 0; ++i) {
		int rh = mModel->getItemHeight(i);
		if (mEnableTrack && mTrackItem == i) {
			// draw track row background
			mTrackBgImage->draw(dc, x, y, w, rh);
		} else if (mSelectItem == i) {
			// draw select row background
			mSelBgImage->draw(dc, x, y, w, rh);
		}
		if (mItemRender == NULL) {
			drawItem(dc, i, x, y, w, rh);
		} else {
			mItemRender->onDrawItem(dc, i, x, y, w, rh);
		}
		y += rh;
	}
}

void VList::drawItem( HDC dc, int item, int x, int y, int w, int h ) {
	VListModel::ItemData *data = mModel->getItemData(item);
	if (data != NULL && data->mText != NULL) {
		RECT r = {x + 10, y, x + w - 10, y + h};
		DrawText(dc, data->mText, strlen(data->mText), &r, DT_SINGLELINE | DT_VCENTER);
	}
}

void VList::getVisibleRows( int *from, int *num ) {
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
void VList::setModel( VListModel *model ) {
	if (mModel != model) {
		mModel = model;
		notifyModelChanged();
	}
}

VListModel *VList::getModel() {
	return mModel;
}

void VList::setItemRender( ItemRender *render ) {
	mItemRender = render;
}

VList::ItemRender *VList::getItemRender() {
	return mItemRender;
}

int VList::findItem( int x, int y ) {
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

int VList::getItemY(int item) {
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

void VList::updateTrackItem( int x, int y ) {
	int old = mTrackItem;
	if (mModel == NULL || !mEnableTrack) {
		mTrackItem = -1;
		goto _end;
	}
	
	int idx = findItem(x, y);
	if (idx < 0) {
		mTrackItem = -1;
	} else {
		VListModel::ItemData *item = mModel->getItemData(idx);
		if (item != NULL && item->mSelectable) {
			mTrackItem = idx;
		}
	}

	_end:
	if (old != mTrackItem) {
		repaint();
	}
}

int VList::getTrackItem() {
	return mTrackItem;
}

void VList::setEnableTrack(bool enable) {
	mEnableTrack = enable;
}

void VList::setSelectItem(int item) {
	mSelectItem = item;
}

int VList::getSelectItem() {
	return mSelectItem;
}

//-------------------------VTreeNode--------------------
VTreeNode::VTreeNode( const char *text ) {
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

void VTreeNode::insert( int pos, VTreeNode *child ) {
	if (child == NULL || pos > getChildCount()) 
		return;
	if (mChildren == NULL) mChildren = new std::vector<VTreeNode*>();
	if (pos < 0) pos = getChildCount();
	mChildren->insert(mChildren->begin() + pos, child);
	child->mParent = this;
}

void VTreeNode::remove( int pos ) {
	if (pos >= 0 && pos < getChildCount()) {
		mChildren->erase(mChildren->begin() + pos);
	}
}

void VTreeNode::remove(VTreeNode *child) {
	remove(indexOf(child));
}

int VTreeNode::indexOf( VTreeNode *child ) {
	if (child == NULL || mChildren == NULL) 
		return -1;
	for (int i = 0; i < mChildren->size(); ++i) {
		if (mChildren->at(i) == child) 
			return i;
	}
	return -1;
}

int VTreeNode::getChildCount() {
	if (mChildren == NULL) return 0;
	return mChildren->size();
}

VTreeNode * VTreeNode::getChild( int idx ) {
	if (idx >= 0 && idx < getChildCount())
		return mChildren->at(idx);
	return NULL;
}

void * VTreeNode::getUserData() {
	return mUserData;
}

void VTreeNode::setUserData( void *userData ) {
	mUserData = userData;
}

int VTreeNode::getContentWidth() {
	return mContentWidth;
}

void VTreeNode::setContentWidth( int w ) {
	mContentWidth = w;
}

bool VTreeNode::isExpand() {
	return mExpand;
}

void VTreeNode::setExpand( bool expand ) {
	mExpand = expand;
}

char * VTreeNode::getText() {
	return mText;
}

void VTreeNode::setText( char *text ) {
	mText = text;
	mContentWidth = -1;
}

VTreeNode::PosInfo VTreeNode::getPosInfo() {
	if (mParent == NULL) return PI_FIRST;
	int v = 0;
	int idx = mParent->indexOf(this);
	if (idx == 0) v |= PI_FIRST;
	if (idx == mParent->getChildCount() - 1) v |= PI_LAST;
	if (idx > 0 && idx < mParent->getChildCount() - 1) v |= PI_CENTER;
	return PosInfo(v);
}

int VTreeNode::getLevel() {
	VTreeNode *p = mParent;
	int level = -1;
	for (; p != NULL; p = p->mParent) ++level;
	return level;
}

VTreeNode * VTreeNode::getParent() {
	return mParent;
}

bool VTreeNode::isCheckable() {
	return mCheckable;
}

void VTreeNode::setCheckable( bool cb ) {
	mCheckable = cb;
}
bool VTreeNode::isChecked() {
	return mChecked;
}

void VTreeNode::setChecked( bool cb ) {
	mChecked = cb;
}

static const int TREE_NODE_HEIGHT = 30;
static const int TREE_NODE_HEADER_WIDTH = 40;
static const int TREE_NODE_BOX = 16;
static const int TREE_NODE_BOX_LEFT = 5;

VTree::VTree( XmlNode *node ) : VScroll(node) {
	mModel = NULL;
	COLORREF color = RGB(0x3A, 0x9D, 0xF9);
	AttrUtils::parseColor(mNode->getAttrValue("lineColor"), &color);
	mLinePen = CreatePen(PS_SOLID, 1, color);
	color = RGB(0x64, 0x95, 0xED);
	AttrUtils::parseColor(mNode->getAttrValue("checkBoxColor"), &color);
	mCheckPen = CreatePen(PS_SOLID, 1, color);
	color = 0xE6E0B0;
	AttrUtils::parseColor(mNode->getAttrValue("selBgColor"), &color);
	mSelectBgBrush = CreateSolidBrush(color);
	mSelectNode = NULL;
	mNodeRender = NULL;
	mWhenSelect = WHEN_CLICK;
	char *ws = mNode->getAttrValue("whenSelect");
	if (ws != NULL && strcmp(ws, "click") == 0) {
		mWhenSelect = WHEN_CLICK;
	} else if (ws != NULL && strcmp(ws, "dbclick") == 0) {
		mWhenSelect = WHEN_DBCLICK;
	}
}

void VTree::setModel( VTreeNode *root ) {
	mModel = root;
}

VTreeNode* VTree::getModel() {
	return mModel;
}

VTree::~VTree() {
	DeleteObject(mLinePen);
	DeleteObject(mCheckPen);
	DeleteObject(mSelectBgBrush);
}

void VTree::onPaint(Msg *m) {
	SIZE sz = getClientSize();
	// draw background
	eraseBackground(m);
	drawData(m->paint.dc, sz.cx, sz.cy);
}

bool VTree::onMouseEvent(Msg *msg) {
	if (msg->mId == Msg::LBUTTONDOWN) {
		if (mEnableFocus) setFocus();
		onLBtnDown(msg->mouse.x, msg->mouse.y);
		return true;
	} else if (msg->mId == Msg::DBCLICK) {
		onLBtnDbClick(msg->mouse.x, msg->mouse.y);
		return true;
	}
	return VScroll::onMouseEvent(msg);
}

static void CalcDataSize(HDC dc, VTreeNode *n, int *nodeNum, int *maxWidth, int level) {
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

SIZE VTree::calcDataSize() {
	SIZE sz = {0};
	if (mModel == NULL) {
		return sz;
	}
	int nn = 0, mw = 0;
	HWND wnd = getWnd();
	HDC dc = GetDC(wnd);
	SelectObject(dc, getFont());
	mModel->setExpand(true); // always expand root node
	CalcDataSize(dc, mModel, &nn, &mw, 0);
	ReleaseDC(wnd, dc);
	sz.cy = (nn - 1) * TREE_NODE_HEIGHT;
	sz.cx = mw + 5;
	return sz;
}

void VTree::drawData( HDC dc, int w, int h) {
	if (mModel == NULL) {
		return;
	}
	int y = -mVerBar->getPos();
	SelectObject(dc, mLinePen);
	SetBkMode(dc, TRANSPARENT);
	SelectObject(dc, getFont());

	for (int i = 0; i < mModel->getChildCount(); ++i) {
		VTreeNode *child = mModel->getChild(i);
		drawNode(dc, child, 0, w, h, &y);
	}
}

void VTree::drawNode( HDC dc, VTreeNode *n, int level, int clientWidth, int clientHeight, int *py ) {
	if (*py >= clientHeight) {
		return;
	}
	if (*py + TREE_NODE_HEIGHT <= 0) goto _drawChild;
	int y = *py;
	// draw level ver line
	VTreeNode *pa = n->getParent();
	for (int i = 0, j = level - 1; i < level; ++i, --j, pa = pa->getParent()) {
		if (pa->getPosInfo() & VTreeNode::PI_LAST)
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
	VTreeNode::PosInfo pi = n->getPosInfo();
	int ly = y + TREE_NODE_HEIGHT, fy = y;
	if (pi & VTreeNode::PI_LAST) {
		ly = y + TREE_NODE_HEIGHT / 2;
	}
	if ((pi & VTreeNode::PI_FIRST) && level == 0) {
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
			VTreeNode *ss = n->getChild(i);
			drawNode(dc, ss, level + 1, clientWidth, clientHeight, py);
		}
	}
}

void VTree::notifyChanged() {
	if (mMesureWidth != 0 && mMesureHeight != 0) {
		onMeasure(mMesureWidth | MS_FIX, mMesureHeight | MS_FIX);
		repaint();
		updateWindow();
	}
}

void VTree::onLBtnDown( int x, int y ) {
	POINT pt = {x, y};
	int y2 = 0;
	VTreeNode * node = getNodeAtY(y, &y2);
	if (node == NULL) {
		return;
	}
	int x2 = -mHorBar->getPos() + (node->getLevel() + 1) * TREE_NODE_HEADER_WIDTH;
	RECT cntRect = {x2, y2, x2 + node->getContentWidth(), y2 + TREE_NODE_HEIGHT};

	if (node->isCheckable()) {
		int lx = x2;
		int ly = y2 + (TREE_NODE_HEIGHT - TREE_NODE_BOX) / 2;
		RECT checkBoxRect = {lx, ly, lx + TREE_NODE_BOX, ly + TREE_NODE_BOX};
		if (PtInRect(&checkBoxRect, pt)) { // click in check box
			node->setChecked(! node->isChecked());
			repaint();
			return;
		}
	}
	if (PtInRect(&cntRect, pt)) { // click in node content
		if (mWhenSelect == WHEN_CLICK) {
			setSelectNode(node);
		}
		return;
	}
	if (node->getChildCount() == 0) {// has no child
		return;
	}
	x2 = -mHorBar->getPos() + node->getLevel() * TREE_NODE_HEADER_WIDTH + TREE_NODE_BOX_LEFT;
	int yy = y2 + (TREE_NODE_HEIGHT - TREE_NODE_BOX) / 2;
	RECT r = {x2, yy, x2 + TREE_NODE_BOX, yy + TREE_NODE_BOX};
	if (PtInRect(&r, pt)) { // click in expand box
		node->setExpand(! node->isExpand());
		notifyChanged();
	}
}

static VTreeNode * GetNodeAtY(VTreeNode *n, int y, int *py) {
	if (y >= *py && y < *py + TREE_NODE_HEIGHT) {
		return n;
	}
	*py = *py + TREE_NODE_HEIGHT;
	if (n->isExpand()) {
		for (int i = 0; i < n->getChildCount(); ++i) {
			VTreeNode *cc = GetNodeAtY(n->getChild(i), y, py);
			if (cc != NULL) return cc;
		}
	}
	return NULL;
}

VTreeNode * VTree::getNodeAtY( int y, int *py ) {
	if (mModel == NULL) return NULL;
	int y2 = -mVerBar->getPos();
	VTreeNode *n = NULL;
	for (int i = 0; i < mModel->getChildCount(); ++i) {
		n = GetNodeAtY(mModel->getChild(i), y, &y2);
		if (n != NULL) break;
	}
	*py = y2;
	return n;
}

void VTree::onLBtnDbClick( int x, int y ) {
	POINT pt = {x, y};
	int y2 = 0;
	VTreeNode * node = getNodeAtY(y, &y2);
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

void VTree::setNodeRender( NodeRender *render ) {
	mNodeRender = render;
}

static bool GetNodeRect(VTreeNode *n, VTreeNode *target, int *py) {
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

bool VTree::getNodeRect( VTreeNode *node, RECT *r ) {
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

VTreeNode * VTree::getAtPoint(int x, int y) {
	int y2 = 0;
	VTreeNode * node = getNodeAtY(y, &y2);
	if (node == NULL) {
		return NULL;
	}
	int x2 = (node->getLevel() + 1) * TREE_NODE_HEADER_WIDTH;
	if (x >= x2 && x < x2 + node->getContentWidth()) {
		return node;
	}
	return NULL;
}

VTree::WhenSelect VTree::getWhenSelect() {
	return mWhenSelect;
}

void VTree::setWhenSelect(WhenSelect when) {
	mWhenSelect = when;
}

VTreeNode * VTree::getSelectNode() {
	return mSelectNode;
}

void VTree::setSelectNode(VTreeNode *node) {
	if (mSelectNode == node) {
		return;
	}
	mSelectNode = node;
	if (mSelectNode != NULL && mListener != NULL) {
		Msg m;
		m.mId = Msg::SELECT_ITEM;
		m.def.wParam = (WPARAM)mSelectNode;
		mListener->onEvent(this, &m);
	}
	repaint();
}


//--------------------------XHLineLayout--------------------------
VHLineLayout::VHLineLayout(XmlNode *node) : VExtComponent(node) {
}

void VHLineLayout::onLayoutChildren( int width, int height ) {
	width -= mAttrPadding[0] + mAttrPadding[2];
	height -= mAttrPadding[1] + mAttrPadding[3];
	int x = mAttrPadding[0], weightAll = 0, childWidths = 0, lessWidth = width;
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		weightAll += child->getAttrWeight();
		childWidths += child->getMesureWidth() + child->getAttrMargin()[0] + child->getAttrMargin()[2];
	}
	lessWidth -= childWidths;
	double perWeight = 0;
	if (weightAll > 0 && lessWidth > 0) {
		perWeight = lessWidth / weightAll;
	}
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		int hh = height - child->getAttrMargin()[1] - child->getAttrMargin()[3];
		int y  = calcSize(child->getAttrY(), (hh > 0 ? hh : 0) | MS_ATMOST);
		y += mAttrPadding[1] + child->getAttrMargin()[1];
		x += child->getAttrMargin()[0];
		if (child->getAttrWeight() > 0 && perWeight > 0) {
			int nw = child->getMesureWidth() + child->getAttrWeight() * perWeight;
			child->onMeasure(nw | MS_FIX , child->getMesureHeight() | MS_FIX);
		}
		child->onLayout(x, y, child->getMesureWidth(), child->getMesureHeight());
		x += child->getMesureWidth() + child->getAttrMargin()[2];
	}
}
//--------------------------XVLineLayout--------------------------
VVLineLayout::VVLineLayout(XmlNode *node) : VExtComponent(node) {
}

void VVLineLayout::onLayoutChildren( int width, int height ) {
	width -= mAttrPadding[0] + mAttrPadding[2];
	height -= mAttrPadding[1] - mAttrPadding[3];
	int y = mAttrPadding[1], weightAll = 0, childHeights = 0, lessHeight = height;
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		weightAll += child->getAttrWeight();
		childHeights += child->getMesureHeight() + child->getAttrMargin()[1] + child->getAttrMargin()[3];
	}
	lessHeight -= childHeights;
	double perWeight = 0;
	if (weightAll > 0 && lessHeight > 0) {
		perWeight = lessHeight / weightAll;
	}
	for (int i = 0; i < mNode->getChildCount(); ++i) {
		VComponent *child = mNode->getChild(i)->getComponentV();
		int ww = width - child->getAttrMargin()[0] - child->getAttrMargin()[2];
		int x = calcSize(child->getAttrX(), (ww > 0 ? ww : 0) | MS_ATMOST);
		x += mAttrPadding[0] + child->getAttrMargin()[0];
		y += child->getAttrMargin()[1];
		if (child->getAttrWeight() > 0 && perWeight > 0) {
			int nh = child->getMesureHeight() + child->getAttrWeight() * perWeight;
			child->onMeasure(child->getMesureWidth() | MS_FIX , nh | MS_FIX);
		}
		child->onLayout(x, y, child->getMesureWidth(), child->getMesureHeight());
		y += child->getMesureHeight() + child->getAttrMargin()[3];
	}
}

//--------------------------VBaseCombobox--------------------------
VBaseCombobox::VBaseCombobox(XmlNode *node) : VExtComponent(node) {
	mArrowImg[0] = XImage::load(node->getAttrValue("arrowNormal"));
	mArrowImg[1] = XImage::load(node->getAttrValue("arrowPush"));
	mPopup = new VPopup(new XmlNode("Popup", node));
	mPopup->setMouseAction(VPopup::MA_CLOSE);
	mArrowWidth = AttrUtils::parseInt(node->getAttrValue("arrowWidth"));
	if (mArrowWidth <= 0) {
		mArrowWidth = 20;
	}
}

void VBaseCombobox::openPopup() {
	/*POINT pt = getDrawPoint();
	SIZE sz = getPopupSize();
	p->show(pt.x, pt.y + mHeight, sz.cx, sz.cy);
	repaint();
	*/
}

void VBaseCombobox::closePopup() {
	mPopup->close();
	repaint();
}

bool VBaseCombobox::onMouseEvent(Msg *m) {
	if (m->mId == Msg::LBUTTONDOWN) {
		if (m->mouse.x >= mWidth - mArrowWidth) {
			// press in arrow
			openPopup();
			return true;
		}
	}
	return VExtComponent::onMouseEvent(m);
}

//-----------VComboBox------------------------------------
class VBoxRender : public VList::ItemRender {
public:
	VBoxRender(VList *list) {
		mList = list;
	}
	virtual void onDrawItem(HDC dc, int item, int x, int y, int w, int h) {
		if (item < 0) {
			return;
		}
		VListModel *model = mList->getModel();
		VListModel::ItemData *data = model->getItemData(item);
		if (data != NULL && data->mText != NULL) {
			RECT r = {x + 10, y, x + w - 10, y + h};
			DrawText(dc, data->mText, strlen(data->mText), &r, DT_SINGLELINE | DT_VCENTER);
		}
	}
	VList *mList;
};

VComboBox::VComboBox(XmlNode *node) : VBaseCombobox(node) {
	mList = new VList(new XmlNode("List", mPopup->getNode()));
	mList->setAttrWidth(100 | MS_PERCENT);
	mList->setAttrHeight(100 | MS_PERCENT);
	mList->setItemRender(new VBoxRender(mList));
	mPopup->getNode()->addChild(mList->getNode());

}

void VComboBox::openPopup() {
	int cy = 0;
	POINT pt = getDrawPoint();
	VListModel *model = mList->getModel();
	if (model == NULL || model->getItemCount() == 0) {
		cy = 20;
	} else {
		int c = model->getItemCount();
		int h = model->getItemHeight(0);
		cy = h * (c > 15 ? 15 : c); // max show item is 15
	}
	mPopup->show(pt.x, pt.y + mHeight, mWidth, cy);
	repaint();
}

VList* VComboBox::getList() {
	return mList;
}

void VComboBox::onPaint(Msg *m) {
	eraseBackground(m);
	VList::ItemRender *rr = mList->getItemRender();
	if (rr != NULL) {
		int si = mList->getSelectItem();
		rr->onDrawItem(m->paint.dc, si, 0, 0, mWidth - mArrowWidth, mHeight);
	}
	// draw arrow
	XImage *arrow = mPopup->isShowing() ? mArrowImg[1] : mArrowImg[0];
	if (arrow != NULL) {
		arrow->draw(m->paint.dc, mWidth - mArrowWidth, 0, mArrowWidth, mHeight);
	}
}
