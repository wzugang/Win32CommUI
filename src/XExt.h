#pragma once
#include "XComponent.h"

class XExtComponent : public XComponent {
public:
	XExtComponent(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	void layout(int x, int y, int width, int height);
	virtual ~XExtComponent();
protected:
	void eraseBackground(HDC dc);
	XImage *mBgImageForParnet;
};

class XExtLabel : public XExtComponent {
public:
	XExtLabel(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	char *getText();
	void setText(char *text);
protected:
	char *mText;
};

class XExtButton : public XExtComponent {
public:
	XExtButton(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
protected:
	enum BtnImage {
		BTN_IMG_NORMAL,
		BTN_IMG_HOVER,
		BTN_IMG_PUSH,
		BTN_IMG_DISABLE
	};
	virtual BtnImage getBtnImage();
	XImage *mImages[8];
	bool mIsMouseDown;
	bool mIsMouseMoving;
	bool mIsMouseLeave;
};

class XExtOption : public XExtButton {
public:
	XExtOption(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	bool isSelect();
	void setSelect(bool select);
protected:
	virtual BtnImage getBtnImage();
	enum {
		BTN_IMG_SELECT = 5
	};
	bool mIsSelect;
};

class XExtCheckBox : public XExtOption {
public:
	XExtCheckBox(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
};

class XExtRadio : public XExtCheckBox {
public:
	XExtRadio(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
protected:
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
	virtual void createWnd();
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void onMeasure(int widthSpec, int heightSpec);
	virtual void onLayout(int width, int height);
	virtual ~XExtScroll();
protected:
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
	enum Attr {
		ATTR_HAS_HEADER = 1,
		ATTR_HAS_COL_LINE = 2,
		ATTR_HAS_ROW_LINE = 4
	};
	XExtTable(XmlNode *node);
	void setModel(XExtTableModel *model);
	virtual ~XExtTable();
protected:
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual void drawHeader(HDC dc, int w, int h);
	void drawData( HDC dc, int x, int y, int w, int h );
	virtual void drawRow(HDC dc, int row, int x, int y, int w, int h );
	virtual void drawCell(HDC dc, int row, int col, int x, int y, int w, int h );
	virtual void drawGridLine(HDC dc, int from, int to, int y);

	void onMeasure( int widthSpec, int heightSpec );
	void onLayout( int width, int height );
	void mesureColumn(int width, int height);
	virtual SIZE calcDataSize();
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
};

class XExtEdit : public XComponent {
public:
	XExtEdit(XmlNode *node);
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
};

class XExtComboBox : public XComponent {
public:
	XExtComboBox(XmlNode *node);
};