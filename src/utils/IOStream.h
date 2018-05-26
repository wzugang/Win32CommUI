#pragma once

class InputStream {
public:
	InputStream() {}

	// Returns an estimate of the number of bytes that can be read
	virtual int available() = 0;
	
	// Reads up to len bytes of data from the input stream into buf
	// return real read number bytes, 0 is reached the end of stream
	virtual int read(void *buf, int len) = 0;

	virtual void close() = 0;

	virtual ~InputStream() {}
};


class OutputStream {
public:
	OutputStream() {}

	//  Writes len bytes from buf, return real write number bytes
	virtual int write(void *buf, int len) = 0;

	// Flushes this output stream and forces any buffered output bytes to be written out
	virtual void flush() = 0;

	//  Closes this output stream and free any system resources
	virtual void close() = 0;

	virtual ~OutputStream() {}
};

class BufferOutputStream : public OutputStream {
public:
	BufferOutputStream(int capacity = 512);
	virtual int write(void *buf, int len);
	virtual void flush();
	virtual void close();
	virtual ~BufferOutputStream();
protected:
	char *mBuf;
	int mCapacity;
	int mLen;
};
