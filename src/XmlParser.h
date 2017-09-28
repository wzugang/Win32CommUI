#pragma once
#include <vector>
class XComponent;

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
};

class XmlParser {
public:
	XmlParser();
	void parseString(const char *xml);
	void parseFile(const char *path);
	bool hasError();
	char *getError();
	XmlNode *getRoot();
	~XmlParser();
protected:
	void reset();
	void doParse();
	XmlNode* parseNode(XmlNode *parent);
	void endNode(XmlNode *parent);
	char *getNearText(int pos);
	void skipSpace();
	int nextTo(char ch);
	int nextWord();
	char *mXml;
	char *mError;
	XmlNode *mRoot;
	int mXmlLen;
	int mPos;
	bool mHasError;
};

class XmlPartLoader {
public:
	XmlPartLoader(const char *filePath);
	char *getPartXml(const char *name);
	char *getPartXml(int idx);
	int getPartNum() {return mPartNum;}
	~XmlPartLoader();
protected:
	char *mContent;
	int mContentLen;
	int mPartNum;
	struct PartItem {
		char *mName;
		char *mContent;
	};
	PartItem mPartItems[100];
protected:
	void doParse();
};