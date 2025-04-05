#pragma once

typedef BOOL(*ThreadCallStackIterCallback)(
	HANDLE threadHandle,
	void* stackFrameAddress,
	void* userData);

void ThreadsCallStackIterate(
	ThreadCallStackIterCallback callback,
	void* userData);
