#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

EXTERN void *PlatformLoadLibrary(const char *name);

EXTERN void *PlatformFindSymbol(void *library, const char *name);

EXTERN void PlatformUnloadLibrary(void *library);
