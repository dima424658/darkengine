///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/filepath.h $
// $Author: darrenl $
// $Date: 1999/03/29 18:10:05 $
// $Revision: 1.4 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#ifndef __FILEPATH_H
#define __FILEPATH_H

#if defined(__SC__) || defined(__RCC__)
#pragma once
#endif

#include <lg.h>
#include <stdlib.h>
#include <filekind.h>
#include <str.h>
#include <fnamutil.h>

#if defined(_WIN32)

// @TBD (toml 03-22-96): Make export/importable
#define __CPPTOOLSAPI

#ifndef _WINNT_
typedef void * HANDLE;
#endif

struct __CPPTOOLSAPI sFileFind
    {
    sFileFind();
    HANDLE hContext;
    };

inline sFileFind::sFileFind()
    : hContext(0)
    {
    }


#else

// FROM <dos.h>
struct sFileFind
    {
    char reserved[21];

    // attribute found (FA_XXXX)
    char attrib;

    // file's last write
    unsigned short wr_time,wr_date;

    // file's size
    unsigned long size;

    // filename followed by 0 byte
    char name[13];
    };

#endif

class cIStore;
class cOStore;
class cFilePath;

struct __CPPTOOLSAPI sFindContext
    {
public:
    sFindContext() : Finding(kFindingNone) {}
    friend class cFilePath;

private:
    struct sFileFind PlatformFindContext;

    enum tFindState
        {
        kFindingNone,
        kFindingFiles,
        kFindingPaths
        };

    BOOL DoFindFirst(tFindState WhatToFind, const cStr & nesSpec,
                     cStr & nesResult);

    BOOL DoFindNext(cStr & nesResult);

    tFindState Finding;
    };


class cFileSpec;

#if defined(_WIN32)
#define FSKDEFFILSYS kNTFSFileSystem
#elif defined(_WINDOWS)
#define FSKDEFFILSYS kFATFileSystem
#else
#define FSKDEFFILSYS kUnknownFileSystem
#endif

enum eFileSystemKind
{
    kFATFileSystem,
    kHPFSFileSystem,
    kNTFSFileSystem,
    kUnknownFileSystem,
    kDefaultFileSystem = FSKDEFFILSYS
};

// cFilePath encapsulates a file location. On DOS, the
// cFilePath contains the drive and path as text, on the
// Mac this could be a volume and dir ID.

class __CPPTOOLSAPI cFilePath
    {
    friend class cFileSpec;

public:
    cFilePath();
    cFilePath(const cFilePath&);
    cFilePath(const cFileSpec&);
    cFilePath(const char *);

    cFilePath &operator=(const cFilePath &);
    cFilePath &operator=(const cFileSpec &);

    BOOL operator==(const cFilePath&) const;
    BOOL operator!=(const cFilePath&) const;
	int Compare(const cFilePath &) const;

    // Create a file specification from a string
    void FromText(const char *);

    // Get the string for this path
    const char *GetPathName() const;

    // Get a copy of the string for this path
    void AsText(cStr &) const;

    // Set this path based on a file specification
    void FromFileSpec(const char *);

    BOOL IsEmpty() const;
    void Empty();

    BOOL PathExists() const;

    // Create the specified directory (one level deep only!)
    BOOL CreatePath() const;

    // Get the full path
    BOOL MakeFullPath();
    BOOL GetFullPath(cStr &) const;

    BOOL GetFullPath(cStr &, const cFilePath &) const;
    BOOL MakeFullPath(const cFilePath &);

    // Set default drive/directory to this one
    BOOL SetCurrentPath() const;

    // Append the specified relative path to this path
    BOOL AddRelativePath(const cFilePath &);

    // Return a string for the specified path anchored to this path
    BOOL ComputeAnchoredPath(const cFilePath &, cStr &) const;

    // Return true if this a relative path (no root, no drive)
    BOOL IsRelativePath() const;

    // Return true if this a drive relative path (no root, but perhaps a drive)
    BOOL IsDriveRelativePath() const;

    // Return true if this path is full (i.e., has a drive)
    BOOL IsFullPath() const;

    // Streaming functions
    BOOL ToStream(cOStore &) const;
    BOOL FromStream(cIStore &);

    // Iteration
    BOOL FindFirst(cFileSpec &,sFindContext&) const;
    BOOL FindFirst(cFileSpec &,sFindContext&, const char *pWildcard) const;
    BOOL FindNext(cFileSpec &,sFindContext&) const;

    BOOL FindFirst(cFilePath &,sFindContext&) const;
    BOOL FindNext(cFilePath &,sFindContext&) const;

    void FindDone(sFindContext&) const;

    // Volume information
    BOOL GetRootDir(cStr &) const;
    BOOL GetVolumeName(cStr &) const;
    eFileSystemKind GetFileSystemKind() const;
    BOOL GetFileSystemName(cStr &) const;
    ulong GetVolumeSerialNumber() const;
    int GetCaseConventions() const;
    ulong GetMaxLegalComponentLen() const;
    BOOL IsValidChar(char c) const;
    BOOL IsValidComponentName(const char * psz) const;

    // Static functions for volume information
    static BOOL GetRootDir(const char *pPath, cStr &);
    static BOOL GetVolumeName(const char *pPath, cStr &);
    static BOOL GetFileSystemName(const char *pPath, cStr &);
    static ulong GetVolumeSerialNumber(const char *pPath);
    static int GetCaseConventions(const char *pPath);
    static ulong GetMaxLegalComponentLen(const char *pPath);
    static BOOL IsValidChar(const char *pPath, char c);
    static BOOL IsValidComponentName(const char *pPath, const char *pComponent);

    // Return true if this path is full (i.e., has a drive and root)
    static BOOL IsFullPath(const char *);

    // Return true if this a relative path (no root, no drive)
    static BOOL IsRelativePath(const char *);

    // Return true if this a drive relative path (no root, but perhaps a drive)
    static BOOL IsDriveRelativePath(const char *);

protected:
    BOOL ReducePathDots();

    static BOOL ReduceDots(char *);
    cStr m_path;
    };


inline const char *cFilePath::GetPathName() const
    {
    return m_path;
    }

//
// Add file spec (internal versions)
//
inline BOOL AddFileSpec(const cFilePath & FilePath, const sFileDesc & FileSpec, cStr & Result)
    {
    const BOOL fReturn = AddFileSpec(FilePath.GetPathName(), FileSpec.GetName(), Result.BufOut());
    Result.BufDone();
    return fReturn;
    }

inline BOOL AddFileSpec(const cFilePath & FilePath, const sFileDesc & FileSpec, sFileDesc & Result)
    {
    char * pszResult;
    const BOOL fReturn = AddFileSpec(FilePath.GetPathName(), FileSpec.GetName(), &pszResult);
    if (Result.pName)
        free(Result.pName);
    Result.pName = pszResult;
    Result.Kind = kSpecKindFile;
    return fReturn;
    }


inline void cFilePath::Empty()
    {
    m_path.Empty();
    }

inline BOOL cFilePath::IsEmpty() const
    {
    return m_path.IsEmpty();
    }

inline BOOL cFilePath::operator!=(const cFilePath &FilePath) const
    {
    return !operator ==(FilePath);
    }

inline int cFilePath::Compare(const cFilePath &FilePath) const
    {
    return m_path.Compare(FilePath.m_path);
    }


//
// What's the name of the root?
//
inline BOOL cFilePath::GetRootDir(cStr &Str) const
    {
    return GetRootDir(m_path, Str);
    }


//
// What's the volume called
//
inline BOOL cFilePath::GetVolumeName(cStr &Str) const
    {
    return GetVolumeName(m_path, Str);
    }


//
// What kind of file system does it use (FAT, HPFS, Mac, NFS, NTFS, etc.)
//
inline BOOL cFilePath::GetFileSystemName(cStr &Str) const
    {
    return GetFileSystemName(m_path, Str);
    }


//
// Get the serial number
//
inline ulong cFilePath::GetVolumeSerialNumber() const
    {
    return GetVolumeSerialNumber(m_path);
    }


//
// Get the volume case relevance
//
inline int cFilePath::GetCaseConventions() const
    {
    return GetCaseConventions(m_path);
    }


//
// Get the maximum length of a component name
//
inline ulong cFilePath::GetMaxLegalComponentLen() const
    {
    return GetMaxLegalComponentLen(m_path);
    }


//
// Character okay for volume?
//
inline BOOL cFilePath::IsValidChar(char c) const
    {
    return IsValidChar(m_path, c);
    }


//
// Component name okay for volume?
//
inline BOOL cFilePath::IsValidComponentName(const char * psz) const
    {
    return IsValidComponentName(m_path, psz);
    }


//
// Return TRUE if this is a full path
//
inline BOOL cFilePath::IsFullPath() const
    {
    return IsFullPath(m_path);
    }

//
// Return TRUE if this is a relative path
//
inline BOOL cFilePath::IsRelativePath() const
    {
    return IsRelativePath(m_path);
    }

//
// Return TRUE if this is a drive relative path (or a relative path)
//
inline BOOL cFilePath::IsDriveRelativePath() const
    {
    return IsDriveRelativePath(m_path);
    }

//
// Return true if this path is full (i.e., has a drive and root)
//
inline BOOL cFilePath::IsFullPath(const char * p)
    {
    return ::IsFullPath(p);
    }

//
// Return true if this a relative path (no root, no drive)
//
inline BOOL cFilePath::IsRelativePath(const char * p)
    {
    return ::IsRelativePath(p);
    }

//
// Return true if this a drive relative path (no root, but perhaps a drive)
//
inline BOOL cFilePath::IsDriveRelativePath(const char * p)
    {
    return ::IsDriveRelativePath(p);
    }

//
//
//
inline BOOL cFilePath::ReduceDots(char * p)
    {
    return ::ReduceDots(p);
    }

//
// Reduce dots out of path
//
inline BOOL cFilePath::ReducePathDots()
    {
    BOOL fLegal = ReduceDots(m_path.GetBuffer(m_path.GetLength()));
    m_path.ReleaseBuffer();
    return fLegal;
    }


//
// Compute a relative path between this path and a target
//
inline BOOL cFilePath::ComputeAnchoredPath(const cFilePath& targetDir, cStr &relPath) const
    {
    const BOOL fSuccess = ::ComputeAnchoredPath(GetPathName(), targetDir.GetPathName(), relPath.BufOut());
    relPath.BufDone();
    return fSuccess;
    }

//
// Get a full path
//
inline BOOL cFilePath::GetFullPath(cStr &FullPath) const
    {
    const BOOL fSuccess = ::GetFullPath(GetPathName(), FullPath.BufOut());
    FullPath.BufDone();
    return fSuccess;
    }

#endif /* __FILEPATH_H */