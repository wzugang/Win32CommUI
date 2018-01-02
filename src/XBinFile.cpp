#include "XBinFile.h"
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <stdarg.h>
#include <string>
/*
	Bin File Struct
	(4 byte) magic num
	(4 byte) num of items
	[
		{(128 byte) name, (4 byte)data len, data, (2 byte) \0\0 }  item struct
	, ...] an array of items
	(4 byte) magic num end
*/

extern char *ReadFileContent(const char *path, int *pLen);

static const int BIN_FILE_MAGIC_NUM = 0x8A9F3DEA;
static const int ITEM_NAME_LEN = 128;

XBinFile::XBinFile() {
	mBuffer = NULL;
	mItems = NULL;
	mItemsNum = 0;
}

XBinFile * XBinFile::getInstance() {
	static XBinFile *ins = new XBinFile();
	return ins;
}

void XBinFile::load( const char *path ) {
	int len = 0;
	char *cnt = ReadFileContent(path, &len);
	char *p = cnt;
	mBuffer = cnt;
	if (cnt == NULL || len < 4) {
		mItemsNum = 0;
		return;
	}
	if (*(int *)cnt != BIN_FILE_MAGIC_NUM) {
		mItemsNum = 0;
		return;
	}
	p += sizeof(int);// skip magic num
	mItemsNum = *(int *)p;
	mItems = (Item*) malloc(sizeof(Item) * mItemsNum);
	p += sizeof(int);
	for (int i = 0; i < mItemsNum; ++i) {
		mItems[i].mName = p;
		p += ITEM_NAME_LEN;
		mItems[i].mLen = *(int *)p;
		p += sizeof(int);
		mItems[i].mData = p;
		p += mItems[i].mLen;
		p += 2; // skip 2 byte \0\0
	}
	if (*(int *)p != BIN_FILE_MAGIC_NUM) {
		mItemsNum = 0;
	}
}

XBinFile::~XBinFile() {
	delete[] mBuffer;
}

void *XBinFile::find( const char *name, int *len ) {
	for (int i = 0; i < mItemsNum; ++i) {
		if (strcmp(name, mItems[i].mName) == 0) {
			*len = mItems[i].mLen;
			return mItems[i].mData;
		}
	}
	*len = 0;
	return NULL;
}
static void WriteData(FILE *f, void *data, int len) {
	int less = len;
	while (less > 0) {
		int w = fwrite(data, 1, less, f);
		if (w <= 0) break;
		less -= w;
	}
}
static void ReplaceXml(char *cnt) {
	while (TRUE) {
		char *p = strstr(cnt, "file://");
		if (p == NULL) break;
		memcpy(p, "xbin", 4);
		cnt = p;
	}
}
static void WriteData(std::string path, FILE *f, int *num) {
	bool isXml = path.find_last_of(".xml") > 0;
	int len = 0;
	char name[ITEM_NAME_LEN] = {0};
	char *cnt = ReadFileContent(path.c_str(), &len);
	strcpy(name, path.c_str());
	fwrite(name, ITEM_NAME_LEN, 1, f);
	fwrite(&len, sizeof(int), 1, f);
	if (isXml) {
		ReplaceXml(cnt);
	}
	WriteData(f, cnt, len);
	char zero[2] = {0};
	fwrite(zero, 2, 1, f);
	delete[] cnt;
	*num = *num + 1;
	printf("Write %d:  %s : %d \n", *num, path.c_str(), len);
}

static void TraceDir(std::string path, FILE *f, int *num) {
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(path.c_str(), &data);
	if(INVALID_HANDLE_VALUE == hFind) {
		return;
	}
	std::string prefix = path.substr(0, path.find_last_of('//') + 1);
	while(TRUE) {
		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {  
			if(data.cFileName[0] != '.') {
				TraceDir(prefix + data.cFileName+ "//" + "*.*", f, num);  
			}
		} else {
			WriteData(prefix + data.cFileName, f, num);
		} 
		if(! FindNextFile(hFind, &data)) {
			break;
		}
	}  
	FindClose(hFind);   
}  

void BuildBinFile(const char *destBinName, const char *inputDirs[]) {
	int num = 0;
	FILE *f = fopen(destBinName, "wb");
	fwrite(&BIN_FILE_MAGIC_NUM, sizeof(int), 1, f);
	fwrite(&num, sizeof(int), 1, f);
	for (int i = 0; inputDirs[i] != NULL; ++i) {
		TraceDir(inputDirs[i], f, &num);
	}
	fwrite(&BIN_FILE_MAGIC_NUM, sizeof(int), 1, f);
	fseek(f, sizeof(int), SEEK_SET);
	fwrite(&num, sizeof(int), 1, f);
	fclose(f);
}
