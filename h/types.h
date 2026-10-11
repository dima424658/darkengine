/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: x:/prj/tech/hsrc/RCS/types.h $
 * $Revision: 1.31 $
 * $Author: TOML $
 * $Date: 1998/09/29 11:23:32 $
 *
 * extra typedefs and macros for use by all code.
 *
 */

#ifndef __TYPES_H
#define __TYPES_H

#include <limits.h>
#include <stdint.h>

/*
 * glibc exposes a legacy LP64 "ulong" typedef from <sys/types.h> whenever
 * GNU APIs are enabled.  Dark's ulong is intentionally a persisted 32-bit
 * type, so let glibc declare its compatibility name under a private spelling
 * before the engine typedef below.  The sys/types include guard prevents a
 * later system header from reintroducing the conflicting name.
 */
#if defined(__linux__)
#define ulong __dark_system_ulong
#include <sys/types.h>
#undef ulong
#endif

#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif
#endif

#ifndef CDECL
#define CDECL
#endif

#if !defined(_WIN32)
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
#ifndef interface
#define interface struct
#endif

#include <legacy_compat.h>
#endif

#ifndef FAR
#define FAR
#endif

#ifndef _DARK_HANDLE_DEFINED
#define _DARK_HANDLE_DEFINED
typedef void *HANDLE;
#endif

#ifndef DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name
#endif

#ifndef _DARK_SIZE_T_DEFINED
#define _DARK_SIZE_T_DEFINED
#ifdef _WIN64
typedef unsigned __int64 SIZE_T;
#else
typedef unsigned long SIZE_T;
#endif

#ifndef _DARK_DWORD_PTR_DEFINED
#define _DARK_DWORD_PTR_DEFINED
#ifdef _WIN64
typedef unsigned __int64 DWORD_PTR;
#else
typedef unsigned long DWORD_PTR;
#endif
#endif
#endif

//
// COMPILER VENDOR SPECIFIC ADJUSTEMENTS (placed first so will affect this file)
//

//
// NULL
//

#if defined(__GNUC__) && defined(__cplusplus)
    // GCC's __null extension diagnoses the engine's historical integer uses
    // of NULL. Dark predates nullptr and consistently expects literal zero.
    #undef NULL
    #define NULL 0
#elif !defined(NULL)
    #define NULL 0
#endif


//
// COMMON SCALAR TYPES
//

#ifndef NO_SCALAR_TYPEDEFS

// Size neutral
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
#if ULONG_MAX == UINT32_MAX
typedef unsigned long   ulong;
#else
typedef uint32_t        ulong;
#define DARK_UINT_ULONG_SAME_TYPE 1
#endif

typedef   signed char   schar;
typedef   signed short  sshort;
typedef   signed int    sint;
#if LONG_MAX == INT32_MAX
typedef   signed long   slong;
#else
typedef int32_t         slong;
#endif

// Size sensitive
typedef   signed char   sbyte;
typedef unsigned char   ubyte;
typedef unsigned char   uint8;
typedef   signed char   sint8;
typedef   signed char   int8;
typedef unsigned char   BYTE;

typedef unsigned short  uint16;
typedef   signed short  sint16;
typedef   signed short  int16;
typedef unsigned short  WORD;

#if ULONG_MAX == UINT32_MAX
typedef unsigned long   uint32;
typedef   signed long   sint32;
typedef   signed long   int32;
typedef unsigned long   DWORD;
#else
typedef uint32_t        uint32;
typedef int32_t         sint32;
typedef int32_t         int32;
typedef uint32_t        DWORD;
#endif

#endif /* !NO_SCALAR_TYPEDEFS */

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef int32 HRESULT;
#endif


//
// BOOLEANS
//

#ifndef NO_BOOLEAN_TYPEDEFS

typedef int BOOL;                       // Microsoft-style
typedef unsigned char sbool;            // small bool

// "True" bool that is own type and matches C++ standard behavior
#ifndef NO_TRUE_BOOL
#ifdef __cplusplus
struct true_bool
{
   true_bool()         : value(0)        {}
   true_bool(int v)    : value(v != 0)   {}
   true_bool & operator=(int v)          { value = (v != 0); return (*this); }
   operator int() const                  { return (value); }

private:
   int value;
};
#else
typedef int true_bool;
#endif
#endif

#ifndef  __cplusplus
#if defined(_MSC_VER) || defined(WAT110)
#define bool sbool
#else
typedef unsigned char bool;             // Warning: future C++ keyword collision
#endif
#endif // ! __cplusplus

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#endif /* !NO_BOOLEAN_TYPEDEFS */


//
// GLOBALLY UNIQUE IDENTIFIERS (GUIDs)
//

// Forward declare a guid
#define F_DECLARE_GUID(guid) \
    EXTERN_C const GUID CDECL FAR guid

#if !defined(NO_GUIDS) && !defined(GUID_DEFINED)
#define GUID_DEFINED

typedef struct  _GUID
    {
    DWORD Data1;
    WORD  Data2;
    WORD  Data3;
    BYTE  Data4[ 8 ];
    }	GUID;

#if !defined(_MSC_VER) && !defined(WAT110)
   #define DEFINE_GUID_UNCONDITIONAL(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            EXTERN_C const GUID CDECL name \
                     = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
   #define DEFINE_GUID_UNCONDITIONAL(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            EXTERN_C const GUID name \
                     = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif

// VC 4.2 changed the definition of DEFINE_GUID.  Other compilers are sure to follow (toml 11-19-96)
#if !defined(_MSC_VER) && !defined(WAT110)
   #ifndef INITGUID
      #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            EXTERN_C const GUID CDECL FAR name
   #else
      #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
               EXTERN_C const GUID CDECL name \
                        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
   #endif /* !INITGUID */
#else
   #ifndef INITGUID
      #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
            EXTERN_C const GUID FAR name
   #else
      #define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
               EXTERN_C const GUID name \
                        = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
   #endif /* !INITGUID */
#endif

#endif /* !NO_GUIDS && !GUID_DEFINED */

#define MAKE_GUID(p, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
   do \
   { \
      const GUID _temp = { (ulong)(l), (ushort)(w1), (ushort)(w2), \
         { (uchar)(b1), (uchar)(b2), (uchar)(b3), (uchar)(b4), \
           (uchar)(b5), (uchar)(b6), (uchar)(b7), (uchar)(b8) } }; \
      *p = _temp; \
   } while (0)

//
// Looking Glass GUID definition macro
//
#ifndef NO_LG_GUID

#define MAKE_LG_GUID_HIGH_DWORD(number) ((((ulong)number & 0xff) << 24) | (((ulong)number & 0xff00) << 16) | ((ulong)number & 0xffff))

#define DEFINE_LG_GUID(ident, number) \
    DEFINE_GUID(ident, MAKE_LG_GUID_HIGH_DWORD((number)), \
                       (0x7a80 + (number)), \
                       (0x11cf + (number)), \
                       0x83, 0x48, 0x00, 0xaa, 0x00, 0xa8, 0x2b, 0x51)

#define DEFINE_LG_GUID_UNCONDITIONAL(ident, number) \
    DEFINE_GUID(ident, MAKE_LG_GUID_HIGH_DWORD((number)), \
                       (0x7a80 + (number)), \
                       (0x11cf + (number)), \
                       0x83, 0x48, 0x00, 0xaa, 0x00, 0xa8, 0x2b, 0x51)

#define MAKE_LG_GUID(p, number) \
    MAKE_GUID(p, MAKE_LG_GUID_HIGH_DWORD((number)), \
                       (0x7a80 + (number)), \
                       (0x11cf + (number)), \
                       0x83, 0x48, 0x00, 0xaa, 0x00, 0xa8, 0x2b, 0x51)

// Convert an existing guid you know to be a looking glass guid
// back into the 16 bit number, so you can store it or use
// it for smaller comparisons, etc
#define GET_LG_GUID(_pGuid) (*(ushort *)_pGuid)


#endif /* !NO_LG_GUID */

//
// FUNCTION & DATA LINKAGE, STORAGE, AND TYPE QUALIFIERS
//

#ifndef NO_TYPE_QUALIFIERS

#ifndef EXPORT
    #define EXPORT __export
#endif

// These type qualifiers are archaic, thus defined away

#ifndef BASED_CODE
    #define BASED_CODE
#endif

#ifndef _WIN32
    #define __based(a)
    #define __segname(a)
    #define __export
    #define __import
#endif

// Qualifiers for publicly exposed functions and data

#ifdef __cplusplus
    #define EXTERN      extern "C"
#else
    #define EXTERN      extern
#endif

#if defined(_WIN32)
    #define LGAPI   __stdcall
    #define LGDATA
#else
    #define LGAPI
    #define LGDATA
#endif

// Export/Import qualifiers, used for further macro building

#if defined(_WIN32) && defined(_WINDLL)
    #define __LGIMPORT __declspec(dllimport)
    #define __LGEXPORT __declspec(dllexport)
#else
    #define __LGIMPORT
    #define __LGEXPORT
#endif

#define __LGAX      __LGEXPORT LGAPI
#define __LGDX      __LGEXPORT LGDATA
#define __LGAI      __LGIMPORT LGAPI
#define __LGDI      __LGIMPORT LGDATA

#endif /* !NO_TYPE_QUALAIFIERS */


//
// CROSS-VENDOR DEFINES (WC/VC/Windows/3rd Party/LG compatability)
//

#if defined(ONEOPT)
    // Build system has turned off debugging for the current compile
    #undef DEBUG
    #undef DBG_ON
    #undef SPEW_ON
    #undef WARN_ON
#endif

#if defined(DBG_ON) && !defined(DEBUG)
    #define DEBUG 1
#endif

#if defined(DEBUG) && !defined(DBG_ON)
    #define DBG_ON 1
#endif

#if !defined(DEBUG) && !defined(NDEBUG)
    #define NDEBUG
#endif


//
// COMMON ERROR TYPE
//
// These model 1-1 Windows HRESULTS, but dispense with some
// Microsoft-isms, and add support for "subsystem ids"
//

typedef int32           tResult;
typedef unsigned char   tResultCode;

#define kNoError        0L

// Severity values
#define kSuccess        0
#define kError          1


// Generic test for success on any status value (non-negative numbers indicate success).
#define Succeeded(Status) ((HRESULT)(Status) >= 0)

// and the inverse
#define Failed(Status) ((HRESULT)(Status)<0)

// Generic test for error on any status value.
#define IsError(Status) ((unsigned long)(Status) & 0x80000000L)


// Make a result
#define MakeResult(severity, subsystem, code) \
    MAKE_HRESULT(severity, FACILITY_ITF, ((unsigned long)(subsystem) << 8) | ((unsigned long)(code)))

#define FACILITY_ITF    4
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )


#endif /* !__TYPES_H */
