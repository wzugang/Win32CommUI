#pragma once
#pragma warning(disable:4996)
#include <windows.h>
#include <stdlib.h>
#include <string.h>

template <class T>
class XTString {
public:
	enum Charset {
		GBK, UTF8, UNICODE2
	};
	explicit XTString(int capacity);
	XTString(const T *str = NULL);
	// copy of string [beginIdx, endIdx)
	XTString(const T *str, int beginIdx, int endIdx);
	XTString(const XTString<T> &rh);

	XTString &operator=(const XTString<T> &rh);
	bool operator == (const XTString<T> &rh);
	bool operator != (const XTString<T> &rh);
	// always not NULL
	T *str();
	int length();

	T charAt(int idx);
	bool startsWith(const T *prefix, int fromIndex);
	bool startWith(const T *prefix);
	bool endWith(const T *suffix);

	int indexOf(T ch, int fromIndex);
	int indexOf(const T *str, int fromIndex);
	int lastIndexOf(T ch, int fromIndex);
	int lastIndexOf(const T *str, int fromIndex);

	// sub string [beginIndex, endIndex)
	XTString<T> subString(int beginIndex, int endIndex = -1);
	XTString<T> &append(const XTString<T> &str);
	void replace(T oldChar, T newChar);
	// replace string [start, end) to str
	void replace(int start, int end, const XTString<T>& str);
	// delete char [start, end)
	void del(int start, int end);
	void insert(int index, const XTString<T> &str);

	void toUpperCase();
	void toLowerCase();
	void trim();

	// need free yourself
	static void *toBytes(void *str, Charset from, Charset to);
	static void *dup(void *str, Charset ch);
	static char *dups(const char *str);
	static wchar_t *dupws(const wchar_t *str);
	~XTString();
protected:
	void needBuffer(int need);
	int indexOf(const T *source, int sourceOffset, int sourceCount,
		const T *target, int targetOffset, int targetCount, int fromIndex);
	int lastIndexOf(const T *source, int sourceOffset, int sourceCount,
		const T *target, int targetOffset, int targetCount, int fromIndex);
protected:
	T *mBuffer;
	int mLen;
	int mCapacity;
};

template <class T>
void XTString<T>::insert(int index, const XTString<T> &str) {
	if (index < 0 || index > mLen || str.mLen == 0) {
		return;
	}
	needBuffer(mLen + str.mLen);
	memmove(mBuffer + index + str.mLen, mBuffer + index, (mLen - index) * sizeof(T));
	memcpy(mBuffer + index, str.mBuffer, str.mLen * sizeof(T));
	mLen += str.mLen;
	mBuffer[mLen] = 0;
}

template <class T>
void XTString<T>::del(int start, int end) {
	if (end > mLen) {
		end = mLen;
	}
	if (start < 0 || start >= end || start >= mLen) {
		return;
	}
	int len = end - start;
	memmove(mBuffer + start, mBuffer + end, sizeof(T) * len);
	mLen -= len;
	mBuffer[mLen] = 0;
}

template <class T>
void XTString<T>::trim() {
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
	memmove(mBuffer, mBuffer + sp, len * sizeof(T));
	mLen = len;
	mBuffer[mLen] = 0;
}

template <class T>
bool XTString<T>::startsWith(const T *prefix, int offset) {
	if (prefix == NULL || offset < 0) {
		return false;
	}
	int prefixLen = sizeof(T) == sizeof(char) ? strlen((const char *)prefix) : wcslen((const wchar_t *)prefix);
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

template <class T>
int XTString<T>::indexOf(const T *str, int fromIndex) {
	if (str == NULL) {
		return -1;
	}
	int len = sizeof(T) == sizeof(char) ? strlen((const char *)str) : wcslen((const wchar_t *)str);
	return indexOf(mBuffer, 0, mLen, str, 0, len, fromIndex);
}

template <class T>
int XTString<T>::indexOf(T ch, int fromIndex) {
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

template <class T>
int XTString<T>::lastIndexOf(T ch, int fromIndex) {
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

template <class T>
int XTString<T>::lastIndexOf(const T *str, int fromIndex) {
	if (str == NULL) {
		return -1;
	}
	int len = sizeof(T) == sizeof(char) ? strlen((const char *)str) : wcslen((const wchar_t *)str);
	return lastIndexOf(mBuffer, 0, mLen, str, 0, len, fromIndex);
}

template <class T>
int XTString<T>::indexOf(const T *source, int sourceOffset, int sourceCount, const T *target, int targetOffset, int targetCount, int fromIndex) {
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
	T first  = target[targetOffset];
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

template <class T>
int XTString<T>::lastIndexOf(const T *source, int sourceOffset, int sourceCount, const T *target, int targetOffset, int targetCount, int fromIndex) {
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
	T strLastChar = target[strLastIndex];
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

template <class T>
bool XTString<T>::endWith(const T *suffix) {
	if (suffix == NULL) {
		return false;
	}
	int len = sizeof(T) == sizeof(char) ? strlen((const char *)suffix) : wcslen((const wchar_t *)suffix);
	return startsWith(suffix, mLen - len);
}

template <class T>
bool XTString<T>::startWith(const T *prefix) {
	return startsWith(prefix, 0);
}

template <class T>
XTString<T> XTString<T>::subString(int beginIndex, int endIndex) {
	if (endIndex == -1) {
		endIndex = mLen;
	}
	return XTString<T>(mBuffer, beginIndex, endIndex);
}

template <class T>
XTString<T> &XTString<T>::append(const XTString<T> &str) {
	needBuffer(str.mLen + mLen);
	memcpy(mBuffer + mLen, str.mBuffer, sizeof(T) * (str.mLen + 1));
	return *this;
}

template <class T>
void XTString<T>::replace(T oldChar, T newChar) {
	if (oldChar == newChar) {
		return;
	}
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] == oldChar) {
			mBuffer[i] = newChar;
		}
	}
}

template <class T>
void XTString<T>::replace(int start, int end, const XTString<T>& str) {
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
	memcpy(mBuffer + start, str.mBuffer, sizeof(T) * str.mLen);
	mLen = newLen;
	mBuffer[mLen] = 0;
}

template <class T>
void XTString<T>::toLowerCase() {
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] >= 'A' && mBuffer[i] <= 'Z') {
			mBuffer[i] = mBuffer[i] - 'A' + 'a';
		}
	}
}

template <class T>
void XTString<T>::toUpperCase() {
	for (int i = 0; i < mLen; ++i) {
		if (mBuffer[i] >= 'a' && mBuffer[i] <= 'z') {
			mBuffer[i] = mBuffer[i] - 'a' + 'A';
		}
	}
}

template <class T>
XTString<T>::XTString(const T *str) {
	if (sizeof(T) == sizeof(char)) {
		mLen = (str == NULL ? 0 : strlen((char *)str));
	} else {
		mLen = (str == NULL ? 0 : wcslen((wchar_t *)str));
	}

	mCapacity = mLen;
	mBuffer = (T *)malloc((mCapacity + 1)* sizeof(T));
	*mBuffer =  0;
	if (str != NULL) {
		memcpy(mBuffer, str, (mLen + 1) * sizeof(T));
	}
}

template <class T>
XTString<T>::XTString(const T *str, int beginIdx, int endIdx) {
	int nn = 0;
	if (str == NULL || beginIdx < 0 || beginIdx >= endIdx) {
		goto _end;
	}

	int mlen = 0;
	if (sizeof(T) == sizeof(char)) {
		mlen = strlen((char *)str);
	} else {
		mlen = wcslen((wchar_t *)str);
	}
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
	mBuffer = (T *)malloc((mCapacity + 1)* sizeof(T));
	if (nn > 0) {
		memcpy(mBuffer, str + beginIdx, sizeof(T) * nn);
	}
	mBuffer[nn] = 0;
}

template <class T>
XTString<T>::XTString(int capacity) {
	if (capacity < 0) capacity = 0;
	mLen = 0;
	mCapacity = capacity;
	mBuffer = (T *)malloc(sizeof(T) * (mCapacity + 1));
	mBuffer[0] = 0;
}

template <class T>
XTString<T>::XTString(const XTString<T> &rh) {
	mLen = 0;
	mBuffer = NULL;
	mCapacity = 0;
	needBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(T));
	mLen = rh.mLen;
}

template <class T>
void XTString<T>::needBuffer(int need) {
	int old = mCapacity;
	if (mCapacity <= need) {
		mCapacity *= 2;
	}
	if (mCapacity <= need) {
		mCapacity = need;
	}
	if (old != mCapacity) {
		mBuffer = (T *)realloc(mBuffer, sizeof(T) * (mCapacity + 1));
	}
}

template <class T>
XTString<T>& XTString<T>::operator=(const XTString<T> &rh) {
	needBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(T));
	mLen = rh.mLen;
	return *this;
}

template <class T>
T * XTString<T>::str() {
	return mBuffer;
}

template <class T>
int XTString<T>::length() {
	return mLen;
}

template <class T>
bool XTString<T>::operator==(const XTString &rh) {
	if (mLen != rh.mLen) {
		return false;
	}
	return memcmp(mBuffer, rh.mBuffer, mLen * sizeof(T)) == 0;
}

template <class T>
bool XTString<T>::operator!=(const XTString &rh) {
	return !(operator==(rh));
}

template <class T>
void * XTString<T>::toBytes(void *str, Charset from, Charset to) {
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

template <class T>
void * XTString<T>::dup(void *str, Charset ch) {
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

template <class T>
wchar_t * XTString<T>::dupws( const wchar_t *str ) {
	return (wchar_t *)dup((void *)str, XTString<T>::UNICODE2);
}

template <class T>
char * XTString<T>::dups( const char *str ) {
	return (char *)dup((void *)str, XTString<T>::GBK);
}

template <class T>
T XTString<T>::charAt(int idx) {
	if (idx < 0 || idx >= mLen) {
		return 0;
	}
	return mBuffer[idx];
}

template <class T>
XTString<T>::~XTString() {
	if (mBuffer != NULL) {
		free(mBuffer);
	}
}


template class XTString<char>;
template class XTString<wchar_t>;

typedef XTString<char> XString;
typedef XTString<wchar_t> XWideString;

typedef XTString<char> String;
typedef XTString<wchar_t> WString;
