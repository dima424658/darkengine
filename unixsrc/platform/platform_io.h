#pragma once

#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

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

#define _access access
#define _chdir chdir
#define _close close
#define _filelength(fd) lseek((fd), 0, SEEK_END)
#define _getcwd getcwd
#define _lseek lseek
#define _open open
#define _read read
#define _rmdir rmdir
#define _unlink unlink
#define _write write
#define _mkdir(path) mkdir((path), 0755)
