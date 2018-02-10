#include "Http.h"
#include "XString.h"

#pragma comment(lib, "winhttp.lib")

HttpSession::HttpSession(const wchar_t *host, int port) {
	mSession = NULL;
	mConnection = NULL;
	mHost = (wchar_t *)malloc((wcslen(host) + 1) * sizeof(wchar_t));
	wcscpy(mHost, host);
	mPort = port;
}

HttpSession::HttpSession(const char *host, int port) {
	mSession = NULL;
	mConnection = NULL;
	mPort = port;
	mHost = (wchar_t *)XString::toBytes((void *)host, XString::GBK, XString::UNICODE);
}

void HttpSession::connect() {
	// userAgent
	mSession = WinHttpOpen(NULL, NULL, NULL, NULL, NULL);
	mConnection = WinHttpConnect(mSession, mHost, (INTERNET_PORT) mPort, 0);
}

HttpRequest * HttpSession::createRequest(const wchar_t *path, Method m, Protocal pro) {
	const wchar_t *method = m == GET ? L"GET" : L"POST";
	// scheme is : INTERNET_SCHEME_HTTPS or INTERNET_SCHEME_HTTP
	DWORD flags = WINHTTP_FLAG_ESCAPE_PERCENT | WINHTTP_FLAG_REFRESH;
	if (pro == HTTPS) {
		flags |= WINHTTP_FLAG_SECURE;
	}
	const wchar_t *acceptTypes[] = {L"*/*", NULL};
	HINTERNET req = WinHttpOpenRequest(mConnection, method, path, NULL, NULL, acceptTypes, flags);
	if (req == NULL) {
		return NULL;
	}
	return new HttpRequest(req);
}

HttpRequest * HttpSession::createRequest(const char *path, Method m, Protocal pro) {
	wchar_t *wpath = (wchar_t *)XString::toBytes((void *)path, XString::GBK, XString::UNICODE);
	HttpRequest *req = createRequest(wpath, m, pro);
	free(wpath);
	return req;
}

void HttpSession::close() {
	WinHttpCloseHandle(mConnection);
	WinHttpCloseHandle(mSession);
}

HttpSession::~HttpSession() {
	free(mHost);
	close();
}

//------------HttpRequest------------------
HttpRequest::HttpRequest(HINTERNET req) {
	mRequest = req;
	mRecevied = false;
}

bool HttpRequest::setReqHeaders(const wchar_t *headers) {
	if (headers == NULL) {
		return false;
	}
	int len = wcslen(headers);
	return WinHttpAddRequestHeaders(mRequest, headers, len, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
}

bool HttpRequest::sendRequest(void *data, int len, int totalLen) {
	// WINHTTP_NO_ADDITIONAL_HEADERS
	if (totalLen < len) {
		totalLen = len;
	}
	bool ok = WinHttpSendRequest(mRequest, NULL, 0, data, len, totalLen, 0);
	ok = ok && WinHttpReceiveResponse(mRequest, 0);
	return ok;
}

int HttpRequest::read(void *buf, int bufLen) {
	DWORD rd = 0;
	bool ok = WinHttpReadData(mRequest, buf, bufLen, &rd);
	return ok ? rd : -1;
}

bool HttpRequest::close() {
	return WinHttpCloseHandle(mRequest);
}

int HttpRequest::getAvailableBytes() {
	int n = 0;
	bool ok = WinHttpQueryDataAvailable(mRequest, (LPDWORD)&n);
	return ok ? n : -1;
}

int HttpRequest::write(void *data, int dataLen) {
	DWORD wn = 0;
	bool ok = WinHttpWriteData(mRequest, data, dataLen, &wn);
	return ok ? wn : -1;
}

wchar_t * HttpRequest::getResHeaders() {
	DWORD len = 0;
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, 
									WINHTTP_HEADER_NAME_BY_INDEX, NULL, &len, WINHTTP_NO_HEADER_INDEX);
	wchar_t *buf = (wchar_t *)malloc(len + 2);
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, buf, &len, NULL);
	return buf;
}

int HttpRequest::getContentLength() {
	wchar_t tmp[24] = {0};
	char tmp2[24] = {0};
	DWORD len = sizeof(tmp);
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_CONTENT_LENGTH, NULL, tmp, &len, NULL);
	wchar_t *s = tmp;
	char *d = tmp2;
	while (*s != 0) {
		*d = (char)*s;
		d++;
		s++;
	}
	return atoi(tmp2);
}

wchar_t * HttpRequest::getResHeader(const wchar_t *name) {
	DWORD len = 0;
	// WINHTTP_QUERY_CONTENT_TRANSFER_ENCODING
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_CUSTOM, name, NULL, &len, NULL);
	if (len <= 0) {
		return NULL;
	}
	wchar_t *buf = (wchar_t *)malloc(len);
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_CUSTOM, name, buf, &len, NULL);
	return buf;
}

int HttpRequest::getStatusCode() {
	wchar_t tmp[8] = {0};
	DWORD len = sizeof(tmp);
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, tmp, &len, NULL);
	char tmp2[8];
	for (int i = 0; i < 8; ++i) {
		tmp2[i] = (char)tmp[i];
	}
	return atoi(tmp2);
}

char *HttpRequest::getContentType() {
	wchar_t *ct = getResHeader(L"Content-Type");
	char *cc = (char *)XString::toBytes((void *)ct, XString::UNICODE, XString::GBK);
	if (ct != NULL) {
		free(ct);
	}
	return cc;
}

HttpRequest::Encode HttpRequest::getEncode() {
	Encode e = UNKNOW;
	char *ct = getContentType();
	if (ct == NULL) {
		return UNKNOW;
	}
	char *p = ct;
	while (*p) {
		*p = toupper(*p);
		++p;
	}
	p = strstr(ct, "CHARSET");
	if (p == NULL) {
		goto _end;
	}
	while (*p != '=' && *p != 0) {
		++p;
	}
	if (*p == '=') ++p;
	while (*p != 0 && (*p == ' ' || *p == '\t')) {
		++p;
	}
	if (memcmp(p, "GBK", 3) == 0) {
		e = GBK;
	} else if (memcmp(p, "UTF8", 3) == 0 || memcmp(p, "UTF-8", 4) == 0) {
		e = UTF8;
	}

	_end:
	free(ct);
	return e;
}
