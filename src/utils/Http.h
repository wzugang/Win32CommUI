#pragma once
#include "XString.h"
#include "Array.h"

class HttpConnection {
public:
	/************************************************************************/
	/* @param url http[s]://www.dd.com:port/aa.jsp
	/* @param method  is GET or POST
	/************************************************************************/
	HttpConnection(const char *url, const char *method);

	// @return connect whether ok
	bool connect();

	void setConnectTimeout(int timeout);

	void setReadTimeout(int timeout);

	int getResponseCode();

	XString getStatesLine();

	int getContentLength();

	XString getContentType();

	// return UTF8, GBK, ...
	XString getContentEncoding();

	XString getResponseHeader(const char *name);

	bool setRequestHeader(const char *name, const char *val);

	int read(void *buf, int len);

	// must call this before write(), if has write data
	void setWriteTotalLength(int total);

	// write request params for POST
	int write(void *buf, int len);

	void close();

protected:
	enum Status {S_NONE, S_CONNECTED, S_SEND_REQUESTED, S_RECEVICED, S_CLOSED};
	void send();
	void recieve();
	void recieveHeader();
protected:
	void *mConnection;
	void *mRequest;
	XString mUrl;
	XString mMethod;
	Status mStatus;
	int mWriteDataLen;
	char *mRawResHeaders;
	Array<char*> mResHeaders;
};

/*
Example:

HttpConnection con("https://www.zhihu.com/question/20615748", "GET");
bool ok = con.connect();
int rc = con.getResponseCode();
const int N =  1024 * 1024 * 4;
char *buf = new char[1024 * 1024 * 4];
memset(buf, 0, N);
int len = con.getContentLength();
int rlen = con.read(buf, N - 1);
char *gbk = (char *)String::toBytes(buf, String::UTF8, String::GBK);

*/

