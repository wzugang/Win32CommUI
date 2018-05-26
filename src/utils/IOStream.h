#pragma once
class InputStream {
public:
	InputStream();

	// Returns an estimate of the number of bytes that can be read
	int available();
	
	// Reads up to len bytes of data from the input stream into buf
	// return real read number bytes, 0 is reached the end of stream
	int read(void *buf, int len);


	virtual ~InputStream();
};


class OutputStream {
public:
	OutputStream();



	virtual ~OutputStream();
};

