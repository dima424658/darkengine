///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/virtmem.h $
// $Author: TOML $
// $Date: 1997/03/10 13:34:23 $
// $Revision: 1.2 $
//
// Prototypes of virtual memory functions so LG clients don't have to
// include windows.h
//

#ifndef __VIRTMEM_H
#define __VIRTMEM_H

#ifdef __cplusplus
extern "C"
{
#endif

#define DECLSPEC_IMPORT __declspec(dllimport)
#define WINBASEAPI DECLSPEC_IMPORT
#define WINAPI      __stdcall

#define PAGE_NOACCESS          0x01
#define PAGE_READONLY          0x02
#define PAGE_READWRITE         0x04
#define PAGE_WRITECOPY         0x08
#define PAGE_EXECUTE           0x10
#define PAGE_EXECUTE_READ      0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD            0x100
#define PAGE_NOCACHE          0x200
#define MEM_COMMIT           0x1000
#define MEM_RESERVE          0x2000
#define MEM_DECOMMIT         0x4000
#define MEM_RELEASE          0x8000
#define MEM_FREE            0x10000
#define MEM_PRIVATE         0x20000
#define MEM_MAPPED          0x40000
#define MEM_TOP_DOWN       0x100000

WINBASEAPI
void *
WINAPI
VirtualAlloc(
    void * lpAddress,
    DWORD dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    );

WINBASEAPI
BOOL
WINAPI
VirtualFree(
    void * lpAddress,
    DWORD dwSize,
    DWORD dwFreeType
    );

WINBASEAPI
BOOL
WINAPI
VirtualProtect(
    void * lpAddress,
    DWORD dwSize,
    DWORD flNewProtect,
    DWORD * lpflOldProtect
    );

WINBASEAPI
BOOL
WINAPI
VirtualProtectEx(
    HANDLE hProcess,
    void * lpAddress,
    DWORD dwSize,
    DWORD flNewProtect,
    DWORD * lpflOldProtect
    );

WINBASEAPI
BOOL
WINAPI
VirtualLock(
    void * lpAddress,
    DWORD dwSize
    );

WINBASEAPI
BOOL
WINAPI
VirtualUnlock(
    void * lpAddress,
    DWORD dwSize
    );

#ifdef __cplusplus
}
#endif

#endif /* !__VIRTMEM_H */
