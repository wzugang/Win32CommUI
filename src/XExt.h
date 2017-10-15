#pragma once
#include "XComponent.h"

class XExtComponent : public XComponent {
public:
	XExtComponent(XmlNode *node);
	virtual void layout(int x, int y, int width, int height);
	virtual ~XExtComponent();
protected:
	void eraseBackground(HDC dc);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	XImage *mBgImageForParnet;
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
	enum BtnImage {
		BTN_IMG_NORMAL,
		BTN_IMG_HOVER,
		BTN_IMG_PUSH,
		BTN_IMG_DISABLE
	};
	virtual BtnImage getBtnImage();
protected:
	XImage *mImages[8];
	bool mIsMouseDown;
	bool mIsMouseMoving;
	bool mIsMouseLeave;
};

class XExtOption : public XExtButton {
public:
	XExtOption(XmlNode *node);
	bool isSelect();
	void setSelect(bool select);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual BtnImage getBtnImage();
	enum {
		BTN_IMG_SELECT = 5
	};
	bool mIsSelect;
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

class XScrollBar : public XExtComponent {
public:
	XScrollBar(XmlNode *node, bool horizontal);
	int getMax();
	int getPage();
	void setMaxAndPage(int maxn, int page);
	int getPos();
	void setPos(int pos); // pos = [0 ... max - page]
	void setImages(XImage *track, XImage *thumb);
	bool isNeedShow();
	int getThumbSize();
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
	XImage *mBuffer;
};

class XExtScroll : public XExtComponent {
public:
	XExtScroll(XmlNode *node);
	virtual ~XExtScroll();
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	virtual SIZE calcDataSize();
	virtual void moveChildrenPos( int x, int y );
	void invalide(XComponent *c);
	XScrollBar *mHorBar, *mVerBar;
	XmlNode *mHorNode, *mVerNode;
};

class XExtPopup : public XComponent {
public:
	XExtPopup(XmlNode *node);
	virtual void show(int x, int y);
	virtual void close();
	virtual ~XExtPopup();
protected:
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onLayout(int width, int height);
	virtual int messageLoop();
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
		virtual void onDraw(HDC dc, int row, int col, int x, int y, int w, int h) = 0;
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
	XImage *mBuffer;
	SIZE mDataSize;
	HPEN mLinePen;
	int mSelectedRow;
	COLORREF mSelectBgColor;
	HBRUSH mSelectBgBrush;
	CellRender *mCellRender;
};

class XExtEdit : public XComponent {
public:
	XExtEdit(XmlNode *node);
	void setEnableBorder(bool enable);
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	void onChar(wchar_t ch);
	void onLButtonDown(int keyState, int x, int y);
	void onLButtonUp(int keyState, int x, int y);
	void onMouseMove(int x, int y);
	void onPaint(HDC hdc);
	void drawSelRange(HDC hdc, int begin, int end);
	void onKeyDown(int key);
	void insertText(int pos, char *txt);
	void insertText(int pos, wchar_t *txt, int len);
	int deleteText(int pos, int len);
	int getPosAt(int x, int y);
	BOOL getXYAt(int pos, POINT *pt);
	void move(int key);
	void ensureVisible(int pos);

	void back();
	void del();
	void copy();
	void paste();
protected:
	wchar_t *mText;
	int mCapacity;
	int mLen;
	int mInsertPos;
	int mBeginSelPos, mEndSelPos;
	bool mReadOnly;
	HPEN mCaretPen;
	bool mCaretShowing;
	int mScrollPos;
	HPEN mBorderPen, mFocusBorderPen;
	bool mEnableBorder;
};

class XListModel {
public:
	struct ItemData {
		char *mText;
		bool mSelectable;
		bool mSelected;
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
		virtual void onDraw(HDC dc, int item, int x, int y, int w, int h) = 0;
	};
	XExtList(XmlNode *node);
	void setModel(XListModel *model);
	void setItemRender(ItemRender *render);
	virtual ~XExtList();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure( int widthSpec, int heightSpec );
	virtual void onLayout( int width, int height );
	virtual SIZE calcDataSize();
	SIZE getClientSize();
	virtual void moveChildrenPos( int dx, int dy );
	virtual void drawData( HDC memDc, int x, int y,  int w, int h );
	virtual void drawItem(HDC dc, int item, int x, int y, int w, int h);
	void getVisibleRows(int *from, int *to);
	int findItem(int x, int y);
	void updateTrackItem(int x, int y);
protected:
	XListModel *mModel;
	ItemRender *mItemRender;
	XImage *mBuffer;
	SIZE mDataSize;
	int mMouseTrackItem;
	HBRUSH mSelectBgBrush;
};


class XExtComboBox : public XComponent {
public:
	XExtComboBox(XmlNode *node);
protected:
	XExtEdit *mEdit;
};