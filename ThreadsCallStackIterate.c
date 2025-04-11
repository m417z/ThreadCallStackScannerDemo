#include <phnt_windows.h>
#include <phnt.h>

#include "ThreadsCallStackIterate.h"

#include "Thread.h"
#include "Memory.h"

#if defined(_X86_)
#define CONTEXT_PC Eip
#define CONTEXT_SP Esp
#elif defined(_AMD64_)
#define CONTEXT_PC Rip
#define CONTEXT_SP Rsp
#elif defined(_ARM64_)
#define CONTEXT_PC Pc
#define CONTEXT_SP Sp
#endif

#if defined(_AMD64_) || defined(_ARM64_)

static BOOL ThreadCallStackIterate(
    HANDLE threadHandle,
    ThreadCallStackIterCallback callback,
    void* userData) {
    CONTEXT context;
    /*
     * Work-around an issue in Arm64 (and Arm64EC) in which LR and FP registers may become zeroed
     * when CONTEXT_CONTROL is used without CONTEXT_INTEGER.
     *
     * See also: https://github.com/microsoft/Detours/pull/313
     */
#if defined(_ARM64_)
    context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
#else
    context.ContextFlags = CONTEXT_CONTROL;
#endif
    NTSTATUS status = NtGetContextThread(threadHandle, &context);
    if (!NT_SUCCESS(status)) {
        // Continue to the next thread if we can't get the context.
        return TRUE;
    }

    // References:
    // http://www.nynaeve.net/Code/StackWalk64.cpp
    // https://blog.s-schoener.com/2025-01-24-stack-walking-generated-code/
    while (context.CONTEXT_PC != 0) {
        if (!callback(threadHandle, (void*)context.CONTEXT_PC, userData)) {
            // If the callback returns FALSE, we stop iterating.
            return FALSE;
        }

        DWORD64 imageBase;
        RUNTIME_FUNCTION* function = RtlLookupFunctionEntry(context.CONTEXT_PC, &imageBase, NULL);

        // If there is no function entry, then this is a leaf function.
        if (!function) {
            // Unwind the leaf: SP points to the old PC.
            context.CONTEXT_PC = *(DWORD64*)context.CONTEXT_SP;
            context.CONTEXT_SP += sizeof(DWORD64);
        } else {
            void* handlerData;
            DWORD64 establisherFrame;
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.CONTEXT_PC, function, &context, &handlerData, &establisherFrame, NULL);
        }
    }

    return TRUE;
}

#elif defined(_X86_)

static BOOL ThreadCallStackIterate(
    HANDLE threadHandle,
    ThreadCallStackIterCallback callback,
    void* userData) {
    CONTEXT context;
    context.ContextFlags = CONTEXT_CONTROL;
    NTSTATUS status = NtGetContextThread(threadHandle, &context);
    if (!NT_SUCCESS(status)) {
        // Continue to the next thread if we can't get the context.
        return TRUE;
    }

    if (!callback(threadHandle, (void*)context.Eip, userData)) {
        // If the callback returns FALSE, we stop iterating.
        return FALSE;
    }

    THREAD_BASIC_INFORMATION threadInfo;
    if (!NT_SUCCESS(NtQueryInformationThread(
        threadHandle,
        ThreadBasicInformation,
        &threadInfo,
        sizeof(threadInfo),
        NULL
    ))) {
        // Continue to the next thread if we can't get the thread information.
        return TRUE;
    }

    void* stackBase = threadInfo.TebBaseAddress->NtTib.StackBase;
    void* stackLimit = threadInfo.TebBaseAddress->NtTib.StackLimit;

    void* lastStackLimit = stackLimit;

    // Walk the stack using the frame-pointer stored in the RBP register.
    // NOTE: Requires the binary to be compiled with frame-pointers.
    struct frame {
        struct frame* prev;
        DWORD retAddr;
    };

    struct frame* frame = (struct frame*)context.Ebp;

    while ((void*)frame >= lastStackLimit && (void*)(frame + 1) <= stackBase) {
        lastStackLimit = frame + 1;

        DWORD callpc = frame->retAddr;
        if (callpc == 0) {
            break;
        }

        if (!callback(threadHandle, (void*)callpc, userData)) {
            // If the callback returns FALSE, we stop iterating.
            return FALSE;
        }

        frame = frame->prev;
    }

    return TRUE;
}

#endif

void ThreadsCallStackIterate(
    ThreadCallStackIterCallback callback,
    void* userData) {
    HANDLE* suspendedHandles = NULL;
    ULONG suspendedHandleCount = 0;
    NTSTATUS status = detour_thread_suspend(&suspendedHandles, &suspendedHandleCount);
    if (NT_SUCCESS(status) && suspendedHandles) {
        for (ULONG i = 0; i < suspendedHandleCount; i++) {
            if (!ThreadCallStackIterate(suspendedHandles[i], callback, userData)) {
                // If the callback returns FALSE, we stop iterating.
                break;
            }
        }

        detour_thread_resume(suspendedHandles, suspendedHandleCount);
    }

    detour_memory_uninitialize();
}
