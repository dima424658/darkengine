///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/filepath.cpp $
// $Author: JUSTIN $
// $Date: 1998/07/09 09:46:29 $
// $Revision: 1.7 $
//
// (c) Copyright 1993-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#if defined(_WIN32)
#include <windows.h>
#endif

#include <lg.h>
#include <assert.h>
#include <filepath.h>
#include <fnamutil.h>

#include <ctype.h>

#include <direct.h>
#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>

#include <filespec.h>
#include <splitpat.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef DEBUG_REDUCE
#define DEBUG_REDUCE 0
#else
#pragma message "DEBUG_REDUCE"
#endif

#ifndef DEBUG_CASE
#define DEBUG_CASE   0
#else
#pragma message "DEBUG_CASE"
#endif


#ifdef __WATCOMC__
#define FA_NORMAL       _A_NORMAL
#define FA_RDONLY       _A_RDONLY
#define FA_HIDDEN       _A_HIDDEN
#define FA_SYSTEM       _A_SYSTEM
#define FA_LABEL        _A_VOLID
#define FA_DIREC        _A_SUBDIR
#define FA_ARCH         _A_ARCH
#endif

#ifdef __WATCOMC__
static int _chdrive(int iDrive)
{
    unsigned nDummy;
    _dos_setdrive(iDrive, &nDummy);
    return !(_getdrive() == iDrive);
}
#endif

static char BASED_CODE pcszWildAll[] = "*.*";

//
// Default constructor
//
cFilePath::cFilePath()
{
}


//
// Copy constructor
//
cFilePath::cFilePath(const cFilePath &filePath)
{
    *this = filePath;
}


//
// Construct a file path from a file specification
//
cFilePath::cFilePath(const cFileSpec &fileSpec)
{
    FromFileSpec(fileSpec.GetName());
}


//
// Construct a file path from a text string
//
cFilePath::cFilePath(const char *pPath)
{
    FromText(pPath);
}


//
// Set the path from another path
//
cFilePath &cFilePath::operator=(const cFilePath& filePath)
{
    m_path = filePath.m_path;
    return *this;
}


//
// Set the path from a file specification
//
cFilePath &cFilePath::operator=(const cFileSpec& fileSpec)
{
    FromFileSpec(fileSpec.GetName());
    return *this;
}


//
// Set the path from a file specification string
//
void cFilePath::FromFileSpec(const char *pFileSpec)
{
    assert(pFileSpec);

    cPathSplitter PathSplitter(pFileSpec);
    cStr &s = m_path;

    unsigned nLengthName = PathSplitter.GetDriveLen() + PathSplitter.GetDirectoryLen();
    if (nLengthName)
    {
        char * p = s.GetBuffer(nLengthName);
        strncpy(p, PathSplitter.GetDrive(), PathSplitter.GetDriveLen());
        strncpy(p + PathSplitter.GetDriveLen(), PathSplitter.GetDirectory(), PathSplitter.GetDirectoryLen());
        p[nLengthName] = '\0';
        s.ReleaseBuffer(nLengthName);
    }
    else
        s.Empty();

    // If the path isn't empty...
    if (!m_path.IsEmpty())
    {
        char c = m_path[m_path.GetLength()-1];

        // If there is no trailing slash...
        if (c != '\\')
            m_path += '\\';
    }
}

#ifdef _MSC_VER
#define S_ISDIR(m) (m & _S_IFDIR)
#endif

//
// Check for existence
//
BOOL cFilePath::PathExists() const
{
    struct _stat StatBuf;
    cStr StrFullPath;
    if (IsEmpty() || !GetFullPath(StrFullPath))
        return FALSE;

    const unsigned nLenFull = StrFullPath.GetLength();

    if (nLenFull < 2)
        return FALSE;

    char * const pName = StrFullPath.Detach();

    AssertMsg(pName[nLenFull-1] == '\\' || pName[nLenFull-1] == '/', "Bad file path detected");
    pName[nLenFull - 1] = '\0';  // get rid of '\\'

    // Previously used access(), but that doesn't distinguish between
    // a file and a directory:
    // int result = access(pName, 0);
    int result = _stat(pName, &StatBuf);
    if (result == 0) {
       // Okay, it exists -- is it actually a directory?
       if (S_ISDIR(StatBuf.st_mode)) {
          // Yep, it is
       } else {
          // Nope -- unset result
          result = -1;
       }
    }

    free(pName);

    return result == 0;
}


//
// Check for existence
//
BOOL cFilePath::CreatePath() const
{
    cStr StrFullPath;
    if (IsEmpty() || !GetFullPath(StrFullPath))
        return FALSE;

    const unsigned nLenFull = StrFullPath.GetLength();

    if (nLenFull < 2)
        return FALSE;

    char * const pName = StrFullPath.Detach();

    AssertMsg(pName[nLenFull-1] == '\\' || pName[nLenFull-1] == '/', "Bad file path detected");
    pName[nLenFull - 1] = '\0';  // get rid of '\\'

    int result = mkdir(pName);

    free(pName);

    return result != -1;
}


//
// Set the path from a full path string
//
void cFilePath::FromText(const char *pPath)
{
    assert(pPath);
    m_path = pPath;

    // If the path isn't empty...
    if (!m_path.IsEmpty())
    {
        char c = m_path[m_path.GetLength()-1];

        // If there is no trailing slash...
        if (c != '\\')
            m_path += '\\';

    }

}


//
// Return the path name as a text string
//
void cFilePath::AsText(cStr& str) const
{
    str = m_path;
}


//
// Change directory to this path
//
BOOL cFilePath::SetCurrentPath() const
{
    BEGIN_DEBUG_MSG("cFilePath::SetCurrentPath()")

    // Get the file and extension
    cPathSplitter PathSplitter(m_path);

    cStr drive;
    cStr dir;
    PathSplitter.GetSplit(&drive, &dir);

    DebugMsg2("Split into '%s' and '%s'",drive.BufIn(),dir.BufIn());

    // Need to strip trailing '\' from dir
    if (dir.GetLength() > 1)
        dir.Remove(dir.GetLength()-1, 1);

    DebugMsg2("Stripped down to '%s' and '%s'",drive.BufIn(),dir.BufIn());

    if (drive.GetLength())
    {
        int driveNum = toupper(drive[0]) - 'A'+1;

        if (_chdrive(driveNum) != 0)
            return FALSE;
    }

    if (chdir(dir) != 0)
        return FALSE;

    return TRUE;

    END_DEBUG;
}


//
// Add a relative path to the current path
//
BOOL cFilePath::AddRelativePath(const cFilePath &relPath)
{
    // Should already be trailed by \...
    AssertMsg(m_path[m_path.GetLength()-1]=='\\', "Invalid file path");

    if (relPath.IsEmpty())
        return TRUE;

    if (!relPath.IsRelativePath())
        return FALSE;

    m_path += relPath.m_path;

    return ReducePathDots();
}


//
// Make this path a full path
//
BOOL cFilePath::MakeFullPath()
{
    cStr FullPath;
    if (!GetFullPath(FullPath))
        return FALSE;

    m_path = FullPath;
    return TRUE;
}


//
// Save the path to the stream
//
BOOL cFilePath::ToStream(cOStore &OStore) const
{
    return m_path.ToStream(OStore);
}


//
// Load the path from the stream
//
BOOL cFilePath::FromStream(cIStore &IStore)
{
    return m_path.FromStream(IStore);
}


//
// See if two paths are logically equal
//
BOOL cFilePath::operator==(const cFilePath &FilePath) const
{
    cStr Path1;
    if (!GetFullPath(Path1))
        return FALSE;

    cStr Path2;
    if (!FilePath.GetFullPath(Path2))
        return FALSE;

    return Path1 == Path2;
}

//
// Filespec Iteration
//

#if defined(_WIN32)
#define IsFoundDir(fd) ((fd).dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
#endif

BOOL sFindContext::DoFindFirst(tFindState WhatToFind, const cStr & StrSpec,
                   cStr & StrResult)
{
    BEGIN_DEBUG_MSG2("sfc DoFindFirst(%d,%s,)",WhatToFind,StrSpec.BufIn());
#if defined(_WIN32)
    DebugMsg("...Win32");
    if (PlatformFindContext.hContext)
        if (!FindClose(PlatformFindContext.hContext))
            return FALSE;

    WIN32_FIND_DATA FindData;

    if ((PlatformFindContext.hContext = FindFirstFile(StrSpec, &FindData)) != INVALID_HANDLE_VALUE)
    {
        DebugMsg2("FindFirstFile(%s) %c",(char*)FindData.cFileName, IsFoundDir(FindData)?'d':'f');
        while ((WhatToFind == kFindingFiles) ? IsFoundDir(FindData) : !IsFoundDir(FindData))
        {
            if (!FindNextFile(PlatformFindContext.hContext, &FindData))
            {
                FindClose(PlatformFindContext.hContext);
                PlatformFindContext.hContext = NULL;
                Finding = kFindingNone;
                return FALSE;
            }
            DebugMsg2("... skipped, trying  %s %c",(char*)FindData.cFileName, IsFoundDir(FindData)?'d':'f');
        }
        Finding = WhatToFind;
        StrResult = FindData.cFileName;
        return TRUE;
    }

    PlatformFindContext.hContext = NULL;
    Finding = kFindingNone;

#else   /* Windows or Dos */
    DebugMsg("...Win/Dos");

    if (WhatToFind == kFindingFiles)
    {
        if(0 == _dos_findfirst((char*)((const char *) StrSpec),
                               ~FA_DIREC & ~FA_LABEL,
                               (find_t*)&PlatformFindContext))
        {
            StrResult = PlatformFindContext.name;
            Finding = kFindingFiles;
            return TRUE;
        }
    }
    else if (WhatToFind == kFindingPaths)
    {
        // dos_findfirst(...FA_DIREC...) does not exclude non-directories;
        // it simply prevents the normal exclusion of directories.
        //   (See MS DOS Ref for 21h#4Eh, 21h#11h.)
        // Thus, must post-filter normals out of FA_DIREC results etc.
        if(0 == _dos_findfirst((char*)((const char *) StrSpec),
                               FA_DIREC,
                               (find_t*)&PlatformFindContext))
        {
            // Usually the first will be "." which has FA_DIREC attrib,
            // but Novell Roots don't have "." even if Novell is supposed to show ".."
            // in which case the first may be a normal file not a directory.
            // (Even nonroot Novell dirs may not have "." if user doesn't set SHOWDOTS ?)
            if (!(PlatformFindContext.attrib & FA_DIREC))
            {
                do
                {
                    if (0 != _dos_findnext((find_t*)&PlatformFindContext))
                    {
                        WhatToFind = kFindingNone;
                        return FALSE;             // none/no-more found
                    }
                } while (!(PlatformFindContext.attrib & FA_DIREC));
            }

            StrResult = PlatformFindContext.name;
            Finding = kFindingPaths;
            DebugMsg1("... accepting DOS dir '%s'",StrResult.BufIn());
            return TRUE;
        }
    }

#endif
    return FALSE;
    END_DEBUG;
}

BOOL sFindContext::DoFindNext(cStr & StrResult)
{
    #if defined(_WIN32)
    if (!PlatformFindContext.hContext)
        return FALSE;

    WIN32_FIND_DATA FindData;

    if (FindNextFile(PlatformFindContext.hContext, &FindData))
    {
        while ((Finding == kFindingFiles) ? IsFoundDir(FindData) : !IsFoundDir(FindData))
        {
            if (!FindNextFile(PlatformFindContext.hContext, &FindData))
            {
                FindClose(PlatformFindContext.hContext);
                PlatformFindContext.hContext = NULL;
                Finding = kFindingNone;
                return FALSE;
            }
        }
        StrResult = FindData.cFileName;
        return TRUE;
    }

    FindClose(PlatformFindContext.hContext);
    PlatformFindContext.hContext = NULL;
    Finding = kFindingNone;
    return FALSE;

#else   /* Windows or Dos */

    if (Finding == kFindingFiles)
    {
        if (0 != _dos_findnext((find_t*)&PlatformFindContext))
        {
            Finding = kFindingNone;
            return FALSE;             // none/no-more found
        }
    }
    else if (Finding == kFindingPaths)
    {
        do
        {
            if (0 != _dos_findnext((find_t*)&PlatformFindContext))
            {
                Finding = kFindingNone;
                return FALSE;             // none/no-more found
            }
        } while (!(PlatformFindContext.attrib & FA_DIREC));
    }
    StrResult = PlatformFindContext.name;
    return TRUE;

#endif
}

//
// File path portion of find
//
BOOL cFilePath::FindFirst(cFileSpec &fs, sFindContext& FC) const
{
    cStr WcString = m_path + pcszWildAll;
    cStr StrFoundFile;

    if (FC.DoFindFirst(sFindContext::kFindingFiles, WcString, StrFoundFile))
    {
        fs.Empty();
        fs.SetFileName(StrFoundFile);
        fs.SetFilePath(*this);
        return TRUE;
    }

    FC.Finding = sFindContext::kFindingNone;
    return FALSE;
}

BOOL cFilePath::FindNext(cFileSpec &fs, sFindContext& FC) const
{
    if (FC.Finding != sFindContext::kFindingFiles)
    {
        CriticalMsg("Must FindFirst before FindNext using cFilePath");
        return FindFirst(fs,FC);
    }
    else
    {
        cStr StrFoundFile;
        if (FC.DoFindNext(StrFoundFile))
        {
            fs.Empty();
            fs.SetFileName(StrFoundFile);
            fs.SetFilePath(*this);
            return TRUE;
        }
        FC.Finding = sFindContext::kFindingNone;
        return FALSE;
    }
}

//
// Filepath Iteration
//
BOOL cFilePath::FindFirst(cFilePath &fp, sFindContext& FC) const
{
    cStr WcString = m_path + pcszWildAll;
    cStr StrFoundFile;

    if (FC.DoFindFirst(sFindContext::kFindingPaths, WcString, StrFoundFile))
    {
#if defined(WIN32)
        if(StrFoundFile  == "." || StrFoundFile ==  "..")
        {
            return FindNext(fp,FC);
        }

        fp.m_path = m_path;
        fp.m_path += StrFoundFile;
        fp.m_path += '\\';
#ifdef DEBUG_FILEPATH
        int iPathLen = m_path.GetLength();
        AssertMsg(iPathLen==0||m_path[iPathLen-1]=='\\',
                    "Pathname not ending in \\");
#endif
        return TRUE;
#endif
    }
    FC.Finding = sFindContext::kFindingNone;
    return FALSE;
}

BOOL cFilePath::FindNext(cFilePath &fp, sFindContext& FC) const
{
    if (FC.Finding != sFindContext::kFindingPaths)
    {
        CriticalMsg("Must FindFirst before FindNext using cFilePath");
        return FindFirst(fp,FC);
    }

    cStr StrFoundFile;

#if defined(WIN32)
    do
    {
        if (!FC.DoFindNext(StrFoundFile))
        {
            FC.Finding = sFindContext::kFindingNone;
            return FALSE;             // none/no-more found
        }
    }
    while( StrFoundFile  == "." || StrFoundFile ==  "..");

    fp.m_path = m_path;
    fp.m_path += StrFoundFile;
    fp.m_path += '\\';

#ifdef DEBUG_FILEPATH
        int iPathLen = m_path.GetLength();
        AssertMsg(iPathLen==0||m_path[iPathLen-1]=='\\',
                    "Pathname not ending in \\");
#endif

    return TRUE; // Found one.
#else
    return FALSE;
#endif
}


//
// End the file system search
//
void cFilePath::FindDone(sFindContext & fc) const
{
    #if defined(_WIN32)
    if (fc.PlatformFindContext.hContext)
    {
        FindClose(fc.PlatformFindContext.hContext);
        fc.PlatformFindContext.hContext = NULL;
        fc.Finding = sFindContext::kFindingNone;
    }
    #endif
}


//
// What's the name of the root?
//
BOOL cFilePath::GetRootDir(const char *pPath, cStr &Str)
{
    if (!pPath || !*pPath)
    {
        Str.Empty();
        return FALSE;
    }

    if (::IsRelativePath(pPath))
    {
        if (::GetFullPath(pPath, Str.BufOut()))
        {
            Str.BufDone();
            cPathSplitter PathSplitter(Str);
            PathSplitter.GetDrive(Str);
            Str += '\\';
            return TRUE;
        }
    }
    else if (*(pPath+1) == ':')
    {
        Str += *pPath;
        Str += ":\\";
    }
    else if (*pPath == '\\' && *(pPath+1) == '\\')
    {
        const char * pEndDomain = strchr(pPath+2, '\\');
        unsigned nAppendLen;
        if (pEndDomain)
            nAppendLen = pEndDomain - pPath;
        else
            nAppendLen = strlen(pPath);
        Str.Empty();
        Str.Append(nAppendLen, pPath);
    }

    return FALSE;
}


//
// What's the volume called
//
BOOL cFilePath::GetVolumeName(const char * /* pPath */, cStr & /* Str */)
{
    return FALSE;
}

//
// What kind of file system does it use (FAT, HPFS, Mac, NFS, NTFS, etc.)
//

eFileSystemKind cFilePath::GetFileSystemKind() const
{
    // @TBD: This is crude:
    if (GetCaseConventions() & kNameCaseIgnored)
        return kFATFileSystem;
    return kDefaultFileSystem;
}

BOOL cFilePath::GetFileSystemName(const char * /* pPath */, cStr & /* Str */)
{
    return FALSE;
}


//
// Get the serial number
//
ulong cFilePath::GetVolumeSerialNumber(const char * /* pPath */)
{
    return 0L;
}


//
// Get the volume case relevance
//
int cFilePath::GetCaseConventions(const char *pPath)
{
    #if defined(_WIN32)
    BEGIN_DEBUG_MSG_EX1(CASE, "int cFilePath::GetCaseConventions(%s)", pPath);
    DWORD dwFileSysFlags;
    cStr StrRootDir;
    GetRootDir(pPath, StrRootDir);
    DebugMsgEx1(CASE, "Calling ::GetVolumeInformation(%s, ...)", StrRootDir.operator const char *());
    if (::GetVolumeInformation(StrRootDir, NULL, 0, NULL, NULL, &dwFileSysFlags, NULL, 0))
    {
        DebugMsgEx1(CASE, "::GetVolumeInformation() returned %lx", dwFileSysFlags);

        DebugMsgTrueEx(CASE, (dwFileSysFlags & FS_CASE_IS_PRESERVED), "...volume is case retaining");
        DebugMsgTrueEx(CASE, (dwFileSysFlags & FS_CASE_SENSITIVE), "...volume is case sensitive");
        DebugMsgTrueEx(CASE, (dwFileSysFlags & FS_UNICODE_STORED_ON_DISK), "...volume stores unicode");
        DebugMsgTrueEx(CASE, !(dwFileSysFlags & (FS_CASE_IS_PRESERVED | FS_CASE_SENSITIVE)), "...volume is case ignorant");

        if (dwFileSysFlags)
        {
            int iReturn = 0;
            if (dwFileSysFlags & FS_CASE_IS_PRESERVED)
                iReturn |= kNameCasePreserved;
            if (dwFileSysFlags & FS_CASE_SENSITIVE)
                iReturn |= kNameCaseSensitive;
            if (dwFileSysFlags & FS_UNICODE_STORED_ON_DISK)
                iReturn |= kUnicodeStored;
            if (!(dwFileSysFlags & (FS_CASE_IS_PRESERVED | FS_CASE_SENSITIVE)))
                iReturn |= kNameCaseIgnored;
            return iReturn;
        }
    }
    DebugMsgEx(CASE, "GetVolumeInformation() returned FALSE, ...assuming volume is case ignorant");
    return kNameCaseIgnored;
    END_DEBUG;
    #else
    return kNameCaseIgnored;
    #endif
}


//
// Get the maximum length of a component name
//
ulong cFilePath::GetMaxLegalComponentLen(const char * /* pPath */)
{
    #if defined(_WIN32)
    return 255;
    #else
    return 8;
    #endif
}


//
// Character okay for volume?
//
BOOL cFilePath::IsValidChar(const char * /* pPath */, char /*c*/)
{
    return TRUE;
}


//
// Component name okay for volume?
//
BOOL cFilePath::IsValidComponentName(const char * /* pPath */, const char * /* psz */)
{
    return TRUE;
}


//
// Get full path relative to arg
//
BOOL cFilePath::GetFullPath(cStr &Str, const cFilePath &fp) const
{
    cFilePath fpFullPath(fp);
    if (fpFullPath.AddRelativePath(*this))
        return fpFullPath.GetFullPath(Str);
    return GetFullPath(Str);
}


//
// Make path full relative to arg
//
BOOL cFilePath::MakeFullPath(const cFilePath &fp)
{
    if (IsRelativePath())
    {
        cFilePath fpFullPath(fp);
        cFilePath fpRelative(*this);
        if (fpFullPath.AddRelativePath(*this))
            *this = fpFullPath;
    }

    return MakeFullPath();
}

