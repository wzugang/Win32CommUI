#pragma once
#include <windows.h>
#include <winhttp.h>

class HttpRequest;

class HttpSession {
	HttpSession();
	enum Method {
		M_GET, M_POST
	};
	void connect(const wchar_t *host, int port);
	HttpRequest *createRequest(Method m, const wchar_t *path);
protected:
protected:
	HINTERNET mSession;
	HINTERNET mConnection;
};

class HttpRequest {
public:
	// split every header by \r\n
	bool setHeaders(const wchar_t *headers);
	bool sendRequest(void *data, int len);
	// headerId : see WINHTTP_QUERY_CONTENT_LENGTH
	bool getHeader(int headerId, char *buf, int *len);
	// return read len
	int read(void *buf, int bufLen);
	bool close();
protected:
	HttpRequest(HINTERNET req);
protected:
	HINTERNET mRequest;
	friend class HttpSession;
};


