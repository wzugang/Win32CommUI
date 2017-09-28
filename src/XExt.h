#pragma once
#include "XComponent.h"
#include <CommCtrl.h>

class XExtTable {
public:
	XExtTable(XTable *table);
	void apply();
	~XExtTable();
protected:
	virtual int getColumnCount() = 0;
	virtual int getRowCount() = 0;
	virtual int getColumnWidth(int col) = 0;
	virtual char *getColumnTitle(int col) = 0;

	virtual void getColumn(int col, LVCOLUMN *lvc);
	virtual void getItem(int row, int col, LVITEM *item);
protected:
	XTable *mTable;
};

