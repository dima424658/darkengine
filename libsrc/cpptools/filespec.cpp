///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/filespec.cpp $
// $Author: JUSTIN $
// $Date: 1998/07/09 09:47:42 $
// $Revision: 1.12 $
//
// (c) Copyright 1993 - 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <direct.h>
#include <io.h>
#include <errno.h>
#include <ctype.h>

#include <lg.h>
#include <filespec.h>
#include <filepath.h>
#include <fnamutil.h>

#include <splitpat.h>

#include <sys/types.h>
#include <sys/stat.h>

#if 0
//include <storintr.h>
#endif

#ifndef DEBUG_PATTERN
#define DEBUG_PATTERN 0
#else
#pragma message "DEBUG_PATTERN"
#endif

#ifndef DEBUG_CONSTRUCT
#define DEBUG_CONSTRUCT 0
#else
#pragma message "DEBUG_CONSTRUCT"
#endif

#ifndef DEBUG_TIME
#define DEBUG_TIME 0
#else
#pragma message "DEBUG_TIME"
#endif

static BOOL __stdcall PatternMatch(const char *pString, const char *pPattern);
static BOOL __stdcall StarMatch(const char *pString, const char *pPattern);

#ifndef SHIP
static const char BASED_CODE NotFile[] = "Not a file specification";
static const char BASED_CODE NotUntitled[] = "Not an untitled specification";
static const char BASED_CODE NotSpecial[] = "Not a special specification";
#endif


cStr &GetCondensePathStr(cStr & Path,
        unsigned nCondenseToLen,
        eFileCondenseCase CaseChange = kFileNoChange,
        BOOL bRequireDrive = FALSE);


static cStr &GetCondensePathStr(cStr & Path,
        unsigned nCondenseToLen,
        eFileCondenseCase CaseChange,
        BOOL bRequireDrive)

{
    cStr OldPath(Path);
    char *pszPath = Path.GetBuffer(0);
    GetCondensePathStr(OldPath, pszPath, nCondenseToLen, CaseChange, bRequireDrive);
    Path.ReleaseBuffer();
    return Path;
}



//
// Construct a file spec
//
cFileSpec::cFileSpec(const cFileSpec &fs)
    : sFileDesc(0, fs.Kind)
{
    DebugMsgEx2(CONSTRUCT, "%p->cFileSpec::cFileSpec(%s)", this, fs.GetName());
    if (Kind != kSpecKindUnknown)
        pName = strdup(fs.GetName());
}


//
// Construct a file spec
//
cFileSpec::cFileSpec(const sFileDesc &fs)
    : sFileDesc(0, fs.Kind)
{
    DebugMsgEx2(CONSTRUCT, "%p->cFileSpec::cFileSpec(%s)", this, fs.GetName());

    if (Kind != kSpecKindUnknown)
        pName = strdup(fs.GetName());
}


//
// Construct a file spec
//
cFileSpec::cFileSpec(eFileSpecKind kind, const char *s)
    : sFileDesc(0, kind)
{
    DebugMsgEx3(CONSTRUCT, "%p->cFileSpec::cFileSpec(%d, %s)", this, (int)Kind, s);

    if (kind != kSpecKindUnknown)
    {
        cStr FileName(s);
        pName = FileName.Detach();
    }
}


//
// Initialize with a name (if null then this is unknown type)
//
cFileSpec::cFileSpec(const char *name)
{
    DebugMsgEx2(CONSTRUCT, "%p->cFileSpec::cFileSpec(%s)", this, name);

    if (name && *name)
    {
        cStr FileName(name);
        pName = FileName.Detach();
        Kind = kSpecKindFile;
    }
}


//
// Construct a file spec
//
cFileSpec::cFileSpec(const cFilePath &path, const char *name)
    : sFileDesc(0, kSpecKindFile)
{
    DebugMsgEx3(CONSTRUCT, "%p->cFileSpec::cFileSpec(%s, %s)", this, path.GetPathName(), name);

    AssertMsg(name && name[0], "No file name given");

    cStr FileName;
    path.AsText(FileName);
    FileName += name;
    pName = FileName.Detach();
}


//
// Destroy the file specification
//
cFileSpec::~cFileSpec()
{
    DebugStr(cFmtStr("%p->cFileSpec::~cFileSpec(%p, %s)", this, pName, pName));
    free(pName);
}


//
// Return TRUE if the two file specifications are equal
//
cFileSpec &cFileSpec::operator=(const sFileDesc &fs)
{
    if (this != &fs)
    {
        Kind = fs.Kind;

        free(pName);

        if (Kind == kSpecKindUnknown)
            pName = 0;
        else
            pName = strdup(fs.GetName());
    }

    return *this;
}


//
// Set the file specification path and name
//
void cFileSpec::Set(const cFilePath &Path, const char *psz)
{
    AssertMsg(Kind == kSpecKindFile || Kind == kSpecKindUnknown, NotFile);

    cStr FileName;
    Path.AsText(FileName);
    FileName += psz;

    free(pName);
    pName = FileName.Detach();

    Kind = kSpecKindFile;
}


//
// Make the file specification a full path
//
BOOL cFileSpec::MakeFullPath()
{
    if (Kind != kSpecKindFile)
        return TRUE;

    cStr s;
    if (!GetFullPath(s))
        return FALSE;

    free(pName);
    pName = s.Detach();

    return TRUE;
}


//
// Get the file specification as a full path
//
BOOL cFileSpec::GetFullPath(cStr &s) const
{
    BEGIN_DEBUG_MSG1("%p->cFileSpec::GetFullPath()", this)

    if (IsEmpty())
    {
        s.Empty();
        return TRUE;
    }

    if (Kind != kSpecKindFile)
    {
        s = GetName();
        return TRUE;
    }

    if (*GetName() == 0)
    {
        s.Empty();
        return FALSE;
    }

    const BOOL fSuccess = ::GetFullPath(GetName(), s.BufOut());
    s.BufDone();
    return fSuccess;
    END_DEBUG
}


//
// Get the file specification as a text string
//
void cFileSpec::GetRelativePath(cStr &s) const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);
    s = GetName();
}


//
// Set the file specification from a text string
//
void cFileSpec::SetRelativePath(const char *s)
{
    AssertMsg(Kind == kSpecKindFile || Kind == kSpecKindUnknown, NotFile);
    AssertMsg(s != 0 && s[0] != '\0', "File specification cannot be NULL");

    cStr FileName(s);

    free(pName);
    pName = FileName.Detach();

    Kind = kSpecKindFile;
}

//
// Is this path absolute or relative?
//

BOOL cFileSpec::IsRelativePath() const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    if (!GetName())
        return FALSE;

    const char chFirst = GetName()[0];

    // If the relative path has a top-level directory...
    if (chFirst == '\\' || chFirst == '/')
        return FALSE;

    // If we have a drive specification...
    if (chFirst && GetName()[1] == ':')
        return FALSE;

    return TRUE;
}


//
// Get the root of the file
//
void cFileSpec::GetFileRoot(cStr &s) const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    cPathSplitter PathSplitter(GetName());
    PathSplitter.GetName(s);
}


//
// Replace parts of the file specification
//
void cFileSpec::ReplaceParts(const char *pDrive, const char *pDir, const char *pRoot, const char *pExt)
{
    AssertMsg(Kind == kSpecKindFile || Kind == kSpecKindUnknown, NotFile);

    cPathSplitter PathSplitter(GetName());

    // @TBD: this usage should be made more efficent
    cStr drive;
    cStr dir;
    cStr root;
    cStr ext;

    if (GetName())
        PathSplitter.GetSplit((pDrive) ? 0 : &drive, (pDir) ? 0 : &dir, (pRoot) ? 0 : &root, (pExt) ? 0 : &ext);
    else
        drive[0] = dir[0] = root[0] = ext[0] = 0;

    cStr FileName(pDrive ? pDrive : drive.BufIn());
    FileName += (pDir ? pDir : dir.BufIn());
    FileName += (pRoot ? pRoot : root.BufIn());
    if (pExt && *pExt != '.' && *pExt != 0)
        FileName += '.';
    FileName += (pExt ? pExt : ext.BufIn());

    free(pName);
    pName = FileName.Detach();

    Kind = kSpecKindFile;
}


//
// Set the root of the file while leaving everything else the same
//
void cFileSpec::SetFileRoot(const char *root)
{
    DebugMsg1("cFileSpec::SetFileRoot(%s)", root);
    ReplaceParts(0, 0, root, 0);
}


//
// Get the file extension
//
const char *cFileSpec::GetFileExtension() const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);
    if (Kind != kSpecKindFile || !GetName() || !*GetName())
        return GetName();

    const char * pExtReturn = GetName();
    while (*pExtReturn)
        pExtReturn++;

    const char * pEOS = pExtReturn;
    pExtReturn--;

    while (pExtReturn >= GetName())
    {
        if (*pExtReturn == '.')
        {
            if (*(pExtReturn + 1))
            {
                return pExtReturn;
            }
            break;
        }
        if (*pExtReturn == '\\')
            break;
        pExtReturn--;
    }

    return pEOS;
}


//
// Get the file extension
//
void cFileSpec::GetFileExtension(cStr &s) const
{
    s = GetFileExtension();
}


//
// Set the extension of the file while leaving everything else the same
//
void cFileSpec::SetFileExtension(const char *ext)
{
    ReplaceParts(0, 0, 0, ext);
}


//
// Get the name of the file
//
const char * cFileSpec::GetFileName() const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);
    if (Kind != kSpecKindFile || !GetName() || !*GetName())
        return GetName();

    const char * pNameReturn = GetName();
    while (*pNameReturn)
        pNameReturn++;
    pNameReturn--;

    char cChar;
    BOOL fFoundDelimiter = FALSE;
    while (pNameReturn >= GetName())
    {
        cChar = *pNameReturn;
        if (cChar == '\\' || cChar == ':')
        {
            fFoundDelimiter = TRUE;
            break;
        }
        pNameReturn--;
    }

    if (fFoundDelimiter)
        pNameReturn++;
    else
        pNameReturn = GetName();

    return pNameReturn;
}

//
// Get the name of the file
//
void cFileSpec::GetFileName(cStr &s) const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);
    s = GetFileName();
}


//
// Set the name of the file
//
void cFileSpec::SetFileName(const char *filename)
{
    DebugMsg1("cFileSpec::SetFileName(%s)",filename);
    ReplaceParts(0, 0, filename, "");
}


//
// Get the path of the file directory
//
void cFileSpec::GetFilePath(cFilePath &fp) const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    cPathSplitter PathSplitter(GetName());
    cStr &s = fp.m_path;

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
    if (!fp.m_path.IsEmpty())
    {
        char c = fp.m_path[fp.m_path.GetLength()-1];

        // If there is no trailing slash...
        if (c != '\\' && c != '/')
            fp.m_path += '\\';
    }

}


//
// Set the path of the file directory
//
void cFileSpec::SetFilePath(const cFilePath &path)
{
    DebugMsg2("%p->cFileSpec::SetFilePath(%s)", this, path.m_path.BufIn());
    AssertMsg(Kind == kSpecKindFile || Kind == kSpecKindUnknown, NotFile);
    cStr pathname;
    path.AsText(pathname);
    ReplaceParts(pathname, "");
    Kind = kSpecKindFile;
}

#ifdef _MSC_VER
#define S_ISDIR(m) (m & _S_IFDIR)
#endif

//
// Return TRUE if the file exists
//
BOOL cFileSpec::FileExists() const
{
    struct _stat StatBuf;

    if (Kind != kSpecKindFile)
        return FALSE;

    // Previously used access(), but that doesn't distinguish between
    // a file and a directory:
    // int result = access(GetName(), 0);
    int result = _stat(GetName(), &StatBuf);
    if (result == 0) {
       // Okay, it exists -- is it actually a file?
       if (S_ISDIR(StatBuf.st_mode)) {
          // Nope -- unset result
          result = -1;
       } else {
          // Yep, it is
       }
    }


    DebugMsg2("%s %s exist", GetName(), (result == 0) ? "does" : "does not");
    return result == 0;
}


//
// Delete the file
//
BOOL cFileSpec::UnlinkFile() const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    return remove(GetName())==0;
}


//
// Get the date of the file
//
ulong cFileSpec::GetModificationTime() const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    if (strpbrk(GetName(),"*?"))
    {
        DebugMsgEx(TIME, "Bad name");
        return 0;
    }

    WORD date;
    WORD time;

    #if defined(_WIN32)
    WIN32_FIND_DATA find;
    HANDLE FindHandle = FindFirstFile ((LPTSTR)GetName(), &find);
    if (FindHandle == INVALID_HANDLE_VALUE)
    {
        DebugMsgEx(TIME, "INVALID_HANDLE_VALUE");
        return 0;
    }

    FindClose (FindHandle);

    FILETIME lft;

    if (!FileTimeToLocalFileTime (&find.ftLastWriteTime, &lft))
    {
        DebugMsgEx(TIME, "Bad FileTimeToLocalFileTime");
        return 0;
    }

    if (!FileTimeToDosDateTime(&lft, &date, &time))
    {
        DebugMsgEx(TIME, "Bad FileTimeToDosDateTime");
        return 0;
    }
    #else
    struct find_t find;

    if (_dos_findfirst(GetName(), _A_SYSTEM | _A_HIDDEN, &find) != 0)
    {
        DebugMsgEx(TIME, "findfirst failed");
        return 0;
    }

    date = find.wr_date;
    time = find.wr_time;
    #endif

    DebugMsgEx3(TIME, "Time value is %lx(%x,%x)", (static_cast<ulong>(date)<<16)+time,
            date, time);

    return ((ulong)(date)<<16)+time;
}


//
// Set the untitled file number for the file
//
void cFileSpec::SetUntitledNumber(tUntitledNumber iUntitled)
{
    static const char BASED_CODE szUntitled[] = "Untitled-%d";
    AssertMsg(Kind == kSpecKindUntitled || Kind == kSpecKindUnknown, NotUntitled);
    Kind = kSpecKindUntitled;
    cStr s;
    s.FmtStr(szUntitled, iUntitled);
    free(pName);
    pName = s.Detach();
}


//
// Set a special name
//
void cFileSpec::SetSpecialName(const char *psz)
{
    AssertMsg(Kind == kSpecKindSpecial || Kind == kSpecKindUnknown, NotSpecial);
    Kind = kSpecKindSpecial;

    free(pName);
    pName = strdup(psz);
}


//
// Compare this file specification to another.
//
// Return value:
// -1 : This file specification sorts before the given
//  0 : The file specifications are equal
// +1 : This file specification sorts after the given
//
int cFileSpec::Compare(const sFileDesc &FileSpec) const
{
    // If there not the same kind...
    int Result = (int)Kind - (int)FileSpec.Kind;
    if (Result != 0)
        return Result;

    // If the kind is not file...
    if (Kind != kSpecKindFile)
        return IsEmpty() ? 0 : strcmp(GetName(), FileSpec.GetName());
    else
    {
    #if 0 // Currently (08-14-95) no presently supported file systems are case sensitive, only retaining or ignored
        // @TBD: what if right param is case sensitive?
        cFileSpec * pfsRight = (cFileSpec *) &FileSpec;
        if (GetCaseConventions() & kNameCaseSensitive || pfsRight->GetCaseConventions() & kNameCaseSensitive)
            return IsEmpty() ? 0 : strcmp(GetName(), FileSpec.GetName());
        else
    #endif
            return IsEmpty() ? 0 : stricmp(GetName(), FileSpec.GetName());
    }
}



//
// Save the file specification to the stream
//
BOOL cFileSpec::ToStream(cOStore &OStore) const
{
#if 0
    if (!OStore.WriteHeader("FILESPEC"))
        return FALSE;

    if (!OStore.To((int)Kind))
        return FALSE;

    switch (Kind)
    {
        case kSpecKindFile:
        case kSpecKindSpecial:
        case kSpecKindUntitled:
            if (!OStore.To(GetName(), (long)strlen(GetName()), TRUE))
                return FALSE;
            break;
    }

    return OStore.WriteTrailer();
#endif
    return FALSE;
}


//
// Load the file specification from the stream
//
BOOL cFileSpec::FromStream(cIStore &IStore)
{
#if 0
    if (!IStore.ReadHeader("FILESPEC"))
        return FALSE;

    if (!IStore.From((int)Kind))
        return FALSE;

    free(pName);

    if (Kind == kSpecKindUnknown)
        pName = 0;

    else
    {
        cStr Name;
        if (!Name.FromStream(IStore))
            return FALSE;

        pName = Name.Detach();
    }

    return IStore.ReadTrailer();
#endif
    return FALSE;
}

//
// Return TRUE if two file specifications are logically equal
//
BOOL cFileSpec::operator==(const sFileDesc &FileSpec) const
{
    if (Kind != FileSpec.Kind)
        return FALSE;

    return IsEmpty() ? TRUE : !strcmp(GetName(), FileSpec.GetName());
}


//
// Set anchor for get name string
//
cFilePath cFileSpec::AnchorPath;

void cFileSpec::SetAnchorPath(const cFilePath &NewAnchorPath)
{
    DebugMsg1("cFileSpec::SetAnchorPath(%s)", NewAnchorPath.m_path.BufIn());
    AnchorPath = NewAnchorPath;
}

//
// Return a name string for the specification
//
void cFileSpec::GetNameString(cStr &Name, eFileSpecNameStringStyle Style) const
{
    const char BASED_CODE pcszNullName[] = "<NULL>";
    const char BASED_CODE pcszAnonName[] = "<unknown or anonymous>";
    const char BASED_CODE pcszBadName[] = "<bad_name>";

    switch (Kind)
    {
        case kSpecKindFile:
            if (GetName())
            {
                if (Style == kNameOnlyNameStyle)
                    GetFileName(Name);
                else if (Style == kFullPathNameStyle)
                {
                    if (!GetFullPath(Name))
                        Name = GetName();
                }
                else if (Style == kAnchorRelativeNameStyle)
                {
                    GetNameString(Name, GetAnchorPath());
                }
                else
                {
                    GetNameString(Name, ".");
                }

                if (GetCaseConventions() & kNameCaseIgnored)
                    Name.MakeLower();
                else
                {
                    // Lowercase any all-caps components
                    const unsigned nLength = Name.GetLength();
                    char * pStart = Name.GetBuffer(nLength);
                    char * p = pStart;
                    char * pStartComponent = pStart;
                    BOOL bIsComponentAllUpper = TRUE;
                    while (1)
                    {
                        if (*p == '\\' || !*p)
                        {
                            if (pStartComponent && bIsComponentAllUpper)
                            {
                                char c = *p;
                                *p = 0;
                                strlwr(pStartComponent);
                                *p = c;
                            }
                            if (!*p)
                                break;
                            pStartComponent = p+1;
                            bIsComponentAllUpper = TRUE;
                        }
                        bIsComponentAllUpper = (bIsComponentAllUpper && (!isalpha(*p) || isupper(*p)));
                        p++;
                    }
                    Name.ReleaseBuffer(nLength);
                }
            }
            else
            {
                CriticalMsg("NULL named filespec");
                Name = pcszNullName;
            }
            break;

        case kSpecKindSpecial:
        case kSpecKindUntitled:
            AssertMsg(GetName() != 0, "NULL named filespec");
            if (GetName())
                Name = GetName();
            else
                Name = pcszNullName;
            break;

        case kSpecKindUnknown:
            Name = pcszAnonName;
            break;

        default:
            CriticalMsg("Bad file spec name detected");
            Name = pcszBadName;
    }
}


//
// Get a condensed name string
//
void cFileSpec::GetNameString(cStr &Str, int iDesiredLength) const
{
    GetNameString(Str);
    if (Kind == kSpecKindFile && !Str.IsEmpty() && iDesiredLength > 0)
        GetCondensePathStr(Str, iDesiredLength);
}


//
// Get the name string relative to the specified path
//
void cFileSpec::GetNameString(cStr &Str, const cFilePath & fpRelativeTo) const
{
    // Simple case
    if (fpRelativeTo.IsEmpty())
        Str = GetName();
    else
    {
        // General case
        cFilePath fp;
        GetFilePath(fp);

        if (!fpRelativeTo.ComputeAnchoredPath(fp, Str))
            GetRelativePath(Str);
        else
        {
            cStr StrName;
            GetFileName(StrName);
            Str += StrName;
        }
    }

    if (GetCaseConventions() & kNameCaseIgnored)
        Str.MakeLower();
}


//
// Return TRUE if the file is read-only
//
BOOL cFileSpec::IsReadOnly() const
{
    if (Kind != kSpecKindUnknown)
        return IsAttributeSet(_A_RDONLY);
    return FALSE;
}

//
// Return TRUE if the file is write-only
//
BOOL cFileSpec::IsWriteOnly() const
{
    return FALSE;
}

//
// Return TRUE if the file is modified
//
BOOL cFileSpec::IsModified() const
{
    if (Kind == kSpecKindUnknown)
    {
        DebugMsg("IsModified() on Unknown");
        return TRUE;
    }
    return IsAttributeSet(_A_ARCH);
}

//
// Return TRUE if the file is hiddent
//
BOOL cFileSpec::IsHidden() const
{
    if (Kind == kSpecKindUnknown)
    {
        DebugMsg("IsHidden() on Unknown");
        return FALSE; // @TBD Are Unknown files Hidden or not?
    }
    return IsAttributeSet(_A_HIDDEN);
}

//
// Return TRUE if the file is a system file
//
BOOL cFileSpec::IsSystem() const
{
    if (Kind == kSpecKindUnknown)
    {
        DebugMsg("IsSystem() on Unknown");
        return FALSE; // @TBD Are Unknown files System or not?
    }
    return IsAttributeSet(_A_SYSTEM);
}

//
// Return TRUE if the attribute is set
//
BOOL cFileSpec::IsAttributeSet(unsigned MatchAttr) const
{
    DebugMsgTrue(Kind==kSpecKindUnknown,"IsAttributeSet() of Unknown");
#ifndef _MSC_VER
    unsigned FoundAttr;
    if (!_dos_getfileattr(GetName(), &FoundAttr))
        return !!(FoundAttr & MatchAttr);
#else
	// The _dos_getfileattr is not available in VC++, so we've got to use
	// _findfirst. - KLC, 8/30/96
    struct _finddata_t fileData;
    if (_findfirst((char *)GetName(), &fileData) != -1L)
		return !!(fileData.attrib & MatchAttr);
#endif

    return errno != ENOENT;
}


//
// Return TRUE if name matches
//
BOOL cFileSpec::NameMatches(const char *pPattern)
{
    if (Kind != kSpecKindFile || !GetName() || !*GetName())
        return FALSE;

    return PatternMatch(GetFileName(), pPattern);
}


//
// Do a pattern match
//
static BOOL __stdcall PatternMatch(const char *pString, const char *pPattern)
{
    BEGIN_DEBUG_MSG_EX2(PATTERN, "PatternMatch(\"%s\", \"%s\")", pString, pPattern);
    for (;;)
    {
        // @TBD: The pattern matching stuff has to be restructured to efficently use
        //       AnsiLower'd versions of both pString & pPattern, or the RTL tolower()
        //       has to be verified for non-English filenames
        int scc = tolower(*pString);
        int lc = 077777;
        int c;
        switch (c = tolower(*pPattern))
        {
            case '[':
            {
                int k = 0;
                register cc;

                while ((cc = *++pPattern) != 0)
                {
                    cc = tolower(cc); // tolower() is a macro that doesn't support operator++ usage
                    switch (cc)
                    {
                        case ']':
                            if (k)
                            {
                                ++pString;
                                ++pPattern;
                                continue;
                            };
                            DebugMsgEx(PATTERN, "    PM: return FALSE. (!k on ])");
                            return FALSE;

                        case '-':
                            k |= lc <= scc & scc <= (cc = pPattern[1]);

                        default:
                            if (scc == (lc = cc))
                                k++;
                    }
                }

                DebugMsgEx(PATTERN, "    PM: return FALSE. (on [)");
                return FALSE;
            }

            case '*':
                return StarMatch(pString, ++pPattern);

            case '.':
                // If we are at the end of the string...
                if (!scc)
                {
                    // ...then we have a match
                    DebugMsgEx(PATTERN, "    PM: return TRUE.");
                    return TRUE;
                }

                // If we don't have a dot...
                if (scc != '.')
                {
                    // ...then we don't have a match
                    DebugMsgEx(PATTERN, "    PM: return FALSE. (scc != '.')");
                    return FALSE;
                }

                // Check the rest of the string
                ++pString;
                ++pPattern;
                continue;

            case 0:
                DebugMsgEx1(PATTERN, "    PM: return %s", (scc) ? "FALSE" : "TRUE");
                return !scc;

            case '?':
                c = scc;

            default:
                if (c == scc && scc)
                {
                    ++pString;
                    ++pPattern;
                    continue;
                }
        }

        break;
    }
    DebugMsgEx(PATTERN, "    PM: return FALSE.");
    return FALSE;
    END_DEBUG;
}


//
// Match a star operator
//
static BOOL __stdcall StarMatch(const char *pString, const char *pPattern)
{
    BEGIN_DEBUG_MSG_EX2(PATTERN, "StarMatch(\"%s\", \"%s\")", pString, pPattern);
    if (*pPattern == 0)
    {
        DebugMsgEx(PATTERN, "    SM: return TRUE.");
        return TRUE;
    }

    while (*pString)
    {
        if (PatternMatch(pString, pPattern))
        {
            DebugMsgEx(PATTERN, "    SM: return TRUE (from PM).");
            return TRUE;
        }

        // If we are at the extension separator
        if (*pString == '.')
            break; // ...then we don't have a match

        pString++;
    }
    DebugMsgEx(PATTERN, "    SM: return FALSE.");

    return FALSE;
    END_DEBUG;
}


//
// Return TRUE if this is a valid file specification
//
BOOL cFileSpec::IsValidFileSpec(const char *psz)
{
    return ::IsValidFileSpec(psz);
}


//
// Return TRUE if this is a valid file specification
//
BOOL cFileSpec::IsValidFileSpec() const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    return ::IsValidFileSpec(pName);
}


//
// Empty the file specification
//
void cFileSpec::Empty()
{
    free(pName);
    pName = 0;
    Kind = kSpecKindUnknown;
}


//
// What's the name of the root?
//
BOOL cFileSpec::GetRootDir(cStr &Str) const
{
    return cFilePath::GetRootDir(GetName(), Str);
}


//
// What's the volume called
//
BOOL cFileSpec::GetVolumeName(cStr &Str) const
{
    return cFilePath::GetVolumeName(GetName(), Str);
}


//
// What kind of file system does it use (FAT, HPFS, Mac, NFS, NTFS, etc.)
//
BOOL cFileSpec::GetFileSystemName(cStr &Str) const
{
    return cFilePath::GetFileSystemName(GetName(), Str);
}


//
// Get the serial number
//
ulong cFileSpec::GetVolumeSerialNumber() const
{
    return cFilePath::GetVolumeSerialNumber(GetName());
}


//
// Get the volume case relevance
//
int cFileSpec::GetCaseConventions() const
{
    return cFilePath::GetCaseConventions(GetName());
}


//
// Get the maximum length of a component name
//
ulong cFileSpec::GetMaxLegalComponentLen() const
{
    return cFilePath::GetMaxLegalComponentLen(GetName());
}


//
// Character okay for volume?
//
BOOL cFileSpec::IsValidChar(char c) const
{
    return cFilePath::IsValidChar(GetName(), c);
}


//
// Component name okay for volume?
//
BOOL cFileSpec::IsValidComponentName(const char * psz) const
{
    return cFilePath::IsValidComponentName(GetName(), psz);
}


//
// Make this a full path relative to the default anchor path
//
BOOL cFileSpec::MakeUnanchoredPath()
{
    return MakeFullPath(AnchorPath);
}


//
// Make this a full path, relative to specified file path.  If the specified
// path is a full path, then make this path a full path relative to the current
// directory.  If this path is a full path, just return.
//
BOOL cFileSpec::MakeFullPath(const cFilePath &fp)
{
    if (Kind != kSpecKindFile)
        return TRUE;

    if (!fp.IsEmpty() && IsRelativePath() && *GetName())
    {
        cFilePath fpFullPath(fp);
        cFilePath fpRelative(*this);
        if (fpFullPath.AddRelativePath(fpRelative))
            SetFilePath(fpFullPath);
    }

    return MakeFullPath();
}


//
// Return a full path, relative to specified file path.  If the specified path
// is a full path, then return a full path relative to the current directory.
// If this path is a full path, just return this path.
//
BOOL cFileSpec::GetFullPath(cStr &Str, const cFilePath &fp) const
{
    AssertMsg(Kind == kSpecKindFile, NotFile);

    if (IsRelativePath())
    {
        cFileSpec fsReturn(*this);
        cFilePath fpFullPath(fp);
        cFilePath fpRelative(*this);
        if (fpFullPath.AddRelativePath(fpRelative))
        {
            fsReturn.SetFilePath(fpFullPath);
            return fsReturn.GetFullPath(Str);
        }
    }

    return GetFullPath(Str);
}


//
// Return a path was is relative to the default anchor path
//
BOOL cFileSpec::GetAnchoredPath(cStr &Str) const
{
    return GetAnchoredPath(Str, AnchorPath);
}


//
// Set the path to be relative to the default anchor path
//
BOOL cFileSpec::MakeAnchoredPath()
{
    return MakeAnchoredPath(AnchorPath);
}


//
// Return a path was is relative to the specified anchor path
//
BOOL cFileSpec::GetAnchoredPath(cStr &Str, const cFilePath &path) const
{
    if (Kind == kSpecKindFile && *GetName() && !path.IsEmpty())
    {
        cFilePath fp;
        GetFilePath(fp);

        if (path.ComputeAnchoredPath(fp, Str))
        {
            cStr StrName;
            GetFileName(StrName);
            Str += StrName;
        }
        else
            Str = GetName();

        return TRUE;
    }

    return GetFullPath(Str);
}


//
// Set the path to be relative to the specified anchor
//
BOOL cFileSpec::MakeAnchoredPath(const cFilePath &path)
{
    if (Kind == kSpecKindFile && *GetName() && !path.IsEmpty())
    {
        cStr s;
        if (GetAnchoredPath(s, path))
        {
            free(pName);
            pName = s.Detach();
            return TRUE;
        }
    }
    return MakeFullPath();
}

