#include "XmlParser.h"
#include <string.h>
#include <ctype.h>

XmlNode::XmlNode( char *name , XmlNode *parent) {
	mName = name;
	mParent = parent;
	mComponent = NULL;
	mParser = NULL;
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
	// do not delete children
	if (mParser != NULL)
		delete mParser;
	mParser = NULL;
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

//----------------------------XmlParser---------------------
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

XmlParser *XmlParser::create() {
	return new XmlParser();
}

void XmlParser::parseString( const char *xml, int xmlLen) {
	reset();
	if (xml == NULL || *xml == 0) {
		strcpy(mError, "xml is null");
		mHasError = true;
		return;
	}
	int len = xmlLen < 0 ? strlen(xml) : xmlLen;
	mXml = new char[len + 1];
	strcpy(mXml, xml);
	mXmlLen = len;
	doParse();
	// replace all include node
	replaceAllIncludeNode(mRoot);
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

#if 0
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
#endif

XmlNode * XmlParser::getRoot() {
	if (mRoot != NULL) {
		mRoot->mParser = this;
	}
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
	delete [] mXml;
}

bool XmlParser::hasError() {
	return mHasError;
}

void XmlParser::replaceIncludeNode( XmlNode *n ) {
	char *resPath = n->getAttrValue("src");
	char *part = n->getAttrValue("part");
	XmlPartLoader *loader = NULL;
	XmlParser *parser = NULL;
	XmlNode *inclueRoot = NULL;
	if (resPath == NULL || *resPath == 0 || part == NULL || *part == 0)
		goto _end;
	// build include node
	loader = XmlPartLoader::fetch(resPath);
	if (loader == NULL) goto _end;
	XmlPartLoader::PartItem *item = loader->getPartXml(part);
	if (item == NULL) goto _end;
	parser = XmlParser::create();
	parser->parseString(item->mContent, item->mCntLen);
	if (parser->hasError()) goto _end;
	inclueRoot = parser->getRoot();
_end:
	// now replace the include node
	XmlNode *parent = n->getParent();
	int idx = 0;
	for (int i = 0; i < parent->mChildren.size(); ++i) {
		if (parent->mChildren[i] == n) {
			idx = i;
			break;
		}
	}
	parent->mChildren.erase(parent->mChildren.begin() + idx);
	if (inclueRoot != NULL) {
		parent->mChildren.insert(parent->mChildren.begin() + idx, inclueRoot);
		inclueRoot->mParent = parent;
	}
}

void XmlParser::replaceAllIncludeNode( XmlNode *n ) {
	if (n == NULL) return;
	for (int i = 0; i < n->mChildren.size(); ++i) {
		XmlNode *child = n->mChildren.at(i);
		if (strcmp(child->mName, "include") == 0) {
			replaceIncludeNode(child);
			--i;
		} else {
			replaceAllIncludeNode(child);
		}
	}
}

//---------------------------XmlPartLoader---------------------------
static XmlPartLoader *mPartCache[20];

XmlPartLoader * XmlPartLoader::fetch( const char *resPath ) {
	if (resPath == NULL)
		return NULL;
	for (int i = 0; i < sizeof(mPartCache)/sizeof(XmlPartLoader*); ++i) {
		if (mPartCache[i] != NULL && strcmp(mPartCache[i]->mResPath, resPath) == 0)
			return mPartCache[i];
	}
	XmlPartLoader *ldr = new XmlPartLoader(resPath);
	// save in cache
	for (int i = 0; i < sizeof(mPartCache)/sizeof(XmlPartLoader*); ++i) {
		if (mPartCache[i] == NULL) {
			mPartCache[i] = ldr;
			break;
		}
	}
	return ldr;
}

XmlPartLoader::XmlPartLoader( const char *filePath ) {
	mContent = NULL;
	mPartNum = 0;
	strcpy(mResPath, filePath);
	if (memcmp(filePath, "file://", 7) == 0) {
		mContent = ReadFileContent(filePath + 7, &mContentLen);
	} else if (memcmp(filePath, "res://", 6) == 0) {
		// TODO:
	}
	doParse();
}

XmlPartLoader::PartItem * XmlPartLoader::getPartXml( const char *name ) {
	if (name == NULL || *name == 0)
		return NULL;
	for (int i = 0; i < mPartNum; ++i) {
		if (strcmp(mPartItems[i].mName, name) == 0)
			return &mPartItems[i];
	}
	return NULL;
}

XmlPartLoader::PartItem * XmlPartLoader::getPartXml( int idx ) {
	if (idx >= mPartNum) return NULL;
	return &mPartItems[idx];
}

void XmlPartLoader::doParse() {
	if (mContent == NULL) return;
	char *p = mContent;
	char *EP = mContent + mContentLen;
	while (p < EP) {
		char *ap = strstr(p, "#####");
		if (ap == NULL) break;
		if (ap != mContent && ap[-1] != '\n') {
			p = ap + 5;
			continue;
		}
		*ap = 0;
		p = ap + 5;
		while (p < EP && (isspace(*p) || *p == '#')) ++p;
		if (p >= EP) break;
		char *sp = p;
		while (p < EP && !isspace(*p) && *p != '#') ++p;
		*p = 0;
		mPartItems[mPartNum].mName = sp;
		++p;
		while (p < EP && *p != '\n') ++p;
		if (p >= EP) break;
		mPartItems[mPartNum].mContent = p;
		++mPartNum;
	}
	if (p == mContent) {
		mPartItems[mPartNum].mName = NULL;
		mPartItems[mPartNum].mContent = mContent;
		++mPartNum;
	}
	for (int i = 0; i < mPartNum; ++i) {
		mPartItems[i].mCntLen = strlen(mPartItems[i].mContent);
	}
}

XmlPartLoader::PartItem * XmlPartLoader::findPartXml( const char *name ) {
	// find in cache
	for (int i = 0; i < sizeof(mPartCache)/sizeof(XmlPartLoader*); ++i) {
		if (mPartCache[i] == NULL) continue;
		PartItem *dat = mPartCache[i]->getPartXml(name);
		if (dat != NULL) return dat;
	}
	return NULL;
}

XmlPartLoader::~XmlPartLoader() {
	if (mContent) delete[] mContent;
	// remove in cache
	for (int i = 0; i < sizeof(mPartCache)/sizeof(XmlPartLoader*); ++i) {
		if (mPartCache[i] == this) {
			mPartCache[i] = NULL;
			break;
		}
	}
}
