#pragma once
#include "XString.h"
#include <windows.h>

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

class FileMonitor {
public:
	enum FileAction {
		FA_ADDED = 1, FA_REMOVED, FA_MODIFIED, FA_RENAMED_OLD_NAME, FA_RENAMED_NEW_NAME
	};

	FileMonitor(const char *path);

	/* @param flags is :
			FILE_NOTIFY_CHANGE_FILE_NAME  FILE_NOTIFY_CHANGE_DIR_NAME FILE_NOTIFY_CHANGE_ATTRIBUTES
			FILE_NOTIFY_CHANGE_SIZE FILE_NOTIFY_CHANGE_LAST_WRITE FILE_NOTIFY_CHANGE_LAST_ACCESS
			FILE_NOTIFY_CHANGE_CREATION  FILE_NOTIFY_CHANGE_SECURITY
		@return listen whether is ok
	*/
	bool listen(int flags);

	int getNotifyNum();

	FileAction getNotify(int idx, char *fileName);

	~FileMonitor();
protected:
	HANDLE mHandle;
	char *mBuffer;
	bool mListenStatus;
};