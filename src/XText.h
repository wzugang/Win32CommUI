#pragma once
#include <windows.h>

class XAreaText {
public:
	struct LineInfo {
		int mBeginPos; int mLen; int mStartX;
		int mStartY; // 相对于前一行的Y轴距离, 除了第一行，其它都是0
		int mTextWidth;
	};

	XAreaText();

	int getLineNum();
	int getLineHeight();
	LineInfo *getLineInfo(int line);

	virtual char *getText();
	virtual wchar_t *getWideText();
	virtual void setText(const char *txt);
	virtual void setWideText(const wchar_t *txt);
	virtual void insertText(int pos, wchar_t *txt, int len);
	virtual void insertText(int pos, char *txt);
	virtual int deleteText(int pos, int len);
	virtual int getWideTextLength();

	virtual void setTextFont(HFONT font);
	virtual HFONT getTextFont() = 0;
	bool isAutoNewLine();
	void setAutoNewLine(bool autoNewLine);

	virtual ~XAreaText();
protected:
	virtual SIZE getClientSize() = 0;
	virtual HWND getBindWnd() = 0;

	virtual int getPosAt(int x, int y);
	virtual bool getPointAt(int pos, POINT *pt);
	virtual void buildLines();
	virtual void buildLinesWrap();
	virtual void buildLinesNoWrap();
	void tryIncLineSpace();
	void calcFontInfo();
	void addLine(HDC dc, wchar_t *beginPos, int len);
	// get line no, if not find, return -1
	int getLineNoByY(int y);
	int getLineNoByPos(int pos);
	int getYAtLine(int lineNo);
protected:
	wchar_t *mWideText;
	int mWideTextCapacity;
	int mWideTextLen;
	
	int mTextWidth, mTextHeight; // text really size A 
	int mLineHeight, mCharWidth;

	bool mAutoNewLine;
	LineInfo *mLines;
	int mLinesCapacity;
	int mLinesNum;

	char *mGbkText;
	int mGbkTextLen;
	bool mNeedRebuildLines;
};


