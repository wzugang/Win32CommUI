#pragma once

class BinFile {
public:
	static BinFile *getInstance();
	void load(const char *path);
	void *find(const char *name, int *len);
protected:
	BinFile();
	~BinFile();
	struct Item {
		char *mName;
		int mLen;
		void *mData;
	};
	void *mBuffer;
	Item *mItems;
	int mItemsNum;
};

// @param inputDirs is an array of String, the end is NULL
void BuildBinFile(const char *destBinName, const char *inputDirs[]);