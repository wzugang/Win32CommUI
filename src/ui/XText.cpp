#include "XText.h"
#include <stdlib.h>

XAreaText::XAreaText() : mWideText(NULL), mWideTextCapacity(0), mWideTextLen(0),
	mTextWidth(0), mTextHeight(0), mLineHeight(0), mCharWidth(0),
	mAutoNewLine(false), mLines(NULL), mLinesCapacity(0), mLinesNum(0),
	mGbkText(NULL), mGbkTextLen(0), mNeedRebuildLines(true) {
}
int XAreaText::getLineNum() {
	return mLinesNum;
}
int XAreaText::getLineHeight() {
	return mLineHeight;
}
XAreaText::LineInfo *XAreaText::getLineInfo(int line) {
	if (line >= 0 && line < mLinesNum) {
		return &mLines[line];
	}
	return NULL;
}
char * XAreaText::getText() {
	if (mWideText == NULL) {
		return NULL;
	}
	int len = WideCharToMultiByte(CP_ACP, 0, mWideText, mWideTextLen, NULL, 0, NULL, NULL);
	if (len + 1 > mGbkTextLen) {
		mGbkTextLen = len;
		mGbkText = (char *)realloc(mGbkText, len + 1);
	}
	WideCharToMultiByte(CP_ACP, 0, mWideText, -1, mGbkText, len, NULL, NULL);
	mGbkText[len] = 0;
	return mGbkText;
}
wchar_t * XAreaText::getWideText() {
	return mWideText;
}
void XAreaText::setText( const char *txt ) {
	deleteText(0, mWideTextLen);
	insertText(0, (char *)txt);
	mNeedRebuildLines = true;
}
void XAreaText::setWideText( const wchar_t *txt ) {
	deleteText(0, mWideTextLen);
	if (txt != NULL) {
		insertText(0, (wchar_t*)txt, wcslen(txt));
	}
	mNeedRebuildLines = true;
}
void XAreaText::insertText( int pos, wchar_t *txt, int len ) {
	if (pos < 0 || pos > mWideTextLen) {
		pos = mWideTextLen;
	}
	if (len <= 0) {
		return;
	}
	if (mWideTextLen + len >= mWideTextCapacity - 10) {
		mWideTextCapacity = max(mWideTextLen + len + 50, mWideTextCapacity * 2);
		mWideTextCapacity = (mWideTextCapacity & (~63)) + 64;
		mWideText = (wchar_t *)realloc(mWideText, sizeof(wchar_t) * mWideTextCapacity);
	}
	for (int i = mWideTextLen - 1; i >= pos; --i) {
		mWideText[i + len] = mWideText[i];
	}
	memcpy(&mWideText[pos], txt, len * sizeof(wchar_t));
	mWideTextLen += len;
	if (mWideTextLen > 0) {
		mWideText[mWideTextLen] = 0;
	}
	mNeedRebuildLines = true;
}

void XAreaText::insertText( int pos, char *txt ) {
	if (txt == NULL) return;
	int slen = strlen(txt);
	int len = MultiByteToWideChar(CP_ACP, 0, txt, slen, NULL, 0);
	wchar_t *wb = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, txt, slen, wb, len);
	insertText(pos, wb, len);
	delete[] wb;
	mNeedRebuildLines = true;
}

int XAreaText::deleteText( int pos, int len ) {
	if (pos < 0 || pos >= mWideTextLen || len <= 0) 
		return 0;
	if (len + pos > mWideTextLen) len = mWideTextLen - pos;
	for (; pos < mWideTextLen; ++pos) {
		mWideText[pos] = mWideText[pos + len];
	}
	mWideTextLen -= len;
	if (len > 0) {
		mNeedRebuildLines = true;
	}
	return len;
}
void XAreaText::setTextFont( HFONT font ) {
	if (font != getTextFont()) {
		mNeedRebuildLines = true;
	}
}
void XAreaText::calcFontInfo() {
	mLineHeight = 0;
	mCharWidth = 0;
	SIZE sz;
	const wchar_t *txt = L"¹ú";
	HDC dc = GetDC(getBindWnd());
	HGDIOBJ old = SelectObject(dc, getTextFont());
	GetTextExtentPoint32W(dc, txt, 1, &sz);
	SelectObject(dc, old);
	ReleaseDC(getBindWnd(), dc);
	mLineHeight = sz.cy + 4; // 4 is line height padding
	mCharWidth = sz.cx;
}
bool XAreaText::isAutoNewLine() {
	return mAutoNewLine;
}
void XAreaText::setAutoNewLine(bool a) {
	if (mAutoNewLine != a) {
		mNeedRebuildLines = true;
	}
	mAutoNewLine = a;
}
void XAreaText::buildLines() {
	if (! mNeedRebuildLines || getBindWnd() == NULL) {
		return;
	}
	mNeedRebuildLines = false;
	mTextWidth = 0;
	mTextHeight = 0;
	calcFontInfo();
	if (mAutoNewLine) {
		buildLinesWrap();
	} else {
		buildLinesNoWrap();
	}
	for (int i = 0; i < mLinesNum; ++i) {
		mTextWidth = max(mTextWidth, mLines[i].mTextWidth);
	}
	mTextHeight = mLinesNum * mLineHeight;
}
void XAreaText::buildLinesWrap() {
	wchar_t *p = mWideText;
	const int DEF_WORD_NUM = 20;
	SIZE sz;
	int pos = 0;
	int rowWords = DEF_WORD_NUM;
	SIZE clientSize = getClientSize();
	int w = clientSize.cx;
	int less = mWideTextLen;
	mLinesNum = 0;

	if (w <= 2) return;
	HDC hdc = GetDC(getBindWnd());
	HGDIOBJ old = SelectObject(hdc, getTextFont());
	while (less > 0) {
		rowWords = min(less, rowWords);
		GetTextExtentPoint32W(hdc, p, rowWords, &sz);
		if (sz.cx < w) {
			while (sz.cx < w && rowWords < less) {
				++rowWords;
				GetTextExtentPoint32W(hdc, p, rowWords, &sz);
			}
			if (sz.cx > w) --rowWords;
		} else if (sz.cx > w) {
			while (sz.cx > w && rowWords > 1) {
				--rowWords;
				GetTextExtentPoint32W(hdc, p, rowWords, &sz);
			}
		}
		if (rowWords == 0) break;
		int curRowWords = rowWords;
		for (int i = 0; i < rowWords; ++i) {
			if (p[i] != '\n') continue;
			bool hasR = (i > 0 && p[i - 1] == '\r');
			int pss = hasR ? 1 : 0;
			//line head is \n or \r\n
			if ((i == 0 || (hasR && i == 1)) && mLinesNum != 0) {
				continue;
			}
			curRowWords = i;
			break;
		}
		addLine(hdc, p, curRowWords);
		p += curRowWords;
		less -= curRowWords;
	}
	SelectObject(hdc, old);
	ReleaseDC(getBindWnd(), hdc);
}
void XAreaText::buildLinesNoWrap() {
	wchar_t *ps = mWideText, *p = mWideText;
	SIZE clientSize = getClientSize();
	int w = clientSize.cx;
	SIZE sz;
	mLinesNum = 0;
	HDC hdc = GetDC(getBindWnd());
	SelectObject(hdc, getTextFont());
	for (int i = 0; i < mWideTextLen; i++, ++p) {
		if (*p != '\n') continue;
		bool hasR = i > 0 && p[-1] == '\r';
		if (hasR) --p;
		// line head is \n or \r\n
		if (ps == p && ps != mWideText) {
			continue;
		}
		addLine(hdc, ps, p - ps);
		ps = p;
	}
	if (p > ps) {
		addLine(hdc, ps, p - ps);
	}
	ReleaseDC(getBindWnd(), hdc);
}
int XAreaText::getPosAt( int x, int y ) {
	if (mLinesNum <= 0 || mLineHeight <= 0) {
		return 0;
	}
	int ln = getLineNoByY(y);
	if (ln < 0) {
		return 0;
	}
	if (ln >= mLinesNum) {
		return mWideTextLen;
	}
	HDC hdc = GetDC(getBindWnd());
	SelectObject(hdc, getTextFont());
	//int i = 0, j = mLines[ln].mBeginPos;
	int len = mLines[ln].mLen;
	int bg = mLines[ln].mBeginPos;
	int j = 0, k = len;
	if (len > 0 && mWideText[bg] == '\n') {
		j = 1;
	}
	if (len > 1 && mWideText[bg] == '\r' && mWideText[bg + 1] == '\n') {
		j = 2;
	}
	for (int i = j; i < len; ++i) {
		SIZE sz;
		GetTextExtentPoint32W(hdc, &mWideText[bg], i + 1, &sz);
		if (sz.cx >= x) {
			SIZE nsz;
			GetTextExtentPoint32W(hdc, &mWideText[bg + i], 1, &nsz);
			int nx = x - (sz.cx - nsz.cx);
			if (nx > nsz.cx / 2) ++i;
			k = i;
			break;
		}
	}
	ReleaseDC(getBindWnd(), hdc);
	return bg + k;
}
bool XAreaText::getPointAt( int pos, POINT *pt ) {
	if (pos < 0 || pos > mWideTextLen || pt == NULL) 
		return false;
	pt->x = pt->y = 0;
	int line = getLineNoByPos(pos);
	if (line < 0) {
		return false;
	}
	if (mLinesNum == 0) {
		return true;
	}
	int bg = mLines[line].mBeginPos;
	bool isNewLine = (mWideText[pos] == '\n') || 
		(mWideText[pos] == '\r' && mWideText[pos + 1] == '\n');
	HDC hdc = GetDC(getBindWnd());
	HGDIOBJ old = SelectObject(hdc, getTextFont());
	
	if (bg == pos && isNewLine) {
		if (line > 0) {
			pt->y = getYAtLine(line - 1);
			SIZE sz = {0};
			int bg2 = mLines[line - 1].mBeginPos;
			int len2 = mLines[line - 1].mLen;
			GetTextExtentPoint32W(hdc, &mWideText[bg2], len2, &sz);
			pt->x = sz.cx + mLines[line - 1].mStartX;
		}
	} else {
		pt->y = getYAtLine(line);
		SIZE sz;
		GetTextExtentPoint32W(hdc, &mWideText[bg], pos - bg, &sz);
		pt->x = sz.cx + mLines[line].mStartX;
	}
	
	SelectObject(hdc, old);
	ReleaseDC(getBindWnd(), hdc);
	return true;
}

void XAreaText::tryIncLineSpace() {
	if (mLinesNum >= mLinesCapacity) {
		mLinesCapacity *= 2;
		if (mLinesCapacity == 0) mLinesCapacity = 10;
		mLines = (LineInfo *)realloc(mLines, sizeof(LineInfo) * mLinesCapacity);
	}
}
XAreaText::~XAreaText() {
	if (mLines) free(mLines);
	if (mWideText) free(mWideText);
	if (mGbkText) free(mGbkText);
}
void XAreaText::addLine(HDC dc, wchar_t *beginPos, int len) {
	tryIncLineSpace();
	mLines[mLinesNum].mBeginPos = beginPos - mWideText;
	mLines[mLinesNum].mLen = len;
	mLines[mLinesNum].mStartX = 0;
	mLines[mLinesNum].mStartY = 0;
	SIZE sz = {0};
	GetTextExtentPoint32W(dc, beginPos, len, &sz);
	mLines[mLinesNum].mTextWidth = sz.cx;
	mLinesNum++;
}
int XAreaText::getLineNoByY( int y ) {
	int by = 0;
	if (y < 0) {
		return -1;
	}
	for (int i = 0; i < mLinesNum; ++i) {
		if (y >= mLines[i].mStartY + by && y < by + mLines[i].mStartY + mLineHeight) {
			return i;
		}
		by += mLineHeight;
	}
	return mLinesNum - 1;
}
int XAreaText::getLineNoByPos( int pos ) {
	int line = 0;
	if (pos < 0) {
		return -1;
	}
	for (int i = 0; i < mLinesNum; ++i, ++line) {
		int bg = mLines[i].mBeginPos;
		int len = mLines[i].mLen;
		if (pos >= bg && pos < bg + len) {
			return line;
		}
	}
	return max(mLinesNum - 1, 0);
}
int XAreaText::getYAtLine( int lineNo ) {
	if (lineNo < 0 || mLinesNum == 0) {
		return 0;
	}
	int y = 0;
	for (int i = 0; i < lineNo && i < mLinesNum; ++i) {
		y += mLines[i].mStartY + mLineHeight;
	}
	if (lineNo < mLinesNum) {
		y += mLines[lineNo].mStartY;
	}
	return y;;
}
int XAreaText::getWideTextLength() {
	return mWideTextLen;
}


