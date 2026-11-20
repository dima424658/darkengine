/**
 * Adapted from mingw-w64 headers, revision 93f3505a758fe70e56678f00e753af3bc4f640bb.
 * Modified for Dark Engine native POSIX/Windows builds on 2026-11-20.
 * See COPYING.mingw-w64 and DISCLAIMER in this directory.
 */
#pragma once

#ifdef _WIN32
#include <windef.h>
#include <basetsd.h>
#else
#include "_mingw.h"

// Selected minwindef.h/winnt.h/basetsd.h declarations, with fixed Windows
// scalar widths and native pointer widths. Do not guard the full SDK headers.
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned __LONG32 DWORD;
typedef __LONG32 LONG;
typedef unsigned __LONG32 ULONG;
typedef int BOOL;
typedef int INT;
typedef unsigned int UINT;
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef __LONG32 HRESULT;
#endif
#ifndef _DARK_HANDLE_DEFINED
#define _DARK_HANDLE_DEFINED
typedef void *HANDLE;
#endif
#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name
#endif
#ifndef _DARK_SIZE_T_DEFINED
#define _DARK_SIZE_T_DEFINED
typedef uintptr_t SIZE_T;
#endif
#ifndef _DARK_DWORD_PTR_DEFINED
#define _DARK_DWORD_PTR_DEFINED
typedef uintptr_t DWORD_PTR;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
