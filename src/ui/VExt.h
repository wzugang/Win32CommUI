#pragma once
#include "VComponent.h"
#include "XText.h"


class VExtComponent : public VComponent {
public:
	enum StateImage {
		STATE_IMG_NORMAL = 0,
		STATE_IMG_HOVER = 1,
		STATE_IMG_PUSH = 2,
		STATE_IMG_FOCUS = 3,
		STATE_IMG_DISABLE = 4
	};
	VExtComponent(XmlNode *node);
	virtual ~VExtComponent();
protected:
	virtual StateImage getStateImage(void *param1, void *param2);
	virtual bool doStateImage(Msg *m);
protected:
	bool mEnableState;
	XImage *mStateImages[8];
	bool mMouseDown;
	bool mMouseMoving;
	bool mMouseLeave;
};


class VLabel : public VExtComponent {
public:
	VLabel(XmlNode *node);
	char *getText();
	void setText(char *text);
protected:
	virtual void onPaint(Msg *m);
protected:
	char *mText;
	int mTextAlign;
};


class VButton : public VExtComponent {
public:
	VButton(XmlNode *node);
protected:
	virtual void onPaint(Msg *m);
	bool onMouseEvent(Msg *m);
};


class VOption : public VButton {
public:
	VOption(XmlNode *node);
	bool isSelect();
	virtual void setSelect(bool select);
	void setAutoSelect(bool autoSelect);
protected:
	virtual StateImage getStateImage(void *param1, void *param2);
	virtual bool doStateImage(Msg *m);
	enum {
		BTN_IMG_SELECT = 5
	};
	bool mSelected;
	bool mAutoSelect;
};


class VCheckBox : public VOption {
public:
	VCheckBox(XmlNode *node);
protected:
	virtual void onPaint(Msg *m);
};


class VRadio : public VCheckBox {
public:
	VRadio(XmlNode *node);
	virtual void setSelect(bool select);
protected:
	void unselectOthers();
};


class VIconButton : public VOption {
public:
	VIconButton(XmlNode *node);
protected:
	virtual void onPaint(Msg *m);
	RECT getRectBy(int *attr);
protected:
	XImage *mIcon;
	int mAttrIconRect[4]; // left, top, width, height
	int mAttrTextRect[4]; // left, top, width, height
};


class VScrollBar : public VExtComponent {
public:
	VScrollBar(XmlNode *node, bool horizontal = false);
	int getMax();
	int getPage();
	int getStart();
	void setStart(int start);
	int getPos();
	void setPos(int pos); // pos = [0 ... max - page]
	void setMaxAndPage(int maxn, int page);
protected:
	virtual void onPaint(Msg *m);
	virtual bool onMouseEvent(Msg *m);
	XRect getThumbRect();
	int getPosBy(int start);
protected:
	bool mHorizontal;
	int mMax;
	int mPage;
	int mPos;
	XImage *mTrack, *mThumb;
	bool mPressed;
	int mMouseX, mMouseY;
};


class VTextArea : public VExtComponent, public XAreaText, public VListener {
public:
	VTextArea(XmlNode *node);
	void setReadOnly(bool r);
	void setEnableShowCaret(bool enable);
	virtual void setText( const char *txt );
	virtual void setWideText( const wchar_t *txt );

	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayoutChildren( int width, int height );
	virtual HFONT getTextFont();
protected:
	virtual bool onEvent(VComponent *evtSource, Msg *msg);
	virtual void notifyChanged();
	virtual bool dispatchMessage(Msg *msg);
	virtual void onChar( wchar_t ch );
	virtual void onLButtonDown(Msg *m);
	virtual void onLButtonUp(Msg *m);
	virtual void onMouseMove(int x, int y);
	virtual void onPaint(Msg *m);
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
	VScrollBar *mVerBar;
	int mScrollX, mScrollY;
};


class VLineEdit : public VTextArea {
public:
	VLineEdit(XmlNode *node);
	virtual void insertText( int pos, wchar_t *txt, int len );
protected:
	virtual void onChar(wchar_t ch);
};


class VScroll : public VExtComponent, public VListener {
public:
	VScroll(XmlNode *node);
	VScrollBar* getHorBar();
	VScrollBar* getVerBar();
protected:
	virtual bool onEvent(VComponent *evtSource, Msg *msg);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayoutChildren(int width, int height);
	virtual bool onMouseEvent(Msg *msg);
	virtual SIZE calcDataSize();
protected:
	VScrollBar *mHorBar, *mVerBar;
};


#if 0
class VExtTableModel {
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

class VExtTable : public VExtScroll {
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
	VExtTable(XmlNode *node);
	void setModel(VExtTableModel *model);
	void setCellRender(CellRender *render);
	virtual ~VExtTable();
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
	VExtTableModel *mModel;
	int mColsWidth[50];
	SIZE mDataSize;
	HPEN mLinePen;
	int mSelectedRow;
	HBRUSH mSelectBgBrush;
	CellRender *mCellRender;
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

class VExtList : public VExtScroll {
public:
	class ItemRender {
	public:
		virtual void onDrawItem(HDC dc, int item, int x, int y, int w, int h) = 0;
	};
	VExtList(XmlNode *node);
	void setModel(XListModel *model);
	XListModel *getModel();
	void setItemRender(ItemRender *render);
	int getItemY(int item);
	int getMouseTrackItem();

	virtual ~VExtList();
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

class VExtArrowButton : public VExtButton {
public:
	// when click, send MSG_COMMAND, wParam is true means click at arrow
	VExtArrowButton(XmlNode *node);
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

class VExtComboBox : public VExtComponent, public VListener {
public:
	class BoxRender {
	public:
		virtual void onDrawBox(HDC dc, int x, int y, int w, int h) = 0;
	};
	VExtComboBox(XmlNode *node);
	void setEnableEditor(bool enable);
	VExtList *getExtList();
	void setBoxRender(BoxRender *r);
	int getSelectItem();
	void setSelectItem(int idx);
	virtual ~VExtComboBox();

	virtual bool onEvent(VComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	virtual void createWnd();
protected:
	virtual void drawBox(HDC dc, int x, int y, int w, int h);
	void openPopup();
	StateImage getStateImage();
protected:
	VExtLineEdit *mEdit;
	VExtPopup *mPopup;
	VExtList *mList;
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

class VExtMenuModel;
class VExtMenuManager;

struct VExtMenuItem {
	VExtMenuItem(const char *name, char *text);
	char mName[40];
	char *mText;
	bool mActive;
	bool mVisible;
	bool mCheckable;
	bool mChecked;
	bool mSeparator;
	VExtMenuModel *mChild;
};

class VExtMenuModel {
public:
	VExtMenuModel();
	void add(VExtMenuItem *item);
	void insert(int pos, VExtMenuItem *item);
	int getCount();
	VExtMenuItem *get(int idx);
	VExtMenuItem *findByName(const char *name);
	virtual int getItemHeight(int pos);
	virtual int getWidth();
	virtual int getHeight();
	~VExtMenuModel();
protected:
	VExtMenuItem *mItems[50];
	int mCount;
	int mWidth;
};

class VExtMenu : public VExtComponent {
public:
	VExtMenu(XmlNode *node, VExtMenuManager *mgr);
	void setModel(VExtMenuModel *list);
	VExtMenuModel *getMenuList() {return mModel;}
	virtual void show(int screenX, int screenY);
	virtual ~VExtMenu();
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	int getItemIndexAt(int x, int y);
	virtual void calcSize();
	void drawItems(HDC dc);
	RECT getItemRect(int idx);
protected:
	VExtMenuModel *mModel;
	int mSelectItem;
	HPEN mSeparatorPen;
	HPEN mCheckedPen;
	HBRUSH mSelectBrush;
	VExtMenuManager *mManager;
};

class VExtMenuManager {
public:
	class ItemListener {
	public:
		virtual void onClickItem(VExtMenuItem *item) = 0;
	};

	VExtMenuManager(VExtMenuModel *mlist, VComponent *owner, ItemListener *listener);
	void show(int screenX, int screenY);
	virtual ~VExtMenuManager();

	void notifyItemClicked(VExtMenuItem *item);
	void closeMenu(VExtMenuModel *mlist);
	void openMenu(VExtMenuModel *mlist, int x, int y);
protected:
	void messageLoop();
	int whereIs(int x, int y);
	void closeMenuTo(int idx);
protected:
	VExtMenuModel *mMenuList;
	VExtMenu *mMenus[10];
	int mLevel;
	VComponent *mOwner;
	ItemListener *mListener;
};

class VExtTreeNode {
public:
	enum PosInfo { PI_FIRST = 1, PI_LAST = 2, PI_CENTER = 4 };
	VExtTreeNode(const char *text);
	// pos : -1 means append
	void insert(int pos, VExtTreeNode *child);
	void remove(int pos);
	void remove(VExtTreeNode *child);
	int indexOf(VExtTreeNode *child);
	int getChildCount();
	void *getUserData();
	void setUserData(void *userData);
	int getContentWidth();
	void setContentWidth(int w);
	VExtTreeNode *getChild(int idx);
	bool isExpand();
	void setExpand(bool expand);
	char *getText();
	void setText(char *text);
	PosInfo getPosInfo();
	int getLevel();
	VExtTreeNode *getParent();
	bool isCheckable();
	void setCheckable(bool cb);
	bool isChecked();
	void setChecked(bool cb);
protected:
	char *mText;
	VExtTreeNode *mParent;
	std::vector<VExtTreeNode*> *mChildren;
	DWORD mAttrFlags;
	bool mExpand;
	void *mUserData;
	int mContentWidth;
	bool mCheckable;
	bool mChecked;
};
class VExtTree : public VExtScroll {
public:
	class NodeRender {
	public:
		virtual void onDrawNode(HDC dc, VExtTreeNode *node, int x, int y, int w, int h) = 0;
	};
	enum WhenSelect {
		WHEN_CLICK, WHEN_DBCLICK, WHEN_NONE
	};
	VExtTree(XmlNode *node);
	//根结点为虚拟结点，不显示
	void setModel(VExtTreeNode *root);
	VExtTreeNode* getModel();
	WhenSelect getWhenSelect();
	void setWhenSelect(WhenSelect when);
	void notifyChanged();
	void setNodeRender(NodeRender *render);
	// @return false:表示node是被折叠的，没有大小; true:表示node是未折叠的，取得了rect
	bool getNodeRect(VExtTreeNode *node, RECT *r);
	VExtTreeNode *getAtPoint(int x, int y);
	VExtTreeNode *getSelectNode();
	void setSelectNode(VExtTreeNode *selNode);
	virtual ~VExtTree();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	SIZE calcDataSize();
	SIZE getClientSize();
	virtual void moveChildrenPos( int dx, int dy );
	void drawData(HDC dc, int w, int h);
	void drawNode(HDC dc, VExtTreeNode *n, int level, int clientWidth, int clientHeight, int *y);
	void onLBtnDown(int x, int y);
	void onLBtnDbClick(int x, int y);
	VExtTreeNode *getNodeAtY(int y, int *py);
protected:
	SIZE mDataSize;
	VExtTreeNode *mModel;
	HPEN mLinePen, mCheckPen;
	HBRUSH mSelectBgBrush;
	VExtTreeNode *mSelectNode;
	int mWidthSpec, mHeightSpec;
	NodeRender *mNodeRender;
	WhenSelect mWhenSelect;
};

class VExtCalendar : public VExtComponent {
public:
	struct Date {
		Date();
		bool isValid();
		bool equals(const Date &d);
		int mYear; int mMonth; int mDay;
	};
	VExtCalendar(XmlNode *node);
	Date getSelectDate();
	void setSelectDate(Date d);
	virtual ~VExtCalendar();
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

class VExtMaskEdit : public VExtTextArea {
public:
	typedef bool (*InputValidate)(int pos, char ch);
	enum Case { C_NONE, C_UPPER, C_LOWER };
	VExtMaskEdit(XmlNode *node);
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
class VExtPassword : public VExtTextArea {
public:
	VExtPassword(XmlNode *node);
protected:
	virtual void onChar( wchar_t ch );
	virtual void paste();
	virtual void onPaint(HDC hdc);
};

class VExtDatePicker : public VExtComponent, public VListener {
public:
	VExtDatePicker(XmlNode *node);
	char *getText();
	virtual ~VExtDatePicker();
protected:
	virtual bool onEvent(VComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	virtual void createWnd();
	void openPopup();
protected:
	VExtMaskEdit *mEdit;
	VExtPopup *mPopup;
	VExtCalendar *mCalendar;
	XmlNode *mEditNode, *mPopupNode, *mCalendarNode;
	RECT mArrowRect;
	SIZE mAttrPopupSize;
	SIZE mAttrArrowSize;
	XImage *mArrowNormalImage, *mArrowDownImage;
	bool mPoupShow;
};

#endif


class VHLineLayout : public VExtComponent {
public:
	VHLineLayout(XmlNode *node);
	virtual void onLayoutChildren(int width, int height);
};

class VVLineLayout : public VExtComponent {
public:
	VVLineLayout(XmlNode *node);
	virtual void onLayoutChildren(int width, int height);
};