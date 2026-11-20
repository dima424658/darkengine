#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

EXTERN uint32 PlatformMilliseconds(void);

// Changes once for each completed OS-level application suspension. UI modes
// can use this to rebuild content lost with a platform presentation surface.
EXTERN uint32 PlatformApplicationResumeGeneration(void);

EXTERN uint64_t PlatformCurrentThreadId(void);

EXTERN void PlatformExitProcess(int status);
