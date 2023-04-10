///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/findfile.h $
// $Author: TOML $
// $Date: 1997/01/30 13:50:09 $
// $Revision: 1.2 $
//
// Platform/compiler independent find first/find next protocol.
//

#ifndef __FINDFILE_H
#define __FINDFILE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _WINBASE_

#define MAX_PATH          260

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

typedef struct _WIN32_FIND_DATA {
    DWORD       dwFileAttributes;
    FILETIME    ftCreationTime;
    FILETIME    ftLastAccessTime;
    FILETIME    ftLastWriteTime;
    DWORD       nFileSizeHigh;
    DWORD       nFileSizeLow;
    DWORD       dwReserved0;
    DWORD       dwReserved1;
    char        cFileName[ MAX_PATH ];
    char        cAlternateFileName[ 16 ];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;

__declspec(dllimport)
BOOL
__stdcall
FindClose(
    HANDLE hFindFile
    );

__declspec(dllimport)
HANDLE
__stdcall
FindFirstFileA(
    const char * lpFileName,
    LPWIN32_FIND_DATA lpFindFileData
    );

#define FindFirstFile  FindFirstFileA

__declspec(dllimport)
BOOL
__stdcall
FindNextFileA(
    HANDLE hFindFile,
    LPWIN32_FIND_DATA lpFindFileData
    );

#define FindNextFile  FindNextFileA

#endif

#ifdef __cplusplus
}
#endif

#endif /* !__FINDFILE_H */
