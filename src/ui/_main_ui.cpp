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
		return 20;
	}
	virtual int getItemHeight(int item) {
		return 25;
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

	win->createWnd();
	win->show();
	win->msgLoop();
	UIFactoryV::destory(win->getNode());
#endif
	return 0;
}
