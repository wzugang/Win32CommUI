#pragma once
#include "XString.h"
#include "Array.h"
class HttpClient;

class HttpRequest {
public:
	enum Method {
		METHOD_GET, METHOD_POST
	};
	HttpRequest(const char *url, Method method);

	void setConnectTimeout(int milis);
	void setReadTimeout(int milis);
	void addHeader(const char *name, const char *val);
	void delHeader(const char *name);
	// used for POST, is request params
	void setBody(void *body, int bodyLength);

	int getResponseCode();
	int getContentLength();
	XString getResponseHeader(const char *name);
	Array<char*> *getResponseHeaders();
	
	~HttpRequest();
protected:
	XString mUrl;
	Method mMethod;
	void *mCurl;
	int mConnectTimeout, mReadTimeout;
	void *mBody;
	int mBodyLength;
	void *mReqHeaders;
	void *mContentBuf, *mResHeadersBuf;
	Array<char*> mResHeadersArray;
	friend class HttpClient;
};

class HttpClient {
public:
	HttpClient();

	// return status code. 200 is OK
	int execute(HttpRequest *req);

	void close();
	~HttpClient();
protected:
	static int mInitedNum;
	void *mCurl;
};