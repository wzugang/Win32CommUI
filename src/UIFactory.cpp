#include "UIFactory.h"
#include "XmlParser.h"
#include "XComponent.h"
#include "XExt.h"
#include <string.h>
#include <map>
#include <string>
#include <atlimage.h>

static std::map<std::string, XImage*> mCache;

static XImage *findInCache(const char *name) {
	std::map<std::string, XImage*>::iterator it = mCache.find(name);
	if (it == mCache.end())
		return NULL;
	return it->second;
}
static char *getCacheName(ResPath *r, bool basic) {
	static char CACHE_NAME[128];
	if (!r->mHasRect || basic) {
		sprintf(CACHE_NAME, "%s", r->mPath);
	} else {
		sprintf(CACHE_NAME, "%s [%d %d %d %d]", r->mPath, r->mX, r->mY, r->mWidth, r->mHeight);
	}
	return CACHE_NAME;
}

XImage::XImage(HBITMAP bmp, int w, int h, void *bits, int bitPerPix, int rowBytes) {
	mHBitmap = bmp;
	mWidth = w;
	mHeight = h;
	mBits = bits;
	mBitPerPix = bitPerPix;
	mRowBytes = rowBytes;
	mHasAlphaChannel = (bitPerPix == 32);
	mTransparentColor = -1;
}

XImage * XImage::load( const char *resPath ) {
	ResPath info;
	if (! info.parse(resPath))
		return NULL;
	XImage *img = findInCache(getCacheName(&info, false));
	if (img != NULL)
		return img;
	img = findInCache(getCacheName(&info, true));
	if (img == NULL) {
		img = loadImage(&info);
		if (img == NULL)
			return NULL;
		mCache[getCacheName(&info, true)] = img;
	}
	if (! info.mHasRect)
		return img;
	img = createPart(img, info.mX, info.mY, info.mWidth, info.mHeight);
	if (img == NULL)
		return NULL;
	mCache[getCacheName(&info, false)] = img;
	return img;
}

XImage * XImage::loadImage( ResPath *info) {
	CImage cimg;
	if (info->mResType == ResPath::RT_FILE) {
		cimg.Load(info->mPath);
	} else {
		cimg.LoadFromResource(XComponent::getInstance(), info->mPath);
	}
	if (cimg.GetWidth() == 0)
		return NULL;

	XImage *img = new XImage(NULL, cimg.GetWidth(), cimg.GetHeight(), cimg.GetBits(), cimg.GetBPP(), cimg.GetPitch());
	img->mHBitmap = cimg.Detach();
	if (img->mHasAlphaChannel) {
		img->buildAlphaChannel();
	}
	return img;
}

XImage * XImage::createPart( XImage *org, int x, int y, int width, int height ) {
	if (org == NULL) 
		return NULL;
	if (x < 0 || y < 0 || width <= 0 || height <= 0)
		return NULL;
	if (x + width > org->mWidth || y + height > org->mHeight)
		return NULL;
	XImage *img = create(width, height, org->mBitPerPix);
	for (int i = 0, r = y; i < height; ++i, ++r) {
		BYTE* src = (BYTE*)org->getRowBits(r) + x * org->mBitPerPix / 8;
		BYTE* dst = (BYTE*)img->getRowBits(i);
		memcpy(dst, src, org->mBitPerPix / 8 * width);
	}
	return img;
}

void * XImage::getRowBits( int row ) {
	if (mBits == NULL || row >= mHeight)
		return NULL;
	return (BYTE*)mBits + (mRowBytes * row);
}

XImage * XImage::create( int width, int height, int bitPerPix ) {
	int rowByteNum = width * bitPerPix / 8;
	rowByteNum = (rowByteNum + 3) / 4 * 4;
	BITMAPINFOHEADER header = {0};
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = -height;
	header.biPlanes = 1;
	header.biBitCount = bitPerPix;
	header.biCompression = BI_RGB;
	header.biClrUsed = 0;
	header.biSizeImage = rowByteNum * height;
	PVOID pvBits = NULL;
	HBITMAP bmp = CreateDIBSection(NULL, (PBITMAPINFO)&header, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (bmp == NULL)
		return NULL;
	return new XImage(bmp, width, height, pvBits, bitPerPix, rowByteNum);
}

void XImage::buildAlphaChannel() {
	for (int r = 0; r < mHeight; ++r) {
		BYTE *p = (BYTE *)getRowBits(r);
		for (int c = 0; c < mWidth; ++c, p += 4) {
			if (p[3] == 255) continue;
			p[0] = p[0] * p[3] / 255;
			p[1] = p[1] * p[3] / 255;
			p[2] = p[2] * p[3] / 255;
		}
	}
}

HBITMAP XImage::getHBitmap() {
	return mHBitmap;
}

void * XImage::getBits() {
	return mBits;
}

int XImage::getWidth() {
	return mWidth;
}

int XImage::getHeight() {
	return mHeight;
}

XImage::~XImage() {
	if (mHBitmap) DeleteObject(mHBitmap);
}

bool XImage::hasAlphaChannel() {
	return mHasAlphaChannel;
}

//----------------------------UIFactory-------------------------
struct NodeCreator {
	NodeCreator() {
		mNodeName[0] = 0;
		mCreator = NULL;
	}
	char mNodeName[32];
	UIFactory::Creator mCreator;
};

static NodeCreator g_creators[64];
int UIFactory::mNum = 0;

XComponent* UIFactory::buildComponent( XmlNode *root) {
	if (root == NULL) return NULL;
	Creator c = getCreator(root->getName());
	if (c == NULL) {
		printf("UIFactory.buildingTree: node name %s has no creator\n", root->getName());
		return NULL;
	}
	XComponent *x = c(root);
	root->setComponent(x);
	for (int i = 0; i < root->getChildCount(); ++i) {
		XmlNode *child = root->getChild(i);
		XComponent *ix = buildComponent(child);
		child->setComponent(ix);
	}
	return x;
}

XmlNode* UIFactory::buildNode( const char *resPath, const char *partName ) {
	if (resPath == NULL || partName == NULL)
		return NULL;
	XmlPartLoader *loader = XmlPartLoader::fetch(resPath);
	if (loader == NULL) return NULL;
	XmlPartLoader::PartItem * item = loader->getPartXml(partName);
	if (item == NULL) return NULL;
	XmlParser *parser = XmlParser::create();
	parser->parseString(item->mContent);
	XmlNode *rootNode = parser->getRoot();
	return rootNode;
}

XComponent* UIFactory::fastBuild( const char *resPath, const char *partName, HWND parent ) {
	XmlNode *root = UIFactory::buildNode(resPath, partName);
	if (root == NULL) return NULL;
	XComponent *cc = UIFactory::buildComponent(root);
	cc->createWndTree(parent);
	return cc;
}

void UIFactory::registCreator( const char *nodeName, Creator c ) {
	if (nodeName == NULL || c == NULL)
		return;
	strcpy(g_creators[mNum].mNodeName, nodeName);
	g_creators[mNum].mCreator = c;
	++mNum;
}

UIFactory::Creator UIFactory::getCreator( const char *nodeName ) {
	if (nodeName == NULL) return NULL;
	for (int i = 0; i < mNum; ++i) {
		if (strcmp(nodeName, g_creators[i].mNodeName) == 0)
			return g_creators[i].mCreator;
	}
	return NULL;
}

void UIFactory::destory( XmlNode *root ) {
	for (int i = 0; i < root->getChildCount(); ++i) {
		XmlNode *child = root->getChild(i);
		destory(child);
	}
	delete root->getComponent();
	delete root;
}

static XComponent *XAbsLayout_Creator(XmlNode *n) {return new XAbsLayout(n);}
static XComponent *XHLineLayout_Creator(XmlNode *n) {return new XHLineLayout(n);}
static XComponent *XVLineLayout_Creator(XmlNode *n) {return new XVLineLayout(n);}

static XComponent *XButton_Creator(XmlNode *n) {return new XButton(n);}
static XComponent *XLabel_Creator(XmlNode *n) {return new XLabel(n);}
static XComponent *XCheckBox_Creator(XmlNode *n) {return new XCheckBox(n);}
static XComponent *XRadio_Creator(XmlNode *n) {return new XRadio(n);}
static XComponent *XGroupBox_Creator(XmlNode *n) {return new XGroupBox(n);}
static XComponent *XEdit_Creator(XmlNode *n) {return new XEdit(n);}
static XComponent *XComboBox_Creator(XmlNode *n) {return new XComboBox(n);}
static XComponent *XTable_Creator(XmlNode *n) {return new XTable(n);}
static XComponent *XTree_Creator(XmlNode *n) {return new XTree(n);}
static XComponent *XTab_Creator(XmlNode *n) {return new XTab(n);}
static XComponent *XListBox_Creator(XmlNode *n) {return new XListBox(n);}
static XComponent *XDateTimePicker_Creator(XmlNode *n) {return new XDateTimePicker(n);}
static XComponent *XWindow_Creator(XmlNode *n) {return new XWindow(n);}
static XComponent *XDialog_Creator(XmlNode *n) {return new XDialog(n);}
static XComponent *XScroller_Creator(XmlNode *n) {return new XScroll(n);}

static XComponent *XExtButton_Creator(XmlNode *n) {return new XExtButton(n);}
static XComponent *XExtOption_Creator(XmlNode *n) {return new XExtOption(n);}
static XComponent *XExtLabel_Creator(XmlNode *n) {return new XExtLabel(n);}
static XComponent *XExtCheckBox_Creator(XmlNode *n) {return new XExtCheckBox(n);}
static XComponent *XExtRadio_Creator(XmlNode *n) {return new XExtRadio(n);}
static XComponent *XExtPopup_Creator(XmlNode *n) {return new XExtPopup(n);}
static XComponent *XExtScroll_Creator(XmlNode *n) {return new XExtScroll(n);}

struct InitUIFactory {
	InitUIFactory() {
		UIFactory::registCreator("AbsLayout", XAbsLayout_Creator);
		UIFactory::registCreator("HLineLayout", XHLineLayout_Creator);
		UIFactory::registCreator("VLineLayout", XVLineLayout_Creator);

		UIFactory::registCreator("Button", XButton_Creator);
		UIFactory::registCreator("Label", XLabel_Creator);
		UIFactory::registCreator("CheckBox", XCheckBox_Creator);
		UIFactory::registCreator("Radio", XRadio_Creator);
		UIFactory::registCreator("GroupBox", XGroupBox_Creator);
		UIFactory::registCreator("Edit", XEdit_Creator);
		UIFactory::registCreator("ComboBox", XComboBox_Creator);
		UIFactory::registCreator("Table", XTable_Creator);
		UIFactory::registCreator("Tree", XTree_Creator);
		UIFactory::registCreator("Tab", XTab_Creator);
		UIFactory::registCreator("ListBox", XListBox_Creator);
		UIFactory::registCreator("DateTimePicker", XDateTimePicker_Creator);
		UIFactory::registCreator("Window", XWindow_Creator);
		UIFactory::registCreator("Dialog", XDialog_Creator);
		UIFactory::registCreator("Scroll", XScroller_Creator);

		UIFactory::registCreator("ExtButton", XExtButton_Creator);
		UIFactory::registCreator("ExtOption", XExtOption_Creator);
		UIFactory::registCreator("ExtLabel", XExtLabel_Creator);
		UIFactory::registCreator("ExtCheckBox", XExtCheckBox_Creator);
		UIFactory::registCreator("ExtRadio", XExtRadio_Creator);
		UIFactory::registCreator("ExtPopup", XExtPopup_Creator);
		UIFactory::registCreator("ExtScroll", XExtScroll_Creator);
	}

	~InitUIFactory() {
	}
};

static InitUIFactory s_init_ui_factory;

