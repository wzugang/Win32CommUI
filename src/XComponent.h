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
	enum MSG_ID {
		MSG_COMMAND = 0x6000,
		MSG_NOTIFY,
		MSG_MOUSEWHEEL_BUBBLE,
		MSG_LBUTTONDOWN_BUBBLE,

		// wParam is click item index, my be -1, lParam is pointer of POINT
		MSG_LIST_CLICK_ITEM,
		MSG_LIST_DBCLICK_ITEM, 

		// wParam is click item index, my be -1
		MSG_COMBOBOX_CLICK_ITEM, 
		//  popup has closed, wParam = 0 or 1 (0:normal close, 1:cancel close)
		MSG_POPUP_CLOSED, 
		// calendar select a date wParam = XExtCalenar::Date pointer
		MSG_CALENDAR_SEL_DATE, 
		
		// ext tree select node changed; wParam = XExtTreeNode pointer
		MSG_TREE_SEL_CHANGED, 
		// checkable node checked changed; wParam = XExtTreeNode pointer
		MSG_TREE_CHECK_CHANGED 
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
	XComponent* getParent();

	void setListener(XListener *v);
	XListener* getListener();

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
	virtual void applyIcon();
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

