# Thread Call Stack Scanner

## Overview

**Thread Call Stack Scanner** demonstrates how to safely manage the unloading of
DLLs that have been hooked into a process to intercept or modify its behavior.
It ensures that no thread is executing within specified memory regions, such as
those belonging to a module, before proceeding with the unloading operation.
This is particularly useful in scenarios where dynamically loaded modules need
to be safely unloaded without causing crashes.

## Motivation

When working with hooking libraries like
[Detours](https://github.com/Microsoft/Detours) or in other cases such as
subclassing (a technique to intercept and modify window messages), unloading the
module containing the hooks/callbacks can be risky. Even after removing the
hooks, the hook functions may still be running in some of the threads. This can
lead to a crash, as the memory associated with the module is no longer valid
after unloading.

## Solution

**Thread Call Stack Scanner** solves this problem by iterating over the call
stacks of all threads in the process and ensuring that no thread is executing
within the specified memory regions. It provides a function,
`ThreadsCallStackWaitForRegions`, that waits until all threads have exited the
specified regions.

## Example Usage

```c
#include "ThreadsCallStackWaitForRegions.h"

ThreadCallStackRegionInfo regions[] = {
    { moduleBaseAddress1, moduleSize1 },
    { moduleBaseAddress2, moduleSize2 },
    // ...
};

DWORD maxIterations = 10; // Maximum number of iterations to try
DWORD timeoutPerIteration = 100; // Timeout in milliseconds per iteration
if (ThreadsCallStackWaitForRegions(regions, ARRAYSIZE(regions), 
                                   maxIterations, timeoutPerIteration)) {
    // Safe to unload the module
    FreeLibrary(moduleHandle);
} else {
    // Timeout or error occurred
    printf("Failed to ensure safe unloading.\n");
}
```

## Avoiding Deadlocks in `RtlLookupFunctionEntry`

When performing stack walking, the code suspends threads to safely inspect their
call stacks. During this process, on 64-bit architectures, the
`RtlLookupFunctionEntry` function is called to retrieve runtime function
information. However, `RtlLookupFunctionEntry` internally acquires the
`LdrpInvertedFunctionTableSRWLock` lock, which is necessary for its operation.
If a suspended thread is holding this lock, the thread performing the stack
walking will block, causing a deadlock.

To mitigate this issue, **Thread Call Stack Scanner** uses a worker thread and a
timeout. On timeout, all threads are resumed, allowing the thread holding the
lock to complete its operation and resolve the deadlock. The stack walking
operation can then be retried.

## Limitations

On x86, reliable stack walking requires the binary to be compiled with frame
pointers enabled. Without frame pointers, compiler optimizations like inlining
or tail-call elimination can make stack walking unreliable.

## References

- [KNSoft.SlimDetours GitHub Discussion
  #15](https://github.com/KNSoft/KNSoft.SlimDetours/discussions/15): Discusses
  the challenges of safely unloading hooked modules.
- [The case of the UI thread that hung in a kernel
  call](https://devblogs.microsoft.com/oldnewthing/20250411-00/?p=111066):
  Discusses a potential deadlock when performing stack walking with suspended
  threads.
