#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

EXTERN BOOL PlatformReadUserUInt(const char *store, const char *name,
                                 uint32 *value);

EXTERN BOOL PlatformWriteUserUInt(const char *store, const char *name,
                                  uint32 value);

EXTERN const char *PlatformGetLanguageCode(void);
