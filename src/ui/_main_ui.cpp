#include "UIFactory.h"
#include <Windows.h>
#include <CommCtrl.h>
#include "XmlParser.h"
#include "XExt.h"
#include <atlimage.h>
#include "VComponent.h"
#include "VExt.h"

VWindow *win;

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

	UIFactoryV::init();

	win = (VWindow *) UIFactoryV::fastBuild("file://skin/vtest.xml", "main-page", NULL);
	win->createWnd();
	win->show();
	win->msgLoop();
	UIFactoryV::destory(win->getNode());
	return 0;
}
