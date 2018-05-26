#include "Http.h"
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

static HINTERNET s_session;

#if 0

int HttpRequest::getAvailableBytes() {
	int n = 0;
	bool ok = WinHttpQueryDataAvailable(mRequest, (LPDWORD)&n);
	return ok ? n : -1;
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

#endif

HttpConnection::HttpConnection(const char *url, const char *method) {
	mUrl = url;
	mMethod = method;
	mConnection = NULL;
	mRequest = NULL;
	mStatus = S_NONE;
	mRawResHeaders = NULL;
	mWriteDataLen = 0;
}

bool HttpConnection::connect() {
	if (s_session == NULL) {
		s_session = WinHttpOpen(L"HTTP_SESSION", NULL, NULL, NULL, NULL);
	}
	if (s_session == NULL) {
		return false;
	}

	// 1.connect to server
	String u, path;
	bool https = false;
	if (mUrl.startWith("http://")) {
		https = false;
		u = mUrl.subString(7);
	} else if (mUrl.startWith("https://")) {
		https = true;
		u = mUrl.subString(8);
	} else {
		https = false;
		u = mUrl;
	}
	
	int i = u.indexOf('/', 0);
	if (i == 0) {
		return false;
	}
	String host = i > 0 ? u.subString(0, i) : u;
	path = u.subString(i);
	i = host.indexOf(':', 0);
	int port = https ? 443 : 80;
	if (i > 0) {
		String sp = host.subString(i);
		port = atoi(sp.str());
		host = host.subString(0, i);
	}
	wchar_t *whost = (wchar_t *)String::toBytes(host.str(), String::GBK, String::UNICODE);
	mConnection = WinHttpConnect(s_session, whost, (INTERNET_PORT) port, 0);
	free(whost);

	if (mConnection == NULL) {
		return false;
	}

	// 2. send a request
	DWORD flags = WINHTTP_FLAG_ESCAPE_PERCENT | WINHTTP_FLAG_REFRESH;
	if (https) {
		flags |= WINHTTP_FLAG_SECURE;
	}
	const wchar_t *acceptTypes[] = {L"*/*", NULL};
	wchar_t *method = (wchar_t *)String::toBytes(mMethod.str(), String::GBK, String::UNICODE);
	wchar_t *wpath = (wchar_t *)String::toBytes(path.str(), String::GBK, String::UNICODE);
	HINTERNET req = WinHttpOpenRequest(mConnection, method, wpath, NULL, NULL, acceptTypes, flags);
	free(method);
	free(wpath);
	if (req == NULL) {
		return false;
	}
	mRequest = req;
	mStatus = S_CONNECTED;

	if (https) {
		DWORD dwFlags = 0;
		DWORD dwBuffLen = sizeof(dwFlags);
		WinHttpQueryOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, &dwBuffLen);
		dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
		dwFlags |= SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
		dwFlags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
		// dwFlags |= SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
		WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
	}

	return true;
}

void HttpConnection::setConnectTimeout(int timeout) {
	if (timeout > 0 && mConnection != NULL) {
		WinHttpSetOption(mConnection, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
	}
}

void HttpConnection::setReadTimeout(int timeout) {
	if (timeout > 0 && mConnection != NULL) {
		WinHttpSetOption(mConnection, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &timeout, sizeof(timeout));
	}
}

int HttpConnection::getResponseCode() {
	String sl = getStatesLine();
	if (sl.length() == 0) {
		return 0;
	}
	char *p = sl.str();
	while (*p != 0 && *p != ' ') ++p;
	return atoi(p + 1);
}

String HttpConnection::getStatesLine() {
	recieveHeader();
	return mRawResHeaders;
}

int HttpConnection::getContentLength() {
	String cl = getResponseHeader("Content-Length");
	return atoi(cl.str());
}

String HttpConnection::getContentType() {
	return getResponseHeader("Content-Type");
}

String HttpConnection::getContentEncoding() {
	String ct = getResponseHeader("Content-Type");
	ct.toUpperCase();
	int idx = ct.indexOf("CHARSET=", 0);
	if (idx > 0) {
		ct = ct.subString(idx + 8);
		if (ct == "UTF8") ct = "UTF-8";
		return ct;
	}
	return "";
}

bool HttpConnection::setRequestHeader(const char *name, const char *val) {
	if (name == NULL || mRequest == NULL || mStatus != S_CONNECTED) {
		return false;
	}
	String nv(name);
	nv.append(": ").append(val).append("\r\n");
	wchar_t *hd = (wchar_t *)String::toBytes(nv.str(), String::GBK, String::UNICODE);
	int len = wcslen(hd);
	return WinHttpAddRequestHeaders((HINTERNET)mRequest, hd, len, 
		WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE) == TRUE;
}

int HttpConnection::read(void *buf, int len) {
	send();
	recieve();
	if (mStatus != S_RECEVICED) {
		return -1;
	}
	DWORD rd = 0;
	bool ok = WinHttpReadData(mRequest, buf, len, &rd) == TRUE;
	return ok ? rd : -1;
}

int HttpConnection::write(void *buf, int len) {
	send();
	if (mStatus != S_SEND_REQUESTED) {
		return -1;
	}
	DWORD wn = 0;
	bool ok = WinHttpWriteData(mRequest, buf, len, &wn) == TRUE;
	return ok ? wn : -1;
}

void HttpConnection::send() {
	if (mStatus == S_CONNECTED) {
		if (WinHttpSendRequest(mRequest, NULL, 0, NULL, 0, mWriteDataLen, 0) == TRUE) {
			mStatus = S_SEND_REQUESTED;
		}
	}
}

void HttpConnection::recieve() {
	if (mStatus == S_SEND_REQUESTED) {
		if (WinHttpReceiveResponse(mRequest, 0) == TRUE) {
			mStatus = S_RECEVICED;
		}
	}
}

void HttpConnection::setWriteDataLen(int len) {
	if (mStatus <= S_CONNECTED) {
		mWriteDataLen = len;
	}
}

void HttpConnection::close() {
	WinHttpCloseHandle(mRequest);
	WinHttpCloseHandle(mConnection);
}

void HttpConnection::recieveHeader() {
	send();
	recieve();
	if (S_RECEVICED != mStatus || mRawResHeaders != NULL) {
		return;
	}
	DWORD sz = 0;
	WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, 
		WINHTTP_HEADER_NAME_BY_INDEX, NULL, &sz, WINHTTP_NO_HEADER_INDEX);
	wchar_t *whd = (wchar_t*)malloc(sz + 2);
	bool ok = WinHttpQueryHeaders(mRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, 
		WINHTTP_HEADER_NAME_BY_INDEX, whd, &sz, WINHTTP_NO_HEADER_INDEX) == TRUE;
	if (! ok) {
		free(whd);
		return;
	}
	mRawResHeaders = (char *)String::toBytes(whd, String::UNICODE, String::GBK);
	free(whd);

	// split every row
	char *p = mRawResHeaders;
	char *EOP = mRawResHeaders + strlen(mRawResHeaders);

	// skip status line
	p = strstr(p, "\r\n");
	*p = 0;
	p += 2;

	while (p != NULL && p < EOP) {
		char *ep = strstr(p, "\r\n");
		if (ep == NULL) {
			break;
		}
		*ep = 0;
		ep += 2;
		if (ep >= EOP) {
			break;
		}
		char *dp = strchr(p, ':');
		*dp = 0;
		mResHeaders.add(p);
		char *vp = dp + 1;
		while (*(--dp) == ' ') {*dp = 0;}
		while (*vp == ' ') ++vp;
		mResHeaders.add(vp);

		p = ep;
	}
}

String HttpConnection::getResponseHeader(const char *name) {
	recieveHeader();
	if (mRawResHeaders == NULL || name == NULL || *name == 0) {
		return String();
	}
	for (int i = 0; i < mResHeaders.size(); i += 2) {
		if (strcmp(mResHeaders.get(i), name) == 0) {
			return mResHeaders.get(i + 1);
		}
	}
	return String();
}
