#include "thread.h"

#ifdef _WIN32
#define WINAPI __stdcall
#define DWORD unsigned int
#define HANDLE void*

extern "C" {
	int WINAPI CloseHandle(HANDLE hObject);
	void* WINAPI CreateMutexA(void* lpMutexAttributes, int bInitialOwner, const char* lpName);
	void* WINAPI CreateThread(void* lpThreadAttributes, int dwStackSize, io::thread::fnroutine lpStartAddress, void* lpParameter, int dwCreationFlags, unsigned* lpThreadId);
	DWORD WINAPI ResumeThread(HANDLE hThread);
	DWORD WINAPI SuspendThread(HANDLE hThread);
	int WINAPI WaitForSingleObject(HANDLE hHandle, int dwMilliseconds);
}

void io::thread::start(fnroutine proc, void* v) {
	if(s != 0xFFFFFFFF)
		return;
	unsigned ThreadID;
	s = (int)CreateThread(0, 0, proc, v, 0, &ThreadID);
}

void io::thread::suspend() {
	SuspendThread((HANDLE)s);
}

void io::thread::resume() {
	ResumeThread((HANDLE)s);
}

void io::thread::close() {
	if(s == 0xFFFFFFFF)
		return;
	CloseHandle((void*)s);
	s = 0xFFFFFFFF;
}

void io::thread::join() {
	WaitForSingleObject((void*)s, 0xFFFFFFFF);
}

#endif // _WIN32