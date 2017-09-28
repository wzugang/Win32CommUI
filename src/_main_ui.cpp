#include "UIFactory.h"
#include "XComponent.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "XmlParser.h"
#include "XExt.h"



HINSTANCE hInst;
XComponent *rootComponent;
HWND mainWnd, tmpWnd;

ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void InitMyTable();
void InitMyTree();
void InitMyTab2();

class ButtonListener : public XListener {
public:
	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (msg == WM_COMMAND) {
			DWORD wndId = LOWORD(wParam);
			char txt[32];
			XComponent *src = evtSource->getChildById(wndId);
			if (src == NULL) return false;
			GetWindowText(src->getWnd(), txt, 32);
			MessageBox(evtSource->getWnd(), txt, "", MB_OK);
			return false;
		}
		return false;
	}
};

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable = NULL;

	InitCommonControls();
	XmlParser parser;
	parser.parseFile("D:\\CPP\\WinUI\\Debug\\base.xml");
	XmlNode *rootNode = parser.getRoot();
	rootComponent = UIFactory::build(rootNode, hInstance);

	MyRegisterClass(hInstance);
	if (!InitInstance (hInstance, nCmdShow)) {
		return FALSE;
	}

	/*rootComponent->createWndTree(mainWnd);
	RECT rr = {0};
	GetClientRect(mainWnd, &rr);
	SendMessage(mainWnd, WM_SIZE, 0, rr.right | (rr.bottom << 16));*/

	// rootComponent->setListener(new ButtonListener());
	// rootComponent->findById("sub-layout")->setListener(new ButtonListener());

	// InitMyTable();
	// InitMyTree();
	InitMyTab2();

	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UIFactory::destory(rootNode);

	return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL; //LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTWINUI));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= "TestWinUI";
	wcex.hIconSm		= NULL; //LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	HWND hWnd;
	hInst = hInstance;
	mainWnd = hWnd = CreateWindow("TestWinUI", NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	if (!hWnd) {
		return FALSE;
	}
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) {
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// 分析菜单选择:
		/* switch (wmId) {
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		} */
		break;
	case WM_CREATE: {
		rootComponent->createWndTree(hWnd);
		// tmpWnd = CreateWindow("BUTTON", "什么怎样", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 5, 5, 150, 25,  hWnd, (HMENU)2000, hInst, NULL);
		// tmpWnd = CreateWindow("ComboBox", NULL,  WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_AUTOHSCROLL,  5, 5, 150, 10, hWnd, NULL, hInst, NULL);
		break;}
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN :{
		hdc = (HDC)wParam;
		HWND btn = (HWND)lParam;
		SetTextColor(hdc, RGB(255, 0, 0));
		SetBkMode(hdc, OPAQUE);
		SetBkColor(hdc, RGB(0, 180, 70));
		return (LRESULT)CreateSolidBrush(RGB(150, 150, 0));
		break;
	}
	case WM_SIZE:
		if (rootComponent && lParam > 0) {
			rootComponent->layout(LOWORD(lParam) | XComponent::MS_ATMOST, HIWORD(lParam) | XComponent::MS_ATMOST);
		}
		break;
	/* case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此添加任意绘图代码...
		EndPaint(hWnd, &ps);
		break; */
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

class MyTable : public XExtTable {
public:
	MyTable(XTable *tab) : XExtTable(tab) {
	}
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
		XExtTable::getItem(row, col, item);
		sprintf(v, "%d  %d", row, col);
		item->pszText = v;
	}
};

void InitMyTable() {
	MyTable *t = new MyTable((XTable*) rootComponent->findById("tab"));
	t->apply();
}

void InitMyTree() {
	XTree * t = (XTree*) rootComponent->findById("tab");
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

class Tab2Listener : public XListener {
public:
	Tab2Listener(XTab2 *tab) {
		mTab2 = tab;
	}
	virtual bool onEvent(XComponent *evtSource, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (msg == WM_NOTIFY) {
			NMHDR *hd = (NMHDR *)lParam;
			if (hd->hwndFrom == mTab2->getWnd()) {
				if (hd->code == TCN_SELCHANGING) { //Tab改变前
					int sel=TabCtrl_GetCurSel(mTab2->getWnd());
					ShowWindow(mTab2->getChild(sel)->getWnd(), SW_HIDE);
					return true;
				} else if (hd->code == TCN_SELCHANGE) { //Tab改变后
					int sel=TabCtrl_GetCurSel(mTab2->getWnd());
					ShowWindow(mTab2->getChild(sel)->getWnd(), SW_SHOW);
					return true;
				}
			}
		}
		return false;
	}
	XTab2 *mTab2;
};

void InitMyTab2() {
	XTab2 * t = (XTab2*) rootComponent->findById("tab");
	rootComponent->setListener(new Tab2Listener(t));

}