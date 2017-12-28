#pragma once

class XBaseText {
public:
	enum HorAlign {ALIGN_LEFT, ALIGN_HCENTER, ALIGN_RIGHT};
	enum VerAlign {ALIGN_TOP, ALIGN_VCENTER, ALIGN_BOTTOM};
	XText();
	virtual wchar_t *getWideText();
	virtual void setWideText(const wchar_t *txt);
	virtual void insertText(int pos, wchar_t *txt, int len);
	virtual int deleteText(int pos, int len);
	virtual ~XText();
protected:
	wchar_t *mText;
	int mTextCapacity;
	int mTextLen;
	
	HorAlign mHorAlign;
	VerAlign mVerAlign;
	int mWidth, mHeight; // text show size
	int mTextWidth, mTextHeight; // text really size A 
	int mLineHeight;
};

class XAreaText : public XBaseText {
protected:	
	struct LineInfo { int mBeginPos; int mLen;};
	virtual void buildLines();
	virtual int getPosAt(int x, int y);
	virtual BOOL getPointAt(int pos, POINT *pt);
	virtual void ensureVisible(int pos);
	virtual void getVisibleRows(int *from, int *to);
protected:
	bool mAutoNewLine;
	LineInfo *mLines;
	int mLinesCapacity;
	int mLineNum;
};

class XLineText : public XAreaText {
	
};


