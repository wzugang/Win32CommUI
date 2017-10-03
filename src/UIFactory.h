#pragma once
#include <windows.h>
class XComponent;
class XmlNode;

class XImage {
public:
	XImage(HBITMAP bmp);
	static XImage *loadFromFile(const char *path);
	static XImage *loadFromResource(int resId);
	static XImage *loadFromResource(const char * resName);
	// @param resPath res://xxx   file://abc/xx.bmp
	static XImage *load(const char *resPath);
	static XImage *create(int width, int height);

	HBITMAP getHBitmap();
	void *getBits();
	int getWidth();
	int getHeight();
	~XImage();
protected:
	XImage();
	void *mBits;
	HBITMAP mHBitmap;
	int mWidth;
	int mHeight;
};

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

