#include "XmlParser.h"
#include <string.h>
#include <ctype.h>

XmlNode::XmlNode( char *name , XmlNode *parent) {
	mName = name;
	mParent = parent;
	mComponent = NULL;
}

char * XmlNode::getName() {
	return mName;
}

int XmlNode::getChildCount() {
	return mChildren.size();
}
XmlNode* XmlNode::getChild( int idx ) {
	if (idx >= getChildCount())
		return NULL;
	return mChildren.at(idx);
}

int XmlNode::getAttrsCount() {
	return mAttrs.size();
}

XmlNode::Attr* XmlNode::getAttr( int idx ) {
	if (idx >= getAttrsCount())
		return NULL;
	return & mAttrs.at(idx);
}

char * XmlNode::getAttrValue( const char *name ) {
	if (name == NULL) 
		return NULL;
	for (int i = 0; i < mAttrs.size(); ++i) {
		const Attr &a = mAttrs.at(i);
		if (strcmp(name, a.mName) == 0)
			return a.mValue;
	}
	return NULL;
}

void XmlNode::addChild( XmlNode *n ) {
	mChildren.push_back(n);
}

void XmlNode::addAttr( char *name, char *val ) {
	Attr a;
	a.mName = name;
	a.mValue = val;
	mAttrs.push_back(a);
}

XmlNode::~XmlNode() {
	/*for (int i = 0; i < mChildren.size(); ++i) {
		delete mChildren.at(i);
	}*/
}

void XmlNode::print(int step) {
	for (int i = 0; i < step; ++i) {
		printf("   ");
	}
	printf("{ %s ", mName);
	for (int i = 0; i < getAttrsCount(); ++i) {
		printf("[%s=%s] ", getAttr(i)->mName, getAttr(i)->mValue);
	}
	printf("} \n");
	for (int i = 0; i < getChildCount(); ++i) {
		getChild(i)->print(step + 1);
	}
}

void XmlNode::setComponent( XComponent *c ) {
	mComponent = c;
}

XComponent* XmlNode::getComponent() {
	return mComponent;
}

XmlNode* XmlNode::findById( const char *id ) {
	if (id == NULL || *id == 0)
		return NULL;
	char *v = getAttrValue("id");
	if (v != NULL && strcmp(v, id) == 0)
		return this;
	for (int i = 0; i < getChildCount(); ++i) {
		XmlNode *cc = getChild(i)->findById(id);
		if (cc) return cc;
	}
	return NULL;
}

XmlNode* XmlNode::getChildById( const char *id ) {
	if (id == NULL || *id == 0)
		return NULL;
	for (int i = 0; i < getChildCount(); ++i) {
		XmlNode *cc = getChild(i);
		char *v = cc->getAttrValue("id");
		if (v != NULL && strcmp(v, id) == 0)
			return cc;
	}
	return NULL;
}

//-------------------------------------------------


XmlParser::XmlParser() {
	mError = new char[256];
	reset();
}

void XmlParser::reset() {
	mXml = NULL;
	*mError = 0;
	mRoot = NULL;
	mXmlLen = 0;
	mPos = 0;
	mHasError = false;
}

void XmlParser::parseString( const char *xml ) {
	reset();
	if (xml == NULL || *xml == 0) {
		strcpy(mError, "xml is null");
		mHasError = true;
		return;
	}
	int len = strlen(xml);
	mXml = new char[len + 1];
	strcpy(mXml, xml);
	mXmlLen = len;
	doParse();
}

static char *ReadFileContent(const char *path, int *pLen) {
	if (pLen != NULL) *pLen = 0;
	char *cnt = NULL;
	if (path == NULL) {
		return NULL;
	}
	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	cnt = new char[len + 1];
	fseek(f, 0, SEEK_SET);
	int rlen = 0;
	char *p = cnt;
	while (rlen < len) {
		int n = fread(cnt + rlen, 1, len - rlen, f);
		if (n <= 0) break;
		rlen += n;
	}
	fclose(f);
	if (rlen == len) {
		cnt[rlen] = 0;
		if (pLen != NULL) *pLen = len;
		return cnt;
	}
	delete[] cnt;
	return NULL;
}

void XmlParser::parseFile( const char *path ) {
	reset();
	mXml = ReadFileContent(path, &mXmlLen);
	if (mXml == NULL) {
		sprintf(mError, "Read file fail: %s", path);
		mHasError = true;
		return;
	}
	doParse();
}

XmlNode * XmlParser::getRoot() {
	return mRoot;
}

char * XmlParser::getError() {
	return mError;
}

void XmlParser::skipSpace() {
	while (mPos < mXmlLen && isspace(mXml[mPos])) ++mPos;
}

void XmlParser::doParse() {
	mRoot = parseNode(NULL);
	XmlNode *parent = mRoot;
	while (!mHasError && parent != NULL) {
		parent = parseNode(parent);
	}
}

XmlNode* XmlParser::parseNode(XmlNode *parent) {
	skipSpace();
	if (mPos >= mXmlLen) {
		return NULL; // is end
	}
	if (mXml[mPos] != '<') {
		sprintf(mError, "Except '<' near: %s", getNearText(mPos));
		mHasError = true;
		return NULL;
	}
	++mPos;
	// is  end </ ...> ?
	if (mXml[mPos] == '/') {
		endNode(parent);
		if (mHasError) return NULL;
		if (parent != NULL)
			return parent->getParent();
		return NULL;
	}

	skipSpace();
	char *name = mXml + mPos;
	int px = nextWord();
	if (px == -1) {
		sprintf(mError, "Except a word near: %s", getNearText(mPos));
		mHasError = true;
		return NULL;
	}
	mPos = px;
	mXml[mPos] = 0;
	XmlNode *node = new XmlNode(name, parent);
	// parse attrs
	while (mPos < mXmlLen) {
		++mPos;
		skipSpace();
		char *an = mXml + mPos;
		if (*an == '>') {
			mPos++;
			if (parent != NULL) {
				parent->addChild(node);
			}
			return node;
		}
		if (*an == '/') {
			if (an[1] == '>') {
				mPos += 2;
				if (parent != NULL) {
					parent->addChild(node);
				} else {
					return node;
				}
				return parent;
			} else {
				sprintf(mError, "Except '>' near: %s", getNearText(mPos));
				mHasError = true;
				return NULL;
			}
		}
		int p1 = nextWord();
		if (p1 == -1) {
			sprintf(mError, "Except a word near: %s", getNearText(mPos));
			mHasError = true;
			return NULL;
		}
		char ch = mXml[p1];
		mXml[p1] = 0;
		if (ch == '=') {
			mPos = p1 + 1;
		} else {
			int tp = mPos;
			mPos = p1 + 1;
			skipSpace();
			if (mPos >= mXmlLen || mXml[mPos] != '=') {
				sprintf(mError, "Except a word near: %s", getNearText(tp));
				mHasError = true;
				return NULL;
			}
			++mPos;
		}
		skipSpace();
		if (mPos >= mXmlLen || mXml[mPos] != '"') {
			sprintf(mError, "Except \" near: %s", getNearText(mPos));
			mHasError = true;
			return NULL;
		}
		++mPos;
		char *av = mXml + mPos;
		int p2 = nextTo('"');
		if (p2 == -1) {
			sprintf(mError, "Except \" near: %s", getNearText(mPos));
			mHasError = true;
			return NULL;
		}
		mXml[p2] = 0;
		mPos = p2;
		node->addAttr(an, av);
	}
	sprintf(mError, "Except a > to end node");
	mHasError = true;
	return NULL;
}

void XmlParser::endNode(XmlNode *parent) {
	if (mXml[mPos] != '/') {
		return;
	}
	if (parent == NULL) {
		sprintf(mError, "Error near: %s", getNearText(mPos));
		mHasError = true;
		return;
	}
	++mPos;
	skipSpace();
	char *ac = mXml + mPos;
	int tpx = nextWord();
	if (tpx == -1) {
		sprintf(mError, "Except '%s' near: %s", parent->getName(), getNearText(mPos));
		mHasError = true;
		return;
	}
	if (memcmp(parent->getName(), ac, tpx - mPos) != 0) {
		sprintf(mError, "Except '%s' near: %s", parent->getName(), getNearText(mPos));
		mHasError = true;
		return;
	}
	mPos = tpx;
	if (mPos >= mXmlLen) {
		sprintf(mError, "Except '>' to END", getNearText(mPos));
		mHasError = true;
		return;
	}
	skipSpace();
	if (mXml[mPos] != '>') {
		sprintf(mError, "Except '>' near: %s", getNearText(mPos));
		mHasError = true;
		return;
	}
	++mPos;
}

char * XmlParser::getNearText( int pos ) {
	static char tmp[32] = {0};
	int ml = mXmlLen - pos;
	if (ml > 30) ml = 30;
	memcpy(tmp, mXml + pos, ml);
	return tmp;
}

int XmlParser::nextTo( char ch ) {
	int pos = mPos;
	while (mPos < mXmlLen && mXml[pos] != ch) ++pos;
	if (mXml[pos] == ch) return pos;
	return -1;
}

int XmlParser::nextWord() {
	int pos = mPos;
	if (pos >= mXmlLen)
		return -1;
	while (pos < mXmlLen && isalnum(mXml[pos])) ++pos;
	return pos;
}

XmlParser::~XmlParser() {
	// delete [] mXml;
}

bool XmlParser::hasError() {
	return mHasError;
}

XmlPartLoader::XmlPartLoader( const char *filePath ) {
	mPartNum = 0;
	mContent = ReadFileContent(filePath, &mContentLen);
	doParse();
}

char * XmlPartLoader::getPartXml( const char *name ) {
	if (name == NULL || *name == 0)
		return NULL;
	for (int i = 0; i < mPartNum; ++i) {
		if (strcmp(mPartItems[i].mName, name) == 0)
			return mPartItems[i].mContent;
	}
	return NULL;
}

char * XmlPartLoader::getPartXml( int idx ) {
	if (idx >= mPartNum) return NULL;
	return mPartItems[idx].mContent;
}

void XmlPartLoader::doParse() {
	if (mContent == NULL) return;
	char *p = mContent;
	char *EP = mContent + mContentLen;
	while (p < EP) {
		char *ap = strstr(p, "#####");
		if (ap == NULL) break;
		*ap = 0;
		p = ap + 5;
		while (p < EP && (isspace(*p) || *p == '#')) ++p;
		if (p >= EP) break;
		char *sp = p;
		while (p < EP && !isspace(*p) && *p != '#') ++p;
		*p = 0;
		mPartItems[mPartNum].mName = sp;
		++p;
		while (p < EP && (isspace(*p) || *p == '#')) ++p;
		if (p >= EP) break;
		mPartItems[mPartNum].mContent = p;
		++mPartNum;
	}
	if (p == mContent) {
		mPartItems[mPartNum].mName = NULL;
		mPartItems[mPartNum].mContent = mContent;
		++mPartNum;
	}
}

XmlPartLoader::~XmlPartLoader() {
	if (mContent) delete[] mContent;
}
