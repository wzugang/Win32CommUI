#pragma once
#include "XComponent.h"
#include <CommCtrl.h>
class XTable;

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

class XExtLabel : public XComponent {
public:
	XExtLabel(XmlNode *node);
	virtual void createWnd();
	virtual void layout(int x, int y, int width, int height);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	char *getText();
	void setText(char *text);
protected:
	XImage *mBgImageForParnet;
	char *mText;
};

class XExtButton : public XComponent {
public:
	XExtButton(XmlNode *node);
	virtual void createWnd();
	virtual void layout(int x, int y, int width, int height);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual ~XExtButton();
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
protected:
	XImage *mBgImageForParnet;
};

class XExtRadio : public XExtCheckBox {
public:
	XExtRadio(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
};

class XScrollBar {
public:
	XScrollBar(XComponent *owner, bool horizontal);
	virtual void onPaint(HDC hdc);
	virtual void onEvent(UINT msg, WPARAM wParam, LPARAM lParam);
	RECT getRect();
	void setRange(int range);
	void setVisible(bool visible);
protected:
	XComponent *mOwner;
	bool mHorizontal;
	int mX, mY, mWidth, mHeight;
	int mRange;
	bool mVisible;
};

class XExtScroll : public XScroll {
public:
	XExtScroll(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	virtual ~XExtScroll();
protected:
	XScrollBar *mHorBar, *mVerBar;
};