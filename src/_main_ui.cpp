#include "UIFactory.h"
#include "XComponent.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "XmlParser.h"
#include "XExt.h"
#include <atlimage.h>

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
		char *id = evtSource->getNode()->getAttrValue("id");

		if (msg != XComponent::WM_COMMAND_SELF && msg != WM_COMMAND) {
			return false;
		}
		if (strcmp("btn_1", id) == 0) {
			dlg = (XDialog *) UIFactory::fastBuild("file://skin/base.xml", "dialog-page", win->getWnd());
			dlg->findById("tool_btn_1")->setListener(new ButtonListener());
			rt = dlg->showModal();
			UIFactory::destory(dlg->getNode());
			return true;
		}
		if (strcmp("tool_btn_1", id) == 0) {
			dlg->close(100);
			return true;
		}
		if (strcmp("ext_btn_1", id) == 0) {
			popup = (XExtPopup*) UIFactory::fastBuild("file://skin/base.xml", "my-popup", win->getWnd());
			popup->findById("pop_btn_1")->setListener(this);
			// set window owner
			// SetWindowLong(popup->getWnd(), GWL_HWNDPARENT, (LONG)win->getWnd());
			POINT pt;
			GetCursorPos(&pt);
			popup->show(pt.x, pt.y);
			return true;
		}
		if (strcmp("pop_btn_1", id) == 0) {
			popup->close();
			MessageBox(win->getWnd(), "Hello Popup", NULL, MB_OK);
			return true;
		}
		return false;
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
	SetCurrentDirectory(path);

	/*HRSRC mm = FindResource(NULL, "MY_ANY_MM", "ANY");
	HGLOBAL hmm = LoadResource(NULL, mm);
	char *mmDat = (char *)LockResource(hmm);
	int mmLen = SizeofResource(NULL, mm);
	FreeResource(hmm);*/

	XComponent::init();

	win = (XWindow *) UIFactory::fastBuild("file://skin/base.xml", "main-page", NULL);
	// InitMyTree();
	InitMyTable();
	win->findById("btn_1")->setListener(new ButtonListener());
	win->findById("ext_btn_1")->setListener(new ButtonListener());
	win->show(nCmdShow);
	win->messageLoop();
	UIFactory::destory(win->getNode());
	return 0;
}

class MyTableModel : public XExtTableModel {
public:
	virtual int getColumnCount() {return 4;}
	virtual int getRowCount() {return 100;}
	virtual ColumnWidth getColumnWidth(int col) {
		ColumnWidth cw = {20|XComponent::MS_PERCENT, 0};
		if (col == 3) cw.mWeight = 1;
		return cw;
	}
	virtual int getRowHeight(int row) {return 25;}
	virtual int getHeaderHeight() {return 35;}
	virtual XImage *getHeaderImage() {
		static XImage *img = NULL;
		if (img == NULL)
			img = XImage::load("file://skin/ext_table_header.bmp");
		return img;
	}
	virtual char *getHeaderText(int col) {
		static char txtBuf[50];
		sprintf(txtBuf, "Head %d", col);
		return txtBuf;
	}
	virtual char *getCellData(int row, int col) {
		static char txtBuf[50];
		sprintf(txtBuf, "Cell(%d %d)", row, col);
		return txtBuf;
	}
};
void InitMyTable() {
	XExtTable *table = (XExtTable*)win->findById("my-table");
	MyTableModel *model = new MyTableModel();
	table->setModel(model);
}

void InitMyTree() {
	XTree * t = (XTree*) win->findById("my-tree");
	HWND tw = t->getWnd();
	XTreeRootNode *root = new XTreeRootNode(tw);
	XTreeNode *n = (new XTreeNode("Root A"))->appendTo(root);
	(new XTreeNode("Hello"))->appendTo(n);
	(new XTreeNode("World"))->appendTo(n);
	XTreeNode *n2 = (new XTreeNode("Root B"))->appendTo(root);
	n2->append(new XTreeNode("How"));
	n2->append(new XTreeNode("Are"));
	(new XTreeNode("You"))->appendTo(n2);
	n2->insert(1, new XTreeNode("Fine"));
	root->apply();
}