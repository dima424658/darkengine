#pragma once

#include <types.h>
#include <stddef.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef enum ePlatformFileAttribute
{
   kPlatformFileHidden = 1,
   kPlatformFileSystem = 2,
   kPlatformFileDirectory = 4
} ePlatformFileAttribute;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct sPlatformFileFind
{
   void *implementation;
   char name[260];
   uint64_t size;
   uint32 attributes;
} sPlatformFileFind;
#pragma pack(pop)

// Directory paths accept legacy separators; POSIX lookup ignores case.
// Creating an existing directory succeeds; parent directories must exist.
EXTERN BOOL PlatformDirectoryExists(const char *path);

EXTERN BOOL PlatformCreateDirectory(const char *path);

EXTERN BOOL PlatformGetCurrentDirectory(char *path, int capacity);

EXTERN BOOL PlatformGetExecutableDirectory(char *path, int capacity);

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
