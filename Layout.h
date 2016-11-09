#pragma once
#include <windows.h>

class LayoutParent {
public:
	typedef int Val;
	enum Model {
		MM_UNKNOW = 0,
		MM_ATMOST = (1 << 31),
		MM_FIX = (1 << 30),
		MM_PERCENT = (1 << 29),
		MM_WRAP_CONTENT = (1 << 28),

		MW_MASK = 0x0ff00000,  // weight mask
		MM_MASK = 0xf0000000,
	};

	LayoutParent(int x, int y, Val width, Val height);
	virtual ~LayoutParent() {}

	// val = [0, 65535] weight = [0~100]
	static Val makeVal(int val, Model model, int weight);
	static Val makeVal(int val, Model model);
	static Model getModel(Val v);
	static int getVal(Val v);
	static int getWeight(Val v);
	static Val calcVal(Val target, Val parent);
	void setPadding(int left, int top, int right, int bottom);
	virtual void layout(int x, int y, int width, int height);
	virtual void measure(Val width, Val height) = 0;
	virtual int getXToTop();
	virtual int getYToTop();
	int getMeasureWidth();
	int getMeasureHeight();
	virtual BOOL isMeasured();
	virtual void clearMeasured();
	int getX();
	int getY();
	void setParent(LayoutParent *parent);
	Val getWidthVal();
	Val getHeightVal();
protected:
	int mX, mY;
	Val mWidth, mHeight;
	int mMeasureWidth, mMeasureHeight;
	int mPadLeft, mPadTop, mPadRight, mPadBottom;
	LayoutParent *mParent;
	BOOL mMeasured;
};

class Layout : public LayoutParent {
public:
	Layout(int x, int y, Val width, Val height);
	virtual ~Layout();
	virtual void addChild(LayoutParent *child);
	virtual BOOL isMeasured();
	virtual void clearMeasured();
	int getChildNum();
protected:
	LayoutParent *mChild[50];
	int mChildNum;
	friend class LayoutManager;
};

// targetWnd must set WNDCLASSEX.cbWndExtra = 32
class LayoutManager {
public:
	LayoutManager();
	void bind(HWND targetWnd, Layout *lay, BOOL autoLayout);
	~LayoutManager();
	void layout(int width, int height);
	void draw(HDC hdc);
private:
	static LRESULT CALLBACK LMWndProc(HWND, UINT, WPARAM, LPARAM);
	void drawLayout(LayoutParent* lay, HDC hdc, HBRUSH *br, int brIdx);
	HWND mTargetWnd;
	Layout *mLayout;
	WNDPROC mWndProc;
};

class WndLayout : public LayoutParent {
public:
	WndLayout(HWND wnd, int x, int y, Val width, Val height);
	virtual void layout(int x, int y, int width, int height);
	virtual void measure(Val width, Val height);
protected:
	HWND mWnd;
};

/*
	|---------------------------|
	|          top              |
	|---------------------------|
	| left |   center    | right|
	|---------------------------|
	|        bottom             |
	|---------------------------|
*/
class BorderLayout : public Layout {
public:
	enum Anchor {
		A_LEFT, A_TOP, A_RIGHT, A_BOTTOM, A_CENTER, A_NUM
	};
	BorderLayout(int x, int y, Val width, Val height);
	void addChild(LayoutParent *child, Anchor a);
	virtual void layout(int x, int y, int width, int height);
	virtual void measure(Val width, Val height);
};

class HLineLayout : public Layout {
public:
	HLineLayout(int x, int y, Val width, Val height);
	virtual void layout(int x, int y, int width, int height);
	virtual void measure(Val width, Val height);
	void setSpace(int space);
protected:
	int mSpace;
};