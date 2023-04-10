///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/filekind.h $
// $Author: TOML $
// $Date: 1996/03/22 11:00:55 $
// $Revision: 1.1 $
//
// (c) Copyright 1994-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//
// @TBD (toml 03-22-96): Need to make C-friendly
//
// The file descriptor is intended to represent both files, and
// file-like things (like contents of compound files)
//

#ifndef __FILEKIND_H
#define __FILEKIND_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

//
// Describes a particular type of file
//
enum eFileSpecKind
    {
    // Empty file specification
    kSpecKindUnknown,

    // Normal file in the file system
    kSpecKindFile,

    // Untitled file (e.g., Untitled-1)
    kSpecKindUntitled,

    // @TBD (toml 01-20-96): Will add support for custom filespecs based on subsystem id

    // Special file
    kSpecKindSpecial
    };


enum eVolumeFileNameCaseConventionFlags
    {
    kNameCasePreserved  = 1,
    kNameCaseSensitive  = 2,
    kUnicodeStored      = 4,
    kNameCaseIgnored    = 8
    };


typedef enum eFileSpecKind eFileSpecKind;
typedef enum eVolumeFileNameCaseConventionFlags eVolumeFileNameCaseConventionFlags;


//
// The sFileDesc structure specifies a file by a file name string and
// a file kind.
//
struct sFileDesc
    {
    // File name string.  In most cases, the string must be allocated using
    // the IAlloc interface.  The exception to this rule is if the structure
    // is never passed to an interface function, unless as a constant
    // reference.
    char *pName;

    // Kind of file
    eFileSpecKind Kind;

#ifdef __cplusplus

    // Construct an empty file descriptor
    sFileDesc();

    // Make a file descriptor from a file name string and a file kind
    sFileDesc(char *, eFileSpecKind);

    // Return TRUE if the file specification is empty
    BOOL IsEmpty() const;

    // Get the file name string
    const char *GetName() const;

    // Get the kind of file
    eFileSpecKind GetKind() const;

#endif /* __cplusplus */
    };

typedef struct sFileDesc sFileDesc;


#ifdef __cplusplus

//
// Construct and empty file descriptor
//
inline sFileDesc::sFileDesc()
    : pName(0),
      Kind(kSpecKindUnknown)
    {
    }


//
// Make a file descriptor from a file name string and a file kind
//
inline sFileDesc::sFileDesc(char *pns, eFileSpecKind k)
    : pName(pns),
      Kind(k)
    {
    }


//
// Return TRUE if the file specification is empty
//
inline BOOL sFileDesc::IsEmpty() const
    {
    return Kind == kSpecKindUnknown;
    }


//
// Return the kind of buffer
//
inline eFileSpecKind sFileDesc::GetKind() const
    {
    return Kind;
    }


//
// Get the file name
//
inline const char *sFileDesc::GetName() const
    {
    return pName;
    }

#endif /* __cplusplus */

#endif /* __FILEKIND_H */
