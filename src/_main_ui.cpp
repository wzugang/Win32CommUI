#include "ui/UIFactory.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "XmlParser.h"
#include <atlimage.h>
#include "ui/VComponent.h"
#include "ui/VExt.h"
#include "utils/XString.h"
#include "utils/Thread.h"
#include "utils/Http.h"
#include "utils/HttpClient.h"

VWindow *win;

class BtnListener : public VListener {
public:
	virtual bool onEvent(VComponent *src, Msg *msg) {
		if (msg->mId == Msg::CLICK) {
			XmlNode *node = UIFactory::buildNode("file://skin/vtest.xml", "my-menu");
			VMenuModel *model = UIFactory::buildMenuModel(node);

			VPopupMenu *pp = (VPopupMenu *)UIFactory::fastBuild("file://skin/vtest.xml", "my-popmenu", win);
			pp->setModel(model);
			pp->setMouseAction(VPopup::MA_CLOSE);
			pp->show(50, 50);
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

class MyRun : public Runnable {
public:
	MyRun() {mCount = 0;}
	int mCount;
	virtual void onRun() {
		while (mCount < 10) {
			printf("count : %d \n", mCount);
			++mCount;
			Sleep(1000);
		}
	}
};

void test_httpclient() {
	HttpClient client;
	HttpRequest req("http://www.ip138.com:8080/search.asp?mobile=1887926&action=mobile", HttpRequest::METHOD_GET);
	req.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8");
	req.addHeader("Accept-Language", "zh-CN,zh;q=0.9");
	req.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.181 Safari/537.36");
	client.execute(&req);  
	XString hd = req.getResponseHeader("Set-Cookie");
	client.close();
}

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

	test_httpclient();
	return 0;

	//--------begin test------------
	UIFactory::init();
	win = (VWindow *) UIFactory::fastBuild("file://skin/vtest.xml", "main-page", NULL);

	win->findById("ext_btn_1")->setListener(new BtnListener());
	// VList *list = (VList *)(win->findById("list"));
	// list->setModel(new ListModel());
	// VTree *tree = (VTree *)(win->findById("tree"));
	// tree->setModel(UIFactoryV::buildTreeNode(UIFactoryV::buildNode("file://skin/vtest.xml", "my-tree")));
	VComboBox *combo = (VComboBox *)(win->findById("combo"));
	combo->getList()->setModel(new ListModel());

	win->createWnd();
	win->show();
	win->msgLoop();
	UIFactory::destory(win->getNode());

	return 0;
}
