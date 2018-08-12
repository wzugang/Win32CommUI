#include "HttpClient.h"
#include "curl/curl.h"

#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wldap32.lib")

int HttpClient::mInitedNum;

class Buffer {
public:
	Buffer(int capacity) {
		mCapacity = capacity;
		mLen = 0;
		mBuf = NULL;
		if (capacity > 0) {
			mBuf = (char *)malloc(mCapacity);
		}
	}

	void append(void *data, int len) {
		if (len <= 0) {
			return;
		}
		if (mCapacity - mLen - 4 < len) {
			mCapacity *= 2;
		}
		if (mCapacity - mLen - 4 < len) {
			mCapacity = mLen + len + 4;
		}
		mBuf = (char *)realloc(mBuf, mCapacity);
		memcpy(mBuf + mLen, data, len);
		mLen += len;
		mBuf[mLen] = 0;
		mBuf[mLen + 1] = 0;
	}

	~Buffer() {
		if (mBuf != NULL) {
			free(mBuf);
		}
	}
	int mCapacity;
	int mLen;
	char *mBuf;
};

HttpRequest::HttpRequest(const char *url, Method method) {
	mCurl = NULL;
	mUrl = url;
	mMethod = method;
	mConnectTimeout = 0;
	mReadTimeout = 0;
	mReqHeaders = NULL;
	mBodyLength = 0;
	mBody = NULL;
	mContentBuf = new Buffer(0);
	mResHeadersBuf = new Buffer(4 * 1024);
}

void HttpRequest::setConnectTimeout(int milis) {
	mConnectTimeout = milis;
}

void HttpRequest::setReadTimeout(int milis) {
	mReadTimeout = milis;
}

void HttpRequest::addHeader(const char *name, const char *val) {
	static char hdLine[1024];
	if (name == NULL || val == NULL) {
		return;
	}
	char *p = hdLine;
	int len = strlen(name) + 3 + strlen(val);
	if (len > sizeof(hdLine)) {
		p = (char *)malloc(len);
	}
	*p = 0;
	strcat(p, name);
	strcat(p, ": ");
	strcat(p, val);
	mReqHeaders = curl_slist_append((curl_slist *)mReqHeaders, p);
	if (p != hdLine) {
		free(p);
	}
}

void HttpRequest::delHeader(const char *name) {
	static char sName[256];
	if (mReqHeaders == NULL || name == NULL) {
		return;
	}
	sName[0] = 0;
	strcat(sName, name);
	strcat(sName, ": ");
	mReqHeaders = curl_slist_append((curl_slist *)mReqHeaders, sName);
}

void HttpRequest::setBody(void *body, int bodyLength) {
	mBody = body;
	mBodyLength = bodyLength;
}

int HttpRequest::getResponseCode() {
	if (mCurl != NULL) {
		long code = 0;
		CURLcode res = curl_easy_getinfo((CURL *)mCurl, CURLINFO_RESPONSE_CODE, &code);
		return code;
	}
	return 0;
}

int HttpRequest::getContentLength() {
	if (mCurl != NULL) {
		curl_off_t len = 0;
		// CURLINFO_SIZE_DOWNLOAD_T  // Number of bytes downloaded
		CURLcode res = curl_easy_getinfo((CURL *)mCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &len);
		return len;
	}
	return 0;
}

XString HttpRequest::getResponseHeader(const char *name) {
	if (name == NULL) {
		return "";
	}
	Array<char*> *arr = getResponseHeaders();
	for (int i = 0; i < arr->size() - 1; i += 2) {
		if (strcmp(name, arr->get(i)) == 0) {
			return arr->get(i + 1);
		}
	}
	return "";
}

Array<char*> *HttpRequest::getResponseHeaders() {
	if (mResHeadersArray.size() == 0) {
		Buffer *hd = (Buffer *)mResHeadersBuf;
		// skip status line
		char *sp = strstr(hd->mBuf, "\r\n") + 2;
		while (TRUE) {
			char *ep = strstr(sp, "\r\n");
			*ep = 0;
			char *n = strchr(sp, ':');
			*n = 0;
			++n;
			if (*n == ' ') ++n;
			mResHeadersArray.add(sp);
			mResHeadersArray.add(n);
			ep += 2;
			if (*ep == '\r' || *ep == 0) {
				break;
			}
			sp = ep;
		}
	}
	return &mResHeadersArray;
}

HttpRequest::~HttpRequest() {
	if (mReqHeaders != NULL) {
		curl_slist_free_all((curl_slist *)mReqHeaders);
	}
	mReqHeaders = NULL;
	Buffer *b = (Buffer *)mResHeadersBuf;
	delete b;
	b = (Buffer *)mContentBuf;
	delete b;
}


static size_t http_data_writer(void* data, size_t size, size_t nmemb, void* content) {
	Buffer *buf = (Buffer *)content;
	buf->append(data, size * nmemb);
	return size * nmemb;
}

static size_t http_header_writer(void* data, size_t size, size_t nmemb, void* content) {
	Buffer *buf = (Buffer *)content;
	buf->append(data, size * nmemb);
	char *p = (char *)data;
	return size * nmemb;
}
//----------------------------------------------------
HttpClient::HttpClient() {
	++mInitedNum;
	if (mInitedNum == 1) {
		curl_global_init(CURL_GLOBAL_ALL);
	}
	mCurl = curl_easy_init();
}

int HttpClient::execute(HttpRequest *req) {
	if (req == NULL || mCurl == NULL) {
		return 0;
	}
	req->mCurl = mCurl;
	curl_easy_setopt((CURL *)mCurl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt((CURL *)mCurl, CURLOPT_URL, req->mUrl.str());
	if (req->mMethod == HttpRequest::METHOD_POST) {
		// curl_easy_setopt((CURL *)mCurl, CURLOPT_POST, TRUE); // 发送类型为application/x-www-form-urlencoded
		curl_easy_setopt((CURL *)mCurl, CURLOPT_POSTFIELDSIZE, req->mBodyLength);
		curl_easy_setopt((CURL *)mCurl, CURLOPT_POSTFIELDS, req->mBody);
	}
	curl_easy_setopt((CURL *)mCurl, CURLOPT_HTTPHEADER, req->mReqHeaders);
	// curl_easy_setopt((CURL *)mCurl, CURLOPT_HEADER, TRUE); //返回response头部信息
	if (req->mConnectTimeout >= 1000) {
		curl_easy_setopt((CURL *)mCurl, CURLOPT_CONNECTTIMEOUT, req->mConnectTimeout / 1000);
	}
	if (req->mReadTimeout >= 1000) {
		curl_easy_setopt((CURL *)mCurl, CURLOPT_TIMEOUT, req->mReadTimeout / 1000);
	}
	curl_easy_setopt((CURL *)mCurl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1); // http 1.1

	curl_easy_setopt((CURL *)mCurl, CURLOPT_WRITEFUNCTION, http_data_writer);
	curl_easy_setopt((CURL *)mCurl, CURLOPT_WRITEDATA, req->mContentBuf);
	curl_easy_setopt((CURL *)mCurl, CURLOPT_HEADERFUNCTION, http_header_writer);
	curl_easy_setopt((CURL *)mCurl, CURLOPT_HEADERDATA, req->mResHeadersBuf);
	
	CURLcode res = curl_easy_perform((CURL *)mCurl);
	if (res != CURLE_OK) {
		return 0;
	}
	return req->getResponseCode();
}

void HttpClient::close() {
	if (mCurl != NULL) {
		curl_easy_cleanup((CURL *)mCurl);
	}
	mCurl = NULL;
}

HttpClient::~HttpClient() {
	close();
	--mInitedNum;
	if (mInitedNum == 0) {
		curl_global_cleanup();
	}
}
