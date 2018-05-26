#pragma once

class InputStream {
public:
	InputStream() {}

	// Returns an estimate of the number of bytes that can be read
	virtual int available() = 0;
	
	// Reads up to len bytes of data from the input stream into buf
	// return real read number bytes, 0 is reached the end of stream
	virtual int read(void *buf, int len) = 0;

	virtual void close() {}

	virtual ~InputStream() {}
};


class OutputStream {
public:
	OutputStream() {}

	//  Writes len bytes from buf, return real write number bytes
	virtual int write(void *buf, int len) = 0;

	// Flushes this output stream and forces any buffered output bytes to be written out
	virtual void flush() {}

	//  Closes this output stream and free any system resources
	virtual void close() {}

	virtual ~OutputStream() {}
};

class IOBuffer {
public:
	IOBuffer(int initCapacity);
	void *getBuffer();
	int getSize();
	void append(void *data, int len);
	virtual ~IOBuffer();
protected:
	void ensure(int n);
protected:
	void *mBuf;
	int mSize;
	int mCapacity;
};

class BufferOutputStream : public OutputStream, public IOBuffer {
public:
	BufferOutputStream();
	virtual int write(void *buf, int len);
};

class BufferInputStream : public InputStream, public IOBuffer {
public:
	BufferInputStream();
	virtual int available();
	virtual int read(void *buf, int len);
protected:
	int mReadNum;
};
