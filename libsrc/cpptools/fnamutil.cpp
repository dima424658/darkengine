///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/fnamutil.cpp $
// $Author: TOML $
// $Date: 1997/10/15 12:10:20 $
// $Revision: 1.7 $
//
// (c) Copyright 1995-1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//

#if defined(_WIN32)
#include <windows.h>
#else
#define max(x, y) ( ((x) > (y)) ? (x) : (y) )
#define min(x, y) ( ((x) < (y)) ? (x) : (y) )
#endif

#include <lg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <dos.h>

#include <str.h>
#include <fnamutil.h>

#ifndef NO_DB_MEM
// Must be last header
#include <memall.h>
#include <dbmem.h>
#endif

#ifndef DEBUG_REDUCE
#define DEBUG_REDUCE 0
#endif

#ifndef DEBUG_GETFULLPATH
#define DEBUG_GETFULLPATH 0
#endif

#ifdef __WATCOMC__

char * _getdcwd(int iDrive, char * pszDest, int nBufSize)
    {
    char * pszResult;
    int iPrevDrive = _getdrive();;
    unsigned ignored;

    if (iDrive != iPrevDrive)
        {
        _dos_setdrive(iDrive, &ignored);
        if (_getdrive() != iDrive)
            return NULL;
        }

    pszResult = getcwd(pszDest, nBufSize);

    if (iDrive != iPrevDrive)
        _dos_setdrive(iPrevDrive, &ignored);

    // If we've allocated a buffer...
    if (!pszDest)
        {
        // Make sure it's as big as requested
        // @Note (toml 09-06-96): Watcom does getcwd() a little different then other compilers.
        // The allocated size is exact, rather than being at least as big as nBufSize
        //
        // See MSVC doc:
        //
        // The _getdcwd function gets the full path of the current
        // working directory on the specified drive and stores it at
        // buffer. An error occurs if the length of the path (including
        // the terminating null character) exceeds maxlen. The drive
        // argument specifies the drive (0 = default drive, 1 = A, 2 = B,
        // and so on). The buffer argument can be NULL; a buffer of at
        // least size maxlen (more only if necessary) will automatically
        // be allocated, using malloc, to store the path. This buffer can
        // later be freed by calling free and passing it the _getdcwd
        // return value (a pointer to the allocated buffer).
        //

        int allocSizeResult = strlen(pszResult) + 1;
        if (allocSizeResult < nBufSize)
            {
            pszResult = (char *) realloc(pszResult, nBufSize);
            }
        }

    return (pszResult);
    }

#endif

//
// Local utility functions
//
inline void NormalizePathStr(cStr & str)
    {
    if (str.GetLength())
        {
        if (!_IsSlash(str[str.GetLength()-1]))
            str += '\\';
        }
    else
        str.Append(2, ".\\");
    }

//
// Validity tests
//
BOOL __stdcall IsValidFileSpec(const char * pszFileSpec)
    {
    if (pszFileSpec && *pszFileSpec)
        {
        cStr Str = pszFileSpec;
        if (ReduceDots(Str.GetBuffer(Str.GetLength())))
            {
#if defined(_WINDOWS)
#if __NT__
            // @TBD: For NT ReduceDots will handle most validation issues.
            if (Str.GetLength() > MAXPATH)
                {
                DebugMsg("Too long");
                return FALSE;
                }
            return TRUE;
#else
            // For windows this is better
            OFSTRUCT of;
            if (OpenFile(Str, &of, OF_PARSE) != HFILE_ERROR)
                {
                DebugMsg1("%s is a valid file spec.", pszFileSpec);
            	return TRUE;
            	}
#endif
#endif
        	}
    	}
    DebugMsg1("%s is not a valid file spec.", pszFileSpec);
    return FALSE;
    }

BOOL __stdcall IsValidFilePath(const char * pszFilePath)
    {
    if (pszFilePath && *pszFilePath)
        {
        cStr Str = pszFilePath;
        NormalizePathStr(Str);
        if (ReduceDots(Str.GetBuffer(Str.GetLength())))
            {
#if defined(_WINDOWS)
#if __NT__
            // @TBD: For NT ReduceDots will handle most validation issues.
            if (Str.GetLength() > MAXPATH)
                {
                DebugMsg("Too long");
                return FALSE;
                }
            return TRUE;
#else
            OFSTRUCT of;
            Str += 'x'; // make a filename of it
            if (OpenFile(Str, &of, OF_PARSE) != HFILE_ERROR)
                {
                DebugMsg1("%s is a valid file path.", pszFilePath);
            	return TRUE;
            	}
#endif
#endif
        	}
    	}
    DebugMsg1("%s is not a valid file path.", pszFilePath);
    return FALSE;
    }

//
// Get a full path, return FALSE if invalid
//

BOOL __stdcall GetFullPath(const char * pszPath, char ** ppszOut)
    {
    DebugMsgEx1(GETFULLPATH, "Get full path from %s", pszPath);

    BOOL fLegal = TRUE;
    cStr ResultStr;

    if (pszPath && *pszPath)
        {
        // If it's a full path, use it...
        if (IsFullPath(pszPath))
            {
            DebugMsgEx(GETFULLPATH, "Path is already full");
            ResultStr = pszPath;
            }

        // Perhaps it's a root path...
        else if (*pszPath == '\\')
            {
            DebugMsgEx(GETFULLPATH, "Path is off root of current drive");
            const int nLenPath = strlen(pszPath);
            char * pszResult = ResultStr.GetBuffer(nLenPath + 2);

            *pszResult = char(_getdrive() + 'A' - 1);
            *++pszResult = ':';
            pszResult++;
            memcpy(pszResult, pszPath, nLenPath);
            // pszResult[nLenPath] = 0; done by cStr::ReleaseBuffer()
            ResultStr.ReleaseBuffer(nLenPath + 2);
            }

        // Otherwise try to get full path...
        else
            {
            // Determine drive
            int iDrive;

            // If it's a drive:relative path
            if (*(pszPath + 1) == ':')
                {
                DebugMsgEx(GETFULLPATH, "Path is drive relative");
                iDrive = toupper(*pszPath) - 'A' + 1;
                fLegal = isalpha(*pszPath);
                pszPath +=2; // skip "d:"
                }

            // It must be relative...
            else
                {
                AssertMsg(IsRelativePath(pszPath), "Bad code path");
                DebugMsg("Path is relative");
                iDrive = _getdrive();
                }

            // Get cwd for target drive
            const int nLenPath = strlen(pszPath);
            const int nMinLenNewAlloc = min(_MAX_PATH + 1, nLenPath + 40); // 40 is arbitrary, but should make this fast for most cases

            DebugMsgEx3(GETFULLPATH, "Getting cwd for drive %d (%d, %d)", iDrive, int(nLenPath), int(nMinLenNewAlloc));
            char * pszResult = _getdcwd(iDrive, NULL, nMinLenNewAlloc);

            if (pszResult)
                {
                const int nLenCWD = strlen(pszResult);
                const int nLenNewAlloc = max(nLenCWD+1, nMinLenNewAlloc);
                DebugMsgEx3(GETFULLPATH, "_getdcwd() returned %s, Length is %d, new alloc length is %d", pszResult, nLenCWD, nLenNewAlloc);

                ResultStr.Attach(pszResult, nLenCWD, nLenNewAlloc);

                DebugMsgEx1(GETFULLPATH, "Normalizing path (%s)...", ResultStr.operator const char *());
                NormalizePathStr(ResultStr);

                DebugMsgEx3(GETFULLPATH, "(%s).Append(%d, %s)...", ResultStr.operator const char *(), nLenPath, pszPath);
                ResultStr.Append(nLenPath, pszPath);
                }
            else
                {
                // Drive is illegal
                ResultStr = pszPath;
                fLegal = FALSE;
                }
            }
        DebugMsgEx1(GETFULLPATH, "Reducing dots on %s", (const char *) ResultStr);
        fLegal = (fLegal && ReduceDots(ResultStr.GetBuffer(ResultStr.GetLength())));
        // Don't need to ReleaseBuffer() because we're going to Detach()
        }

    DebugMsgTrueEx(GETFULLPATH, !fLegal, "....illegal path detected");
    *ppszOut = ResultStr.Detach();
    return fLegal;
    }

//
// Combine a path and a file spec into a new, reduced path
//
BOOL __stdcall AddFileSpec(const char * pszPath, const char * pszFileSpec, char ** ppszOut)
    {
    if (IsFullPath(pszFileSpec))
        {
        *ppszOut = strdup(pszFileSpec);
        return ReduceDots(*ppszOut);
        }

    const unsigned nLenPath = strlen(pszPath);
    const unsigned nLenFileSpec = strlen(pszFileSpec);
    const unsigned nNewLen = nLenPath + nLenFileSpec;

    // If both strings empty...
    if (!nNewLen)
        {
        *ppszOut = strdup("");
        return FALSE;
        }

    *ppszOut = (char *) malloc(nNewLen + 2); // Making room for extra '\\' if needed (plus 0)

    char * p = *ppszOut;

    if (nLenPath)
        {
        memcpy(p, pszPath, nLenPath);
        p += nLenPath;
        if (!_IsSlash(pszPath[nLenPath-1]))
            *p++ = '\\';
        }

    if (nLenFileSpec)
        {
        memcpy(p, pszFileSpec, nLenFileSpec);
        p += nLenFileSpec;
        }

    *p = 0;

    return ReduceDots(*ppszOut);
    }

//////////////////////////////////////
//
// ReduceDots()
// In-place lexical dot reduction
//
BOOL __stdcall ReduceDots(char *psz)
    {
    BEGIN_DEBUG_MSG_EX1(REDUCE, "cFilePath::ReduceDots(%s)", psz)

    // Don't remove path parts beyond this point
    char * pBackStop = psz;

    BOOL fHasIllegalChar = FALSE;
    BOOL fFailedToReduce = FALSE;

    { // Scope
    const char cFirstChar = *psz;
    if (cFirstChar)
        {
        const cSecondChar = *(psz+1);
        // Set back stop to slash after drive...
        if (cSecondChar == ':')
            {
            fHasIllegalChar = !isalpha(cFirstChar);
            pBackStop = psz + 2;
            }

        // Or set back stop to slash after domain
        else if (cFirstChar == cSecondChar && _IsSlash(cFirstChar))
            {
            pBackStop = psz + 2;
            while (*pBackStop && !_IsSlash(*pBackStop))
                pBackStop++;
            }
        }
    }

    if (!*pBackStop)
        return !fHasIllegalChar; // assume empty path, "c:", and "\\domain" all ok (@TBD: good assumptions?)


    static char Pattern[] = ".\\";
    const char * pPattern = Pattern;

    int cCopyFrom;

    char * pCopyTo = pBackStop;
    const char *pCopyFrom = pBackStop;
    char cLead1;
    char cLead2;

    while ((cCopyFrom = *pCopyFrom) != 0)
        {
        if (cCopyFrom == '/')
            cCopyFrom = '\\';

        DebugMsgEx3(REDUCE, "CopyFrom=%d(%c) CopyTo=%d", pCopyFrom-psz, cCopyFrom, pCopyTo-psz);

        if (!isalnum(cCopyFrom))
            {
            switch (cCopyFrom)
                {
                case '"':
                case '<':
                case '>':
                case '|':
                case '*':
                case '?':
                    fHasIllegalChar = TRUE;
                    break;
                }
            }

        DebugMsgEx1(REDUCE, "Pattern %c", *pPattern);

        if (cCopyFrom == *pPattern)
            {
            pPattern++;

            // If we have found our sequence...
            if (!*pPattern)
                {
                // ...then undo the copying until we find a backslash
                DebugMsgEx1(REDUCE, "....found dot ending at %d", pCopyFrom-psz);

                cLead1 = char(pCopyTo > psz+2 ? pCopyTo[-3] : 0);
                cLead2 = char(pCopyTo > psz+1 ? pCopyTo[-2] : 0);

                DebugMsgEx2(REDUCE, "cLead1 = %c, cLead2 = %c", cLead1, cLead2);

                if (cLead2 == '.')
                    {
                    // If this is a false alarm
                    if (cLead1 != 0 && cLead1 != ':' && cLead1 != '\\')
                        {
                        DebugMsgEx(REDUCE, "...false dot-dots alarm");
                        pPattern = Pattern;
                        }

                    else if (pCopyTo <= pBackStop+3)
                        {
                        pBackStop = pCopyTo;   // Set the backstop to the backslash after to dot-dots
                        pPattern = Pattern;    // Reset the pattern
                        fFailedToReduce = TRUE;
                        DebugMsgEx1(REDUCE, "....can't back up, now at %d", pCopyFrom-psz);
                        }

                    else
                        {
                        pCopyTo -= 4; // Backup before the "\..\"

                        again:
                            {
                            // If we hit another slash...
                            if (*pCopyTo == '\\')
                                {
                                pPattern = Pattern;  // Reset the pattern
                                DebugMsgEx2(REDUCE, "....backed up to %d from %d", pCopyTo-psz, pCopyFrom-psz);
                                continue;
                                }

                            // If we can't back up any further...
                            else if (pCopyTo==pBackStop)
                                {
                                pPattern = Pattern;  // Reset the pattern
                                pCopyFrom++;
                                DebugMsgEx2(REDUCE, "....backed up to backstop To: %d  From: %d", pCopyTo-psz, pCopyFrom-psz);
                                continue;
                                }

                            else
                                {
                                pCopyTo--;
                                goto again;
                                }
                            }
                        }
                    }

                else
                    {
                    // We might have the single dot case
                    pPattern = Pattern;

                    if (cLead2 == '\\' || cLead2 == 0 || cLead2 == ':')
                        {
                        pCopyFrom++;
                        pCopyTo--;
                        DebugMsgEx2(REDUCE, "Dot  From: %d  To: %d",
                                pCopyFrom-psz, pCopyTo-psz);
                        continue;
                        }

                    DebugMsgEx(REDUCE, "...false dot alarm");
                    }
                }
            }
        else
            pPattern = cCopyFrom == '.' ? Pattern+1 : Pattern;

        *pCopyTo++ = char(cCopyFrom);

        pCopyFrom++;
        }

    // If this was a path and it no longer has a trailing slash...
    if (pCopyFrom != psz && pCopyFrom[-1] == '\\' && (pCopyTo == psz || pCopyTo[-1] != '\\'))
        {
        strcpy(pCopyTo, ".\\");
        AssertMsg(pCopyTo-psz == strlen(psz)-2, "Memory overwrite");
        }
    else
        {
        *pCopyTo = 0;
        AssertMsg(pCopyTo-psz == strlen(psz), "Memory overwrite");
        }

    DebugMsgEx1(REDUCE, "pCopyTo = %d", pCopyTo-psz);

    DebugMsgEx1(REDUCE, "Result is %s", psz);

    if (fHasIllegalChar)
        {
        DebugMsgEx(REDUCE, "Failed due to illegal char");
        return FALSE;
        }

    // If we haven't reduced all dots and this is a full path...
    if (fFailedToReduce && !IsDriveRelativePath(psz))
        {
        // ...then we have an illegal path
        DebugMsgEx(REDUCE, "Failed due to reduction");
        return FALSE;
        }

    return TRUE;

    END_DEBUG
    }

//
// Compute a relative path between this path and a target
//
// This file path is the anchor
// targetDir is the goal
// relPath is the path relative to this to get to targetDir,
// or else is the full path.

BOOL __stdcall ComputeAnchoredPath(const char * pszAnchorPath, const char * pszTargetPathIn, char ** ppszOut)
    {
    BEGIN_DEBUG_MSG3("ComputeAnchoredPath(%s, %s, %p)", pszAnchorPath, pszTargetPathIn, ppszOut);

    // Try to make target absolute
    cStr NonRelativeTargetStr;
    const char * pszTargetPath = NULL;

    if (IsRelativePath(pszTargetPathIn))
        {
        if (AddFileSpec(pszAnchorPath, pszTargetPathIn, NonRelativeTargetStr.BufOut()))
            pszTargetPath = NonRelativeTargetStr;
        NonRelativeTargetStr.BufDone();
        }

    if (!pszTargetPath)
        pszTargetPath = pszTargetPathIn;

    // Get full normalized names
    cStr TargetPathStr;
    if (!GetFullPath(pszTargetPath, TargetPathStr.BufOut()))
        {
        DebugMsg("Couldn't get full path for target");
        return FALSE;
        }

    cStr AnchorPathStr;
    if (!GetFullPath(pszAnchorPath, AnchorPathStr.BufOut()))
        {
        DebugMsg("Couldn't get full path for anchor");
        return FALSE;
        }

    TargetPathStr.BufDone();
    AnchorPathStr.BufDone();

    NormalizePathStr(AnchorPathStr);
    NormalizePathStr(TargetPathStr);

    DebugMsg2("Determining relationship of %s to %s", TargetPathStr.BufIn(), AnchorPathStr.BufIn());

    const int iLength = min(AnchorPathStr.GetLength(), TargetPathStr.GetLength());

    int i = 0;
    int iNode = 0;

    // Do a case insensitive scan for first difference
    // Note that while HPFS, NTFS and VFAT preserve case, they
    // are not case sensitive
    for (; i < iLength; i++)
        {
        if (tolower(AnchorPathStr[i]) != tolower(TargetPathStr[i]))
            {
            i = iNode;
            DebugMsg("Found difference, moving back to last component");
            break;
            }
        if (AnchorPathStr[i] == '\\' || AnchorPathStr[i] == '/')
            iNode = i+1;
        }

    DebugMsg3("Uncommon portion is %s, i == %d, iNode == %d", TargetPathStr.BufIn()+i, i, iNode);

    if (i==0)
        {
        DebugMsg("Nothing in common");
        *ppszOut = TargetPathStr.Detach();
        return FALSE;
        }

    // If we didn't hit end of this, deal with up and over
    cStr ReturnStr;
    for (int j = i; j < AnchorPathStr.GetLength(); j++)
        {
        if (AnchorPathStr[j] == '\\')
            ReturnStr.Append(3, "..\\");
        }

    // Extract final result
    if (ReturnStr.IsEmpty())
        TargetPathStr.Mid(ReturnStr, i, TargetPathStr.GetLength() - i);
    else
        {
        cStr TempStr;
        TargetPathStr.Mid(TempStr, i, TargetPathStr.GetLength() - i);
        ReturnStr += TempStr;
        }

    DebugMsg1("Relative path is %s", ReturnStr.BufIn());
    *ppszOut = ReturnStr.Detach();
    return TRUE;

    END_DEBUG;
    }


//
// Reduce the length of the file name
//
char *GetCondensePathStr(const char *pszOrigPath,
        char * pszCondensedPath,
        unsigned nCondenseToLen,
        eFileCondenseCase CaseChange,
        BOOL bRequireDrive)

    {
    if (!pszOrigPath || !pszCondensedPath)
        return 0;

    if ((bRequireDrive && pszOrigPath[1] != ':') || nCondenseToLen == 0)
        strcpy(pszCondensedPath, pszOrigPath);
    else
        {
        // Set up pointers to head, tail & scan pointers
        const char * pFScan = pszOrigPath;
        const char * pEndFull = pszOrigPath;

        while (*pEndFull)
            pEndFull++;

        if (pEndFull - pszOrigPath <= nCondenseToLen)
            {
            strcpy(pszCondensedPath, pszOrigPath);
            }
        else
            {
            const char * pRScan = pEndFull - 1;

            // Set up insertion pointer
            char * pIns = pszCondensedPath;

            // Scan forward and back to '\\'

            while (*pFScan && *pFScan != '\\')
                *pIns++ = *pFScan++;

            while (pRScan != pFScan && *pRScan != '\\')
                pRScan--;

            if (pRScan == pFScan)
                strcpy(pszCondensedPath, pszOrigPath);
            else
                {
                int iPresentLen = (pFScan - pszOrigPath) +
                                  4 /* strlen("\\...") */ +
                                  (pEndFull - pRScan);
                pFScan += 4;
                if (pFScan <= pRScan)
                    {
                    memcpy(pIns, "\\...", 4);
                    pIns += 4;
                    while (iPresentLen < nCondenseToLen && pRScan != pFScan)
                        {
                        iPresentLen++;
                        pRScan--;
                        }
                    strcpy(pIns, pRScan);
                    }
                else
                    strcpy(pszCondensedPath, pszOrigPath);
                }
            }
        }

    if (CaseChange == kFileToLower)
        strlwr(pszCondensedPath);
    else if (CaseChange == kFileToUpper)
        strupr(pszCondensedPath);

    return pszCondensedPath;
    }
