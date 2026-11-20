#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef enum ePlatformAssertAction
{
   kPlatformAssertDebug,
   kPlatformAssertExit,
   kPlatformAssertIgnore
} ePlatformAssertAction;
#pragma pack(pop)

EXTERN void PlatformDebugOutput(const char *text);

// Starts one human-readable diagnostic report in the current game directory.
// DARK_DIAGNOSTICS_PATH may override the destination for unattended tests.
EXTERN void PlatformDiagnosticsStartSession(const char *application,
                                            int argc,
                                            const char *const *argv);

EXTERN void PlatformDiagnosticsInstallCrashHandlers(void);

EXTERN void PlatformDiagnosticsLog(const char *severity,
                                   const char *subsystem,
                                   const char *message);

EXTERN void PlatformDiagnosticsLogStack(const char *severity,
                                        const char *subsystem,
                                        int skip_frames);

EXTERN const char *PlatformDiagnosticsPath(void);

EXTERN ePlatformAssertAction PlatformShowAssert(const char *message);

EXTERN void PlatformDebugBreak(void);
