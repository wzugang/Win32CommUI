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
	virtual bool onMouseEvent(Msg *m);
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
	VScrollBar(XmlNode *node);
	void setOrientation(bool hor);
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
	int getScrollRange();
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
	virtual bool onMouseEvent(Msg *m);
	virtual bool onKeyEvent(Msg *m);

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


class VMaskEdit : public VLineEdit {
public:
	typedef bool (*InputValidate)(int pos, wchar_t ch);
	enum Case { C_NONE, C_UPPER, C_LOWER };

	VMaskEdit(XmlNode *node);
	/*
	*  @param mask  0: 0-9;  9:1-9;    A:A-Z;    a:a-z;
					C: 0-9 a-z A-Z;    H:0-9 A-F a-f   B:0-1
	**/
	void setMask(const char *mask);
	void setCase(Case c);
	void setPlaceHolder(char ch);
	void setInputValidate(InputValidate iv);
protected:
	virtual bool onMouseEvent(Msg *m);
	virtual void onChar( wchar_t ch );
	virtual void onPaint(Msg *m);
	virtual void onKeyDown(int key);
	virtual int getPosAt(int x, int y);
	virtual bool acceptChar(wchar_t ch, int pos);
	bool isMaskChar(char ch);
protected:
	char *mMask;
	Case mCase;
	HBRUSH mCaretBrush;
	wchar_t mPlaceHolder;
	InputValidate mValidate;
};


class VPassword : public VLineEdit {
public:
	VPassword(XmlNode *node);
protected:
	virtual void onChar( wchar_t ch );
	virtual void paste();
	virtual void onPaint(Msg *m);
};


class VCalendar : public VExtComponent {
public:
	struct Date {
		Date();
		bool isValid();
		bool equals(const Date &d);
		int mYear; int mMonth; int mDay;
	};
	VCalendar(XmlNode *node);
	Date getSelectDate();
	void setSelectDate(Date d);
	virtual ~VCalendar();
protected:
	virtual void onPaint(Msg *m);
	virtual bool onMouseEvent(Msg *m);
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

class VScroll : public VExtComponent {
public:
	VScroll(XmlNode *node);
	VScrollBar* getHorBar();
	VScrollBar* getVerBar();
protected:
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayoutChildren(int width, int height);
	virtual void onMouseWheel(Msg *msg);
	virtual bool onMouseEvent(Msg *msg);
	virtual bool dispatchMouseMessage(Msg *m);
	virtual bool dispatchPaintMessage(Msg *m);
	virtual SIZE calcDataSize();
	virtual SIZE getClientSize();
protected:
	VScrollBar *mHorBar, *mVerBar;
	SIZE mDataSize;
};

class VTableModel {
public:
	struct HeaderData {
		char *mText;
		XImage *mBgImage;
	};

	struct CellData {
		char *mText;
	};

	virtual int getColumnCount() = 0;
	virtual int getRowCount() = 0;
	virtual int getColumnWidth(int col, int wholeWidth) = 0;
	virtual int getRowHeight(int row) = 0;
	virtual int getHeaderHeight() = 0;
	virtual HeaderData *getHeaderData(int col) = 0;
	virtual CellData *getCellData(int row, int col) = 0;
	virtual XImage *getHeaderBgImage() = 0;
};

class VTable : public VScroll {
public:
	class Render {
	public:
		// return false: not define draw, use default draw
		// return true: define draw
		virtual bool onDrawCell(HDC dc, int row, int col, int x, int y, int w, int h) = 0;
		virtual bool onDrawColumn(HDC dc, int col, int x, int y, int w, int h) = 0;
	};

	VTable(XmlNode *node);
	void setModel(VTableModel *model);
	void setRender(Render *render);
	virtual ~VTable();
protected:
	virtual void onPaint(Msg *m);
	virtual bool onMouseEvent(Msg *msg);

	virtual void drawHeader(HDC dc, int w, int h);
	virtual void drawColumn(HDC dc, int col, int x, int y, int w, int h);
	void drawData( HDC dc, int x, int y, int w, int h );
	virtual void drawRow(HDC dc, int row, int x, int y, int w, int h );
	virtual void drawCell(HDC dc, int row, int col, int x, int y, int w, int h );
	virtual void drawGridLine(HDC dc, int from, int to, int y);

	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayoutChildren( int width, int height );
	void mesureColumn(int width, int height);
	virtual SIZE calcDataSize();
	virtual SIZE getClientSize();
	void getVisibleRows(int *from, int *to);
	int findCell(int x, int y, int *col);
protected:
	VTableModel *mModel;
	HPEN mHorLinePen, mVerLinePen;
	int mSelectedRow;
	XImage *mSelectBgImage;
	Render *mRender;
};


class VListModel {
public:
	struct ItemData {
		char *mText;
		bool mSelectable;
		void *mUsrData;
	};

	virtual int getItemCount() = 0;
	virtual int getItemHeight(int item) = 0;
	virtual ItemData *getItemData(int item) = 0;
};


class VList : public VScroll {
public:
	class ItemRender {
	public:
		virtual void onDrawItem(HDC dc, int item, int x, int y, int w, int h) = 0;
	};

	VList(XmlNode *node);

	void setModel(VListModel *model);
	VListModel *getModel();
	void setItemRender(ItemRender *render);
	int getItemY(int item);
	int getTrackItem();
	void setEnableTrack(bool enable);
	void setSelectItem(int item);
	int getSelectItem();
	void notifyModelChanged();
	virtual ~VList();
protected:
	virtual SIZE calcDataSize();
	virtual SIZE getClientSize();
	virtual void drawData( HDC dc, int x, int y,  int w, int h );
	virtual void drawItem(HDC dc, int item, int x, int y, int w, int h);
	void getVisibleRows(int *from, int *num);
	int findItem(int x, int y);
	void updateTrackItem(int x, int y);
	virtual void onPaint(Msg *m);
	virtual bool onMouseEvent(Msg *msg);
protected:
	VListModel *mModel;
	ItemRender *mItemRender;
	int mTrackItem, mSelectItem;
	XImage *mSelBgImage, *mTrackBgImage;
	bool mEnableTrack;
};



class VTreeNode {
public:
	enum PosInfo { PI_FIRST = 1, PI_LAST = 2, PI_CENTER = 4 };
	VTreeNode(const char *text);
	// pos : -1 means append
	void insert(int pos, VTreeNode *child);
	void remove(int pos);
	void remove(VTreeNode *child);
	int indexOf(VTreeNode *child);
	int getChildCount();
	void *getUserData();
	void setUserData(void *userData);
	int getContentWidth();
	void setContentWidth(int w);
	VTreeNode *getChild(int idx);
	bool isExpand();
	void setExpand(bool expand);
	char *getText();
	void setText(char *text);
	PosInfo getPosInfo();
	int getLevel();
	VTreeNode *getParent();
	bool isCheckable();
	void setCheckable(bool cb);
	bool isChecked();
	void setChecked(bool cb);
protected:
	char *mText;
	VTreeNode *mParent;
	std::vector<VTreeNode*> *mChildren;
	DWORD mAttrFlags;
	bool mExpand;
	void *mUserData;
	int mContentWidth;
	bool mCheckable;
	bool mChecked;
};

class VTree : public VScroll {
public:
	class NodeRender {
	public:
		virtual void onDrawNode(HDC dc, VTreeNode *node, int x, int y, int w, int h) = 0;
	};
	enum WhenSelect {
		WHEN_CLICK, WHEN_DBCLICK, WHEN_NONE
	};
	VTree(XmlNode *node);
	//根结点为虚拟结点，不显示
	void setModel(VTreeNode *root);
	VTreeNode* getModel();
	WhenSelect getWhenSelect();
	void setWhenSelect(WhenSelect when);
	void notifyChanged();
	void setNodeRender(NodeRender *render);
	// @return false:表示node是被折叠的，没有大小; true:表示node是未折叠的，取得了rect
	bool getNodeRect(VTreeNode *node, RECT *r);
	VTreeNode *getAtPoint(int x, int y);
	VTreeNode *getSelectNode();
	void setSelectNode(VTreeNode *selNode);
	virtual ~VTree();
protected:
	virtual void onPaint(Msg *m);
	virtual bool onMouseEvent(Msg *msg);
	virtual SIZE calcDataSize();
	void drawData(HDC dc, int w, int h);
	void drawNode(HDC dc, VTreeNode *n, int level, int clientWidth, int clientHeight, int *y);
	void onLBtnDown(int x, int y);
	void onLBtnDbClick(int x, int y);
	VTreeNode *getNodeAtY(int y, int *py);
protected:
	VTreeNode *mModel;
	HPEN mLinePen, mCheckPen;
	HBRUSH mSelectBgBrush;
	VTreeNode *mSelectNode;
	NodeRender *mNodeRender;
	WhenSelect mWhenSelect;
};


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