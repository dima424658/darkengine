#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

// Starts a second copy of the current executable and returns immediately.
// Use this when bootstrap UI changes configuration consumed by constructors.
EXTERN BOOL PlatformRelaunchSelf(const char *const *arguments,
                                 int argument_count,
                                 const char *working_directory);
