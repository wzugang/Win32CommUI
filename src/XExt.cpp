#include "XExt.h"
#include <stdio.h>
#include <stdlib.h>
#include "XComponent.h"

void XTableModel::apply(HWND tableWnd) {
	if (tableWnd == NULL)
		return;
	LVCOLUMN c = {0};

	for (int i = 0; i < getColumnCount(); ++i) {
		getColumn(i, &c);
		SendMessage(tableWnd, LVM_INSERTCOLUMN, 0, (LPARAM)&c);
	}

	LVITEM item = {0};
	for (int i = 0; i < getRowCount(); ++i) {
		for (int j = 0; j < getColumnCount(); ++j) {
			getItem(i, j, &item);
			if (j == 0)
				ListView_InsertItem(tableWnd, &item);
			else 
				ListView_SetItem(tableWnd, &item);
		}
	}
}

void XTableModel::getColumn( int col, LVCOLUMN *lv ) {
	memset(lv, 0, sizeof(LVCOLUMN));
	lv->mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lv->fmt = LVCFMT_LEFT;
	lv->cx = getColumnWidth(col);
	lv->pszText = getColumnTitle(col);
}

void XTableModel::getItem( int row, int col, LVITEM *item ) {
	memset(item, 0, sizeof(LVITEM));
	item->mask = LVIF_TEXT;
	item->iItem = row;
	item->iSubItem = col;
	item->pszText = NULL;
}

XImage::XImage( HBITMAP bmp ) {
	mBits = NULL;
	mHBitmap = bmp;
	mWidth = mHeight = 0;
	if (bmp) {
		BITMAP b = {0};
		GetObject(bmp, sizeof(BITMAP), &b);
		mWidth = b.bmWidth;
		mHeight = b.bmHeight;
		mBits = b.bmBits;
	}
}

XImage * XImage::loadFromFile( const char *path ) {
	HBITMAP bmp = (HBITMAP)LoadImage(XComponent::getInstance(), path, IMAGE_BITMAP, 0, 0,
		/*LR_CREATEDIBSECTION | LR_DEFAULTSIZE | */ LR_LOADFROMFILE);
	if (bmp) return new XImage(bmp);
	return NULL;
}

XImage * XImage::loadFromResource( int resId ) {
	HBITMAP bmp = (HBITMAP)LoadImage(XComponent::getInstance(), MAKEINTRESOURCE(resId), IMAGE_BITMAP, 0, 0,
		/*LR_CREATEDIBSECTION | LR_DEFAULTSIZE*/ 0);
	if (bmp) return new XImage(bmp);
	return NULL;
}

XImage * XImage::loadFromResource( const char * resName ) {
	HBITMAP bmp = (HBITMAP)LoadImage(XComponent::getInstance(), resName, IMAGE_BITMAP, 0, 0,
		/*LR_CREATEDIBSECTION  LR_DEFAULTSIZE*/ 0);
	if (bmp) return new XImage(bmp);
	return NULL;
}
