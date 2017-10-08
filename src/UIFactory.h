#pragma once
#include <windows.h>
class XComponent;
class XmlNode;

class XImage {
public:
	// @param resPath res://xxx   file://abc/xx.bmp
	static XImage *load(const char *resPath);
	static XImage *create(int width, int height);

	HBITMAP getHBitmap();
	void *getBits();
	int getWidth();
	int getHeight();

	void incRef();
	void decRef();
protected:
	~XImage();
	XImage(HBITMAP bmp, int w, int h, void *bits);
	void *mBits;
	HBITMAP mHBitmap;
	int mWidth;
	int mHeight;
	int mRefCount;
};

class UIFactory {
public:
	typedef XComponent * (*Creator)(XmlNode*);

	static XComponent* buildComponent(XmlNode *root);

	//@param resPath file://abc.xml  res://abc
	static XmlNode* buildNode(const char *resPath, const char *partName);

	static XComponent* fastBuild(const char *resPath, const char *partName, HWND parent);

	static void destory(XmlNode *root);
	static void registCreator(const char *nodeName, Creator c);
	static Creator getCreator(const char *nodeName);
protected:
	static int mNum;
};

