#pragma once

template <typename T>
class LinkList {
public:
	LinkList() {
		mFirst = NULL;
		mLast = NULL;
		mSize = 0;
	}

	LinkList(const LinkList<T> &a) {
		copy(a);
	}

	LinkList &operator=(const LinkList<T> &a) {
		copy(a);
		return *this;
	}

	void add(const T &e) {
		linkLast(e);
	}

	void add(int index, const T &e) {
		if (index < 0 || index > mSize) {
			return;
		}
		if (index == mSize) {
			linkLast(e);
		} else {
			linkBefore(e, node(index));
		}
	}

	T & get(int index) {
		Node *n = node(index);
		return n->mData;
	}

	void set(int index, const T &e) {
		if (index < 0 || index >= mSize) {
			return;
		}
		Node *n = node(index);
		T *t = (T *)(& n->mData);
		t->~T();
		new (& n->mData) T(e);
		// n->mData = e;
	}

	inline bool isEmpty() {
		return mSize == 0;
	}

	inline int size() {
		return mSize;
	}

	T & getFirst() {
		return get(0);
	}

	T & getLast() {
		return get(mSize - 1);
	}

	void remove(int index) {
		Node *n = node(index);
		if (n != NULL) {
			unlink(n);
		}
	}

	void remove(const T &e) {
		for (Node *f = mFirst; f != NULL; f = f->mNext) {
			if (f->mData == e) {
				unlink(f);
				break;
			}
		}
	}

protected:
	struct Node {
		Node(Node *prev, Node *next, const T &data) : mData(data) {
			mPrev = prev;
			mNext = next;
		}
		Node *mPrev, *mNext;
		T mData;
	};

	void copy(const LinkList<T> &a) {
		mSize = a.mSize;
		mFirst = mLast = NULL;
		for (Node *f = a.mFirst; f != NULL; f = f->mNext) {
			Node *n = new Node(mLast, NULL, f->mData);
			if (mFirst == NULL) {
				mFirst = n;
				mLast = n;
			} else {
				mLast->mNext = n;
			}
			mLast = n;
		}
	}

	void linkLast(const T &e) {
		Node *l = mLast;
		Node *newNode = new Node(l, NULL, e);
		mLast = newNode;
		if (l == NULL) {
			mFirst = newNode;
		} else {
			l->mNext = newNode;
		}
		++mSize;
	}

	void linkFirst(const T &e) {
		Node *f = mFirst;
		Node *newNode = new Node(NULL, f, e);
		mFirst = newNode;
		if (f == NULL) {
			mLast = newNode;
		} else {
			f->mPrev = newNode;
		}
		++mSize;
	}

	void linkBefore(const T & e, Node *succ) {
		Node *pred = succ.mPrev;
		Node *newNode = new Node(pred, succ, e);
		succ->mPrev = newNode;
		if (pred == NULL) {
			mFirst = newNode;
		} else {
			pred->mNext = newNode;
		}
		++mSize;
	}

	Node *node(int index) {
		if (index < 0 || index >= mSize) {
			return NULL;
		}
		Node *f = NULL;
		if (index < (mSize / 2)) {
			f = mFirst;
			for (int i = 0; i < index; i++) {
				f = f->mNext;
			}
		} else {
			f = mLast;
			for (int i = mSize - 1; i > index; i--) {
				f = f->mPrev;
			}
		}
		return f;
	}

	void unlink(Node *n) {
		if (n == NULL) {
			return;
		}
		Node *next = n->mNext;
		Node *prev = n->mPrev;

		if (prev == NULL) { // n is first
			mFirst = next;
		}
		if (next == NULL) { // n is last
			mLast = prev;
		}

		if (next != NULL) {
			next->mPrev = prev;
		}
		if (prev != NULL) {
			prev->mNext = next;
		}
		--mSize;
		delete n;
	}

protected:
	Node *mFirst, *mLast;
	int mSize;
};