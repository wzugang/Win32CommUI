#pragma once
#include <windows.h>
class XmlNode;
class UIFactory;
class XComponent;
class XImage;

class XListener {
public:
	// if event has deal, return true
	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret) = 0;
};

class XComponent {
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
	enum ReflectWM {
		WM_COMMAND_SELF = 0x6000,
		WM_NOTIFY_SELF,
		WM_MOUSEWHEEL_BUBBLE
	};
	XComponent(XmlNode *node);
	static void init();
	static HINSTANCE getInstance();
	virtual void createWnd();
	void createWndTree(HWND parent);

	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	virtual void layout(int x, int y, int width, int height);
	void mesureChildren(int widthSpec, int heighSpec);
	static int getSpecSize(int sizeSpec);
	static int calcSize(int selfSizeSpec, int parentSizeSpec);

	HWND getWnd();
	XmlNode *getNode() {return mNode;}
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result); // return true: has deal
	XComponent* findById(const char *id);
	XComponent* getChildById(const char *id);
	XComponent* getChildById(DWORD wndId);
	XComponent* getChild(int idx);
	void setListener(XListener *v);
	XListener* getListener();
	static DWORD generateWndId();
	virtual bool onCtrlColor(HDC dc, LRESULT *result);

	int getMesureWidth();
	int getMesureHeight();
	int getAttrX();
	int getAttrY();
	int getAttrWeight();
	int *getAttrMargin();

	static LRESULT CALLBACK __WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
	virtual ~XComponent();
protected:
	void parseAttrs();
	void applyAttrs();
	HWND getParentWnd();
	HFONT getFont();
	DWORD mID;
	HWND mWnd, mParentWnd;
	XmlNode *mNode;
	int mX, mY, mWidth, mHeight, mMesureWidth, mMesureHeight;
	int mAttrX, mAttrY, mAttrWidth, mAttrHeight;
	int mAttrPadding[4], mAttrMargin[4];
	COLORREF mAttrColor, mAttrBgColor;
	int mAttrRoundConerX, mAttrRoundConerY;
	int mAttrWeight;
	int mAttrFlags;

	HBRUSH mBgColorBrush;
	XImage *mBgImage;
	XListener *mListener;
	HFONT mFont;
	HRGN mRectRgn;
	char mClassName[32];
	static HINSTANCE mInstance;
	friend class UIFactory;
};

class XAbsLayout : public XComponent {
public:
	XAbsLayout(XmlNode *node);
	virtual void onLayout(int width, int height);
};

class XHLineLayout : public XComponent {
public:
	XHLineLayout(XmlNode *node);
	virtual void onLayout(int width, int height);
};

class XVLineLayout : public XComponent {
public:
	XVLineLayout(XmlNode *node);
	virtual void onLayout(int width, int height);
};

class XBasicWnd : public XComponent {
public:
	XBasicWnd(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res);
	virtual void createWnd();
protected:
	WNDPROC mOldWndProc;
};

class XButton : public XBasicWnd {
public:
	XButton(XmlNode *node);
	virtual void createWnd();
};

class XLabel : public XBasicWnd {
public:
	XLabel(XmlNode *node);
	virtual void createWnd();
};
class XCheckBox : public XButton {
public:
	XCheckBox(XmlNode *node);
	virtual void createWnd();
};
class XRadio : public XButton {
public:
	XRadio(XmlNode *node);
	virtual void createWnd();
};
class XGroupBox : public XButton {
public:
	XGroupBox(XmlNode *node);
	virtual void createWnd();
};
class XEdit : public XBasicWnd {
public:
	XEdit(XmlNode *node);
	virtual void createWnd();
};

class XComboBox : public XBasicWnd {
public:
	XComboBox(XmlNode *node);
	virtual void createWnd();
protected:
	int mDropHeight;
};
class XTable : public XBasicWnd {
public:
	XTable(XmlNode *node);
	virtual void createWnd();
};
class XTree : public XBasicWnd {
public:
	XTree(XmlNode *node);
	virtual void createWnd();
};

class XTab : public XComponent {
public:
	XTab(XmlNode *node);
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
protected:
	WNDPROC mOldWndProc;
};
class XListBox : public XBasicWnd {
public:
	XListBox(XmlNode *node);
	virtual void createWnd();
};
class XDateTimePicker : public XBasicWnd {
public:
	XDateTimePicker(XmlNode *node);
	virtual void createWnd();
};

// only has one child
class XWindow : public XComponent {
public:
	XWindow(XmlNode *node);
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	virtual int messageLoop();
	virtual void show(int nCmdShow);
};

// only has one child
class XDialog : public XComponent {
public:
	XDialog(XmlNode *node);
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	int showModal();
	void close(int nRet);
};

// every time: only has one visible child; but can has many invisible child
class XScroll : public XComponent {
public:
	XScroll(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
protected:
	void moveChildrenPos(int dx, int dy);
};

