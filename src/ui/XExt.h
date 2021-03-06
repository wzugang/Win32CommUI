#pragma once
#include "XComponent.h"
#include "XText.h"

class XExtComponent : public XComponent {
public:
	enum StateImage {
		STATE_IMG_NORMAL,
		STATE_IMG_HOVER,
		STATE_IMG_PUSH,
		STATE_IMG_DISABLE
	};
	XExtComponent(XmlNode *node);
	virtual void layout(int x, int y, int width, int height);
	void setEnableFocus(bool enable);
	virtual ~XExtComponent();
protected:
	void eraseBackground(HDC dc);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onPaint(HDC dc, RECT *paintRect) {}
	virtual void onKeyDown(int vkey) {}
	virtual void onKeyUp(int vkey) {}
	virtual void onChar(wchar_t ch) {}
	// vkeys is one bit of MK_CONTROL | MK_LBUTTON | MK_MBUTTON | MK_MBUTTON | MK_SHIFT
	// if has MK_CONTROL bit, means ctrl is press down
	virtual void onMouseMove(int x, int y, int vkeys) {}
	virtual void onLButtonDown(int x, int y, int vkeys) {}
	virtual void onLButtonUp(int x, int y, int vkeys) {}
	virtual void onLButtonDbClick(int x, int y, int vkeys) {}
	virtual void onRButtonDown(int x, int y, int vkeys) {}
	virtual void onRButtonUp(int x, int y, int vkeys) {}
	virtual void onRButtonDbClick(int x, int y, int vkeys) {}
	// deta is multiples of WHEEL_DELTA(120)
	virtual void onWheel(int x, int y, int deta, int vkeys) {}
	virtual void onMouseLeave() {}
protected:
	XImage *mBgImageForParnet;
	bool mEnableFocus;
	XImage *mMemBuffer;
	bool mEnableMemBuffer;
	bool _mMouseTrack;
};

class XExtEmptyComponent : public XExtComponent {
public:
	XExtEmptyComponent(XmlNode *node);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
};

class XExtLabel : public XExtComponent {
public:
	XExtLabel(XmlNode *node);
	char *getText();
	void setText(char *text);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
protected:
	char *mText;
};

class XExtButton : public XExtComponent {
public:
	XExtButton(XmlNode *node);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual StateImage getStateImage();
protected:
	XImage *mStateImages[8];
	bool mIsMouseDown;
	bool mIsMouseMoving;
	bool mIsMouseLeave;
};

class XExtOption : public XExtButton {
public:
	XExtOption(XmlNode *node);
	bool isSelect();
	void setSelect(bool select);
	void setAutoSelect(bool autoSelect);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual StateImage getStateImage();
	enum {
		BTN_IMG_SELECT = 5
	};
	bool mIsSelect;
	bool mAutoSelect;
};

class XExtCheckBox : public XExtOption {
public:
	XExtCheckBox(XmlNode *node);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
};

class XExtRadio : public XExtCheckBox {
public:
	XExtRadio(XmlNode *node);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	void unselectOthers();
};

class XExtIconButton : public XExtOption {
public:
	XExtIconButton(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
protected:
	RECT getRectBy(int *attr);
protected:
	XImage *mIcon;
	int mAttrIconRect[4]; // left, top, width, height
	int mAttrTextRect[4]; // left, top, width, height
};

class XExtScrollBar : public XExtComponent {
public:
	XExtScrollBar(XmlNode *node, bool horizontal = false);
	int getMax();
	int getPage();
	void setMaxAndPage(int maxn, int page);
	int getPos();
	void setPos(int pos); // pos = [0 ... max - page]
	bool isNeedShow();
	int getThumbSize();
	void setThumbSize(int sz);
	virtual void onMeasure(int widthSpec, int heightSpec);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	void calcThumbInfo();
	bool mHorizontal;
	int mMax;
	int mPage;
	int mPos;
	RECT mThumbRect;
	XImage *mTrack, *mThumb;
	bool mPressed;
	int mMouseX, mMouseY;
};

class XExtScroll : public XExtComponent {
public:
	XExtScroll(XmlNode *node);
	virtual ~XExtScroll();
	XExtScrollBar* getHorBar();
	XExtScrollBar* getVerBar();

	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
protected:
	virtual SIZE calcDataSize();
	virtual void moveChildrenPos( int x, int y );
	void invalide(XComponent *c);
	XExtScrollBar *mHorBar, *mVerBar;
	XmlNode *mHorNode, *mVerNode;
};

class XExtPopup : public XComponent {
public:
	XExtPopup(XmlNode *node);
	virtual void show(int screenX, int screenY);
	virtual void close();
	void disableChildrenFocus();
	virtual ~XExtPopup();
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onLayout(int width, int height);
	virtual int messageLoop();
protected:
	bool mMsgLooping;
};

class XExtTableModel {
public:
	struct ColumnWidth {
		int mWidthSpec;
		int mWeight;
	};
	virtual int getColumnCount() = 0;
	virtual int getRowCount() = 0;
	virtual ColumnWidth getColumnWidth(int col) = 0;
	virtual int getRowHeight(int row) = 0;
	virtual int getHeaderHeight() = 0;
	virtual XImage *getHeaderImage() = 0;
	virtual char *getHeaderText(int col) = 0;
	virtual char *getCellData(int row, int col) = 0;
	// virtual bool canSelect(int row) = 0;
};
class XExtTable : public XExtScroll {
public:
	class CellRender {
	public:
		virtual void onDrawCell(HDC dc, int row, int col, int x, int y, int w, int h) = 0;
	};
	enum Attr {
		ATTR_HAS_HEADER = 1,
		ATTR_HAS_COL_LINE = 2,
		ATTR_HAS_ROW_LINE = 4
	};
	XExtTable(XmlNode *node);
	void setModel(XExtTableModel *model);
	void setCellRender(CellRender *render);
	virtual ~XExtTable();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void drawHeader(HDC dc, int w, int h);
	void drawData( HDC dc, int x, int y, int w, int h );
	virtual void drawRow(HDC dc, int row, int x, int y, int w, int h );
	virtual void drawCell(HDC dc, int row, int col, int x, int y, int w, int h );
	virtual void drawGridLine(HDC dc, int from, int to, int y);

	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	void mesureColumn(int width, int height);
	SIZE calcDataSize();
	virtual void moveChildrenPos( int dx, int dy );
	SIZE getClientSize();
	void getVisibleRows(int *from, int *to);
	int findCell(int x, int y, int *col);
protected:
	XExtTableModel *mModel;
	int mColsWidth[50];
	SIZE mDataSize;
	HPEN mLinePen;
	int mSelectedRow;
	HBRUSH mSelectBgBrush;
	CellRender *mCellRender;
};

class XExtTextArea : public XExtComponent, public XAreaText {
public:
	XExtTextArea(XmlNode *node);
	void setReadOnly(bool r);
	void setEnableShowCaret(bool enable);
	virtual void setText( const char *txt );
	virtual void setWideText( const wchar_t *txt );
	virtual ~XExtTextArea();

	virtual void createWnd();
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	virtual HFONT getTextFont();
protected:
	virtual void notifyChanged();

	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onChar(wchar_t ch);
	virtual void onLButtonDown(int keyState, int x, int y);
	virtual void onLButtonUp(int keyState, int x, int y);
	virtual void onMouseMove(int x, int y);
	virtual void onPaint(HDC hdc);
	virtual void drawSelRange(HDC hdc, int begin, int end);
	virtual void onKeyDown(int key);
	virtual void move(int key);

	// override XAreaText function
	virtual int getScrollX();
	virtual int getScrollY();
	virtual void setScrollX(int x);
	virtual void setScrollY(int y);
	virtual SIZE getClientSize();
	virtual HWND getBindWnd();
	virtual int getRealX(int x);
	virtual int getRealY(int y);

	virtual void getVisibleRows(int *from, int *to);
	virtual void ensureVisible(int pos);

	virtual void back();
	virtual void del();
	virtual void copy();
	virtual void paste();
protected:
	int mInsertPos;
	int mBeginSelPos, mEndSelPos;
	bool mReadOnly;
	HPEN mCaretPen;
	bool mCaretShowing;
	bool mEnableShowCaret;
	bool mEnableScrollBars;
	XExtScrollBar *mVerBar;
	XmlNode *mVerBarNode;
};

class XExtLineEdit : public XExtTextArea {
public:
	XExtLineEdit(XmlNode *node);
	virtual ~XExtLineEdit();
	virtual void insertText( int pos, wchar_t *txt, int len );
protected:
	virtual void onChar(wchar_t ch);
	virtual void createWnd();
};

class XListModel {
public:
	struct ItemData {
		char *mText;
		bool mSelectable;
		bool mSelected;
		void *mUsrData;
	};
	virtual int getItemCount() = 0;
	virtual int getItemHeight(int item) = 0;
	virtual ItemData *getItemData(int item) = 0;
	virtual bool isMouseTrack() = 0;
};

class XExtList : public XExtScroll {
public:
	class ItemRender {
	public:
		virtual void onDrawItem(HDC dc, int item, int x, int y, int w, int h) = 0;
	};
	XExtList(XmlNode *node);
	void setModel(XListModel *model);
	XListModel *getModel();
	void setItemRender(ItemRender *render);
	int getItemY(int item);
	int getMouseTrackItem();

	virtual ~XExtList();
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	void notifyModelChanged();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual SIZE calcDataSize();
	SIZE getClientSize();
	virtual void moveChildrenPos( int dx, int dy );
	virtual void drawData( HDC memDc, int x, int y,  int w, int h );
	virtual void drawItem(HDC dc, int item, int x, int y, int w, int h);
	void getVisibleRows(int *from, int *num);
	int findItem(int x, int y);
	void updateTrackItem(int x, int y);
protected:
	XListModel *mModel;
	ItemRender *mItemRender;
	SIZE mDataSize;
	int mMouseTrackItem;
	HBRUSH mSelectBgBrush;
};

class XExtArrowButton : public XExtButton {
public:
	// when click, send MSG_COMMAND, wParam is true means click at arrow
	XExtArrowButton(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual StateImage getStateImage();
protected:
	bool isPointInArrow(int x, int y);
	enum {
		ARROW_IMG_HOVER = 5,
		ARROW_IMG_PUSH = 6
	};
	bool mMouseAtArrow;
	int mArrowWidth;
};

class XExtComboBox : public XExtComponent, public XListener {
public:
	class BoxRender {
	public:
		virtual void onDrawBox(HDC dc, int x, int y, int w, int h) = 0;
	};
	XExtComboBox(XmlNode *node);
	void setEnableEditor(bool enable);
	XExtList *getExtList();
	void setBoxRender(BoxRender *r);
	int getSelectItem();
	void setSelectItem(int idx);
	virtual ~XExtComboBox();

	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	virtual void createWnd();
protected:
	virtual void drawBox(HDC dc, int x, int y, int w, int h);
	void openPopup();
	StateImage getStateImage();
protected:
	XExtLineEdit *mEdit;
	XExtPopup *mPopup;
	XExtList *mList;
	XmlNode *mEditNode, *mPopupNode, *mListNode;
	int mArrowWidth;
	SIZE mAttrPopupSize;
	bool mEnableEditor;
	XImage *mStateImages[8];
	BoxRender *mBoxRender;
	bool mPoupShow;
	int mSelectItem;
	bool mIsMouseDown, mIsMouseMoving, mIsMouseLeave;
};

class XExtMenuModel;
class XExtMenuManager;

struct XExtMenuItem {
	XExtMenuItem(const char *name, char *text);
	char mName[40];
	char *mText;
	bool mActive;
	bool mVisible;
	bool mCheckable;
	bool mChecked;
	bool mSeparator;
	XExtMenuModel *mChild;
};

class XExtMenuModel {
public:
	XExtMenuModel();
	void add(XExtMenuItem *item);
	void insert(int pos, XExtMenuItem *item);
	int getCount();
	XExtMenuItem *get(int idx);
	XExtMenuItem *findByName(const char *name);
	virtual int getItemHeight(int pos);
	virtual int getWidth();
	virtual int getHeight();
	~XExtMenuModel();
protected:
	XExtMenuItem *mItems[50];
	int mCount;
	int mWidth;
};

class XExtMenu : public XExtComponent {
public:
	XExtMenu(XmlNode *node, XExtMenuManager *mgr);
	void setModel(XExtMenuModel *list);
	XExtMenuModel *getMenuList() {return mModel;}
	virtual void show(int screenX, int screenY);
	virtual ~XExtMenu();
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	int getItemIndexAt(int x, int y);
	virtual void calcSize();
	void drawItems(HDC dc);
	RECT getItemRect(int idx);
protected:
	XExtMenuModel *mModel;
	int mSelectItem;
	HPEN mSeparatorPen;
	HPEN mCheckedPen;
	HBRUSH mSelectBrush;
	XExtMenuManager *mManager;
};

class XExtMenuManager {
public:
	class ItemListener {
	public:
		virtual void onClickItem(XExtMenuItem *item) = 0;
	};

	XExtMenuManager(XExtMenuModel *mlist, XComponent *owner, ItemListener *listener);
	void show(int screenX, int screenY);
	virtual ~XExtMenuManager();

	void notifyItemClicked(XExtMenuItem *item);
	void closeMenu(XExtMenuModel *mlist);
	void openMenu(XExtMenuModel *mlist, int x, int y);
protected:
	void messageLoop();
	int whereIs(int x, int y);
	void closeMenuTo(int idx);
protected:
	XExtMenuModel *mMenuList;
	XExtMenu *mMenus[10];
	int mLevel;
	XComponent *mOwner;
	ItemListener *mListener;
	bool mLooping;
};

class XExtTreeNode {
public:
	enum PosInfo { PI_FIRST = 1, PI_LAST = 2, PI_CENTER = 4 };
	XExtTreeNode(const char *text);
	// pos : -1 means append
	void insert(int pos, XExtTreeNode *child);
	void remove(int pos);
	void remove(XExtTreeNode *child);
	int indexOf(XExtTreeNode *child);
	int getChildCount();
	void *getUserData();
	void setUserData(void *userData);
	int getContentWidth();
	void setContentWidth(int w);
	XExtTreeNode *getChild(int idx);
	bool isExpand();
	void setExpand(bool expand);
	char *getText();
	void setText(char *text);
	PosInfo getPosInfo();
	int getLevel();
	XExtTreeNode *getParent();
	bool isCheckable();
	void setCheckable(bool cb);
	bool isChecked();
	void setChecked(bool cb);
protected:
	char *mText;
	XExtTreeNode *mParent;
	std::vector<XExtTreeNode*> *mChildren;
	DWORD mAttrFlags;
	bool mExpand;
	void *mUserData;
	int mContentWidth;
	bool mCheckable;
	bool mChecked;
};
class XExtTree : public XExtScroll {
public:
	class NodeRender {
	public:
		virtual void onDrawNode(HDC dc, XExtTreeNode *node, int x, int y, int w, int h) = 0;
	};
	enum WhenSelect {
		WHEN_CLICK, WHEN_DBCLICK, WHEN_NONE
	};
	XExtTree(XmlNode *node);
	//根结点为虚拟结点，不显示
	void setModel(XExtTreeNode *root);
	XExtTreeNode* getModel();
	WhenSelect getWhenSelect();
	void setWhenSelect(WhenSelect when);
	void notifyChanged();
	void setNodeRender(NodeRender *render);
	// @return false:表示node是被折叠的，没有大小; true:表示node是未折叠的，取得了rect
	bool getNodeRect(XExtTreeNode *node, RECT *r);
	XExtTreeNode *getAtPoint(int x, int y);
	XExtTreeNode *getSelectNode();
	void setSelectNode(XExtTreeNode *selNode);
	virtual ~XExtTree();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	SIZE calcDataSize();
	SIZE getClientSize();
	virtual void moveChildrenPos( int dx, int dy );
	void drawData(HDC dc, int w, int h);
	void drawNode(HDC dc, XExtTreeNode *n, int level, int clientWidth, int clientHeight, int *y);
	void onLBtnDown(int x, int y);
	void onLBtnDbClick(int x, int y);
	XExtTreeNode *getNodeAtY(int y, int *py);
protected:
	SIZE mDataSize;
	XExtTreeNode *mModel;
	HPEN mLinePen, mCheckPen;
	HBRUSH mSelectBgBrush;
	XExtTreeNode *mSelectNode;
	int mWidthSpec, mHeightSpec;
	NodeRender *mNodeRender;
	WhenSelect mWhenSelect;
};

class XExtCalendar : public XExtComponent {
public:
	struct Date {
		Date();
		bool isValid();
		bool equals(const Date &d);
		int mYear; int mMonth; int mDay;
	};
	XExtCalendar(XmlNode *node);
	Date getSelectDate();
	void setSelectDate(Date d);
	virtual ~XExtCalendar();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	void drawSelDay( HDC dc );
	void drawSelMonth( HDC dc );
	void drawSelYear( HDC dc );
	void drawHeader(HDC dc);
	void fillViewDates(int year, int month);
	static int getDaysNum(int year, int month);
	void onLButtonDownInDayMode(int x, int y);
	void onLButtonDownInMonthMode(int x, int y);
	void onLButtonDownInYearMode(int x, int y);
	void resetSelect();
	void onMouseMoveInDayMode( int x, int y );
	void onMouseMoveInMonthMode( int x, int y );
	void onMouseMoveInYearMode( int x, int y );
protected:
	enum ViewMode {
		VM_SEL_DAY, VM_SEL_MONTH, VM_SEL_YEAR, VM_NUM
	};
	ViewMode mViewMode;
	RECT mLeftArrowRect, mRightArowRect;
	RECT mHeadTitleRect;
	bool mSelectLeftArrow, mSelectRightArrow, mSelectHeadTitle;
	int mTrackSelectIdx;
	HBRUSH mArrowNormalBrush, mArrowSelBrush, mSelectBgBrush, mTrackBgBrush;
	HPEN mLinePen;
	int mYearInDayMode, mMonthInDayMode;
	int mYearInMonthMode;
	int mBeginYearInYearMode, mEndYearInYearMode;
	Date mSelectDate;
	Date mViewDates[42];
	COLORREF mNormalColor, mGreyColor;
	bool mTrackMouseLeave;
};

class XExtMaskEdit : public XExtTextArea {
public:
	typedef bool (*InputValidate)(int pos, char ch);
	enum Case { C_NONE, C_UPPER, C_LOWER };
	XExtMaskEdit(XmlNode *node);
	/*
	*  @param mask  0: 0-9;  9:1-9;    A:A-Z;    a:a-z;
					C: 0-9 a-z A-Z;    H:0-9 A-F a-f   B:0-1
	**/
	void setMask(const char *mask);
	void setCase(Case c);
	void setPlaceHolder(char ch);
	void setInputValidate(InputValidate iv);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onChar( wchar_t ch );
	virtual void onPaint(HDC hdc);
	virtual void onKeyDown(int key);
	virtual int getPosAt(int x, int y);
	virtual bool acceptChar(char ch, int pos);
	bool isMaskChar(char ch);
protected:
	char *mMask;
	Case mCase;
	HBRUSH mCaretBrush;
	wchar_t mPlaceHolder;
	InputValidate mValidate;
};
class XExtPassword : public XExtTextArea {
public:
	XExtPassword(XmlNode *node);
protected:
	virtual void onChar( wchar_t ch );
	virtual void paste();
	virtual void onPaint(HDC hdc);
};

class XExtDatePicker : public XExtComponent, public XListener {
public:
	XExtDatePicker(XmlNode *node);
	char *getText();
	virtual ~XExtDatePicker();
protected:
	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	virtual void createWnd();
	void openPopup();
protected:
	XExtMaskEdit *mEdit;
	XExtPopup *mPopup;
	XExtCalendar *mCalendar;
	XmlNode *mEditNode, *mPopupNode, *mCalendarNode;
	RECT mArrowRect;
	SIZE mAttrPopupSize;
	SIZE mAttrArrowSize;
	XImage *mArrowNormalImage, *mArrowDownImage;
	bool mPoupShow;
};

class XAbsLayout : public XExtComponent {
public:
	XAbsLayout(XmlNode *node);
	virtual void onLayout(int width, int height);
};

class XHLineLayout : public XExtComponent {
public:
	XHLineLayout(XmlNode *node);
	virtual void onLayout(int width, int height);
};

class XVLineLayout : public XExtComponent {
public:
	XVLineLayout(XmlNode *node);
	virtual void onLayout(int width, int height);
};

class XExtWindow : public XWindow {
public:
	XExtWindow(XmlNode *node);
protected:
	virtual void createWnd();
	virtual RECT getClientRect();
protected:
	int mBorders[4]; // left top right bottom border's width/height
	bool mSizable;
};

class XExtDialog : public XDialog {
public:
	XExtDialog(XmlNode *node);
protected:
	virtual void createWnd();
	virtual RECT getClientRect();
protected:
	int mBorders[4]; // left top right bottom border's width/height
	bool mSizable;
};