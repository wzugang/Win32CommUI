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
XExtMenuItemList *mlist;

void InitMyTable();
void InitMyTree();
void InitMyList();
void InitMyMenu();

class ButtonListener : public XListener {
public:
	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *ret) {
		int rt = 0;
		char *id = evtSource->getNode()->getAttrValue("id");

		if (msg != XComponent::WM_COMMAND_SELF && msg != WM_COMMAND) {
			return false;
		}
		if (strcmp("btn_1", id) == 0) {
			return true;
		}
		if (strcmp("tool_btn_1", id) == 0) {
			dlg->close(100);
			return true;
		}
		if (strcmp("ext_btn_1", id) == 0) {
			POINT pt;
			GetCursorPos(&pt);
			if (mlist == NULL) mlist = UIFactory::fastMenu("file://skin/base.xml", "my-menu");
			XExtMenuManager mgr(mlist, evtSource, NULL);
			mgr.show(pt.x, pt.y);
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
	strcpy(path, "D:\\CPP\\WinUI\\WinUI\\");
	SetCurrentDirectory(path);

	/*HRSRC mm = FindResource(NULL, "MY_ANY_MM", "ANY");
	HGLOBAL hmm = LoadResource(NULL, mm);
	char *mmDat = (char *)LockResource(hmm);
	int mmLen = SizeofResource(NULL, mm);
	FreeResource(hmm);*/

	UIFactory::init();

	win = (XWindow *) UIFactory::fastBuild("file://skin/base.xml", "main-page", NULL);
	InitMyTree();
	InitMyTable();
	InitMyList();

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
	virtual int getRowCount() {return 50;}
	virtual ColumnWidth getColumnWidth(int col) {
		ColumnWidth cw = {120 | XComponent::MS_FIX, 0};
		// if (col == 3) {cw.mWeight = 1; }
		return cw;
	}
	virtual int getRowHeight(int row) {
		if (row % 5 == 4)
			return 40;
		return 25;
	}
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

class MyListModel : public XListModel {
public:
	MyListModel() {}
	virtual int getItemCount() {return 50;}
	virtual int getItemHeight(int item) {return 25;}
	virtual ItemData *getItemData(int item) {
		static char buf[100];
		sprintf(buf, "List Item %d", item);
		mTmp.mSelectable =  true; // item % 5 != 0;
		mTmp.mText = buf;
		return &mTmp;
	}
	virtual bool isMouseTrack() {return true;}
	ItemData mTmp;
};
void InitMyList() {
	XExtList *table = (XExtList*)win->findById("list_1");
	MyListModel *model = new MyListModel();
	table->setModel(model);

	XExtComboBox *box = (XExtComboBox*)win->findById("combox_1");
	table = box->getExtList();
	table->setModel(model);
}
void InitMyTree() {
	XExtTree * t = (XExtTree*) win->findById("ext_tree");
	XExtTreeNode *root = UIFactory::fastTree("file://skin/base.xml", "my-tree");
	t->setModel(root);
}

void InitMyMenu() {

}