///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lgassert.cpp $
// $Author: TOML $
// $Date: 1997/04/09 14:10:54 $
// $Revision: 1.14 $
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <lg.h>
#include <lgassert.h>
#include <mprintf.h>
#include <coremutx.h>
#include <platform_services.h>

///////////////////////////////////////////////////////////////////////////////

#if defined (__WATCOMC__)
    #define snprintf    _bprintf
    #define vsnprintf   _vbprintf
#elif (__SC__)
    #define snprintf    _snprintf
    #define vsnprintf   _vsnprintf
#elif (_MSC_VER)
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
#endif

///////////////////////////////////////////////////////////////////////////////

static tCritMsgNotificationHandler pfnNotificationHandler;

tCritMsgNotificationHandler LGAPI CritMsgSetHandler(tCritMsgNotificationHandler pfnNewHandler)	// added LGAPI - phs, 7/1/96
{
    tCritMsgNotificationHandler pfnOldHandler = pfnNotificationHandler;
    pfnNotificationHandler = pfnNewHandler;
    return pfnOldHandler;
}

///////////////////////////////////////////////////////////////////////////////
//
// Utilities to generate an error code
//
#if 0
static unsigned long EncodeFilename(const char * file)
{
    if (!file || !*file)
        return -1;

    // Format is the last 4 chars of filename as number 1-25

    unsigned long ulEncodedFilename = 0L;
    const char * p = file;
    while (*p && *p != '.')
        p++;

    unsigned long c;
    const char * pLimit = p;
    p--;

    for (int i = 0; i < 4 && p >= file; i++)
    {
        if (isalpha(*p))
        {
            c = tolower(*p);
            c -= (unsigned long)('a') - 1;
            c <<= (i * 8);
            ulEncodedFilename |= c;
        }
        p--;
    }

    return ulEncodedFilename;
}

static void DecodeFilename(unsigned long ulEncodedFilename, char * pszTo)
{
    unsigned long c;
    for (int i = 0; i < 4; i++)
    {
        c = ulEncodedFilename;
        c <<= (i * 8);
        c >>= 24;
        if (c)
            *pszTo = char(c + ('a'-1));
        else
            *pszTo = '?';
        pszTo++;
    }
    *pszTo = '\0';
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Override the default library _assert routine
//

#ifndef _MSC_VER
#ifndef __WATCOMC__
extern "C" void __cdecl _assert(void * cond, void * file, unsigned line)
#else
extern "C" void __assert(int, char * cond, char * file, int line)
#endif
{
    _CriticalMsg((char *) cond, (char *) file, line);
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Our own debug routine that permits continuation
//

BOOL g_fQuietAssert = FALSE;

void LGAPI _CriticalMsg(const char * cond, const char * file, unsigned uLine)
{
#ifdef SHIP
    return;
#else
    const char DEBUG_STRING_PLACEMENT pcszError[] = "An invalid state was detected. Please report "
                                                    "the file and line.";

    const char DEBUG_STRING_PLACEMENT pcszUnknown[] = "Unknown";

    volatile static BOOL fInAssert = FALSE;

    const char * pszMessage = cond ? cond : pcszError;
    const char * pszFile = file ? file : pcszUnknown;

    // Trim out any path from the file name
    const char * pszTrimmedFile = pszFile;
    if ((pszTrimmedFile = strrchr(pszFile, '\\')) != 0)
        pszTrimmedFile++;
    else
        pszTrimmedFile = pszFile;

    // Check for reentrancy (often seen on activation or paint assertions)
    if (fInAssert)
    {
    	return; // hope for the best!
    }
    fInAssert = TRUE;

    if (pfnNotificationHandler)
        (*pfnNotificationHandler)(kCritMsgEnter);

    // Display the message
    char szScratch[1024];

    // Always write to error log
    const char * pszLogCriticalMsg = "Assertion Failure: %s";
    snprintf(szScratch, sizeof(szScratch) - 1,  pszLogCriticalMsg, pszMessage);
    _LogWriteMsgFileLine(szScratch, pszTrimmedFile, uLine);

    char szDiagnostic[1280];
    snprintf(szDiagnostic, sizeof(szDiagnostic) - 1,
             "%s (File: %s, Line: %u)", pszMessage, pszTrimmedFile, uLine);
    szDiagnostic[sizeof(szDiagnostic) - 1] = '\0';
    PlatformDiagnosticsLog("ERROR", "assertion", szDiagnostic);
    PlatformDiagnosticsLogStack("ERROR", "assertion", 1);

    mprintf("[%s@%d] %s\n", pszTrimmedFile, uLine, pszMessage);

    // Automated architecture smoke tests must be able to record assertions
    // without stopping on a modal dialog. Normal interactive behavior is
    // unchanged unless this explicit environment override is present.
    const char * pszAssertAction = getenv("DARK_ASSERT_ACTION");
    const BOOL fIgnoreAssert = pszAssertAction &&
        stricmp(pszAssertAction, "ignore") == 0;
    if (!g_fQuietAssert && !fIgnoreAssert)
    {
        const char * pszMessageBoxCriticalMsg = "%s  (File: %s, Line: %d)\n(Yes to trap, No to exit, Cancel to ignore)";
        const char * pszMessageBoxCriticalMsgNoFileLine = "%s\n(Yes to trap, No to exit, Cancel to ignore)";
        if (file && *file)
            snprintf(szScratch, sizeof(szScratch) - 1,  pszMessageBoxCriticalMsg, pszMessage, pszTrimmedFile, uLine);
        else
            snprintf(szScratch, sizeof(szScratch) - 1,  pszMessageBoxCriticalMsgNoFileLine, pszMessage);

        ePlatformAssertAction action = PlatformShowAssert(szScratch);
        if (action == kPlatformAssertDebug)
        {
            _LogWriteMsgFileLine("    Going to debugger...", pszFile, uLine);
            if (pfnNotificationHandler)
                (*pfnNotificationHandler)(kCritMsgDebugging);
            PlatformDebugBreak();

        }
        else if (action == kPlatformAssertExit)
        {
            _LogWriteMsgFileLine("    Exiting process...", pszFile, uLine);
            if (pfnNotificationHandler)
                (*pfnNotificationHandler)(kCritMsgTerminating);
            PlatformExitProcess(1);
        }
        else
        {
           _LogWriteMsgFileLine("    Continuing...", pszFile, uLine);
            if (pfnNotificationHandler)
                (*pfnNotificationHandler)(kCritMsgIgnoring);
            g_fQuietAssert = TRUE; // ho-hum
        }
    }
    else if (fIgnoreAssert)
    {
        _LogWriteMsgFileLine("    Continuing (DARK_ASSERT_ACTION=ignore)...", pszFile, uLine);
    }

    if (pfnNotificationHandler)
        (*pfnNotificationHandler)(kCritMsgExit);
    fInAssert = FALSE;
#endif
}

///////////////////////////////////////////////////////////////////////////////
