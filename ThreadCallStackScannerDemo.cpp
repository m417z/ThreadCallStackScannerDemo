#include <stdio.h>

#include <windows.h>

#include "ThreadsCallStackWaitForRegions.h"

static BOOL IsWinmmRunning() {
	HANDLE winmmModule = GetModuleHandle(L"winmm.dll");
	if (!winmmModule) {
		return FALSE;
	}

	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)winmmModule;
	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

	DWORD sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

    ThreadCallStackRegionInfo regionInfo = {
        .address = (DWORD_PTR)winmmModule,
        .size = sizeOfImage
    };

    return !ThreadsCallStackWaitForRegions(&regionInfo, 1, 3, 100);
}

static DWORD CALLBACK PlaySoundThread(LPVOID lpParameter) {
	while (TRUE) {
		PlaySound(L"SystemAsterisk", NULL, SND_ALIAS | SND_SYNC);
		Sleep(3000);
	}
	return 0;
}

static DWORD CALLBACK ExceptionsThread(LPVOID lpParameter) {
	while (TRUE) {
        try {
            throw 1;
        }
        catch (...) {
        }
        //Sleep(1000);
	}
	return 0;
}

static DWORD CALLBACK LoadLibraryThread(LPVOID lpParameter) {
	while (TRUE) {
        // This will end up taking an exclusive lock on
        // ntdll!LdrpInvertedFunctionTableSRWLock during the load/unload.
        // Helps reproduce the following deadlock if no timeout is used:
        // https://devblogs.microsoft.com/oldnewthing/20250411-00/?p=111066
        HMODULE h = LoadLibrary(L"netapi32.dll");
        if (h) {
            FreeLibrary(h);
        }
        //Sleep(1000);
    }
	return 0;
}

int main() {
    HANDLE playSoundThread = CreateThread(
		NULL, // default security attributes
		0,    // default stack size
		PlaySoundThread,
		NULL, // no arguments to thread function
		0,    // default creation flags
		NULL  // receive thread identifier
	);

    // Enable for stress tests:
#if 1
    HANDLE exceptionsThread = CreateThread(
        NULL, // default security attributes
        0,    // default stack size
        ExceptionsThread,
        NULL, // no arguments to thread function
        0,    // default creation flags
        NULL  // receive thread identifier
    );
#endif
#if 1
    HANDLE exceptionsThread2 = CreateThread(
        NULL, // default security attributes
        0,    // default stack size
        LoadLibraryThread,
        NULL, // no arguments to thread function
        0,    // default creation flags
        NULL  // receive thread identifier
    );
#endif

	BOOL isRunning = FALSE;
	while (TRUE) {
		Sleep(100);
		BOOL isRunningNew = IsWinmmRunning();
		if (isRunning != isRunningNew || TRUE) {
			isRunning = isRunningNew;
			printf("IsWinmmRunning(): %d\n", isRunning);
		}
	}
}
