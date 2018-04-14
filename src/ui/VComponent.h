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

class VListener {
public:
	// if event has deal, return true
	virtual bool onEvent(VComponent *evtSource, Msg *msg) = 0;
};

struct XRect {
	int mX, mY, mWidth, mHeight;
	XRect();
	XRect(int x, int y, int w, int h);
	bool isValid();
	void offset(int dx, int dy);
	XRect intersect(XRect &r);
	XRect join(XRect &r);
	void reset();
	void from(RECT &r);
	RECT to();
};

struct Msg {
	enum ID {
		NONE = -100,
		LBUTTONDOWN, LBUTTONUP, RBUTTONDOWN, RBUTTONUP,MOUSE_CANCEL,
		CLICK, DBCLICK, MOUSE_MOVE, MOUSE_WHEEL, MOUSE_LEAVE, 

		KEY_DOWN, KEY_UP, CHAR,
		LOST_FOCUS, GAIN_FOCUS,
		PAINT,
		SET_CURSOR,
	};
	union VKeys {
		int ctrl:1;
		int shift:1;
		int lbutton:1;
		int mbutton:1;
		int rbutton:1;
	};
	Msg() {memset(this, 0, sizeof(Msg));}
	ID mId;
	// params
	// vkeys is one bit of MK_CONTROL | MK_LBUTTON | MK_MBUTTON | MK_MBUTTON | MK_SHIFT
	// if has MK_CONTROL bit, means ctrl is press down
	struct {int x; int y; VKeys vkey; int deta;} mouse;
	struct {VKeys vkey; wchar_t code;} key;
	struct {HDC dc; XRect rect;} paint;
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

	virtual bool dispatchMessage(Msg *msg);
	POINT getDrawPoint();
	RECT getDrawRect();

	void parseAttrs();
	HFONT getFont();
	virtual bool onMouseEvent(Msg *m);
	virtual bool onKeyEvent(Msg *m);
	virtual bool onFocusEvent(bool gainFocus);
	virtual void dispatchPaintEvent(Msg *m);
	virtual void dispatchPaintMerge(HDC dstDc, XRect &clip, int x, int y);
	// return true means merge cache image to parent layer
	virtual bool onPaint(HDC dc);
	void drawCache(HDC dc);
	virtual void eraseBackground(HDC dc);
	virtual void repaint(XRect *dirtyRect = NULL);
	void updateWindow();
	virtual void setCapture();
	virtual void releaseCapture();
	virtual void setFocus();
	virtual void releaseFocus();
	virtual VBaseWindow *getRoot();
	bool hasBackground();

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
	virtual void notifyLayout();
	void setWnd(HWND hwnd);
	virtual HWND getWnd();
protected:
	virtual RECT getClientRect();
	virtual bool dispatchMessage(Msg *msg);
	virtual void onLayoutChildren(int width, int height);
	virtual void applyIcon();
	virtual void applyAttrs();
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual DWORD getStyle(DWORD def);
protected:
	HWND mWnd;
	VComponent *mCapture, *mFocus;
	friend class VComponent;
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

