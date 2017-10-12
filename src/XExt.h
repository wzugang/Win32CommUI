#pragma once
#include "XComponent.h"

class XExtComponent : public XComponent {
public:
	XExtComponent(XmlNode *node);
	virtual bool wndProc(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *result);
	void layout(int x, int y, int width, int height);
	virtual ~XExtComponent();
protected:
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
	void moveChildrenPos( int dx, int dy );
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