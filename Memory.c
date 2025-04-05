/*
 * KNSoft.SlimDetours (https://github.com/KNSoft/KNSoft.SlimDetours) Memory Management
 * Copyright (c) KNSoft.org (https://github.com/KNSoft). All rights reserved.
 * Licensed under the MIT license.
 */

#include <phnt_windows.h>
#include <phnt.h>

#include "Memory.h"

static HANDLE _detour_memory_heap = NULL;

static
_Ret_notnull_
HANDLE
detour_memory_init(VOID)
{
	HANDLE hHeap;

	/* Initialize private heap */
	hHeap = RtlCreateHeap(HEAP_NO_SERIALIZE | HEAP_GROWABLE, NULL, 0, 0, NULL, NULL);
	if (hHeap == NULL)
	{
		//DETOUR_TRACE("RtlCreateHeap failed, fallback to use process default heap\n");
		hHeap = NtCurrentPeb()->ProcessHeap;
	}

	return hHeap;
}

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
detour_memory_alloc(
	_In_ SIZE_T Size)
{
	/*
	 * detour_memory_alloc is called BEFORE any other detour_memory_* functions,
	 * and only one thread that owning pending transaction could reach here,
	 * so it's safe to do the initialzation here and not use a lock.
	 */
	if (_detour_memory_heap == NULL)
	{
		_detour_memory_heap = detour_memory_init();
	}

	return RtlAllocateHeap(_detour_memory_heap, 0, Size);
}

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
detour_memory_realloc(
	_Frees_ptr_opt_ PVOID BaseAddress,
	_In_ SIZE_T Size)
{
	return RtlReAllocateHeap(_detour_memory_heap, 0, BaseAddress, Size);
}

BOOL
detour_memory_free(
	_Frees_ptr_ PVOID BaseAddress)
{
	return RtlFreeHeap(_detour_memory_heap, 0, BaseAddress);
}

VOID
detour_memory_uninitialize(VOID)
{
	if (_detour_memory_heap != NULL)
	{
		RtlDestroyHeap(_detour_memory_heap);
		_detour_memory_heap = NULL;
	}
}
