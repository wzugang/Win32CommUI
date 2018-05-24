#pragma once
#include "XString.h"

class Runnable {
public:
	virtual void onRun() = 0;
	virtual ~Runnable() {}
};

class Thread : public Runnable {
public:
	Thread();
	Thread(Runnable *r);

	XString getName();
	void setName(const XString &name);

	void start();
	// void stop();
	void sleep(int millis);
	void join(int millis = 0);

	void wait(int millis = 0);
	void notify();
	virtual ~Thread();
protected:
	virtual void onRun();
	static unsigned int __stdcall _threadAddr(void *param);
protected:
	XString mName;
	Runnable *mTarget;
	void *mHandle, *mEvent;
};

class Synchronized {
public:
	Synchronized();
	void begin();
	void end();
	~Synchronized();
protected:
	CRITICAL_SECTION mCS;
};

class Semaphore {
public:
	Semaphore(int initNum, int maxNum);
protected:
	void *mHandle;
};

class WaitObject {
public:
	WaitObject();
	void wait(int milis = 0);
	void notify();
	~WaitObject();
protected:
	void *mHandle;
};