#include "Http.h"
#pragma comment(lib, "winhttp.lib")


HttpSession::HttpSession() {
	mSession = NULL;
	mConnection = NULL;
}

void HttpSession::connect(const wchar_t *host, int port) {
	// userAgent
	if (mSession == NULL) {
		mSession = WinHttpOpen(NULL, NULL, NULL, NULL, NULL);
	}
	if (mConnection == NULL) {
		mConnection = WinHttpConnect(mSession, host, (INTERNET_PORT) port, 0);
	}
}

HttpRequest *HttpSession::createRequest(Method m, const wchar_t *path) {
	const wchar_t *method = m == M_GET ? L"GET" : L"POST";
	// scheme is : INTERNET_SCHEME_HTTPS or INTERNET_SCHEME_HTTP
	// if (scheme == INTERNET_SCHEME_HTTPS) {
	//	flags |= WINHTTP_FLAG_SECURE;
	// }
	HINTERNET req = WinHttpOpenRequest(mConnection, method, path, NULL, NULL, NULL, 0);
	if (req == NULL) {
		return NULL;
	}
	return new HttpRequest(req);
}

//------------HttpRequest------------------
HttpRequest::HttpRequest(HINTERNET req) {
	mRequest = req;
}

bool HttpRequest::setHeaders(const wchar_t *headers) {
	if (headers == NULL) {
		return false;
	}
	int len = wcslen(headers);
	return WinHttpAddRequestHeaders(mRequest, headers, len, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
}

bool HttpRequest::sendRequest(void *data, int len) {
	bool ok = WinHttpSendRequest(mRequest, NULL, 0, data, len, len, 0);
	ok = ok && WinHttpReceiveResponse(mRequest, 0);
}

bool HttpRequest::getHeader(int queryId, char *buf, int *len) {
	return WinHttpQueryHeaders(mRequest, queryId, 0, buf, (LPDWORD)len, 0);
}

int HttpRequest::read(void *buf, int bufLen) {
	DWORD rd = 0;
	bool ok = WinHttpReadData(mRequest, buf, bufLen, &rd);
	return ok ? rd : -1;
}

bool HttpRequest::close() {
	return WinHttpCloseHandle(mRequest);
}
