#pragma once

NTSTATUS
detour_thread_suspend(
    _Outptr_result_maybenull_ PHANDLE* SuspendedHandles,
    _Out_ PULONG SuspendedHandleCount);

VOID
detour_thread_resume(
    _In_reads_(SuspendedHandleCount) _Frees_ptr_ PHANDLE SuspendedHandles,
    _In_ ULONG SuspendedHandleCount);
