#pragma once
#include <windows.h>
#include <CommCtrl.h>
#include <vector>
class XmlNode;
class UIFactory;
class XComponent;
class XImage;
class XTreeRootNode;

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
		WM_MOUSEWHEEL_BUBBLE,
		WM_LBUTTONDOWN_BUBBLE,

		WM_EXT_LIST_CLICK_ITEM,  // wParam is click item index, my be -1
		WM_EXT_POPUP_CLOSED, //  popup has closed, wParam = 0 or 1 (0:normal close, 1:cancel close)
		WM_EXT_CALENDAR_SEL_DATE, // calendar select a date wParam = XExtCalenar::Date pointer
		
		WM_EXT_TREE_SEL_CHANGED, // ext tree select node changed; wParam = XExtTreeNode pointer
		WM_EXT_TREE_CHECK_CHANGED // checkable node checked changed; wParam = XExtTreeNode pointer
	};
	XComponent(XmlNode *node);
	static HINSTANCE getInstance();
	static int getSpecSize(int sizeSpec);
	static int calcSize(int selfSizeSpec, int parentSizeSpec);

	virtual void createWnd();
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	virtual void layout(int x, int y, int width, int height);
	virtual void mesureChildren(int widthSpec, int heighSpec);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result); // return true: has deal
	static LRESULT CALLBACK __WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void createWndTree();
	HWND getWnd();
	XmlNode *getNode() {return mNode;}
	XComponent* findById(const char *id);
	XComponent* getChildById(const char *id);
	XComponent* getChildById(DWORD wndId);
	XComponent* getChild(int idx);
	void setListener(XListener *v);
	XListener* getListener();

	int getMesureWidth();
	int getMesureHeight();
	int getAttrX();
	int getAttrY();
	int getAttrWeight();
	int *getAttrMargin();
	void setBgColor(COLORREF c);
	virtual ~XComponent();
protected:
	virtual bool onCtrlColor(HDC dc, LRESULT *result);
	static DWORD generateWndId();
	void parseAttrs();
	void applyAttrs();
	HWND getParentWnd();
	HFONT getFont();
	static bool bubbleMsg(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
protected:
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

class XWindow : public XComponent {
public:
	XWindow(XmlNode *node);
	virtual int messageLoop();
	virtual void show(int nCmdShow);
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
};

// only has one child
class XDialog : public XComponent {
public:
	XDialog(XmlNode *node);
	int showModal();
	void close(int nRet);
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
};

