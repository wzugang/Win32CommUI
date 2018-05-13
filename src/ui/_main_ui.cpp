#include "UIFactory.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "XmlParser.h"
#include "XExt.h"
#include <atlimage.h>
#include "VComponent.h"
#include "VExt.h"

VWindow *win;

class BtnListener : public VListener {
public:
	virtual bool onEvent(VComponent *src, Msg *msg) {
		if (msg->mId == Msg::CLICK) {
			VPopup *pp = (VPopup *) UIFactoryV::fastBuild("file://skin/vtest.xml", "my-popup", win);
			pp->setMouseAction(VPopup::MA_INTERREPT);
			pp->show(VComponent::getSpecSize(pp->getAttrX()), 
				VComponent::getSpecSize(pp->getAttrY()));
			return true;
		}
		return false;
	}
};

class ListModel : public VListModel {
public:
	virtual int getItemCount() {
		return 30;
	}
	virtual int getItemHeight(int item) {
		return 20;
	}
	virtual ItemData *getItemData(int item) {
		static ItemData it;
		static char buf[80];
		it.mSelectable = true;
		it.mText = buf;
		sprintf(buf, "This is Item %d", item);
		return &it;
	}
};

class TableModel : public VTableModel {
public:
	virtual int getColumnCount() {
		return 5;
	}
	virtual int getRowCount() {
		return 30;
	}
	virtual int getColumnWidth(int col, int wholeWidth) {
		return col % 2 == 0 ? 100 : 120;
	}
	virtual int getRowHeight(int row) {
		return 20;
	}
	virtual int getHeaderHeight() {
		return 30;
	}
	virtual HeaderData *getHeaderData(int col) {
		static HeaderData hd;
		static char buf[80];
		if (hd.mBgImage == NULL) {
			hd.mBgImage = XImage::create(1, 1);
			hd.mBgImage->mStretch = true;
			hd.mBgImage->fillColor(0xffaabbcc);
		}
		sprintf(buf, "Head %d", col);
		hd.mText = buf;
		return &hd;
	}
	virtual CellData *getCellData(int row, int col) {
		static CellData cd;
		static char txt[80];
		cd.mText = txt;
		sprintf(txt, "Cell(%02d, %02d)", row, col);
		return &cd;
	}
	virtual XImage *getHeaderBgImage() {
		static XImage *img = XImage::create(1, 1);
		img->mStretch = true;
		img->fillColor(0xffaabbcc);
		return img;
	}
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	// ---- debug -----
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
	char path[256];
	GetModuleFileName(NULL, path, 256);
	char *p = strrchr(path, '\\') + 1;
	*p = 0;
	// strcpy(path, "D:\\CPP\\WinUI\\WinUI\\");
	// SetCurrentDirectory(path);
	GetCurrentDirectory(240, path);

#if 0
	UIFactory::init();
	XWindow *xwin = (XWindow *) UIFactory::fastBuild("file://skin/base.xml", "main-page", NULL);
	xwin->show(nCmdShow);
	xwin->messageLoop();
#else
	UIFactoryV::init();
	win = (VWindow *) UIFactoryV::fastBuild("file://skin/vtest.xml", "main-page", NULL);

	// win->findById("ext_btn_1")->setListener(new BtnListener());
	// VList *list = (VList *)(win->findById("list"));
	// list->setModel(new ListModel());
	// VTree *tree = (VTree *)(win->findById("tree"));
	// tree->setModel(UIFactoryV::buildTreeNode(UIFactoryV::buildNode("file://skin/vtest.xml", "my-tree")));
	VComboBox *combo = (VComboBox *)(win->findById("combo"));
	combo->getList()->setModel(new ListModel());

	win->createWnd();
	win->show();
	win->msgLoop();
	UIFactoryV::destory(win->getNode());
#endif
	return 0;
}
