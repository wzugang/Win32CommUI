#include "UIFactory.h"
#include "XmlParser.h"
#include "XComponent.h"
#include "XExt.h"
#include <string.h>

XImage::XImage() {
	mBits = NULL;
	mHBitmap = NULL;
	mWidth = mHeight = 0;
}

XImage::XImage( HBITMAP bmp ) {
	mBits = NULL;
	mHBitmap = bmp;
	mWidth = mHeight = 0;
	if (bmp) {
		BITMAP b = {0};
		GetObject(bmp, sizeof(BITMAP), &b);
		mWidth = b.bmWidth;
		mHeight = b.bmHeight;
		mBits = b.bmBits;
	}
}

XImage * XImage::loadFromFile( const char *path ) {
	HBITMAP bmp = (HBITMAP)LoadImage(XComponent::getInstance(), path, IMAGE_BITMAP, 0, 0,
		/*LR_CREATEDIBSECTION | LR_DEFAULTSIZE | */ LR_LOADFROMFILE);
	if (bmp) return new XImage(bmp);
	return NULL;
}

XImage * XImage::loadFromResource( int resId ) {
	HBITMAP bmp = (HBITMAP)LoadImage(XComponent::getInstance(), MAKEINTRESOURCE(resId), IMAGE_BITMAP, 0, 0,
		/*LR_CREATEDIBSECTION | LR_DEFAULTSIZE*/ 0);
	if (bmp) return new XImage(bmp);
	return NULL;
}

XImage * XImage::loadFromResource( const char * resName ) {
	HBITMAP bmp = (HBITMAP)LoadImage(XComponent::getInstance(), resName, IMAGE_BITMAP, 0, 0,
		/*LR_CREATEDIBSECTION  LR_DEFAULTSIZE*/ 0);
	if (bmp) return new XImage(bmp);
	return NULL;
}

XImage * XImage::load( const char *resPath ) {
	if (memcmp(resPath, "res://", 6) == 0)
		return loadFromResource(resPath + 6);
	if (memcmp(resPath, "file://", 7) == 0)
		return loadFromFile(resPath + 7);
	return NULL;
}

XImage * XImage::create( int width, int height ) {
	BITMAPINFOHEADER header = {0};
	int nBytesPerLine = ((width * 32 + 31) & (~31)) >> 3;
	header.biSize = sizeof(BITMAPINFOHEADER);
	header.biWidth = width;
	header.biHeight = height;
	header.biPlanes = 1;
	header.biBitCount = 32;
	header.biCompression = BI_RGB;
	header.biClrUsed = 0;
	header.biSizeImage = nBytesPerLine * height;
	PVOID pvBits = NULL;
	HBITMAP bmp = CreateDIBSection(NULL, (PBITMAPINFO)&header, DIB_RGB_COLORS, &pvBits, NULL, 0);  
	if (bmp == NULL)
		return NULL;
	return new XImage(bmp);
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

XComponent* UIFactory::build( XmlNode *root) {
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
		XComponent *ix = build(child);
		child->setComponent(ix);
	}
	return x;
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

static XComponent *XExtButton_Creator(XmlNode *n) {return new XExtButton(n);}
static XComponent *XExtOption_Creator(XmlNode *n) {return new XExtOption(n);}
static XComponent *XExtLabel_Creator(XmlNode *n) {return new XExtLabel(n);}
static XComponent *XExtCheckBox_Creator(XmlNode *n) {return new XExtCheckBox(n);}
static XComponent *XExtRadio_Creator(XmlNode *n) {return new XExtRadio(n);}

struct InitUIFactory {
	InitUIFactory() {
		UIFactory::registCreator("AbsLayout", XAbsLayout_Creator);
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

		UIFactory::registCreator("ExtButton", XExtButton_Creator);
		UIFactory::registCreator("ExtOption", XExtOption_Creator);
		UIFactory::registCreator("ExtLabel", XExtLabel_Creator);
		UIFactory::registCreator("ExtCheckBox", XExtCheckBox_Creator);
		UIFactory::registCreator("ExtRadio", XExtRadio_Creator);
	}

	~InitUIFactory() {
	}
};

static InitUIFactory s_init_ui_factory;

