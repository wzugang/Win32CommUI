#pragma once
#include <windows.h>

class XComponent;
class VComponent;
class XmlNode;
class ResPath;
class XExtMenuModel;
class XExtTreeNode;
class VTreeNode;
struct XRect;

class XImage {
public:
	enum DrawAction {
		DA_COPY, DA_ALPHA_BLEND
	};
	// @param resPath 
	// res://xxx [x y width height] repeat-x repeat-y stretch 9patch
	// file://abc/xx.bmp ...
	// xbin://abc/xx.bmp ...
	static XImage *load(const char *resPath);
	static XImage *create(int width, int height, int bitPerPix = 32);

	HBITMAP getHBitmap();
	void *getBits();
	int getWidth();
	int getHeight();
	void *getRowBits(int row);
	bool hasAlphaChannel();

	void draw(HDC dc, int destX, int destY, int destW, int destH);

	void fillAlpha(BYTE alpha);
	void fillColor(COLORREF rgba);
	void draw(XImage *src, int dstX, int dstY, int destW, int destH, DrawAction a);
	void drawCopy(XImage *src, int dstX, int dstY, int destW, int destH, int srcX, int srcY);
	void drawAlphaBlend(XImage *src, int dstX, int dstY, int destW, int destH, int srcX, int srcY);
	void drawStretch(XImage *src, const XRect &srcRect, const XRect &destRect, DrawAction a);

	static HICON loadIcon(const char *resPath);
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
	void drawRepeatX(XImage *src, int destX, int destY, int destW, int destH, DrawAction a);
	void drawRepeatY(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void drawRepeatY(XImage *src, int destX, int destY, int destW, int destH, DrawAction a);
	void drawStretch(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void drawStretch(XImage *src, int destX, int destY, int destW, int destH, DrawAction a);
	void draw9Patch(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void draw9Patch(XImage *src, int destX, int destY, int destW, int destH, DrawAction a);
	void drawNormal(HDC dc, int destX, int destY, int destW, int destH, HDC memDc);
	void drawNormal(XImage *src, int destX, int destY, int destW, int destH, DrawAction a);
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
	static XExtMenuModel *buildMenu(XmlNode *rootMenu);
	static XExtTreeNode *buildTree(XmlNode *rootTree);

	//@param resPath file://abc.xml  res://abc
	static XmlNode* buildNode(const char *resPath, const char *partName);

	static XComponent* fastBuild(const char *resPath, const char *partName, XComponent *parent);
	static XExtMenuModel* fastMenu(const char *resPath, const char *partName);
	static XExtTreeNode* fastTree(const char *resPath, const char *partName);

	static void destory(XmlNode *root);
	static void registCreator(const char *nodeName, Creator c);
	static Creator getCreator(const char *nodeName);
};

class UIFactoryV {
public:
	static void init();
	typedef VComponent * (*Creator)(XmlNode*);

	static VComponent* buildComponent(XmlNode *root);
	static VComponent* buildComponentV(XmlNode *root);

	
	//@param resPath file://abc.xml  res://abc
	static XmlNode* buildNode(const char *resPath, const char *partName);

	static VTreeNode *buildTreeNode( XmlNode *rootTree );
	static VComponent* fastBuild(const char *resPath, const char *partName, VComponent *parent);
	
	static void destory(XmlNode *root);
	static void registCreator(Creator c);
	static VComponent *create(XmlNode *node);
};

