#include "stdafx.h"
#include "Layout.h"


LayoutParent::LayoutParent(int x, int y, Val width, Val height) : 
	mX(x), mY(y), mWidth(width), mHeight(height),
	mMeasureWidth(0), mMeasureHeight(0),
	mPadLeft(0), mPadTop(0), mPadRight(0), mPadBottom(0), mParent(0), mMeasured(FALSE) {
}

LayoutParent::Val LayoutParent::makeVal( int val, Model model, int weight) {
	return val | model | (weight << 20);
}

LayoutParent::Val LayoutParent::makeVal( int val, Model model ) {
	return makeVal(val, model, 0);
}

LayoutParent::Model LayoutParent::getModel( Val v ) {
	return Model(v & MM_MASK);
}

int LayoutParent::getVal( Val v ) {
	return v & ~MM_MASK & ~MW_MASK;
}

int LayoutParent::getWeight( Val v ) {
	return v & MW_MASK;
}

LayoutParent::Val LayoutParent::calcVal( Val target, Val parent ) {
	if (getModel(target) == MM_UNKNOW) {
		return parent;
	} else if (getModel(target) ==  MM_FIX) {
		if (getModel(parent) == MM_FIX) {
			return makeVal(getVal(parent), getModel(parent));
		} else if (getModel(parent) == MM_ATMOST) {
			int vv = min(getVal(parent), getVal(target));
			return makeVal(vv, getModel(target), getWeight(target));
		}
		return target;
	} else if (getModel(target) ==  MM_PERCENT) {
		if (getModel(parent) == MM_FIX || getModel(parent) == MM_ATMOST) {
			int v = getVal(parent) * getVal(target) / 100;
			return makeVal(v, MM_FIX);
		}
		//else if (getModel(parent) == MM_WRAP_CONTENT) {
		//	return makeVal(getVal(parent), MM_WRAP_CONTENT, 0);
		//}
	} else if (getModel(target) ==  MM_WRAP_CONTENT) {
		return target;
	} else if (getModel(target) ==  MM_ATMOST) {
		int v = getVal(target);
		if (getModel(parent) == MM_FIX || getModel(parent) == MM_ATMOST) {
			int pv = getVal(parent);
			return makeVal(min(pv, v), getModel(target), getWeight(target));
		}
		return target;
	}
	return makeVal(0, MM_UNKNOW);
}

void LayoutParent::setPadding( int left, int top, int right, int bottom ) {
	mPadLeft = left;
	mPadTop = top;
	mPadRight = right;
	mPadBottom = bottom;
}

int LayoutParent::getXToTop() {
	int v = mX;
	for (LayoutParent *p = mParent; p; p = p->mParent) {
		v += p->mX;
	}
	return v;
}

int LayoutParent::getYToTop() {
	int v = mY;
	for (LayoutParent *p = mParent; p; p = p->mParent) {
		v += p->mY;
	}
	return v;
}

int LayoutParent::getMeasureWidth() {
	return mMeasureWidth;
}

int LayoutParent::getMeasureHeight() {
	return mMeasureHeight;
}

BOOL LayoutParent::isMeasured() {
	return mMeasured;
}

void LayoutParent::clearMeasured() {
	mMeasured = FALSE;
}

void LayoutParent::layout( int x, int y, int width, int height ) {
	mX = x;
	mY = y;
}

int LayoutParent::getX() {
	return mX;
}

int LayoutParent::getY() {
	return mY;
}

void LayoutParent::setParent( LayoutParent *parent ) {
	mParent = parent;
}

LayoutParent::Val LayoutParent::getWidthVal() {
	return mWidth;
}

LayoutParent::Val LayoutParent::getHeightVal() {
	return mHeight;
}

Layout::Layout( int x, int y, Val width, Val height ) : LayoutParent(x, y, width, height), mChildNum(0) {
	memset(mChild, 0, sizeof mChild);
}

Layout::~Layout() {
	for (int i = 0; i < mChildNum; ++i) {
		delete mChild[i];
		mChild[i] = 0;
	}
	mChildNum = 0;
}

void Layout::addChild( LayoutParent *child ) {
	child->setParent(this);
	mChild[mChildNum++] = child;
}

BOOL Layout::isMeasured() {
	if (! mMeasured) return FALSE;
	for (int i = 0; i < mChildNum; ++i) {
		if (mChild[i] && !mChild[i]->isMeasured())
			return FALSE;
	}
	return TRUE;
}

void Layout::clearMeasured() {
	LayoutParent::clearMeasured();
	for (int i = 0; i < mChildNum; ++i) {
		if (mChild[i]) {
			mChild[i]->clearMeasured();
		}
	}
}

int Layout::getChildNum() {
	return mChildNum;
}

LRESULT CALLBACK LayoutManager::LMWndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LayoutManager *mgr = (LayoutManager*)GetWindowLong(wnd, 0);
	if (msg == WM_SIZE && !IsIconic(wnd)) {
		USHORT w = LOWORD(lParam);
		USHORT h = HIWORD(lParam);
		mgr->layout(w, h);
	}
	return CallWindowProc(mgr->mWndProc, wnd, msg, wParam, lParam);
}

LayoutManager::LayoutManager() : mTargetWnd(0), mLayout(0) {
}

LayoutManager::~LayoutManager() {
	delete mLayout;
	mLayout = 0;
}

void LayoutManager::bind(HWND targetWnd, Layout *lay, BOOL autoLayout) {
	mTargetWnd = targetWnd;
	mLayout = lay;
	if (autoLayout) {
		SetWindowLong(targetWnd, 0, (LONG)this);
		mWndProc = (WNDPROC)SetWindowLong(targetWnd, GWL_WNDPROC, (LONG)LMWndProc);
	}
}

void LayoutManager::layout(int width, int height) {
	mLayout->clearMeasured();
	while (! mLayout->isMeasured()) {
		mLayout->measure(LayoutParent::makeVal(width - mLayout->getX(), LayoutParent::MM_FIX),
			LayoutParent::makeVal(height - mLayout->getY(), LayoutParent::MM_FIX));
	}
	mLayout->layout(mLayout->getX(), mLayout->getY(), mLayout->getMeasureWidth(), mLayout->getMeasureHeight());
}

void LayoutManager::draw( HDC hdc ) {
	int sp = SaveDC(hdc);
	HBRUSH cs [3];
	cs[0] = CreateSolidBrush(RGB(255, 0, 0));
	cs[1] = CreateSolidBrush(RGB(0, 255, 0));
	cs[2] = CreateSolidBrush(RGB(0, 0, 255));
	drawLayout(mLayout, hdc, cs, 0);
	RestoreDC(hdc, sp);
}

void LayoutManager::drawLayout(LayoutParent* lay, HDC hdc, HBRUSH *br, int brIdx) {
	if (lay == NULL) return;
	Layout *chs = dynamic_cast<Layout*>(lay);
	if (!chs) {
		int x = lay->getXToTop();
		int y = lay->getYToTop();
		RECT r = {x, y, x + lay->getMeasureWidth(), y + lay->getMeasureHeight()};
		FrameRect(hdc, &r, br[brIdx % 3]);
		return;
	}
	for (int i = 0; i < chs->mChildNum; ++i, ++brIdx) {
		drawLayout(chs->mChild[i], hdc, br, brIdx);
	}
}

WndLayout::WndLayout(HWND wnd, int x, int y, Val width, Val height) : LayoutParent(x, y, width, height), mWnd(wnd) {
}

void WndLayout::layout(int x, int y, int width, int height) {
	LayoutParent::layout(x, y, width, height);
	x = getXToTop();
	y = getYToTop();
	if (mWnd) {
		MoveWindow(mWnd, x, y, width, height, TRUE);
	}
}

void WndLayout::measure(Val width, Val height) {
	Val vw = calcVal(mWidth, width);
	Val vh = calcVal(mHeight, height);
	mMeasureWidth = getVal(vw);
	mMeasureHeight = getVal(vh);
	mMeasured = TRUE;
}

BorderLayout::BorderLayout( int x, int y, Val width, Val height ) : Layout(x, y, width, height) {
}

void BorderLayout::addChild(LayoutParent *child, Anchor a) {
	mChild[a] = child;
	child->setParent(this);
	mChildNum = A_NUM;
}

void BorderLayout::measure(Val width, Val height) {
	// measure self
	Val vw = calcVal(mWidth, width);
	Val vh = calcVal(mHeight, height);

	if (getModel(vw) == MM_UNKNOW || getModel(vh) == MM_UNKNOW) {
		// Error
		mMeasured = TRUE;
		return;
	}
	// measure child
	LayoutParent *child = NULL;
	if ((child = mChild[A_TOP]) != NULL) {
		child->measure(makeVal(getVal(vw), MM_FIX), makeVal(getVal(vh), MM_ATMOST));
	}
	if ((child = mChild[A_BOTTOM]) != NULL) {
		child->measure(makeVal(getVal(vw), MM_FIX), makeVal(getVal(vh), MM_ATMOST, 0));
	}
	if ((child = mChild[A_LEFT]) != NULL) {
		int tp = mChild[A_TOP] ? mChild[A_TOP]->getMeasureHeight() : 0;
		int bp = mChild[A_BOTTOM] ? mChild[A_BOTTOM]->getMeasureHeight() : 0;
		child->measure(makeVal(getVal(vw), MM_ATMOST), makeVal(getVal(vh) - tp - bp, MM_FIX));
	}
	if ((child = mChild[A_RIGHT]) != NULL) {
		int tp = mChild[A_TOP] ? mChild[A_TOP]->getMeasureHeight() : 0;
		int bp = mChild[A_BOTTOM] ? mChild[A_BOTTOM]->getMeasureHeight() : 0;
		child->measure(makeVal(getVal(vw), MM_ATMOST), makeVal(getVal(vh) - tp - bp, MM_FIX));
	}
	
	if ((child = mChild[A_CENTER]) != NULL) {
		int tp = mChild[A_TOP] ? mChild[A_TOP]->getMeasureHeight() : 0;
		int bp = mChild[A_BOTTOM] ? mChild[A_BOTTOM]->getMeasureHeight() : 0;
		int lw = mChild[A_LEFT] ? mChild[A_LEFT]->getMeasureWidth() : 0;
		int rw = mChild[A_RIGHT] ? mChild[A_RIGHT]->getMeasureWidth() : 0;
		child->measure(makeVal(getVal(vw) - lw - rw, MM_FIX),
			makeVal(getVal(vh) - tp - bp, MM_FIX));
	}
	mMeasureWidth = getVal(vw);
	mMeasureHeight = getVal(vh);
	mMeasured = TRUE;
}

void BorderLayout::layout( int x, int y, int width, int height ) {
	Layout::layout(x, y, width, height);
	LayoutParent *child = NULL;

	if ((child = mChild[A_TOP]) != NULL) {
		child->layout(0, 0, child->getMeasureWidth(), child->getMeasureHeight());
	}
	if ((child = mChild[A_BOTTOM]) != NULL) {
		child->layout(0, height - child->getMeasureHeight(),
			child->getMeasureWidth(), child->getMeasureHeight());
	}
	if ((child = mChild[A_LEFT]) != NULL) {
		int tp = mChild[A_TOP] ? mChild[A_TOP]->getMeasureHeight() : 0;
		child->layout(0, tp, child->getMeasureWidth(), child->getMeasureHeight());
	}
	if ((child = mChild[A_RIGHT]) != NULL) {
		int tp = mChild[A_TOP] ? mChild[A_TOP]->getMeasureHeight() : 0;
		int bp = mChild[A_BOTTOM] ? mChild[A_BOTTOM]->getMeasureHeight() : 0;
		child->layout(width - child->getMeasureWidth(), tp,
			child->getMeasureWidth(), child->getMeasureHeight());
	}
	if ((child = mChild[A_CENTER]) != NULL) {
		int tp = mChild[A_TOP] ? mChild[A_TOP]->getMeasureHeight() : 0;
		int lw = mChild[A_LEFT] ? mChild[A_LEFT]->getMeasureWidth() : 0;
		child->layout(lw, tp,
			child->getMeasureWidth(), child->getMeasureHeight());
	}
}

HLineLayout::HLineLayout( int x, int y, Val width, Val height ) : 
	Layout(x, y, width, height), mSpace(0) {
}

void HLineLayout::layout( int x, int y, int width, int height ) {
	Layout::layout(x, y, width, height);
	int cx = 0, cy = 0;
	for (int i = 0; i < mChildNum; ++i) {
		mChild[i]->layout(cx, cy, mChild[i]->getMeasureWidth(), mChild[i]->getMeasureHeight());
		cx += mSpace + mChild[i]->getMeasureWidth();
	}
}

void HLineLayout::measure( Val width, Val height ) {
	// measure self
	Val vw = calcVal(mWidth, width);
	Val vh = calcVal(mHeight, height);

	if (getModel(vw) == MM_UNKNOW || getModel(vh) == MM_UNKNOW) {
		// Error
		mMeasured = TRUE;
		return;
	}
	// measure child
	for (int i = 0; i < mChildNum; ++i) {
		LayoutParent *child = mChild[i];
		child->measure(makeVal(getVal(vw), MM_ATMOST), makeVal(getVal(vh), MM_ATMOST));
	}

	if (getModel(vw) == MM_FIX) {
		LayoutParent* wchild[30] = {0};
		int weight = 0, less = getVal(vw);
		for (int i = 0, j = 0; i < mChildNum; ++i) {
			vw -= mChild[i]->getMeasureWidth();
			if (getWeight(mChild[i]->getWidthVal()) > 0) {
				wchild[j++] = mChild[i];
				weight += getWeight(mChild[i]->getWidthVal());
			}
		}
		less -= (mChildNum - 1) * mSpace;
		if (less > 0 && weight > 0) {
			for (int i = 0; wchild[i]; ++i) {
				int lw = less * getWeight(mChild[i]->getWidthVal()) / weight;
				lw += wchild[i]->getMeasureWidth();
				wchild[i]->measure(makeVal(lw, MM_FIX), makeVal(getVal(vh), MM_ATMOST));
			}
		}
	}

	mMeasureWidth = getVal(vw);
	mMeasureHeight = getVal(vh);
	mMeasured = TRUE;
}

void HLineLayout::setSpace( int space ) {
	mSpace = space;
}
