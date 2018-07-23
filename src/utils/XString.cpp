#include "XString.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>

XString::XString(int capacity) {
	if (capacity < 0) capacity = 0;
	mLen = 0;
	mCapacity = capacity;
	mBuffer = (char *)malloc(sizeof(char) * (mCapacity + 1));
	mBuffer[0] = 0;
}

XString::XString(const char *str) {
	mLen = (str == NULL ? 0 : strlen(str));
	mCapacity = mLen;
	mBuffer = (char *)malloc((mCapacity + 1)* sizeof(char));
	*mBuffer =  0;
	if (str != NULL) {
		memcpy(mBuffer, str, (mLen + 1) * sizeof(char));
	}
}

XString::XString(const char *str, int beginIdx, int endIdx) {
	int nn = 0;
	if (str == NULL || beginIdx < 0 || beginIdx >= endIdx) {
		goto _end;
	}

	int mlen = 0;
	mlen = strlen((char *)str);

	if (endIdx > mlen) {
		endIdx = mlen;
	}
	nn = endIdx - beginIdx;
	if (nn < 0) {
		nn = 0;
	}
_end:
	mLen = nn;
	mCapacity = nn;
	mBuffer = (char *)malloc((mCapacity + 1)* sizeof(char));
	if (nn > 0) {
		memcpy(mBuffer, str + beginIdx, sizeof(char) * nn);
	}
	mBuffer[nn] = 0;
}

XString::XString(const XString &rh) {
	mLen = 0;
	mBuffer = NULL;
	mCapacity = 0;
	needBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(char));
	mLen = rh.mLen;
}

XString& XString::operator=(const XString &rh) {
	needBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(char));
	mLen = rh.mLen;
	return *this;
}

bool XString::operator==(const XString &rh) {
	if (mLen != rh.mLen) {
		return false;
	}
	return memcmp(mBuffer, rh.mBuffer, mLen * sizeof(char)) == 0;
}

bool XString::operator!=(const XString &rh) {
	return !(operator==(rh));
}

void XString::insert(int index, const XString &str) {
	if (index < 0 || index > mLen || str.mLen == 0) {
		return;
	}
	needBuffer(mLen + str.mLen);
	memmove(mBuffer + index + str.mLen, mBuffer + index, (mLen - index) * sizeof(char));
	memcpy(mBuffer + index, str.mBuffer, str.mLen * sizeof(char));
	mLen += str.mLen;
	mBuffer[mLen] = 0;
}

void XString::del(int start, int end) {
	if (end > mLen) {
		end = mLen;
	}
	if (start < 0 || start >= end || start >= mLen) {
		return;
	}
	int len = end - start;
	memmove(mBuffer + start, mBuffer + end, sizeof(char) * len);
	mLen -= len;
	mBuffer[mLen] = 0;
}

void XString::trim() {
	if (mLen == 0) {
		return;
	}
	int sp = 0, ep = mLen - 1;
	while ((sp < mLen) && (mBuffer[sp] == ' ')) {
		++sp;
	}
	while ((ep >= 0) && (mBuffer[ep] == ' ')) {
		--ep;
	}
	if (sp > ep || sp == mLen) {
		// all is space
		mLen = 0;
		mBuffer[mLen] = 0;
		return;
	}
	int len = ep - sp + 1;
	memmove(mBuffer, mBuffer + sp, len * sizeof(char));
	mLen = len;
	mBuffer[mLen] = 0;
}

bool XString::startsWith(const char *prefix, int offset) {
	if (prefix == NULL || offset < 0) {
		return false;
	}
	int prefixLen = strlen((const char *)prefix);
	if (offset > mLen - prefixLen) {
		return false;
	}
	int sx = 0;
	while (--prefixLen >= 0) {
		if (mBuffer[offset++] != prefix[sx++]) {
			return false;
		}
	}
	return true;
}

int XString::indexOf(const char *str, int fromIndex) {
	if (str == NULL) {
		return -1;
	}
	int len = strlen((const char *)str);
	return indexOf(mBuffer, 0, mLen, str, 0, len, fromIndex);
}

int XString::indexOf(char ch, int fromIndex) {
	if (fromIndex >= mLen) {
		return -1;
	}
	if (fromIndex < 0) {
		fromIndex = 0;
	}
	for (int i = fromIndex; i < mLen; ++i) {
		if (mBuffer[i] == ch) {
			return i;
		}
	}
	return -1;
}

int XString::lastIndexOf(char ch, int fromIndex) {
	if (fromIndex >= mLen) {
		fromIndex = mLen - 1;
	}
	for (int i = fromIndex; i >= 0; --i) {
		if (mBuffer[i] == ch) {
			return i;
		}
	}
	return -1;
}

int XString::lastIndexOf(const char *str, int fromIndex) {
	if (str == NULL) {
		return -1;
	}
	int len = strlen((const char *)str);
	return lastIndexOf(mBuffer, 0, mLen, str, 0, len, fromIndex);
}

int XString::indexOf(const char *source, int sourceOffset, int sourceCount, const char *target, int targetOffset, int targetCount, int fromIndex) {
	if (source == NULL || target == NULL) {
		return -1;
	}
	if (fromIndex >= sourceCount) {
		return targetCount == 0 ? sourceCount : -1;
	}
	if (fromIndex < 0) {
		fromIndex = 0;
	}
	if (targetCount == 0) {
		return fromIndex;
	}
	char first  = target[targetOffset];
	int max = sourceOffset + (sourceCount - targetCount);
	for (int i = sourceOffset + fromIndex; i <= max; i++) {
		/* Look for first character. */
		if (source[i] != first) {
			while (++i <= max && source[i] != first);
		}

		/* Found first character, now look at the rest of v2 */
		if (i <= max) {
			int j = i + 1;
			int end = j + targetCount - 1;
			for (int k = targetOffset + 1; j < end && source[j] == target[k]; j++, k++);
			if (j == end) {
				/* Found whole string. */
				return i - sourceOffset;
			}
		}
	}
	return -1;
}

int XString::lastIndexOf(const char *source, int sourceOffset, int sourceCount, const char *target, int targetOffset, int targetCount, int fromIndex) {
	if (source == NULL || target == NULL) {
		return -1;
	}
	int rightIndex = sourceCount - targetCount;
	if (fromIndex < 0) {
		return -1;
	}
	if (fromIndex > rightIndex) {
		fromIndex = rightIndex;
	}
	/* Empty string always matches. */
	if (targetCount == 0) {
		return fromIndex;
	}

	int strLastIndex = targetOffset + targetCount - 1;
	char strLastChar = target[strLastIndex];
	int min = sourceOffset + targetCount - 1;
	int i = min + fromIndex;

startSearchForLastChar:
	while (true) {
		while (i >= min && source[i] != strLastChar) {
			i--;
		}
		if (i < min) {
			return -1;
		}
		int j = i - 1;
		int start = j - (targetCount - 1);
		int k = strLastIndex - 1;

		while (j > start) {
			if (source[j--] != target[k--]) {
				i--;
				goto startSearchForLastChar;
			}
		}
		return start - sourceOffset + 1;
	}
}


bool XString::endWith(const char *suffix) {
	if (suffix == NULL) {
		return false;
	}
	int len = strlen((const char *)suffix);
	return startsWith(suffix, mLen - len);
}


bool XString::startWith(const char *prefix) {
	return startsWith(prefix, 0);
}


XString XString::subString(int beginIndex, int endIndex) {
	if (endIndex == -1) {
		endIndex = mLen;
	}
	return XString(mBuffer, beginIndex, endIndex);
}


XString &XString::append(const XString &str) {
	needBuffer(str.mLen + mLen);
	memcpy(mBuffer + mLen, str.mBuffer, sizeof(char) * (str.mLen + 1));
	return *this;
}


void XString::replace(char oldChar, char newChar) {
	if (oldChar == newChar) {
		return;
	}
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] == oldChar) {
			mBuffer[i] = newChar;
		}
	}
}


void XString::replace(int start, int end, const XString& str) {
	if (start < 0 || start <= end || start >= mLen || str.mLen == 0) {
		return;
	}
	if (end > mLen) {
		end = mLen;
	}
	int newLen = mLen + str.mLen - (end - start);
	needBuffer(newLen);
	int iic = (end - start) - str.mLen;
	memmove(mBuffer + end - iic, mBuffer + end, mLen - end);
	memcpy(mBuffer + start, str.mBuffer, sizeof(char) * str.mLen);
	mLen = newLen;
	mBuffer[mLen] = 0;
}

void XString::toLowerCase() {
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] >= 'A' && mBuffer[i] <= 'Z') {
			mBuffer[i] = mBuffer[i] - 'A' + 'a';
		}
	}
}

void XString::toUpperCase() {
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] >= 'a' && mBuffer[i] <= 'z') {
			mBuffer[i] = mBuffer[i] - 'a' + 'A';
		}
	}
}

void XString::needBuffer(int need) {
	int old = mCapacity;
	if (mCapacity <= need) {
		mCapacity *= 2;
	}
	if (mCapacity <= need) {
		mCapacity = need;
	}
	if (old != mCapacity) {
		mBuffer = (char *)realloc(mBuffer, sizeof(char) * (mCapacity + 1));
	}
}

char * XString::str() {
	return mBuffer;
}

int XString::length() {
	return mLen;
}

void * XString::toBytes(void *str, Charset from, Charset to) {
	if (str == NULL) {
		return NULL;
	}
	if (from == to) {
		if (from == GBK || from == UTF8) {
			int len = strlen((const char *)str);
			char *dst = (char *)malloc(len + 1);
			strcpy(dst, (const char *)str);
			return dst;
		} else {
			int len = wcslen((const wchar_t *)str);
			wchar_t *dst = (wchar_t *)malloc(len * 2 + 2);
			wcscpy(dst, (const wchar_t *)str);
			return dst;
		}
	}
	if ((from == GBK || from == UTF8) && to == UNICODE2) {
		int cp = from == GBK ? CP_ACP : CP_UTF8;
		int len = MultiByteToWideChar(cp, 0, (const char *)str, -1, NULL, 0);
		wchar_t *dst = (wchar_t *)malloc(len * 2 + 2);
		MultiByteToWideChar(cp, 0, (const char *)str, -1, dst, len);
		return dst;
	}
	if (from == UNICODE2 && (to == GBK || to == UTF8)) {
		int cp = to == GBK ? CP_ACP : CP_UTF8;
		int len = WideCharToMultiByte(cp, 0, (const wchar_t*)str, -1, NULL, 0, NULL, NULL);
		char *dst = (char *)malloc(len + 1);
		WideCharToMultiByte(cp, 0, (const wchar_t*)str, -1, dst, len, NULL, NULL);
		return dst;
	}
	// GBK -> UTF8  or UTF8 -> GBK
	if (from != UNICODE2 && to != UNICODE2) {
		wchar_t *tmp = (wchar_t *)toBytes(str, from, UNICODE2);
		char *dst = (char *)toBytes(tmp, UNICODE2, to);
		free(tmp);
		return dst;
	}
	return NULL;
}

void * XString::dup(void *str, Charset ch) {
	if (str == NULL) {
		return NULL;
	}
	if (ch == UNICODE2) {
		int len = wcslen((wchar_t *)str) + 1;
		void *dd = malloc(len * sizeof(wchar_t));
		wcscpy((wchar_t *)dd, (wchar_t *)str);
		return dd;
	}
	if (ch == GBK || ch == UTF8) {
		int len = strlen((char *)str) + 1;
		void *dd = malloc(len);
		strcpy((char *)dd, (char *)str);
		return dd;
	}
	return NULL;
}

wchar_t * XString::dupws( const wchar_t *str ) {
	return (wchar_t *)dup((void *)str, UNICODE2);
}

char * XString::dups( const char *str ) {
	return (char *)dup((void *)str, GBK);
}

char XString::charAt(int idx) {
	if (idx < 0 || idx >= mLen) {
		return 0;
	}
	return mBuffer[idx];
}

XString::~XString() {
	if (mBuffer != NULL) {
		free(mBuffer);
	}
}

//---------------------------------------------------------

XWideString::XWideString(int capacity) {
	if (capacity < 0) capacity = 0;
	mLen = 0;
	mCapacity = capacity;
	mBuffer = (wchar_t *)malloc(sizeof(wchar_t) * (mCapacity + 1));
	mBuffer[0] = 0;
}

XWideString::XWideString(const wchar_t *str) {
	mLen = (str == NULL ? 0 : wcslen((wchar_t *)str));
	mCapacity = mLen;
	mBuffer = (wchar_t *)malloc((mCapacity + 1)* sizeof(wchar_t));
	*mBuffer =  0;
	if (str != NULL) {
		memcpy(mBuffer, str, (mLen + 1) * sizeof(wchar_t));
	}
}

XWideString::XWideString(const wchar_t *str, int beginIdx, int endIdx) {
	int nn = 0;
	if (str == NULL || beginIdx < 0 || beginIdx >= endIdx) {
		goto _end;
	}

	int mlen = 0; 
	mlen = wcslen((wchar_t *)str);
	if (endIdx > mlen) {
		endIdx = mlen;
	}
	nn = endIdx - beginIdx;
	if (nn < 0) {
		nn = 0;
	}
_end:
	mLen = nn;
	mCapacity = nn;
	mBuffer = (wchar_t *)malloc((mCapacity + 1)* sizeof(wchar_t));
	if (nn > 0) {
		memcpy(mBuffer, str + beginIdx, sizeof(wchar_t) * nn);
	}
	mBuffer[nn] = 0;
}

XWideString::XWideString(const XWideString &rh) {
	mLen = 0;
	mBuffer = NULL;
	mCapacity = 0;
	needBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(wchar_t));
	mLen = rh.mLen;
}

XWideString& XWideString::operator=(const XWideString &rh) {
	needBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(wchar_t));
	mLen = rh.mLen;
	return *this;
}

bool XWideString::operator==(const XWideString &rh) {
	if (mLen != rh.mLen) {
		return false;
	}
	return memcmp(mBuffer, rh.mBuffer, mLen * sizeof(wchar_t)) == 0;
}

bool XWideString::operator!=(const XWideString &rh) {
	return !(operator==(rh));
}

void XWideString::insert(int index, const XWideString &str) {
	if (index < 0 || index > mLen || str.mLen == 0) {
		return;
	}
	needBuffer(mLen + str.mLen);
	memmove(mBuffer + index + str.mLen, mBuffer + index, (mLen - index) * sizeof(wchar_t));
	memcpy(mBuffer + index, str.mBuffer, str.mLen * sizeof(wchar_t));
	mLen += str.mLen;
	mBuffer[mLen] = 0;
}

void XWideString::del(int start, int end) {
	if (end > mLen) {
		end = mLen;
	}
	if (start < 0 || start >= end || start >= mLen) {
		return;
	}
	int len = end - start;
	memmove(mBuffer + start, mBuffer + end, sizeof(wchar_t) * len);
	mLen -= len;
	mBuffer[mLen] = 0;
}

void XWideString::trim() {
	if (mLen == 0) {
		return;
	}
	int sp = 0, ep = mLen - 1;
	while ((sp < mLen) && (mBuffer[sp] == ' ')) {
		++sp;
	}
	while ((ep >= 0) && (mBuffer[ep] == ' ')) {
		--ep;
	}
	if (sp > ep || sp == mLen) {
		// all is space
		mLen = 0;
		mBuffer[mLen] = 0;
		return;
	}
	int len = ep - sp + 1;
	memmove(mBuffer, mBuffer + sp, len * sizeof(wchar_t));
	mLen = len;
	mBuffer[mLen] = 0;
}

bool XWideString::startsWith(const wchar_t *prefix, int offset) {
	if (prefix == NULL || offset < 0) {
		return false;
	}
	int prefixLen = wcslen((const wchar_t *)prefix);
	if (offset > mLen - prefixLen) {
		return false;
	}
	int sx = 0;
	while (--prefixLen >= 0) {
		if (mBuffer[offset++] != prefix[sx++]) {
			return false;
		}
	}
	return true;
}

int XWideString::indexOf(const wchar_t *str, int fromIndex) {
	if (str == NULL) {
		return -1;
	}
	int len = wcslen((const wchar_t *)str);
	return indexOf(mBuffer, 0, mLen, str, 0, len, fromIndex);
}

int XWideString::indexOf(wchar_t ch, int fromIndex) {
	if (fromIndex >= mLen) {
		return -1;
	}
	if (fromIndex < 0) {
		fromIndex = 0;
	}
	for (int i = fromIndex; i < mLen; ++i) {
		if (mBuffer[i] == ch) {
			return i;
		}
	}
	return -1;
}

int XWideString::lastIndexOf(wchar_t ch, int fromIndex) {
	if (fromIndex >= mLen) {
		fromIndex = mLen - 1;
	}
	for (int i = fromIndex; i >= 0; --i) {
		if (mBuffer[i] == ch) {
			return i;
		}
	}
	return -1;
}

int XWideString::lastIndexOf(const wchar_t *str, int fromIndex) {
	if (str == NULL) {
		return -1;
	}
	int len = wcslen(str);
	return lastIndexOf(mBuffer, 0, mLen, str, 0, len, fromIndex);
}

int XWideString::indexOf(const wchar_t *source, int sourceOffset, int sourceCount, const wchar_t *target, int targetOffset, int targetCount, int fromIndex) {
	if (source == NULL || target == NULL) {
		return -1;
	}
	if (fromIndex >= sourceCount) {
		return targetCount == 0 ? sourceCount : -1;
	}
	if (fromIndex < 0) {
		fromIndex = 0;
	}
	if (targetCount == 0) {
		return fromIndex;
	}
	wchar_t first  = target[targetOffset];
	int max = sourceOffset + (sourceCount - targetCount);
	for (int i = sourceOffset + fromIndex; i <= max; i++) {
		/* Look for first character. */
		if (source[i] != first) {
			while (++i <= max && source[i] != first);
		}

		/* Found first character, now look at the rest of v2 */
		if (i <= max) {
			int j = i + 1;
			int end = j + targetCount - 1;
			for (int k = targetOffset + 1; j < end && source[j] == target[k]; j++, k++);
			if (j == end) {
				/* Found whole string. */
				return i - sourceOffset;
			}
		}
	}
	return -1;
}

int XWideString::lastIndexOf(const wchar_t *source, int sourceOffset, int sourceCount, const wchar_t *target, int targetOffset, int targetCount, int fromIndex) {
	if (source == NULL || target == NULL) {
		return -1;
	}
	int rightIndex = sourceCount - targetCount;
	if (fromIndex < 0) {
		return -1;
	}
	if (fromIndex > rightIndex) {
		fromIndex = rightIndex;
	}
	/* Empty string always matches. */
	if (targetCount == 0) {
		return fromIndex;
	}

	int strLastIndex = targetOffset + targetCount - 1;
	wchar_t strLastChar = target[strLastIndex];
	int min = sourceOffset + targetCount - 1;
	int i = min + fromIndex;

startSearchForLastChar:
	while (true) {
		while (i >= min && source[i] != strLastChar) {
			i--;
		}
		if (i < min) {
			return -1;
		}
		int j = i - 1;
		int start = j - (targetCount - 1);
		int k = strLastIndex - 1;

		while (j > start) {
			if (source[j--] != target[k--]) {
				i--;
				goto startSearchForLastChar;
			}
		}
		return start - sourceOffset + 1;
	}
}


bool XWideString::endWith(const wchar_t *suffix) {
	if (suffix == NULL) {
		return false;
	}
	int len = wcslen(suffix);
	return startsWith(suffix, mLen - len);
}


bool XWideString::startWith(const wchar_t *prefix) {
	return startsWith(prefix, 0);
}


XWideString XWideString::subString(int beginIndex, int endIndex) {
	if (endIndex == -1) {
		endIndex = mLen;
	}
	return XWideString(mBuffer, beginIndex, endIndex);
}


XWideString &XWideString::append(const XWideString &str) {
	needBuffer(str.mLen + mLen);
	memcpy(mBuffer + mLen, str.mBuffer, sizeof(wchar_t) * (str.mLen + 1));
	return *this;
}


void XWideString::replace(wchar_t oldChar, wchar_t newChar) {
	if (oldChar == newChar) {
		return;
	}
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] == oldChar) {
			mBuffer[i] = newChar;
		}
	}
}


void XWideString::replace(int start, int end, const XWideString& str) {
	if (start < 0 || start <= end || start >= mLen || str.mLen == 0) {
		return;
	}
	if (end > mLen) {
		end = mLen;
	}
	int newLen = mLen + str.mLen - (end - start);
	needBuffer(newLen);
	int iic = (end - start) - str.mLen;
	memmove(mBuffer + end - iic, mBuffer + end, mLen - end);
	memcpy(mBuffer + start, str.mBuffer, sizeof(wchar_t) * str.mLen);
	mLen = newLen;
	mBuffer[mLen] = 0;
}

void XWideString::toLowerCase() {
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] >= 'A' && mBuffer[i] <= 'Z') {
			mBuffer[i] = mBuffer[i] - 'A' + 'a';
		}
	}
}

void XWideString::toUpperCase() {
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] >= 'a' && mBuffer[i] <= 'z') {
			mBuffer[i] = mBuffer[i] - 'a' + 'A';
		}
	}
}

void XWideString::needBuffer(int need) {
	int old = mCapacity;
	if (mCapacity <= need) {
		mCapacity *= 2;
	}
	if (mCapacity <= need) {
		mCapacity = need;
	}
	if (old != mCapacity) {
		mBuffer = (wchar_t *)realloc(mBuffer, sizeof(wchar_t) * (mCapacity + 1));
	}
}

wchar_t * XWideString::str() {
	return mBuffer;
}

int XWideString::length() {
	return mLen;
}

wchar_t XWideString::charAt(int idx) {
	if (idx < 0 || idx >= mLen) {
		return 0;
	}
	return mBuffer[idx];
}

XWideString::~XWideString() {
	if (mBuffer != NULL) {
		free(mBuffer);
	}
}