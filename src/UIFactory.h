#pragma once
#include <windows.h>
class XComponent;
class XmlNode;

class UIFactory {
public:
	typedef XComponent * (*Creator)(XmlNode*);

	static XComponent* build(XmlNode *root);
	static void destory(XmlNode *root);

	static void registCreator(const char *nodeName, Creator c);
	static Creator getCreator(const char *nodeName);
protected:
	static int mNum;
};