#pragma once

#include <fcntl.h>
#include <limits.h>
#ifdef __cplusplus
#include <cstdio>
#else
#include <stdio.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <types.h>

#ifndef _MAX_PATH
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#define _MAX_PATH PATH_MAX
#endif
#ifndef O_BINARY
#define O_BINARY 0
#endif
#ifndef _O_BINARY
#define _O_BINARY O_BINARY
#endif

EXTERN FILE *PosixFopen(const char *path, const char *mode);
EXTERN int PlatformOpen(const char *path, int flags, ...);
EXTERN int PlatformChangeDirectory(const char *path);

// The engine historically assumes Win32's case-preserving, case-insensitive
// lookup semantics. Route its C file opens through the Unix compatibility
// resolver instead of requiring a case-insensitive host filesystem.
#define fopen PosixFopen
#define _chdir PlatformChangeDirectory
#define _open PlatformOpen
