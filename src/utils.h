#pragma  once

class XImage {
public:
	static XImage *loadFromFile(const char *path);
	static XImage *loadFromResource(int resId);
	static XImage *loadFromResource(const char * resName);
	static XImage *create(int width, int height);
	~XImage();
protected:
	XImage();
protected:
	void *mBits;
};