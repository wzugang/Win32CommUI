#include "XString.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#if 0
template <class T>
XTString<T>::XTString(const T *str) {
	if (sizeof(T) == sizeof(char)) {
		mLen = (str == NULL ? 0 : strlen(str));
	} else {
		mLen = (str == NULL ? 0 : wcslen(str));
	}
	mCapacity = mLen + 1;
	mBuffer = (T *)malloc(mCapacity * sizeof(T));
	*mBuffer =  0;
	if (str != NULL) {
		memcpy(mBuffer, str, (mLen + 1) * sizeof(T));
	}
}

template <class T>
XTString<T>::XTString(const XTString<T> &rh) {
	mLen = 0;
	mBuffer = NULL;
	mCapacity = 0;
	incBuffer(rh.mLen);
	memcpy(mBuffer, rh.mBuffer, (rh.mLen + 1) * sizeof(T));
	mLen = rh.mLen;
}

template <class T>
void XTString<T>::incBuffer(int need) {
	int old = mCapacity;
	if (mCapacity <= need) {
		mCapacity *= 2;
	}
	if (mCapacity <= need) {
		mCapacity = need + 1;
	}
	if (old != mCapacity) {
		mBuffer = realloc(mBuffer, sizeof(T) * mCapacity);
	}
}

template <class T>
XTString<T>& XTString<T>::operator=(const XTString<T> &rh) {
	incBuffer(rh.mLen);
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
	if ((from == GBK || from == UTF8) && to == UNICODE) {
		int cp = from == GBK ? CP_ACP : CP_UTF8;
		int len = MultiByteToWideChar(cp, 0, (const char *)str, -1, NULL, 0);
		wchar_t *dst = (wchar_t *)malloc(len * 2 + 2);
		MultiByteToWideChar(cp, 0, (const char *)str, -1, dst, len);
		return dst;
	}
	if (from == UNICODE && (to == GBK || to == UTF8)) {
		int cp = to == GBK ? CP_ACP : CP_UTF8;
		int len = WideCharToMultiByte(cp, 0, (const wchar_t*)str, -1, NULL, 0, NULL, NULL);
		char *dst = (char *)malloc(len + 1);
		WideCharToMultiByte(cp, 0, (const wchar_t*)str, -1, dst, len, NULL, NULL);
		return dst;
	}
	// GBK -> UTF8  or UTF8 -> GBK
	if (from != UNICODE && to != UNICODE) {
		wchar_t *tmp = (wchar_t *)toBytes(str, from, UNICODE);
		char *dst = (char *)toBytes(tmp, UNICODE, to);
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
	if (ch == UNICODE) {
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
XTString<T>::~XTString() {
	if (mBuffer != NULL) {
		free(mBuffer);
	}
}
#endif