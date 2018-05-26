#pragma once
#include "XString.h"
#include "IOStream.h"
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

	String getStatesLine();

	int getContentLength();

	String getContentType();

	// return UTF8, GBK, ...
	String getContentEncoding();

	String getResponseHeader(const char *name);

	bool setRequestHeader(const char *name, const char *val);

	int read(void *buf, int len);

	// must call this before write(), if has write data
	void setWriteDataLen(int len);

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
	String mUrl;
	String mMethod;
	Status mStatus;
	int mWriteDataLen;
	char *mRawResHeaders;
	Array<char*> mResHeaders;
};

