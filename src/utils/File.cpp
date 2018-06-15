#include "File.h"
#include <sys\stat.h>
#include <windows.h>

static const char SEP_CHAR = '/';

File::File(const char *path) {
	mPath.append(path);
	mPath.replace('\\', SEP_CHAR);
	mPrefixLen = 0;
	if (mPath.length() > 3) {
		char *p = mPath.str();
		if (p[1] == ':' && p[2] == SEP_CHAR) {
			mPrefixLen = 3;
		}
	}
	if (mPath.length() > mPrefixLen) {
		char *p = mPath.str();
		if (p[mPath.length() - 1] == SEP_CHAR) {
			// delete last '/'
			mPath.del(mPath.length() - 1, mPath.length());
		}
	}
}

XString File::getString() {
	int index = mPath.lastIndexOf(SEP_CHAR, mPath.length() - 1);
	if (index < mPrefixLen) {
		return mPath.subString(mPrefixLen, mPath.length());
	}
	return mPath.subString(index + 1, mPath.length());
}

XString File::getParent() {
	int index = mPath.lastIndexOf(SEP_CHAR, mPath.length() - 1);
	if (index < mPrefixLen) {
		return "";
	}
	return mPath.subString(0, index);
}

File * File::getParentFile() {
	XString pf = getParent();
	if (pf.length() == 0) {
		return NULL;
	}
	return new File(pf.str());
}

XString File::getPath() {
	return mPath;
}

bool File::isAbsolute() {
	return mPrefixLen > 0;
}

XString File::getAbsolutePath() {
	if (isAbsolute()) {
		return mPath;
	}
	char path[260];
	GetCurrentDirectory(260, path);
	XString p(path);
	p.append(mPath);
	return p;
}

bool File::exists() {
	DWORD dwAttrib = GetFileAttributes(mPath.str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES;
}

bool File::isDirectory() {
	DWORD dwAttrib = GetFileAttributes(mPath.str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

bool File::isFile() {
	DWORD dwAttrib = GetFileAttributes(mPath.str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES&& ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

int File::length() {
	struct _stat s;
	_stat(mPath.str(), &s);
	return s.st_size;
}

bool File::del() {
	if (! exists()) {
		return true;
	}
	if (isDirectory()) {
		return RemoveDirectory(mPath.str());
	}
	return DeleteFile(mPath.str());
}

bool File::rename(const char *path) {
	return MoveFile(mPath.str(), path);
}

bool File::mkdir() {
	// CreateDirectory()
	return true;
}

bool File::mkdirs() {
	return true;
}

XString* File::list(int &num) {
	return NULL;
}





FileMonitor::FileMonitor( const char *path ) {
	mHandle = INVALID_HANDLE_VALUE;
	if (path != NULL) {
		mHandle = CreateFile(path, GENERIC_READ | GENERIC_WRITE | FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 
			FILE_FLAG_BACKUP_SEMANTICS, NULL);
	}
	mBuffer = (char *)malloc(1024);
	mListenStatus = false;
}

bool FileMonitor::listen(int flags) {
	mListenStatus = false;
	if (mHandle == INVALID_HANDLE_VALUE) {
		return false;
	}
	DWORD rb = 0;
	memset(mBuffer, 0, 1024);
	mListenStatus = ReadDirectoryChangesW(mHandle, mBuffer, 1024, true, flags, &rb, NULL, NULL);
	return mListenStatus;
}

int FileMonitor::getNotifyNum() {
	if (! mListenStatus) {
		return 0;
	}
	FILE_NOTIFY_INFORMATION *notify = (FILE_NOTIFY_INFORMATION*)mBuffer;
	int num = 1;
	while (notify->NextEntryOffset != 0 && notify->FileNameLength > 0) {
		char *p = (char *)notify;
		notify = (FILE_NOTIFY_INFORMATION *)(p + notify->NextEntryOffset);
		num++;
	}
	return num;
}

FileMonitor::FileAction FileMonitor::getNotify( int idx, char *fileName ) {
	static char path[256];
	FILE_NOTIFY_INFORMATION *notify = (FILE_NOTIFY_INFORMATION*)mBuffer;
	while (idx > 0) {
		char *p = (char *)notify;
		notify = (FILE_NOTIFY_INFORMATION *)(p + notify->NextEntryOffset);
		--idx;
	}
	if (fileName != NULL) {
		WideCharToMultiByte(CP_ACP, 0, notify->FileName, 
			notify->FileNameLength/2, path, sizeof(path), NULL, NULL);
		strcpy(fileName, path);
	}
	return (FileAction)notify->Action;
}

FileMonitor::~FileMonitor() {
	if (mHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(mHandle);
	}
	free(mBuffer);
}



