#pragma once
#include <windows.h>
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

class XImage {
public:
	XImage(HBITMAP bmp);
	static XImage *loadFromFile(const char *path);
	static XImage *loadFromResource(int resId);
	static XImage *loadFromResource(const char * resName);
	static XImage *create(int width, int height);
	~XImage();
protected:
	void *mBits;
	HBITMAP mHBitmap;
	int mWidth;
	int mHeight;
};

