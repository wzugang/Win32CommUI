#include "IOStream.h"
#include <stdlib.h>
#include <string.h>

IOBuffer::IOBuffer(int initCapacity) {
	mCapacity = initCapacity;
	if (mCapacity <= 0) {
		mCapacity = 128;
	}
	mSize = 0;
	mBuf = malloc(mCapacity);
}

void * IOBuffer::getBuffer() {
	return mBuf;
}

int IOBuffer::getSize() {
	return mSize;
}

void IOBuffer::append(void *data, int len) {
	if (data == NULL || len <= 0) {
		return;
	}
	ensure(len);
	memcpy((char *)mBuf + mSize, data, len);
	mSize += len;
}

void IOBuffer::ensure(int n) {
	if (mCapacity - mSize > n) {
		return;
	}
	mCapacity *= 2;
	if (mCapacity < mSize + n) {
		mCapacity = mSize + n;
	}
	mBuf = realloc(mBuf, mCapacity);
}

IOBuffer::~IOBuffer() {
	free(mBuf);
}

BufferOutputStream::BufferOutputStream() : IOBuffer(256) {
}

int BufferOutputStream::write(void *buf, int len) {
	if (buf == NULL || len < 0) {
		return -1;
	}
	append(buf, len);
	return len;
}

BufferInputStream::BufferInputStream() : IOBuffer(256) {
	mReadNum = 0;
}

int BufferInputStream::available() {
	return mSize - mReadNum;
}

int BufferInputStream::read(void *buf, int len) {
	int a = available();
	if (a == 0 || len == 0) {
		return 0;
	}
	if (buf == NULL || len < 0) {
		return -1;
	}
	int rd = a < len ? a : len;
	memcpy(buf, (char *)mBuf + mReadNum, rd);
	mReadNum += rd;
	return rd;
}

