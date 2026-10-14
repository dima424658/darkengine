#pragma once

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
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

EXTERN FILE *PlatformFopen(const char *path, const char *mode);
EXTERN int PlatformOpen(const char *path, int flags, ...);
EXTERN int PlatformAccess(const char *path, int mode);
EXTERN int PlatformChangeDirectory(const char *path);
EXTERN int PlatformRemoveDirectory(const char *path);
EXTERN int PlatformUnlink(const char *path);

// The engine historically assumes Win32's case-preserving, case-insensitive
// lookup semantics. Route its C file opens through the Unix compatibility
// resolver instead of requiring a case-insensitive host filesystem.
#define fopen PlatformFopen
#define _access PlatformAccess
#define _chdir PlatformChangeDirectory
#define _close close
#define _filelength(fd) lseek((fd), 0, SEEK_END)
#define _getcwd getcwd
#define _lseek lseek
#define _open PlatformOpen
#define _read read
#define _rmdir PlatformRemoveDirectory
#define _unlink PlatformUnlink
#define _write write
#define _mkdir(path) mkdir((path), 0755)
