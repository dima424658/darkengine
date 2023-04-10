///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lgassert.cpp $
// $Author: TOML $
// $Date: 1997/04/09 14:10:54 $
// $Revision: 1.14 $
//

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <lg.h>
#include <lgassert.h>
#include <mprintf.h>
#include <coremutx.h>

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
#else
#error ("Need size-limited printf functions");
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

#if 0
extern "C"
#if !defined(OLD_STUFF) && defined(_WIN32)
int AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine)
#else
void AFXAPI AfxAssertFailedLine(LPCSTR lpszFileName, int nLine)
#endif
{
    _CriticalStr("MFC Error", lpszFileName, nLine);
    #if !defined(OLD_STUFF) && defined(_WIN32)
    return FALSE;
    #endif
}
#endif

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
#if 1
    char szScratch[1024];

    // Always write to error log
    const char * pszLogCriticalMsg = "Assertion Failure: %s";
    snprintf(szScratch, sizeof(szScratch) - 1,  pszLogCriticalMsg, pszMessage);
    _LogWriteMsgFileLine(szScratch, pszTrimmedFile, uLine);

    mprintf("[%s@%d] %s\n", pszTrimmedFile, uLine, pszMessage);

#ifdef _WIN32
    if (!g_fQuietAssert)
    {
        const char * pszMessageBoxCriticalMsg = "%s  (File: %s, Line: %d)\n(Yes to trap, No to exit, Cancel to ignore)";
        const char * pszMessageBoxCriticalMsgNoFileLine = "%s\n(Yes to trap, No to exit, Cancel to ignore)";
        if (file && *file)
            snprintf(szScratch, sizeof(szScratch) - 1,  pszMessageBoxCriticalMsg, pszMessage, pszTrimmedFile, uLine);
        else
            snprintf(szScratch, sizeof(szScratch) - 1,  pszMessageBoxCriticalMsgNoFileLine, pszMessage);

        int previousThreadPriority = GetThreadPriority(GetCurrentThread());
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        int nCode = ::MessageBox(NULL, szScratch, "Assertion Failed", MB_SYSTEMMODAL | MB_ICONHAND | MB_YESNOCANCEL);
        SetThreadPriority(GetCurrentThread(), previousThreadPriority);


        if (nCode == IDYES)
        {
            _LogWriteMsgFileLine("    Going to debugger...", pszFile, uLine);
            if (pfnNotificationHandler)
                (*pfnNotificationHandler)(kCritMsgDebugging);
            DebugBreak(); // bang!

        }
        else if (nCode == IDNO)
        {
            _LogWriteMsgFileLine("    Exiting process...", pszFile, uLine);
            if (pfnNotificationHandler)
                (*pfnNotificationHandler)(kCritMsgTerminating);
            ExitProcess(1); // swoosh
        }
        else
        {
           _LogWriteMsgFileLine("    Continuing...", pszFile, uLine);
            if (pfnNotificationHandler)
                (*pfnNotificationHandler)(kCritMsgIgnoring);
            g_fQuietAssert = TRUE; // ho-hum
        }
    }

#else
    const char * pszCriticalMsg = "Assertion Failure: %s  (File: %s, Line: %d)\n";
    snprintf(szScratch, sizeof(szScratch) - 1,  pszCriticalMsg, pszMessage, pszTrimmedFile, uLine);
    //mprintf(szScratch);
    printf(szScratch);
    abort();
#endif


#else
    // @Note (toml 04-30-96): This code depends on MFC, so it's been disabled for now.
    // Always write to error log
    _LogWriteMsgFileLine("Assertion Failure!", pszFile, uLine);
    _LogWriteMsgFileLine(pszMessage, pszFile, uLine);

    #ifndef QUIET_ASSERT
    const char szWhere[] = "%s  File: %s line: %d";

    cCriticalErrorDlg CritErrDlg;

    char linebuf[20];
    snprintf(linebuf, sizeof(linebuf) - 1, "%u", line);

    CritErrDlg.pMessage = pszMessage;
    CritErrDlg.pFile = pszFile;
    CritErrDlg.pLine = linebuf;

    CritErrDlg.DoModal();
    cCriticalErrorDlg::eCriticalAction action = CritErrDlg.nAction;

    if (CritErrDlg.fShowStack)
    {
        char buf[512];
        snprintf(buf, sizeof(buf) - 1, szWhere, pszMessage, pszFile, line);
        ShowStackTrace(buf);
    }

    switch (action)
    {
        case cCriticalErrorDlg::kContinue:
        break;

        case cCriticalErrorDlg::kAbort:
            #if defined(_WIN32)
            ExitProcess(1);
            #else
            TerminateApp(NULL, NO_UAE_BOX);
            #endif
        break;

        case cCriticalErrorDlg::kTrace:
            char *pNull = 0;
            *pNull = 0; // bang!
            DebugBreak(); // bang again!
        break;
    }
    #else
    static BOOL bFirstError = TRUE;

    if (bFirstError)
    {
        bFirstError = FALSE;
        unsigned long ulEncodedFilename = EncodeFilename(file);
        const char DEBUG_STRING_PLACEMENT pcszInternalError[] = "Internal error: %lx:%x\nExiting is recommended.";
        char buf[128];
        snprintf(buf, sizeof(buf) - 1, pcszInternalError, ulEncodedFilename, line);
        MessageBeep(MB_ICONHAND);
        ::MessageBox(NULL, buf, NULL, MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    }
    #endif
#endif
#ifdef _WIN32
    if (pfnNotificationHandler)
        (*pfnNotificationHandler)(kCritMsgExit);
    fInAssert = FALSE;
#endif
#endif
}

///////////////////////////////////////////////////////////////////////////////
