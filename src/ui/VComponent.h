#pragma once
#include <windows.h>
#include <CommCtrl.h>
#include <vector>
#include <list>

class XmlNode;
class UIFactory;
class VComponent;
class XImage;
struct Msg;
class VBaseWindow;
class VPopup;

class VListener {
public:
	// if event has deal, return true
	virtual bool onEvent(VComponent *evtSource, Msg *msg) = 0;
};

struct XRect {
	int mX, mY, mWidth, mHeight;
	XRect();
	XRect(int x, int y, int w, int h);
	XRect(RECT &r);

	bool isValid() const;
	void offset(int dx, int dy);
	XRect intersect(XRect &r);
	XRect join(XRect &r);
	void reset();
	void set(int x, int y, int w, int h);
	bool contains(int x, int y);

	void from(RECT &r);
	RECT to();
};

struct Msg {
	enum ID {
		NONE = -100,
		MOUSE_MSG_BEGIN,
		LBUTTONDOWN, LBUTTONUP, RBUTTONDOWN, RBUTTONUP,
		DBCLICK, MOUSE_MOVE, MOUSE_WHEEL,
		MOUSE_LEAVE, MOUSE_CANCEL, CLICK,
		MOUSE_MSG_END,

		KEY_MSG_BEGIN,
		KEY_DOWN, KEY_UP, CHAR,
		KEY_MSG_END,

		LOST_FOCUS, GAIN_FOCUS,
		PAINT,
		SET_CURSOR, 
		TIMER,

		HSCROLL, VSCROLL
	};
	struct VKeys {
		bool ctrl;
		bool shift;
		bool lbutton;
		bool mbutton;
		bool rbutton;
	};
	Msg() {memset(this, 0, sizeof(Msg));}
	ID mId;
	// params
	// vkeys is one bit of MK_CONTROL | MK_LBUTTON | MK_MBUTTON | MK_MBUTTON | MK_SHIFT
	// if has MK_CONTROL bit, means ctrl is press down
	struct {int x; int y; VKeys vkey; int deta; VComponent *moveAt; VComponent *pressAt;} mouse;
	struct {VKeys vkey; wchar_t code;} key;
	struct {HDC dc; XRect clip; int x; int y;} paint;
	struct {WPARAM wParam; LPARAM lParam;} def;
	
	LRESULT mResult;
};

class VComponent {
public:
	enum SizeSpec {
		MS_FIX = 1 << 30,
		MS_PERCENT = 1 << 29,
		MS_AUTO = 1 << 28,
		MS_ATMOST = 1 << 27
	};
	enum AttrFlag {
		AF_COLOR = (1 << 0),
		AF_BG_COLOR = (1 << 1),
	};

	enum Visibility {
		VISIBLE, INVISIBLE, VISIBLE_GONE
	};

	VComponent(XmlNode *node);
	static HINSTANCE getInstance();
	static int getSpecSize(int sizeSpec);
	static int calcSize(int selfSizeSpec, int parentSizeSpec);

	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onMesureChildren(int widthSpec, int heighSpec);
	virtual void onLayout(int x, int y, int width, int height);
	virtual void onLayoutChildren(int width, int height);

	virtual HWND getWnd();
	XmlNode *getNode() {return mNode;}
	VComponent* findById(const char *id);
	VComponent* getChildById(const char *id);
	VComponent* getChildById(DWORD wndId);
	VComponent* getChild(int idx);
	VComponent* getParent();

	void setListener(VListener *v);
	VListener* getListener();

	int getWidth();
	int getHeight();
	int getMesureWidth();
	int getMesureHeight();

	int getX();
	int getY();
	int getAttrX();
	int getAttrY();
	void setAttrX(int attrX);
	void setAttrY(int attrY);
	int getAttrWeight();
	void setAttrWeight(int weight);
	int *getAttrMargin();
	void setAttrMargin(int left, int top, int right, int bottom);
	int *getAttrPadding();
	void setAttrPadding(int left, int top, int right, int bottom);
	COLORREF getAttrBgColor();
	void setAttrBgColor(COLORREF c);
	COLORREF getAttrColor();
	void setAttrColor(COLORREF c);
	void parseAttrs();
	HFONT getFont();
	Visibility getVisibility();
	void setVisibility(Visibility v);

	virtual bool dispatchMessage(Msg *msg);
	virtual bool dispatchMouseMessage(Msg *msg);
	virtual bool dispatchPaintMessage(Msg *m);
	
	virtual bool onMouseEvent(Msg *m);
	virtual bool onKeyEvent(Msg *m);
	virtual bool onFocusEvent(bool gainFocus);
	virtual void onPaint(Msg *m);

	virtual void drawCache(HDC dc);
	virtual void eraseBackground(Msg *m);
	virtual void repaint(XRect *dirtyRect = NULL);
	void updateWindow();
	virtual void setCapture();
	virtual void releaseCapture();
	virtual void setFocus();
	virtual void releaseFocus();
	virtual VBaseWindow *getRoot();
	bool hasBackground();
	POINT getDrawPoint();
	RECT getDrawRect();

	virtual ~VComponent();
protected:
	DWORD mID;
	XmlNode *mNode;
	int mX, mY, mWidth, mHeight, mMesureWidth, mMesureHeight;
	int mAttrX, mAttrY, mAttrWidth, mAttrHeight;
	int mAttrPadding[4], mAttrMargin[4];
	COLORREF mAttrColor, mAttrBgColor;
	int mAttrRoundConerX, mAttrRoundConerY;
	int mAttrWeight;
	int mAttrFlags;
	Visibility mVisibility;
	int mTranslateX, mTranslateY; // always >= 0
	bool mEnableFocus, mHasFocus;
	bool mEnable;

	XImage *mBgImage;
	VListener *mListener;
	HFONT mFont;
	HRGN mRectRgn;

	bool mHasDirtyChild;
	bool mDirty;
	XRect mDirtyRect;
	XImage *mCache;

	static HINSTANCE mInstance;
	friend class UIFactory;
};

// layout like AbsLayout
class VBaseWindow : public VComponent {
public:
	VBaseWindow(XmlNode *node);
	void setWnd(HWND hwnd);
	virtual HWND getWnd();
	void setFocus(VComponent *who);
	virtual void notifyLayout();
	void startTimer(VComponent *src, DWORD timerId, int elapse);
	void killTimer(VComponent *src, DWORD timerId);
protected:
	virtual RECT getClientRect();
	virtual bool dispatchMessage(Msg *msg);
	virtual bool dispatchPaintMessage(Msg *m);
	virtual bool dispatchMouseMessage(Msg *msg);
	virtual void onLayoutChildren(int width, int height);
	virtual void applyIcon();
	virtual void applyAttrs();
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual DWORD getStyle(DWORD def);
protected:
	HWND mWnd;
	VComponent *mCapture, *mFocus;
	VComponent *mLastMouseAt;
	enum {
		MAX_POPUP_NUM = 10
	};
	VPopup *mPopups[MAX_POPUP_NUM];
	struct TimerItem {VComponent *src; DWORD mTimerId;};
	std::list<TimerItem> mTimerList;

	friend class VComponent;
	friend class VPopup;
};

class VWindow : public VBaseWindow {
public:
	VWindow(XmlNode *node);
	virtual void createWnd();
	virtual void show();
	virtual int msgLoop();
};

class VDialog : public VBaseWindow {
public:
	// node's parent should be a VBaseWindow
	VDialog(XmlNode *node);
	virtual void createWnd(HWND parent);
	int showModal();
	void showNormal();
	void close(int nRet);
protected:
	void showCenter();
	virtual bool dispatchMessage(Msg *msg);
protected:
	bool mShowModal;
	HWND mParentWnd;
};

class VPopup : public VComponent {
public:
	enum MouseAction {
		MA_INTERREPT, // 拦截
		MA_TO_NEXT,   // 下发到下一层
		MA_CLOSE      // 关闭自己
	};
	// node's parent should be a VBaseWindow
	VPopup(XmlNode *node);
	virtual void show(int x, int y);
	virtual void close();
	void setMouseAction(MouseAction ma);
protected:
	virtual bool onMouseActionWhenOut(Msg *m);

	MouseAction mMouseAction;
	friend class VBaseWindow;
};