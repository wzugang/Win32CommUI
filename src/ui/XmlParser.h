#pragma once
#include <vector>
#include <map>
#include <string>
#include <windows.h>
class XComponent;
class XmlParser;

class XmlNode {
public:
	struct Attr {
		char *mName;
		char *mValue;
	};
	XmlNode(char *name, XmlNode *parent);
	char *getName();
	XmlNode *getParent() {return mParent;}
	void setParent(XmlNode *parent) {mParent = parent;}
	int getChildCount();
	XmlNode* getChild(int idx);
	void addChild(XmlNode *n);

	int getAttrsCount();
	Attr* getAttr(int idx);
	char *getAttrValue(const char *name);
	void addAttr(char *name, char *val);

	void setComponent(XComponent *c);
	XComponent* getComponent();

	XmlNode* findById(const char *id);
	XmlNode* getChildById(const char *id);
	XmlNode* getRoot();
	void copyDefault();
	void print(int step);
	~XmlNode();
protected:
	std::vector<XmlNode*> mChildren;
	std::vector<Attr> mAttrs;
	char *mName;
	XmlNode *mParent;
	XComponent *mComponent;
	XmlParser *mParser;
	std::map<std::string, XmlNode*> *mDefaultNode;
	bool mHasCopyedDefault;
	int mSelfAttrNum;
	friend class XmlParser;
};

class XmlParser {
public:
	static XmlParser *create(const char *resPath);
	void parseString(const char *xml, int xmlLen = -1);
	bool hasError();
	char *getError();
	XmlNode *getRoot();
protected:
	XmlParser();
	~XmlParser();
	void reset();
	void doParse();
	XmlNode* parseNode(XmlNode *parent);
	void endNode(XmlNode *parent);
	char *getNearText(int pos);
	void skipSpace();
	int nextTo(char ch);
	int nextWord();
	void replaceIncludeNode(XmlNode *n);
	void replaceAllIncludeNode(XmlNode *n);
	void dealDefaultNode( XmlNode * node );
	void dealAllDefaultNode( XmlNode * root );
	char *mXml;
	char *mError;
	XmlNode *mRoot;
	int mXmlLen;
	int mPos;
	bool mHasError;
	char mResPath[128];
	friend class XmlNode;
};

class XmlPartLoader {
public:
	struct PartItem {
		char *mName;
		char *mContent;
		int mCntLen;
	};

	// @param resPath is file://abc.xml or res://abc
	static XmlPartLoader *fetch(const char *resPath);
	static PartItem *findPartXml(const char *name);

	PartItem *getPartXml(const char *name);
	PartItem *getPartXml(int idx);
	int getPartNum() {return mPartNum;}
protected:
	XmlPartLoader(const char *filePath);
	~XmlPartLoader();
	void doParse();
protected:
	char mResPath[128];
	char *mContent;
	int mContentLen;
	int mPartNum;
	PartItem mPartItems[100];
};

class ResPath {
public:
	enum ResType { RT_NONE, RT_RES, RT_FILE, RT_XBIN };
	ResPath();
	bool parse(const char *resPath);

	ResType mResType;
	char mPath[128];
	int mX, mY, mWidth, mHeight;
	bool mRepeatX;
	bool mRepeatY;
	bool mStretch;
	bool m9Patch;
	bool mHasRect;
	bool mValidate;
};

class AttrUtils {
public:
	static char *trim(char *str);
	static bool parseFont(LOGFONT *font, char *str);
	static int parseSize(const char *str);
	static void parseArraySize(const char *str, int *arr, int arrNum);
	static int parseInt(const char *str);
	static void parseArrayInt(const char *str, int *arr, int arrNum);
	static bool parseColor(const char *color, COLORREF *colorOut);
	static std::vector<char*> splitBy( char *data, char splitChar);
	static bool parseBool(char *str, bool valueForNULL = false);
};