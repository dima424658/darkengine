#pragma once

// Host adaptation for the imported mingw-w64 declarations. Windows scalar
// widths remain LLP64 even when the native host uses the LP64 data model.
#include <stdint.h>
#include <stddef.h>

#ifdef _WIN32
#define __LONG32 long
#else
#define __LONG32 int
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __fastcall
#define __fastcall
#endif
#ifndef __declspec
#define __declspec(attribute)
#endif
#ifndef __export
#define __export
#endif
#ifndef __import
#define __import
#endif
#define __based(a)
#define __segname(a)
#endif

#ifndef CDECL
#define CDECL
#endif
#ifndef FAR
#define FAR
#endif
#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#ifndef DECLSPEC_SELECTANY
#ifdef _MSC_VER
#define DECLSPEC_SELECTANY __declspec(selectany)
#else
#define DECLSPEC_SELECTANY __attribute__((weak))
#endif
#endif
#ifndef DECLSPEC_NOTHROW
#define DECLSPEC_NOTHROW
#endif
#ifndef DECLSPEC_NOVTABLE
#define DECLSPEC_NOVTABLE
#endif
#ifndef interface
#define interface struct
#endif
#ifndef USE___UUIDOF
#define USE___UUIDOF 1
#endif
// Use the native C/C++ wchar_t; Windows WCHAR is declared separately below.
#ifndef _WCHAR_T_DEFINED
#define _WCHAR_T_DEFINED
#endif
