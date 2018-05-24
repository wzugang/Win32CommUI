#include "XFile.h"
#include <sys\stat.h>
#include <windows.h>

static const char SEP_CHAR = '/';

XFile::XFile(const char *path) {
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

XString XFile::getString() {
	int index = mPath.lastIndexOf(SEP_CHAR, mPath.length() - 1);
	if (index < mPrefixLen) {
		return mPath.subString(mPrefixLen, mPath.length());
	}
	return mPath.subString(index + 1, mPath.length());
}

XString XFile::getParent() {
	int index = mPath.lastIndexOf(SEP_CHAR, mPath.length() - 1);
	if (index < mPrefixLen) {
		return "";
	}
	return mPath.subString(0, index);
}

XFile * XFile::getParentFile() {
	XString pf = getParent();
	if (pf.length() == 0) {
		return NULL;
	}
	return new XFile(pf.str());
}

XString XFile::getPath() {
	return mPath;
}

bool XFile::isAbsolute() {
	return mPrefixLen > 0;
}

XString XFile::getAbsolutePath() {
	if (isAbsolute()) {
		return mPath;
	}
	char path[260];
	GetCurrentDirectory(260, path);
	XString p(path);
	p.append(mPath);
	return p;
}

bool XFile::exists() {
	DWORD dwAttrib = GetFileAttributes(mPath.str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES;
}

bool XFile::isDirectory() {
	DWORD dwAttrib = GetFileAttributes(mPath.str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

bool XFile::isFile() {
	DWORD dwAttrib = GetFileAttributes(mPath.str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES&& ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

int XFile::length() {
	struct _stat s;
	_stat(mPath.str(), &s);
	return s.st_size;
}

bool XFile::del() {
	if (! exists()) {
		return true;
	}
	if (isDirectory()) {
		return RemoveDirectory(mPath.str());
	}
	return DeleteFile(mPath.str());
}

bool XFile::rename(const char *path) {
	return MoveFile(mPath.str(), path);
}

bool XFile::mkdir() {
	// CreateDirectory()
	return true;
}

bool XFile::mkdirs() {
	return true;
}

XString* XFile::list(int &num) {
	return NULL;
}




