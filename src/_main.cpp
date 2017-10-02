#include <stdio.h>
#include "XmlParser.h"
#include "UIFactory.h"
#include <ShellAPI.h>
#include <ShlObj.h>

int aa__main(int argc, char **argv) {
	

	XmlPartLoader loader("D:\\CPP\\WinUI\\Debug\\ui-part.txt");

	printf("---------------------------------\n");
	IShellFolder   *g_psfCurFolder = NULL;  
	SHGetDesktopFolder(&g_psfCurFolder);  
	LPENUMIDLIST   pEnum;//其实就是EnumList *  
	if(SUCCEEDED(g_psfCurFolder->EnumObjects(NULL,SHCONTF_FOLDERS | SHCONTF_NONFOLDERS,&pEnum)))  
	{  
		LPITEMIDLIST   pidlTemp;  
		DWORD          dwFetched = 1;    
		while(S_OK == (pEnum->Next(1, &pidlTemp, &dwFetched)) && dwFetched)  
		{  
			char szPath[255];  
			SHGetPathFromIDList(pidlTemp, szPath);                             
			printf("[%s] \n", szPath);
		}  
	}  
	
	return 0;
}