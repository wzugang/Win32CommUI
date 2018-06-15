#include "Http.h"
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

static HINTERNET s_session;

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
	XString u, path;
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
	XString host = i > 0 ? u.subString(0, i) : u;
	path = u.subString(i);
	i = host.indexOf(':', 0);
	int port = https ? 443 : 80;
	if (i > 0) {
		XString sp = host.subString(i);
		port = atoi(sp.str());
		host = host.subString(0, i);
	}
	wchar_t *whost = (wchar_t *)XString::toBytes(host.str(), XString::GBK, XString::UNICODE);
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
	wchar_t *method = (wchar_t *)XString::toBytes(mMethod.str(), XString::GBK, XString::UNICODE);
	wchar_t *wpath = (wchar_t *)XString::toBytes(path.str(), XString::GBK, XString::UNICODE);
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
	XString sl = getStatesLine();
	if (sl.length() == 0) {
		return 0;
	}
	char *p = sl.str();
	while (*p != 0 && *p != ' ') ++p;
	return atoi(p + 1);
}

XString HttpConnection::getStatesLine() {
	recieveHeader();
	return mRawResHeaders;
}

int HttpConnection::getContentLength() {
	XString cl = getResponseHeader("Content-Length");
	return atoi(cl.str());
}

XString HttpConnection::getContentType() {
	return getResponseHeader("Content-Type");
}

XString HttpConnection::getContentEncoding() {
	XString ct = getResponseHeader("Content-Type");
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
	XString nv(name);
	nv.append(": ").append(val).append("\r\n");
	wchar_t *hd = (wchar_t *)XString::toBytes(nv.str(), XString::GBK, XString::UNICODE);
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

void HttpConnection::setWriteTotalLength(int len) {
	if (mStatus <= S_CONNECTED) {
		mWriteDataLen = len;
	}
}

void HttpConnection::close() {
	WinHttpCloseHandle(mRequest);
	WinHttpCloseHandle(mConnection);
	mStatus = S_CLOSED;
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
	mRawResHeaders = (char *)XString::toBytes(whd, XString::UNICODE, XString::GBK);
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

XString HttpConnection::getResponseHeader(const char *name) {
	recieveHeader();
	if (mRawResHeaders == NULL || name == NULL || *name == 0) {
		return XString();
	}
	for (int i = 0; i < mResHeaders.size(); i += 2) {
		if (strcmp(mResHeaders.get(i), name) == 0) {
			return mResHeaders.get(i + 1);
		}
	}
	return XString();
}
