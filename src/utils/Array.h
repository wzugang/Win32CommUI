#pragma once
#include <stdlib.h>
// #include <type_traits>

// template<typename T> char TArray_IsPtr(T * &a);
// template<typename T> char TArray_IsPtr(const T * &a);
// int TArray_IsPtr(...);
// #define TARRAY_ISPTR(v) (sizeof(TArray_IsPtr(v)) == sizeof(char))

template <typename T>  
class Array {
public:
	Array(int initialCapacity = 0) {
		mNum = 0;
		mCapacity = initialCapacity > 0 ? initialCapacity : 20;
		mElems = (T*)malloc(sizeof(T) * mCapacity);
	}

	Array(const Array<T> &a) {
		copy(a);
	}

	Array &operator=(const Array<T> &a) {
		copy(a);
		return *this;
	}

	void add(const T &e) {
		ensureMem(mNum + 1);
		new (mElems + mNum) T(e);
		++mNum;
	}

	void add(int index, const T &e) {
		if (index < 0 || index > mNum) {
			return;
		}
		ensureMem(mNum + 1);
		for (int i = mNum - 1; i >= 0 && i >= index; --i) {
			new (mElems + i + 1) T(mElems[i]);
		}
		new (mElems + index) T(e);
		++mNum;
	}

	void clear() {
		for (int i = 0; i < mNum; ++i) {
			T *a = mElems + i;
			a->~T();
		}
		mNum = 0;
	}

	bool contains(const T &e) {
		return indexOf(e) >= 0;
	}

	inline T &get(int index) {
		return mElems[index];
	}

	int indexOf(const T &e) {
		for (int i = 0; i < mNum; ++i) {
			if (mElems[i] == e) {
				return i;
			}
		}
		return -1;
	}

	inline bool isEmpty() {
		return mNum == 0;
	}

	void remove(int index) {
		remove(index, 1);
	}

	void remove(int fromIdx, int num) {
		if (fromIdx < 0 || num <= 0 || fromIdx >= mNum) {
			return;
		}
		if (num + fromIdx > mNum) {
			num = mNum - fromIdx;
		}
		for (int i = fromIdx; i < fromIdx + num; ++i) {
			T *a = mElems + i;
			if (! std::is_pointer<T>::value) {
				a->~T();
			}
		}
		for (int i = fromIdx + num, j = fromIdx; i < mNum; ++i, ++j) {
			new (mElems + j) T(mElems[i]);
		}
		mNum -= num;
	}

	void set(int index, const T &e) {
		if (index < 0 || index >= mNum) {
			return;
		}
		new (mElems + index) T(e);
	}

	inline int size() {
		return mNum;
	}

	T* data() {
		return mElems;
	}

	~Array() {
		clear();
		free(mElems);
	}

protected:
	void copy(const Array<T> &a) {
		mNum = a.mNum;
		mCapacity = a.mCapacity;
		mElems = (T*)malloc(sizeof(T) * mCapacity);
		for (int i = 0; i < mNum; ++i) {
			new (mElems + i) T(a.mElems[i]);
		}
	}

	void ensureMem(int n) {
		if (n < mCapacity) {
			return;
		}
		mCapacity *= 2;
		if (mCapacity < n) {
			mCapacity = n + 1;
		}
		T *newElems = (T *)malloc(sizeof(T) * mCapacity);
		for (int i = 0; i < mNum; ++i) {
			new (newElems + i) T(mElems[i]);
		}
		clear();
		free(mElems);
		mElems = newElems;
	}
protected:
	int mNum;
	int mCapacity;
	T *mElems;
};

template <typename T>  
class Array<T*> {
public:
	Array(int initialCapacity = 0) {
		mNum = 0;
		mCapacity = initialCapacity > 0 ? initialCapacity : 20;
		mElems = (T**)malloc(sizeof(T*) * mCapacity);
	}

	Array(const Array<T*> &a) {
		copy(a);
	}

	Array &operator=(const Array<T*> &a) {
		copy(a);
		return *this;
	}

	void add(T *e) {
		ensureMem(mNum + 1);
		mElems[mNum] = e;
		++mNum;
	}

	void add(int index, T *e) {
		if (index < 0 || index > mNum) {
			return;
		}
		ensureMem(mNum + 1);
		for (int i = mNum - 1; i >= 0 && i >= index; --i) {
			mElems[i + 1] = mElems[i];
		}
		mElems[index] = e;
		++mNum;
	}

	void clear() {
		mNum = 0;
	}

	bool contains(const T *e) {
		return indexOf(e) >= 0;
	}

	inline T *get(int index) {
		return mElems[index];
	}

	int indexOf(const T *e) {
		for (int i = 0; i < mNum; ++i) {
			if (mElems[i] == e) {
				return i;
			}
		}
		return -1;
	}

	inline bool isEmpty() {
		return mNum == 0;
	}

	void remove(int index) {
		remove(index, 1);
	}

	void remove(int fromIdx, int num) {
		if (fromIdx < 0 || num <= 0 || fromIdx >= mNum) {
			return;
		}
		if (num + fromIdx > mNum) {
			num = mNum - fromIdx;
		}
		for (int i = fromIdx + num, j = fromIdx; i < mNum; ++i, ++j) {
			mElems[j] = mElems[i];
		}
		mNum -= num;
	}

	void set(int index, T *e) {
		if (index < 0 || index >= mNum) {
			return;
		}
		mElems[index] = e;
	}

	inline int size() {
		return mNum;
	}

	T** data() {
		return mElems;
	}

	~Array() {
		clear();
		free(mElems);
	}

protected:
	void copy(const Array<T*> &a) {
		mNum = a.mNum;
		mCapacity = a.mCapacity;
		mElems = (T**)malloc(sizeof(T*) * mCapacity);
		for (int i = 0; i < mNum; ++i) {
			mElems[i] = a.mElems[i];
		}
	}

	void ensureMem(int n) {
		if (n < mCapacity) {
			return;
		}
		mCapacity *= 2;
		if (mCapacity < n) {
			mCapacity = n + 1;
		}
		mElems = (T **)realloc(mElems, sizeof(T*) * mCapacity);
	}
protected:
	int mNum;
	int mCapacity;
	T **mElems;
};
