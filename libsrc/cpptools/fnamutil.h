///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/fnamutil.h $
// $Author: JAEMZ $
// $Date: 1997/08/13 19:01:13 $
// $Revision: 1.3 $
//
// File name string manipulation functions
//
// (c) Copyright 1995-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __FNAMUTIL_H
#define __FNAMUTIL_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

#ifndef __cplusplus
#error "Need C versions of fnamutil"
#endif

#define __CPPTOOLSAPI

enum eFileCondenseCase
    {
    kFileNoChange,
    kFileToLower,
    kFileToUpper
    };


//
// AddFileSpec()
// Combine a path and a (possibly absolute) file spec into a new, reduced path
// (e.g. AddFileSpec("c:\\foo\\bar", "..\\include\\bar.h")
//          becomes ("c:\\foo\\include\\bar.h)
//  and  AddFileSpec("c:\\foo\\bar", "d:\\bar.h")
//          becomes ("d:\\bar.h))
//
//  Returns combined path in malloc'd string caller must free
//  Returns: FALSE if invalid path (i.e., not all dots were removed,
//           or illegal relative path)

BOOL __stdcall __CPPTOOLSAPI AddFileSpec(const char * pszPath, const char * pszFileSpec, char ** ppszOut);

//
// Compute a relative path between this path and a target
//
// Takes: the anchor, the goal, The output
// The result is the path relative to this to get to the target,
// or else is the full path to the target
//
BOOL __stdcall __CPPTOOLSAPI ComputeAnchoredPath(const char * pszAnchorPath, const char * pszTargetPathIn, char ** ppszOut);

//
// Validity tests
//
BOOL __stdcall __CPPTOOLSAPI IsValidFileSpec(const char * pszFileSpec);
BOOL __stdcall __CPPTOOLSAPI IsValidFilePath(const char * pszFilePath);

//
// Get a full path, return FALSE if invalid
//
BOOL __stdcall __CPPTOOLSAPI GetFullPath(const char * pszPath, char ** ppszOut);

//
// ReduceDots()
// In-place lexical dot reduction (e.g., "\\a\\b\\..\\c" -> "a\\c")
//
//  Takes: File path or file name
//  Returns: FALSE if invalid path (i.e., not all dots were removed
//           or illegal relative path)
//
BOOL __stdcall __CPPTOOLSAPI ReduceDots(char *psz);


//
// Utility functions
//
#define _IsSlash(c) (c == '\\')

//
// Return true if this path is a simple component (i.e., is just a file root & ext)
//
#ifdef __STRING_H
inline BOOL IsPathSimpleComponent(const char *p)
    {
    return (p && *p && *(p+1) != ':' && !strchr(p, '\\'));
    }
#endif

//
// Return true if this path is full (i.e., has a drive and root)
//
inline BOOL IsFullPath(const char *p)
    {
    // Check for start of domain or x:\...
    if (*p)
        {
        if (_IsSlash(*p))
            return _IsSlash(*++p);

        if (*++p == ':')
            return _IsSlash(*++p);
        }
    return FALSE;
    }

//
// Return true if this a relative path (no root, no drive, no domain)
//
inline BOOL IsRelativePath(const char *p)
    {
    return (!*p || (!_IsSlash(*p) && *(p+1) != ':'));
    }

//
// Return true if this a drive relative path (no root, but perhaps a drive)
//
inline BOOL IsDriveRelativePath(const char *p)
    {
    if (!*p)
        return TRUE;

    if (_IsSlash(*p))
        return FALSE;

    if (*++p != ':')
        return TRUE;

    return !_IsSlash(*++p);
    }

char *GetCondensePathStr(
        const char *pOriginalPath,
        char *Path,
        unsigned nCondenseToLen,
        eFileCondenseCase CaseChange = kFileNoChange,
        BOOL bRequireDrive = FALSE);

#endif /* __FNAMUTIL_H */
