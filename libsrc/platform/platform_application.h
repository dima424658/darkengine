#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

// Batch mode is a non-interactive DromEd execution policy. Platform UI
// providers must not display windows or wait for input while it is active.
EXTERN void PlatformSetBatchMode(BOOL enabled);

EXTERN BOOL PlatformBatchMode(void);

EXTERN void PlatformBatchReportError(const char *title, const char *message);

EXTERN int PlatformBatchExitCode(void);

EXTERN void PlatformShowError(const char *title, const char *message);

EXTERN BOOL PlatformAskYesNo(const char *title, const char *message);

EXTERN int PlatformPopupMenu(const char *const *items, int count,
                             int client_x, int client_y);
