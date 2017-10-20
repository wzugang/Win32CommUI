#pragma once
#include <windows.h>
class XComponent;
class XmlNode;
class ResPath;
class XExtMenuItemList;
class XExtTreeNode;

class XImage {
public:
	// @param resPath 
	// res://xxx [x y width height] repeat-x repeat-y stretch 9patch
	// file://abc/xx.bmp ...
	static XImage *load(const char *resPath);
	static XImage *create(int width, int height, int bitPerPix = 32);

	HBITMAP getHBitmap();
	void *getBits();
	int getWidth();
	int getHeight();
	void *getRowBits(int row);
	bool hasAlphaChannel();
	void draw(HDC dc, int destX, int destY, int destW, int destH);
	~XImage();
	bool mRepeatX;
	bool mRepeatY;
	bool mStretch;
	bool m9Patch;
protected:
	XImage(HBITMAP bmp, int w, int h, void *bits, int bitPerPix, int rowBytes);
	static XImage *createPart(XImage *org, int x, int y, int width, int height);
	static XImage *loadImage(ResPath *info);
	void buildAlphaChannel();
	void drawRepeatX(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void drawRepeatY(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void drawStretch(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void draw9Patch(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void drawNormal(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);

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
	bool mDeleteBitmap;
};

class UIFactory {
public:
	static void init();
	typedef XComponent * (*Creator)(XmlNode*);

	static XComponent* buildComponent(XmlNode *root);
	static XExtMenuItemList *buildMenu(XmlNode *rootMenu);
	static XExtTreeNode *buildTree(XmlNode *rootTree);

	//@param resPath file://abc.xml  res://abc
	static XmlNode* buildNode(const char *resPath, const char *partName);

	static XComponent* fastBuild(const char *resPath, const char *partName, XComponent *parent);
	static XExtMenuItemList* fastMenu(const char *resPath, const char *partName);
	static XExtTreeNode* fastTree(const char *resPath, const char *partName);

	static void destory(XmlNode *root);
	static void registCreator(const char *nodeName, Creator c);
	static Creator getCreator(const char *nodeName);
};

