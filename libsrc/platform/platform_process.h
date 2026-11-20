#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

typedef void (*tPlatformProcessOutput)(const char *bytes, int length,
                                       void *context);

// Runs one child synchronously with an explicit argument vector and working
// directory. stdout and stderr are combined and delivered incrementally.
EXTERN BOOL PlatformRunProcess(const char *executable,
                               const char *const *arguments,
                               int argument_count,
                               const char *working_directory,
                               tPlatformProcessOutput output,
                               void *context,
                               uint32 *exit_code);

// Starts a second copy of the current executable and returns immediately.
// Use this when bootstrap UI changes configuration consumed by constructors.
EXTERN BOOL PlatformRelaunchSelf(const char *const *arguments,
                                 int argument_count,
                                 const char *working_directory);
