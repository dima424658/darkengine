///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/splitpat.cpp $
// $Author: TOML $
// $Date: 1996/07/10 14:43:58 $
// $Revision: 1.3 $
//
// (Cribbed from Borland C & extended)
//

#include <lg.h>
#include <ctype.h>

#ifndef macintosh
#include <direct.h>
#else
#define _MAX_PATH 80
#define _MAX_DRIVE 3
#define _MAX_DIR 66
#define  _MAX_FNAME 9
#define _MAX_EXT  5
#endif

#include <string.h>
#include <splitpat.h>

#define LDEBUG
#ifdef LDEBUG
#define LAssertMsg(b, s) AssertMsg(b, s)
#else
#define LAssertMsg(b, s)
#endif

static int __stdcall DotFound(char *);
static void __stdcall CopyIt(char *, const char *, unsigned);
#define WILDCARDS 0x01
#define EXTENSION 0x02
#define FILENAME  0x04
#define DIRECTORY 0x08
#define DRIVE     0x10

// Name        SplitPath - splits a full path name into its components
//
// Usage       int SplitPath(const char *path, char * drive, char * dir,
//              char * name, char * ext);
//
// Description     SplitPath takes a file's full path name (path) as a string
//         in the form
//
//             X:\DIR\SUBDIR\NAME.EXT
//
//         and splits path into its four components. It then stores
//         those components in the strings pointed to by drive, dir,
//         name and ext. (Each component is required but can be a
//         NULL, which means the corresponding component will be
//         parsed but not stored.)
//
//         The maximum sizes for these strings are given by the
//         constants _MAX_DRIVE, _MAX_DIR, _MAX_PATH, _MAX_NAME and _MAX_EXT,
//         (defined in dir.h) and each size includes space for the
//         null-terminator.
//
//             Constant    (Max.)  String
//
//             _MAX_PATH     (80)    path
//             _MAX_DRIVE    (3)     drive; includes colon (:)
//             _MAX_DIR      (66)    dir; includes leading and
//                         trailing backslashes (\)
//              _MAX_FNAME     (9)     name
//             _MAX_EXT      (5)     ext; includes leading dot (.)
//
//         SplitPath assumes that there is enough space to store each
//         non-NULL component. fnmerge assumes that there is enough
//         space for the constructed path name. The maximum constructed
//         length is _MAX_PATH.
//
//         When SplitPath splits path, it treats the punctuation as
//         follows:
//
//         * drive keeps the colon attached (C:, A:, etc.)
//
//         * dir keeps the leading and trailing backslashes
//           (\turboc\include\,\source\, etc.)
//
//         * ext keeps the dot preceding the extension (.c, .exe, etc.)
//

void SplitPath(const char *pszSourcePath, char *driveP, char *dirP,
                char *nameP, char *extP)
    {
    register char *pB;
    register int Wrk;
    int Ret;

    char buf[_MAX_PATH + 2];
    /* Set all string to default value zero */
    Ret = 0;
    if (driveP)
        *driveP = 0;
    if (dirP)
        *dirP = 0;
    if (nameP)
        *nameP = 0;
    if (extP)
        *extP = 0;

    /* Copy filename into template up to _MAX_PATH characters */
    pB = buf;
    while (*pszSourcePath == ' ')
        pszSourcePath++;
    if ((Wrk = strlen(pszSourcePath)) > _MAX_PATH)
        Wrk = _MAX_PATH;
    *pB++ = 0;
    strncpy(pB, pszSourcePath, Wrk);
    *(pB += Wrk) = 0;

    /* Split the filename and fill corresponding nonzero pointers */
    Wrk = 0;
    for (;;)
        {
        switch (*--pB)
            {
            case '.':
                if (!Wrk && (*(pB + 1) == '\0'))
                    Wrk = DotFound(pB);
                if ((!Wrk) && ((Ret & EXTENSION) == 0))
                    {
                    Ret |= EXTENSION;
                    CopyIt(extP, pB, _MAX_EXT - 1);
                    *pB = 0;
                    }
                continue;
            case ':':
                if (pB != &buf[2])
                    continue;
            case '\0':
                if (Wrk)
                    {
                    if (*++pB)
                        Ret |= DIRECTORY;
                    CopyIt(dirP, pB, _MAX_DIR - 1);
                    *pB-- = 0;
                    break;
                    }
            case '/':
            case '\\':
                if (!Wrk)
                    {
                    Wrk++;
                    if (*++pB)
                        Ret |= FILENAME;
                    CopyIt(nameP, pB,  _MAX_FNAME - 1);
                    *pB-- = 0;
                    if (*pB == 0 || (*pB == ':' && pB == &buf[2]))
                        break;
                    }
                continue;
            case '*':
            case '?':
                if (!Wrk)
                    Ret |= WILDCARDS;
            default:
                continue;
                }
        break;
        }
    if (*pB == ':')
        {
        if (buf[1])
            Ret |= DRIVE;
        CopyIt(driveP, &buf[1], _MAX_DRIVE - 1);
        }
    }


//
// Name        CopyIt - copies a string to another
//
// Usage       void  CopyIt(char *dst, const char *src,
//                     unsigned maxlen)
//
// Description     copies string scr to string dst.
//
// Return value    nothing
//
static void __stdcall CopyIt(char *dst, const char *src, unsigned maxlen)
    {
    if (dst)
        {
        if (strlen(src) >= maxlen)
            {
            strncpy(dst, src, maxlen);
            dst[maxlen] = 0;
            }
        else
            strcpy(dst, src);
        }
    }
//
// Name        DotFound - checks for special dir name cases
//
// Usage       int  DotFound(char *pB);
//
// Description     checks for special directory names
//
static int __stdcall DotFound(char *pB)
    {
    if (*(pB - 1) == '.')
        pB--;
    switch (*--pB)
        {
        case ':':
            if (*(pB - 2) != '\0')
                break;
        case '/':
        case '\\':
        case '\0':
            return 1;
            }
    return 0;
    }
//////////////////////////////////////////////////////////////////////////////

static int DotPathFound(const char *p, const char *const pLeftLimit)
    {
    BEGIN_DEBUG_STR("DotPathFound()");
    if (*p == '.')
        {
        if (p <= pLeftLimit)
            return TRUE;

        if (*(p - 1) == '.')
            {
            p--;
            if (p <= pLeftLimit)
                return TRUE;
            }

        p--;
        if ((*p == ':' && p == pLeftLimit + 1) || *p == '/' || *p == '\\' || *p == '\0')
            return TRUE;
        }
    return FALSE;
    END_DEBUG;
    }

///////////////////////////////////////
// Maximum calculators, currently just return current
// target platform values. Use some simple table
// eventually

inline BOOL RemoveLeadingSpaceForKind(eFileSystemKind Kind)
    {
    return !!(Kind == kFATFileSystem);
    }

inline int MaxPathForKind(eFileSystemKind /* Kind */)
    {
#if defined(_WIN32)
    return 255;
#else
    return 80;
#endif
    }

inline int MaxDriveForKind(eFileSystemKind /* Kind */)
    {
    return 3;
    }

inline int MaxDirForKind(eFileSystemKind /* Kind */)
    {
#if defined(_WIN32)
    return 240;                                  // arbitrary
#else
    return 66;
#endif
    }

inline int MaxFileForKind(eFileSystemKind /* Kind */)
    {
#if defined(_WIN32)
    return 240;
#else
    return 9;
#endif
    }

inline int MaxExtForKind(eFileSystemKind /* Kind */)
    {
#if defined(_WIN32)
    return 240;                                  // arbitrary
#else
    return 5;
#endif
    }
///////////////////////////////////////

BOOL cPathSplitter::DoSplitTo(eComponent /* SplitToComponent */)
    {
    char buffer[256];
    sprintf(buffer, "cPathSplitter::DoSplitTo() (%s)", (pcszTarget) ? pcszTarget : "NULL");
    BEGIN_DEBUG_STR(buffer);
    ///////////////////////
    // Immediate conditions
    if (LastSplitTo == kDrive)
        return TRUE;

    if (!pcszTarget)
        return FALSE;

    DebugMsg("Need to split...");

    ///////////////////////
    // Set up baselines, utility macros
    // Maximums *include* terminal NULL
    const int MaxPath = MaxPathForKind(FileSystem);
    const int MaxDrive = MaxDriveForKind(FileSystem);
    const int MaxDir = MaxDirForKind(FileSystem);
    const int MaxFile = MaxFileForKind(FileSystem);
    const int MaxExt = MaxExtForKind(FileSystem);
#define pEndPath    (pcszTarget + nSignificantTargetLen)
#define pEndExt     pEndPath
#define pEndName    Extension.pComponentText
#define pEndDir     Name.pComponentText
#define pEndDrive   Directory.pComponentText

    ///////////////////////
    // First time through -- Get length of target we're concerned with
    if (LastSplitTo == kNone)
        {
        DebugMsg("First time through...");
        if (RemoveLeadingSpaceForKind(FileSystem))
            while (isspace(*pcszTarget))
                pcszTarget++;

        if ((nSignificantTargetLen = strlen(pcszTarget)) >= MaxPath)
            nSignificantTargetLen = MaxPath;

        // @TBD: Should strip trailing space & periods, periods in particular
        }

    if (!*pcszTarget)
        {
        Extension.IfNotFoundAssumeZeroLenAt(pcszTarget);
        Name.IfNotFoundAssumeZeroLenAt(pcszTarget);
        Directory.IfNotFoundAssumeZeroLenAt(pcszTarget);
        Drive.IfNotFoundAssumeZeroLenAt(pcszTarget);
        LastSplitTo = kDrive;
        return TRUE;
        }

    ///////////////////////
    // Do split
    // @TBD: This is just a straight copy of SplitPath() with some
    // modifications.  It should be reworked to:
    // (1) Start from the LastSplitTo state
    // (2) Only go to the SplitToComponent state
    // (3) Give Wrk a better name!

    DebugMsg("Splitting...");

    register const char *pScan;
    register int Wrk = 0;
    register char c;
    int FoundFlags = 0;
    Wrk = 0;
    pScan = pcszTarget + nSignificantTargetLen;

    for (;;)
        {
        --pScan;
        if (pScan < pcszTarget)
            c = '\0';
        else
            c = *pScan;

        switch (c)
            {
            case '.':

                if (!Wrk && (*(pScan + 1) == '\0'))
                    Wrk = DotPathFound(pScan, pcszTarget);

                if ((!Wrk) && ((FoundFlags & EXTENSION) == 0))
                    {
                    DebugMsg("Found extension");
                    FoundFlags |= EXTENSION;
                    Extension.Found(pScan, pEndExt, MaxExt - 1);
                    LastSplitTo = kExtension;
                    }
                continue;
            case ':':
                if (pScan != pcszTarget + 1)
                    continue;
            case '\0':
                if (Wrk)
                    {
                    if (*++pScan)
                        FoundFlags |= DIRECTORY;
                    DebugMsg("Found directory");
                    Extension.IfNotFoundAssumeZeroLenAt(pEndExt);
                    Name.IfNotFoundAssumeZeroLenAt(pEndName);
                    Directory.Found(pScan, pEndDir, MaxDir - 1);
                    pScan--;
                    LastSplitTo = kDirectory;
                    break;
                    }
            case '/':
            case '\\':
                if (!Wrk)
                    {
                    Wrk++;
                    if (*++pScan)
                        FoundFlags |= FILENAME;
                    DebugMsg("Found file name");
                    Extension.IfNotFoundAssumeZeroLenAt(pEndExt);
                    Name.Found(pScan, pEndName, MaxFile - 1);
                    pScan--;
                    LastSplitTo = kName;
                    if ((pScan < pcszTarget) || (*pScan == ':' && pScan == pcszTarget + 1))
                        break;
                    }
                continue;
            case '*':
            case '?':
                if (!Wrk)
                    FoundFlags |= WILDCARDS;
            default:
                continue;
                }
        break;
        }

    Extension.IfNotFoundAssumeZeroLenAt(pEndExt);
    Name.IfNotFoundAssumeZeroLenAt(pEndName);
    Directory.IfNotFoundAssumeZeroLenAt(pEndDir);
    if (pScan >= pcszTarget && *pScan == ':')
        {
        if (*pcszTarget)
            FoundFlags |= DRIVE;
        DebugMsg("Found drive");
        Drive.Found(pScan - 1, pEndDrive, MaxDrive - 1);
        }
    else
        {
        sprintf(buffer, "Drive not found (%d && %d)", !!(pScan >= pcszTarget), !!(pScan >= pcszTarget && *pScan == ':'));
        DebugStr(buffer);
        Drive.IfNotFoundAssumeZeroLenAt(pEndDrive);
        }

    LastSplitTo = kDrive;

#ifdef LDEBUG
    DebugMsg("Split result is:");
    cStr s;
    GetDrive(s);

    char buffer[256];
    sprintf(buffer, "Drive is \"%s\"", s.operator const char* ());
    DebugStr(buffer);
    GetDirectory(s);
    sprintf(buffer, "Directory is \"%s\"", s.operator const char* ());
    DebugStr(buffer);
    GetName(s);
    sprintf(buffer, "Name is \"%s\"", s.operator const char* ());
    DebugStr(buffer);
    GetExtension(s);
    sprintf(buffer, "Extension is \"%s\"", s.operator const char* ());
    DebugStr(buffer);
#endif

    return TRUE;
    END_DEBUG;
    }

BOOL cPathSplitter::GetSplit(cStr * pDrive, cStr * pDir,
                              cStr * pName, cStr * pExt)
    {
    BEGIN_DEBUG_STR("cPathSplitter::GetSplit()");
    if (pDrive)
        GetDrive(*pDrive);
    if (pDir)
        GetDirectory(*pDir);
    if (pName)
        GetName(*pName);
    if (pExt)
        GetExtension(*pExt);
    return TRUE;
    END_DEBUG;
    }

//////////////////////////////////////////////////////////////////////////////
