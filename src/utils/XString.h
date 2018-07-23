#pragma once
#pragma warning(disable:4996)

class XString {
public:
	enum Charset {
		GBK, UTF8, UNICODE2
	};
	explicit XString(int capacity);
	XString(const char *str = 0);
	// copy of string [beginIdx, endIdx)
	XString(const char *str, int beginIdx, int endIdx);
	XString(const XString &rh);

	XString &operator=(const XString &rh);
	bool operator == (const XString &rh);
	bool operator != (const XString &rh);
	// always not NULL
	char *str();
	int length();

	char charAt(int idx);
	bool startsWith(const char *prefix, int fromIndex);
	bool startWith(const char *prefix);
	bool endWith(const char *suffix);

	int indexOf(char ch, int fromIndex);
	int indexOf(const char *str, int fromIndex);
	int lastIndexOf(char ch, int fromIndex);
	int lastIndexOf(const char *str, int fromIndex);

	// sub string [beginIndex, endIndex)
	XString subString(int beginIndex, int endIndex = -1);
	XString &append(const XString &str);
	void replace(char oldChar, char newChar);
	// replace string [start, end) to str
	void replace(int start, int end, const XString& str);
	// delete char [start, end)
	void del(int start, int end);
	void insert(int index, const XString &str);

	void toUpperCase();
	void toLowerCase();
	void trim();
	
	// need free yourself
	static void *toBytes(void *str, Charset from, Charset to);
	static void *dup(void *str, Charset ch);
	static char *dups(const char *str);
	static wchar_t *dupws(const wchar_t *str);
	~XString();
protected:
	void needBuffer(int need);
	int indexOf(const char *source, int sourceOffset, int sourceCount,
		const char *target, int targetOffset, int targetCount, int fromIndex);
	int lastIndexOf(const char *source, int sourceOffset, int sourceCount,
		const char *target, int targetOffset, int targetCount, int fromIndex);
protected:
	char *mBuffer;
	int mLen;
	int mCapacity;
};

class XWideString {
public:
	explicit XWideString(int capacity);
	XWideString(const wchar_t *str = 0);
	// copy of string [beginIdx, endIdx)
	XWideString(const wchar_t *str, int beginIdx, int endIdx);
	XWideString(const XWideString &rh);

	XWideString &operator=(const XWideString &rh);
	bool operator == (const XWideString &rh);
	bool operator != (const XWideString &rh);
	// always not NULL
	wchar_t *str();
	int length();

	wchar_t charAt(int idx);
	bool startsWith(const wchar_t *prefix, int fromIndex);
	bool startWith(const wchar_t *prefix);
	bool endWith(const wchar_t *suffix);

	int indexOf(wchar_t ch, int fromIndex);
	int indexOf(const wchar_t *str, int fromIndex);
	int lastIndexOf(wchar_t ch, int fromIndex);
	int lastIndexOf(const wchar_t *str, int fromIndex);

	// sub string [beginIndex, endIndex)
	XWideString subString(int beginIndex, int endIndex = -1);
	XWideString &append(const XWideString &str);
	void replace(wchar_t oldChar, wchar_t newChar);
	// replace string [start, end) to str
	void replace(int start, int end, const XWideString& str);
	// delete char [start, end)
	void del(int start, int end);
	void insert(int index, const XWideString &str);

	void toUpperCase();
	void toLowerCase();
	void trim();

	~XWideString();
protected:
	void needBuffer(int need);
	int indexOf(const wchar_t *source, int sourceOffset, int sourceCount,
		const wchar_t *target, int targetOffset, int targetCount, int fromIndex);
	int lastIndexOf(const wchar_t *source, int sourceOffset, int sourceCount,
		const wchar_t *target, int targetOffset, int targetCount, int fromIndex);
protected:
	wchar_t *mBuffer;
	int mLen;
	int mCapacity;
};




