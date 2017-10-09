#pragma once
#include <windows.h>
class XComponent;
class XmlNode;
class ResPath;

class XImage {
public:
	// @param resPath 
	// res://xxx [x y width height] 
	// file://abc/xx.bmp [x y width height] 
	static XImage *load(const char *resPath);
	static XImage *create(int width, int height, int bitPerPix = 32);

	HBITMAP getHBitmap();
	void *getBits();
	int getWidth();
	int getHeight();
	void *getRowBits(int row);
	bool hasAlphaChannel();
	~XImage();
protected:
	XImage(HBITMAP bmp, int w, int h, void *bits, int bitPerPix, int rowBytes);
	static XImage *createPart(XImage *org, int x, int y, int width, int height);
	static XImage *loadImage(ResPath *info);
	void buildAlphaChannel();

	void *mBits;
	HBITMAP mHBitmap;
	int mWidth;
	int mHeight;
	int mBitPerPix;
	bool mHasAlphaChannel;
	COLORREF mTransparentColor;
	// ¸º£ºdown-up image;
	// Õý: up-down image
	int mRowBytes;
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

