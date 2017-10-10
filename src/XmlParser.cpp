#include "XmlParser.h"
#include <string.h>
#include <ctype.h>
#include "XComponent.h"

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

char * AttrUtils::trim( char *str ) {
	if (str == NULL) return NULL;
	while (isspace(*str)) ++str; // trim left
	char *p = str + strlen(str) - 1;
	if (p < str) return str;
	while (p >= str && isspace(*p)) {
		*p = 0;
		--p;
	}
	return str;
}

bool AttrUtils::parseFont(LOGFONT *font, char *str) {
	if (str == NULL || *str == 0) return false;
	bool changed = false;
	char *p = strstr(str, "name:");
	char *p2 = NULL;
	if (p != NULL) {
		p2 = strchr(p, ';');
		if (p2 && p2 - p - 5 <= sizeof(font->lfFaceName - 1)) {
			memcpy(font->lfFaceName, p + 5, p2 - p - 5);
			font->lfFaceName[p2 - p - 5] = 0;
			changed = true;
		} else {
			if (strlen(p + 5) < sizeof(font->lfFaceName - 1)) {
				strcpy(font->lfFaceName, p + 5);
				changed = true;
			}
		}
	}

	p = strstr(str, "size:");
	if (p != NULL) {
		int s = atoi(p + 5);
		if (s != 0) {
			font->lfHeight = s;
			changed = true;
		}
	}
	p = strstr(str, "weight:");
	if (p != NULL) {
		int s = atoi(p + 7);
		if (s > 0) {
			font->lfWeight = s;
			changed = true;
		}
	}

	if (strstr(str, "italic") != NULL) {
		font->lfItalic = TRUE;
		changed = true;
	}
	if (strstr(str, "underline") != NULL) {
		font->lfUnderline = TRUE;
		changed = true;
	}
	if (strstr(str, "strikeout") != NULL) {
		font->lfStrikeOut = TRUE;
		changed = true;
	}
	return changed;
}

int AttrUtils::parseSize(const char *str) {
	char *ep = NULL;
	int v = (int)strtod(str, &ep);
	if (str == ep && strstr(str, "auto") != NULL) {
		return XComponent::MS_AUTO;
	}
	if (ep != NULL && *ep == '%') {
		if (v < 0) v = -v;
		v = v | XComponent::MS_PERCENT;
	} else {
		v = v | XComponent::MS_FIX;
	}
	return v;
}

void AttrUtils::parseArraySize(const char *str, int *arr, int arrNum) {
	for (int i = 0; i < arrNum; ++i) {
		while (*str == ' ') ++str;
		arr[i] = parseSize(str);
		while (*str != ' ' && *str) ++str;
	}
}

int AttrUtils::parseInt(const char *str) {
	return (int)strtod(str, NULL);
}

void AttrUtils::parseArrayInt(const char *str, int *arr, int arrNum) {
	for (int i = 0; i < arrNum; ++i) {
		while (*str == ' ') ++str;
		arr[i] = parseInt(str);
		while (*str != ' ' && *str) ++str;
	}
}

COLORREF AttrUtils::parseColor(const char *color, bool *valid) {
	static const char *HEX = "0123456789ABCDEF";
	if (valid) *valid = false;
	if (color == NULL) {
		if (valid) *valid = false;
		return 0;
	}
	while (*color == ' ') ++color;
	if (*color != '#') {
		if (valid) *valid = false;
		return 0;
	}
	++color;
	COLORREF cc = 0;
	for (int i = 0; i < 3; ++i) {
		char *p0 = strchr((char *)HEX, toupper(color[i * 2]));
		char *p1 = strchr((char *)HEX, toupper(color[i * 2 + 1]));
		if (p0 == NULL || p1 == NULL) {
			if (valid) *valid = false;
			return 0;
		}
		cc |= ((p0 - HEX) * 16 + (p1 - HEX)) << i * 8;
	}
	if (valid) *valid = true;
	return cc;
}

std::vector<char*> AttrUtils::splitBy( char *data, char splitChar) {
	std::vector<char*> arr;
	if (data == NULL || *data == 0)
		return arr;
	char *ps = data;
	char *end = data + strlen(data);
	while (ps <= end && *ps != 0) {
		char *p = strchr(ps, splitChar);
		if (p != NULL) {
			*p = 0; 
		}
		arr.push_back(ps);
		ps = p + 1;
		if (p == NULL) break;
	}
	return arr;
}

ResPath::ResPath() {
	mPath[0] = 0;
	mX = mY = mWidth = mHeight = 0;
	mHasRect = false;
	mResType = RT_NONE;
	mValidate = false;
	mRepeatX = mRepeatY = false;
}

bool ResPath::parse(const char *resPath) {
	mValidate = false;
	if (resPath == NULL) return false;
	char path[128];
	strcpy(path, resPath);
	char *ps = AttrUtils::trim(path);
	int len = strlen(ps);
	char *pe = ps + len - 1;
	if (len < 6) {
		return false;
	}
	char *p = strchr(ps, ' ');
	if (p != NULL) *p = 0;
	if (memcmp(ps, "res://", 6) == 0) {
		ps += 6;
		mResType = RT_RES;
	} else if (memcmp(ps, "file://", 7) == 0) {
		ps += 7;
		mResType = RT_FILE;
	} else {
		return false;
	}
	strcpy(mPath, ps);
	if (p == NULL) {
		mValidate = true;
		return true;
	}
	ps = AttrUtils::trim(p + 1);
	if (*ps == '[') {
		AttrUtils::parseArrayInt(ps + 1, &mX, 4);
		p = strchr(ps + 1, ']');
		if (p == NULL) return false;
		mHasRect = true;
		ps = p + 1;
	}
	if (ps > pe) {
		mValidate = true;
		return true;
	}
	if (strstr(ps, "repeat-x")) mRepeatX = true;
	if (strstr(ps, "repeat-y")) mRepeatY = true;
	mValidate = true;
	return true;
}
