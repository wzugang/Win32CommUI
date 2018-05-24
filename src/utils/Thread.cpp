#include "Thread.h"
#include <windows.h>
#include <stdio.h>
#include <process.h>

static int ThreadNum = 0;
Thread::Thread() {
	++ThreadNum;
	mTarget = NULL;
	mHandle = NULL;
	mEvent = NULL;
}

Thread::Thread(Runnable *r) {
	++ThreadNum;
	mTarget = r;
	mHandle = NULL;
	mEvent = NULL;
}

XString Thread::getName() {
	if (mName.length() == 0) {
		char buf[24];
		sprintf(buf, "Thread-%d", GetThreadId(mHandle));
		return buf;
	}
	return mName;
}

void Thread::start() {
	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	// CreateThread(&sa, 0, __onRun, this, 0, &mThradId);
	mHandle = (HANDLE)_beginthreadex(&sa, 0, _threadAddr, this, 0, NULL);
}

void Thread::sleep(int millis) {
	Sleep(millis);
}

void Thread::onRun() {
	if (mTarget != NULL) {
		mTarget->onRun();
	}
}

unsigned int __stdcall Thread::_threadAddr(void *param) {
	Thread *t = (Thread *)param;
	t->onRun();
	return 0;
}

void Thread::join(int millis) {
	millis = millis == 0 ? INFINITE : millis;
	WaitForSingleObject(mHandle, millis);
}

void Thread::wait(int millis) {
	if (mEvent == NULL) {
		mEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	millis = millis == 0 ? INFINITE : millis;
	WaitForSingleObject(mEvent, millis);
}

void Thread::notify() {
	if (mEvent == NULL) {
		return;
	}
	PulseEvent(mEvent);
}

Thread::~Thread() {
	if (mHandle != NULL) {
		CloseHandle(mHandle);
	}
	if (mEvent != NULL) {
		CloseHandle(mEvent);
	}
}

/*
void Thread::stop() {
}*/



Synchronized::Synchronized() {
	InitializeCriticalSection(&mCS);
}

void Synchronized::begin() {
	EnterCriticalSection(&mCS);
}

void Synchronized::end() {
	LeaveCriticalSection(&mCS);
}

Synchronized::~Synchronized() {
	DeleteCriticalSection(&mCS);
}

Semaphore::Semaphore(int initNum, int maxNum) {
	mHandle = CreateSemaphore(NULL, initNum, maxNum, NULL);
}

WaitObject::WaitObject() {
	mHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void WaitObject::wait(int millis) {
	millis = millis == 0 ? INFINITE : millis;
	WaitForSingleObject(mHandle, millis);
}

void WaitObject::notify() {
	PulseEvent(mHandle);
}

WaitObject::~WaitObject() {
	CloseHandle(mHandle);
}
