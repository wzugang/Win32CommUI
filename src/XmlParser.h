#pragma once
#include <vector>
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

	void print(int step);
	~XmlNode();
protected:
	std::vector<XmlNode*> mChildren;
	std::vector<Attr> mAttrs;
	char *mName;
	XmlNode *mParent;
	XComponent *mComponent;
	XmlParser *mParser;
	friend class XmlParser;
};

class XmlParser {
public:
	static XmlParser *create();
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
	char *mXml;
	char *mError;
	XmlNode *mRoot;
	int mXmlLen;
	int mPos;
	bool mHasError;
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