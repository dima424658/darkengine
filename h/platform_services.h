#pragma once

#include <stddef.h>
#include <stdint.h>
#include <types.h>

#pragma pack(push, 1)

typedef struct sPlatformDisplayMode
{
   int width;
   int height;
   int bit_depth;
} sPlatformDisplayMode;

typedef enum ePlatformDisplayScope
{
   kPlatformCurrentDisplay,
   kPlatformAllDisplays
} ePlatformDisplayScope;

typedef struct sPlatformMemoryStatus
{
   uint32 load_percent;
   uint32 total_physical_bytes;
   uint32 available_physical_bytes;
   uint32 total_page_file_bytes;
   uint32 available_page_file_bytes;
   uint32 total_virtual_bytes;
   uint32 available_virtual_bytes;
} sPlatformMemoryStatus;

typedef struct sPlatformMutex
{
   void *implementation;
} sPlatformMutex;

typedef enum ePlatformFileAttribute
{
   kPlatformFileHidden = 1,
   kPlatformFileSystem = 2,
   kPlatformFileDirectory = 4
} ePlatformFileAttribute;

typedef struct sPlatformFileFind
{
   void *implementation;
   char name[260];
   uint64_t size;
   uint32 attributes;
} sPlatformFileFind;

typedef enum ePlatformAssertAction
{
   kPlatformAssertDebug,
   kPlatformAssertExit,
   kPlatformAssertIgnore
} ePlatformAssertAction;

typedef void (*tPlatformProcessOutput)(const char *bytes, int length,
                                       void *context);

EXTERN int PlatformGetDisplayModes(ePlatformDisplayScope scope,
                                   sPlatformDisplayMode *modes,
                                   int capacity,
                                   int *native_width,
                                   int *native_height);
// Returns the platform text scale relative to its baseline desktop density.
// Implementations include the active display scale and, where the platform
// exposes it separately, the user's font-size preference. The shared UI uses
// this value only for text; game geometry and input coordinates are unchanged.
EXTERN float PlatformGetTextScale(void);
EXTERN BOOL PlatformGetMemoryStatus(sPlatformMemoryStatus *status);
EXTERN BOOL PlatformReadUserUInt(const char *store, const char *name,
                                 uint32 *value);
EXTERN BOOL PlatformWriteUserUInt(const char *store, const char *name,
                                  uint32 value);
EXTERN const char *PlatformGetLanguageCode(void);
EXTERN void PlatformShowError(const char *title, const char *message);
EXTERN BOOL PlatformAskYesNo(const char *title, const char *message);
EXTERN int PlatformPopupMenu(const char *const *items, int count,
                             int client_x, int client_y);
EXTERN uint32 PlatformMilliseconds(void);
EXTERN uint64_t PlatformCurrentThreadId(void);
EXTERN int PlatformReadKey(void);
EXTERN size_t PlatformAllocationSize(void *allocation);
EXTERN void PlatformHeapCheck(void);
EXTERN void PlatformHeapMinimize(void);
EXTERN void *PlatformLoadLibrary(const char *name);
EXTERN void *PlatformFindSymbol(void *library, const char *name);
EXTERN void PlatformUnloadLibrary(void *library);
EXTERN void PlatformDebugOutput(const char *text);
// Starts one human-readable diagnostic report in the current game directory.
// DARK_DIAGNOSTICS_PATH may override the destination for unattended tests.
EXTERN void PlatformDiagnosticsStartSession(const char *application,
                                            int argc,
                                            const char *const *argv);
EXTERN void PlatformDiagnosticsLog(const char *severity,
                                   const char *subsystem,
                                   const char *message);
EXTERN void PlatformDiagnosticsLogStack(const char *severity,
                                        const char *subsystem,
                                        int skip_frames);
EXTERN const char *PlatformDiagnosticsPath(void);
EXTERN ePlatformAssertAction PlatformShowAssert(const char *message);
EXTERN void PlatformDebugBreak(void);
EXTERN void PlatformExitProcess(int status);
EXTERN BOOL PlatformSetClipboardText(const char *text);
EXTERN int PlatformGetClipboardText(char *text, int capacity);
EXTERN void PlatformSetTextInputActive(BOOL active);
EXTERN BOOL PlatformGetCurrentDirectory(char *path, int capacity);
EXTERN BOOL PlatformGetExecutableDirectory(char *path, int capacity);
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
// Converts Windows separators and resolves an existing path component by
// component. Exact spelling always wins; on case-sensitive hosts, a unique
// ASCII case-insensitive match is used only when the exact component is
// absent. The normalized candidate is returned even when it does not exist.
EXTERN BOOL PlatformResolvePath(const char *path, char *resolved, int capacity);
EXTERN BOOL PlatformCopyFiles(const char *pattern, const char *source_dir,
                              const char *destination_dir);
EXTERN BOOL PlatformFindFirst(const char *pattern, sPlatformFileFind *find);
EXTERN BOOL PlatformFindNext(sPlatformFileFind *find);
EXTERN void PlatformFindClose(sPlatformFileFind *find);
EXTERN BOOL PlatformMutexInit(sPlatformMutex *mutex);
EXTERN void PlatformMutexTerm(sPlatformMutex *mutex);
EXTERN void PlatformMutexLock(sPlatformMutex *mutex);
EXTERN void PlatformMutexUnlock(sPlatformMutex *mutex);
EXTERN long PlatformAtomicLoad(volatile long *value);
EXTERN long PlatformAtomicIncrement(volatile long *value);
EXTERN long PlatformAtomicDecrement(volatile long *value);
EXTERN long PlatformAtomicExchange(volatile long *value, long replacement);
EXTERN long PlatformAtomicAdd(volatile long *value, long amount);
EXTERN long PlatformAtomicCompareExchange(volatile long *value,
                                          long replacement,
                                          long expected);

#pragma pack(pop)
