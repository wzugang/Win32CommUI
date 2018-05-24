#pragma once
#include <windows.h>
#include <winhttp.h>

class HttpRequest;

class HttpSession {
public:
	enum Method {GET, POST};
	enum Protocal {HTTP, HTTPS};

	HttpSession(const wchar_t *host, int port);
	HttpSession(const char *host, int port);
	
	void connect();

	HttpRequest *createRequest(const wchar_t *path, Method m = GET, Protocal pro = HTTP);
	HttpRequest *createRequest(const char *path, Method m = GET, Protocal pro = HTTP);
	
	void close();
	~HttpSession();
protected:
protected:
	HINTERNET mSession;
	HINTERNET mConnection;
	wchar_t *mHost;
	int mPort;
};

class HttpRequest {
public:
	enum Encode {
		UNKNOW, GBK, UTF8
	};
	// split every header by \r\n
	bool setReqHeaders(const wchar_t *headers);

	bool sendRequest(void *body = NULL, int len = 0, int totalLen = 0);

	int getAvailableBytes();
	int getContentLength();
	int getStatusCode();
	// free your-self
	wchar_t *getResHeaders();
	wchar_t *getResHeader(const wchar_t *name);
	char *getContentType();
	Encode getEncode();
	// return read len
	int read(void *buf, int bufLen);
	int write(void *data, int dataLen);
	bool close();
protected:
	HttpRequest(HINTERNET req);
protected:
	HINTERNET mRequest;
	bool mRecevied;
	friend class HttpSession;
};


