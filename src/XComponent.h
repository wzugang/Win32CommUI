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

class XPopupManager {
public:
	static XPopupManager* getInstance();
	void setPopupWnd(HWND wnd);
	HWND getPopupWnd();
	bool hasPopupShowing();
private:
	XPopupManager();
	HWND mWnd;
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
class XTableModel {
public:
	void apply(HWND tableWnd);
protected:
	virtual int getColumnCount() = 0;
	virtual int getRowCount() = 0;
	virtual int getColumnWidth(int col) = 0;
	virtual char *getColumnTitle(int col) = 0;

	virtual void getColumn(int col, LVCOLUMN *lvc);
	virtual void getItem(int row, int col, LVITEM *item);
};
class XTree : public XBasicWnd {
public:
	XTree(XmlNode *node);
	virtual void createWnd();
};

class XTreeNode {
public:
	XTreeNode(const char *text);
	XTreeNode * append(XTreeNode *node);
	XTreeNode * insert(int pos, XTreeNode *node);
	XTreeNode *appendTo(XTreeNode *parent);
	XTreeNode *insertTo(int pos, XTreeNode *parent);
	TVITEM *getTvItem();
	void create(HWND wnd);
	virtual ~XTreeNode();
protected:
	virtual HWND getWnd();
	TV_INSERTSTRUCT mNodeInfo;
	HTREEITEM mItem;
	XTreeNode *mParent;
	std::vector<XTreeNode*> mChildren;
	friend class XTreeRootNode;
};
//根结点为虚拟结点，不显示
class XTreeRootNode : public XTreeNode {
public:
	XTreeRootNode(HWND treeWnd);
	void apply();
protected:
	virtual HWND getWnd();
	HWND mTreeWnd;
	friend class XTreeNode;
};

class XTab : public XComponent {
public:
	XTab(XmlNode *node);
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *res);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
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

// every time: only has one visible child; but can has many invisible child
class XScroll : public XComponent {
public:
	XScroll(XmlNode *node);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	void moveChildrenPos(int dx, int dy);
};

