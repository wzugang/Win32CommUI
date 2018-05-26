#pragma once
#include "XString.h"

class File {
public:
	File(const char *path);
	XString getString();
	XString getParent();
	File *getParentFile();
	XString getPath();
	bool isAbsolute();
	XString getAbsolutePath();
	bool exists();
	bool isDirectory();
	bool isFile();
	// file length
	int length();
	// delete this file or empty directory
	bool del();
	bool mkdir();
	bool mkdirs();
	bool rename(const char *path);
	XString* list(int &num);

protected:
	XString mPath;
	int mPrefixLen;
};