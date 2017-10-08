#include "UIFactory.h"
#include "XComponent.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "XmlParser.h"
#include "XExt.h"

XWindow *win;
XDialog *dlg;
XExtPopup *popup;

void InitMyTable();
void InitMyTree();
void TestDecoderImage();

class ButtonListener : public XListener {
public:
	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret) {
		int rt = 0;
		if (msg != XComponent::WM_COMMAND_SELF) {
			return false;
		}
		if (strcmp("btn_1", evtSource->getNode()->getAttrValue("id")) == 0) {
			dlg = (XDialog *) UIFactory::fastBuild("file://skin/base.xml", "sub-page", win->getWnd());
			dlg->findById("tool_btn_1")->setListener(new ButtonListener());
			rt = dlg->showModal();
			UIFactory::destory(dlg->getNode());
		} else if (strcmp("tool_btn_1", evtSource->getNode()->getAttrValue("id")) == 0) {
			dlg->close(100);
		} else if (strcmp("ext_btn_1", evtSource->getNode()->getAttrValue("id")) == 0) {
			XExtOption *opt = (XExtOption *)win->findById("ext_opt_1");
			opt->setSelect(! opt->isSelect());

			popup = (XExtPopup*) UIFactory::fastBuild("file://skin/base.xml", "my-popup", win->getWnd());
			popup->findById("pop_btn_1")->setListener(new ButtonListener());
			// set window owner
			// SetWindowLong(popup->getWnd(), GWL_HWNDPARENT, (LONG)win->getWnd());
			POINT pt;
			GetCursorPos(&pt);
			popup->show(pt.x, pt.y);
		}  else if (strcmp("pop_btn_1", evtSource->getNode()->getAttrValue("id")) == 0) {
			popup->close();
			MessageBox(win->getWnd(), "Hello Popup", NULL, MB_OK);
			return true;
		}
		return true;
	}
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
	char path[256];
	GetModuleFileName(NULL, path, 256);
	char *p = strrchr(path, '\\') + 1;
	*p = 0;
	SetCurrentDirectory(path);

	XComponent::init();
	XmlNode *rootNode = UIFactory::buildNode("file://skin/base.xml", "main-page");
	win = (XWindow *) UIFactory::buildComponent(rootNode);

	win->createWndTree(NULL);
	win->findById("btn_1")->setListener(new ButtonListener());
	win->findById("ext_btn_1")->setListener(new ButtonListener());
	win->show(nCmdShow);
	win->messageLoop();

	UIFactory::destory(rootNode);

	return 0;
}

class MyTable : public XTableModel {
public:
	virtual int getColumnCount() {
		return 3;
	}
	virtual int getRowCount() {
		return 10;
	}
	virtual int getColumnWidth(int col) {
		return 100;
	}
	virtual char *getColumnTitle(int col) {
		if (col == 0) return (char *)"学号";
		if (col == 1) return (char *)"姓名";
		if (col == 2) return (char *)"性别";
		return NULL;
	}
	virtual void getItem(int row, int col, LVITEM *item) {
		static char v[128];
		XTableModel::getItem(row, col, item);
		sprintf(v, "%d  %d", row, col);
		item->pszText = v;
	}
};

void InitMyTable() {
	MyTable *t = new MyTable();
	t->apply(win->findById("tab")->getWnd());
}

void InitMyTree() {
	XTree * t = (XTree*) win->findById("tab");
	HWND tw = t->getWnd();
	TV_INSERTSTRUCT tvinsert = {0};
	tvinsert.hParent = NULL;
	tvinsert.hInsertAfter = TVI_ROOT;
	tvinsert.item.mask = TVIF_TEXT|TCIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvinsert.item.pszText = "Root 1";
	HTREEITEM hParent = (HTREEITEM) SendMessage(tw, TVM_INSERTITEM, 0, (LPARAM)&tvinsert);
	tvinsert.hParent = hParent;
	tvinsert.hInsertAfter = TVI_LAST;
	tvinsert.item.pszText = "Child A";
	SendMessage(tw,TVM_INSERTITEM,0 ,(LPARAM)&tvinsert);
	tvinsert.item.pszText = "Child B";
	SendMessage(tw,TVM_INSERTITEM,0,(LPARAM)&tvinsert);

	tvinsert.hParent = NULL;
	tvinsert.item.pszText = "Root 2";
	HTREEITEM hParent2 = (HTREEITEM) SendMessage(tw, TVM_INSERTITEM, 0, (LPARAM)&tvinsert);
}

void TestDecoderImage() {
	// UINT decodersNum, size;
	// GetImageDecodersSize(&decodersNum, &size);
}