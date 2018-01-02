#pragma once
#include <windows.h>

class XAreaText {
public:
	enum HorAlign {ALIGN_LEFT, ALIGN_HCENTER, ALIGN_RIGHT};
	enum VerAlign {ALIGN_TOP, ALIGN_VCENTER, ALIGN_BOTTOM};

	XAreaText();
	virtual char *getText();
	virtual wchar_t *getWideText();
	virtual void setText(const char *txt);
	virtual void setWideText(const wchar_t *txt);
	virtual void insertText(int pos, wchar_t *txt, int len);
	virtual void insertText(int pos, char *txt);
	virtual int deleteText(int pos, int len);

	virtual void setTextFont(HFONT font);
	virtual HFONT getTextFont() = 0;
	HorAlign getHorAlign();
	VerAlign getVerAlign();
	void setHorAlign(HorAlign align);
	void setVerAlign(VerAlign align);
	bool isAutoNewLine();
	void setAutoNewLine(bool autoNewLine);

	virtual ~XAreaText();
protected:
	struct LineInfo {
		int mBeginPos;
		int mLen;
		int mStartX;
		int mStartY; // 相对于前一行的Y轴距离, 除了第一行，其它都是0
		int mTextWidth;
	};
	virtual SIZE getClientSize() = 0;
	virtual HWND getBindWnd() = 0;

	virtual int getPosAt(int x, int y);
	virtual bool getPointAt(int pos, POINT *pt);
	virtual void buildLines();
	virtual void buildLinesWrap();
	virtual void buildLinesNoWrap();
	void tryIncLineSpace();
	void applyAlign();
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
	
	HorAlign mHorAlign;
	VerAlign mVerAlign;
	int mTextWidth, mTextHeight; // text really size A 
	int mLineHeight, mCharWidth;

	bool mAutoNewLine;
	LineInfo *mLines;
	int mLinesCapacity;
	int mLinesNum;

	char *mGbkText;
	int mGbkTextLen;
	bool mNeedRebuildLines;
	bool mAlignChanged;
};


