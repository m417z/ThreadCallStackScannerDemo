#include <stdio.h>

#include <windows.h>

#include "ThreadsCallStackIterate.h"

typedef struct _ThreadCallStackIterateParam {
	void* startAddress;
	void* endAddress;
	BOOL found;
} ThreadCallStackIterateParam;

static BOOL ThreadCallStackIterateProc(HANDLE threadHandle, void* stackFrameAddress, void* userData) {
	ThreadCallStackIterateParam* param = (ThreadCallStackIterateParam*)userData;
	if (stackFrameAddress >= param->startAddress && stackFrameAddress < param->endAddress) {
		param->found = TRUE;
		return FALSE; // Stop iterating
	}
	return TRUE; // Continue iterating
}

static BOOL IsWinmmRunning() {
	HANDLE user32Module = GetModuleHandle(L"winmm.dll");
	if (!user32Module) {
		return FALSE;
	}

	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)user32Module;
	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

	DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

	ThreadCallStackIterateParam param;
	param.startAddress = (void*)user32Module;
	param.endAddress = (void*)((BYTE*)user32Module + sizeOfImage);
	param.found = FALSE;

	ThreadsCallStackIterate(ThreadCallStackIterateProc, &param);

	return param.found;
}

static DWORD CALLBACK PlaySoundThread(LPVOID lpParameter) {
	while (TRUE) {
		PlaySound(L"SystemAsterisk", NULL, SND_ALIAS | SND_SYNC);
		Sleep(3000);
	}
	return 0;
}

int main() {
	HANDLE thread = CreateThread(
		NULL, // default security attributes
		0,    // default stack size
		PlaySoundThread,
		NULL, // no arguments to thread function
		0,    // default creation flags
		NULL  // receive thread identifier
	);

	BOOL isRunning = FALSE;
	while (TRUE) {
		Sleep(100);
		BOOL isRunningNew = IsWinmmRunning();
		if (isRunning != isRunningNew) {
			isRunning = isRunningNew;
			printf("IsWinmmRunning(): %d\n", isRunning);
		}
	}
}
