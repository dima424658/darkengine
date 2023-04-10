///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/splitpat.h $
// $Author: TOML $
// $Date: 1996/04/29 17:54:01 $
// $Revision: 1.2 $
//
// (Cribbed from Borland C & extended)
//


#ifndef __SPLITPAT_H
#define __SPLITPAT_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

#include <string.h>
#include "str.h"
#include "filepath.h"

// Symantec C++ defines MAXEXT as 4
#undef MAXEXT
#define MAXEXT 5

//////////////////////////////////////////////////////////////////////////////

void SplitPath(const char *, char *, char * = 0, char * = 0, char * = 0);

//////////////////////////////////////////////////////////////////////////////

class cPathSplitter
    {
public:
    cPathSplitter();
    cPathSplitter(const char *p, eFileSystemKind = kDefaultFileSystem);

    // Set the target
    void SetTarget(const char *, eFileSystemKind = kDefaultFileSystem);

    // Get result in cStr form
    BOOL GetSplit(cStr * = NULL, cStr * = NULL, cStr * = NULL, cStr * = NULL);
    BOOL GetDrive(cStr &);
    BOOL GetDirectory(cStr &);
    BOOL GetName(cStr &);
    BOOL GetExtension(cStr &);

    // Get result in form of ptr and length.
    const char *GetDrive();
    int GetDriveLen();

    const char *GetDirectory();
    int GetDirectoryLen();

    const char *GetName();
    int GetNameLen();

    const char *GetExtension();
    int GetExtensionLen();

private:

    // Target info
     const char *pcszTarget;
    eFileSystemKind FileSystem;

    // Working variables
    enum eComponent
        {
        kNone,
        kDrive,
        kDirectory,
        kName,
        kExtension
        };

    BOOL DoSplitTo(eComponent);

    eComponent LastSplitTo;

    unsigned nSignificantTargetLen;

    struct sNameComponent
        {
        sNameComponent()
            : pComponentText(0), iLen(-1)
            {
            }

        void Clear()
            {
            pComponentText = 0;
            iLen = -1;
            }

        void IfNotFoundAssumeZeroLenAt(const char *p)
            {
            if (!pComponentText)
                {
                pComponentText = p;
                iLen = 0;
                }
            }

        void Found(const char *p, const char *pEndPlus1, int MaxComponentLen)
            {
            pComponentText = p;
            iLen = int (pEndPlus1 - p);
            if (iLen > MaxComponentLen)
                 iLen = MaxComponentLen;
            }

        BOOL WasFound()
            {
            return !!(iLen > -1);
            }

        const char *pComponentText;
        int iLen;
        };

    sNameComponent Drive;
    sNameComponent Directory;
    sNameComponent Name;
    sNameComponent Extension;
    };
///////////////////////////////////////

inline cPathSplitter::cPathSplitter()
    : pcszTarget(0), FileSystem(kUnknownFileSystem), LastSplitTo(kNone)
    {
    }

inline cPathSplitter::cPathSplitter(const char *p, eFileSystemKind f)
    : pcszTarget(p), FileSystem(f), LastSplitTo(kNone)
    {
    }

// Set the target
inline void cPathSplitter::SetTarget(const char *pNewTarget, eFileSystemKind NewFileSystem)
    {
    pcszTarget = pNewTarget;
    FileSystem = NewFileSystem;
    LastSplitTo = kNone;
    Drive.Clear();
    Directory.Clear();
    Name.Clear();
    Extension.Clear();
    }

// Get result in form of ptr and length
inline const char *cPathSplitter::GetDrive()
    {
    if (Drive.iLen < 0)
        DoSplitTo(kDrive);
    return Drive.pComponentText;
    }

inline int cPathSplitter::GetDriveLen()
    {
    if (Drive.iLen < 0)
        DoSplitTo(kDrive);
    return (Drive.iLen > -1) ? Drive.iLen : 0;
    }

inline const char *cPathSplitter::GetDirectory()
    {
    if (Directory.iLen < 0)
        DoSplitTo(kDirectory);
    return Directory.pComponentText;
    }

inline int cPathSplitter::GetDirectoryLen()
    {
    if (Directory.iLen < 0)
        DoSplitTo(kDirectory);
    return (Directory.iLen > -1) ? Directory.iLen : 0;
    }

inline const char *cPathSplitter::GetName()
    {
    if (Name.iLen < 0)
        DoSplitTo(kName);
    return Name.pComponentText;
    }

inline int cPathSplitter::GetNameLen()
    {
    if (Name.iLen < 0)
        DoSplitTo(kName);
    return (Name.iLen > -1) ? Name.iLen : 0;
    }

inline const char *cPathSplitter::GetExtension()
    {
    if (Extension.iLen < 0)
        DoSplitTo(kExtension);
    return Extension.pComponentText;
    }

inline int cPathSplitter::GetExtensionLen()
    {
    if (Extension.iLen < 0)
        DoSplitTo(kExtension);
    return (Extension.iLen > -1) ? Extension.iLen : 0;
    }

// Get result in cStr form
inline BOOL cPathSplitter::GetDrive(cStr & nes)
    {
    if (Drive.iLen < 0)
        DoSplitTo(kDrive);
    if (Drive.pComponentText && Drive.iLen)
        {
        char *p = nes.GetBuffer(Drive.iLen);
        strncpy(p, Drive.pComponentText, Drive.iLen);
        nes.ReleaseBuffer(Drive.iLen);
        return TRUE;
        }
    nes.Empty();
    return FALSE;
    }

inline BOOL cPathSplitter::GetDirectory(cStr & nes)
    {
    if (Directory.iLen < 0)
        DoSplitTo(kDirectory);
    if (Directory.pComponentText && Directory.iLen)
        {
        char *p = nes.GetBuffer(Directory.iLen);
        strncpy(p, Directory.pComponentText, Directory.iLen);
        nes.ReleaseBuffer(Directory.iLen);
        return TRUE;
        }
    nes.Empty();
    return FALSE;
    }

inline BOOL cPathSplitter::GetName(cStr & nes)
    {
    if (Name.iLen < 0)
        DoSplitTo(kName);
    if (Name.pComponentText && Name.iLen)
        {
        char *p = nes.GetBuffer(Name.iLen);
        strncpy(p, Name.pComponentText, Name.iLen);
        nes.ReleaseBuffer(Name.iLen);
        return TRUE;
        }
    nes.Empty();
    return FALSE;
    }

inline BOOL cPathSplitter::GetExtension(cStr & nes)
    {
    if (Extension.iLen < 0)
        DoSplitTo(kExtension);
    if (Extension.pComponentText && Extension.iLen)
        {
        char *p = nes.GetBuffer(Extension.iLen);
        strncpy(p, Extension.pComponentText, Extension.iLen);
        nes.ReleaseBuffer(Extension.iLen);
        return TRUE;
        }
    nes.Empty();
    return FALSE;
    }

//////////////////////////////////////////////////////////////////////////////

#endif
