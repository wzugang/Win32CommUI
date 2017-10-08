#include "UIFactory.h"
#include "XmlParser.h"
#include "XComponent.h"
#include "XExt.h"
#include <string.h>
#include <map>
#include <string>

static std::map<std::string, XImage*> mCache;

static XImage *findInCache(const char *name) {
	std::map<std::string, XImage*>::iterator it = mCache.find(name);
	if (it == mCache.end())
		return NULL;
	return it->second;
}

XImage::XImage(HBITMAP bmp, int w, int h, void *bits) {
	mHBitmap = bmp;
	mWidth = w;
	mHeight = h;
	mBits = bits;
	mRefCount = 1;
}

XImage * XImage::load( const char *resPath ) {
	XImage *img = findInCache(resPath);
	if (img != NULL) {
		img->incRef();
		return img;
	}
	HBITMAP bmp = NULL;
	if (memcmp(resPath, "res://", 6) == 0) {
		bmp = (HBITMAP)LoadImage(XComponent::getInstance(), resPath + 6, IMAGE_BITMAP, 0, 0, 0);
	} else if (memcmp(resPath, "file://", 7) == 0) {
		bmp = (HBITMAP)LoadImage(XComponent::getInstance(), resPath + 7, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	if (bmp == NULL) return NULL;
	BITMAP bt = {0};
	if (GetObject(bmp, sizeof(BITMAP), &bt) == 0)
		return NULL;
	int height = bt.bmHeight > 0 ? bt.bmHeight : -bt.bmHeight;
	BITMAPINFO bi = {0};
	bi.bmiHeader.biSize = sizeof(BITMAPINFO);
	bi.bmiHeader.biWidth = bt.bmWidth;
	bi.bmiHeader.biHeight = -height;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biSizeImage = bt.bmWidth * height * 4;
	void *pvBits = NULL;
	HBITMAP bmp2 = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (bmp2 == NULL)
		return NULL;
	int row = GetDIBits(GetDC(NULL), bmp, 0, height, pvBits, &bi, DIB_RGB_COLORS);
	if (row != height) {
		return NULL;
	}
	img = new XImage(bmp2, bt.bmWidth, height, pvBits);
	mCache[resPath] = img;
	return img;
}

XImage * XImage::create( int width, int height ) {
	BITMAPINFOHEADER header = {0};
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = -height;
	header.biPlanes = 1;
	header.biBitCount = 32;
	header.biCompression = BI_RGB;
	header.biClrUsed = 0;
	header.biSizeImage = width * height * 4;
	PVOID pvBits = NULL;
	HBITMAP bmp = CreateDIBSection(NULL, (PBITMAPINFO)&header, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (bmp == NULL)
		return NULL;
	return new XImage(bmp, width, height, pvBits);
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

void XImage::incRef() {
	++mRefCount;
}

void XImage::decRef() {
	--mRefCount;
	if (mRefCount <= 0) {
		// mCache.erase();
	}
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
	}

	~InitUIFactory() {
	}
};

static InitUIFactory s_init_ui_factory;

