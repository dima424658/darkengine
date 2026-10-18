///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lglog.cpp $
// $Author: TOML $
// $Date: 1998/02/11 09:29:11 $
// $Revision: 1.13 $
//
// To avoid reentrancy issues, this module has to be almost completely
// independent of almost everything else
//

#include <lg.h>
#include <platform_services.h>

///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <stdarg.h>
#include <stdio.h>

#include <mprintf.h>
#include <coremutx.h>

///////////////////////////////////////////////////////////////////////////////

#if defined (__WATCOMC__)
    #define snprintf    _bprintf
    #define vsnprintf   _vbprintf
#elif defined (__SC__) || defined (_MSC_VER)
    #define snprintf    _snprintf
    #define vsnprintf   _vsnprintf
#endif

#define kMaxPath            260
#define kLogScratchBufLen   1024

///////////////////////////////////////

void LGAPI _LogDefaultLogger(const char *Msg, const char *FN, unsigned LN);

///////////////////////////////////////////////////////////////////////////////

#ifdef __WATCOMC__
int __Debug_True = 1;
int __Debug_False = 0;
#endif

static FILE *hLogFile;

const int kDebugIndentTextLen = 2;
const int kMaxIndent = 250;
const int kMaxIndentSpaces = kDebugIndentTextLen * kMaxIndent;

struct sLGLog
    {
    sLGLog();

    void SetName(const char * pszNewName)
        {
        strncpy(szName, pszNewName, kMaxPath);
        szName[kMaxPath] = 0;
        }

    tLogMessageFunc pfnLogger;
    int             iIndent;
    int             fFlags;
    char            szName[kMaxPath + 1];

    void Lock()
        {
        CoreThreadLock();
        }

    void Unlock()
        {
        CoreThreadUnlock();
        }
    };

///////////////////////////////////////

static sLGLog g_Log;

sLGLog::sLGLog()
    {
    pfnLogger = _LogDefaultLogger;
    iIndent = 0;
    fFlags = 0;
    strcpy(szName, "dark.log");
    }

///////////////////////////////////////

void LGAPI LogSetOptions(int fOptions)
    {
    g_Log.fFlags = fOptions;
    }

///////////////////////////////////////

void LGAPI LogSetLogFile(const char * pszFile)
    {
    // If there is now change....
    if (pszFile && stricmp(g_Log.szName, pszFile) == 0)
        return;

    // Close the old log file as needed
    if (hLogFile)
        fclose(hLogFile);

    g_Log.szName[0] = 0;

    if (pszFile && stricmp("con:", pszFile) == 0)
        g_Log.fFlags |= kLogToMono;
    else if (pszFile)
        {
        g_Log.fFlags |= kLogToFile;
        g_Log.SetName(pszFile);
        remove(g_Log.szName);
        }
    else
        g_Log.fFlags &= ~kLogToFile;
    }

///////////////////////////////////////

int LGAPI LogGetOptions()
    {
    return g_Log.fFlags;
    }

///////////////////////////////////////

const char * LGAPI LogGetLogFile()
    {
    return g_Log.szName;
    }

///////////////////////////////////////////////////////////////////////////////

const char * _LogFmt(const char * pszFormat, ...)
    {
    g_Log.Lock();
    static char szBuf[kLogScratchBufLen];
    szBuf[0] = 0;

    va_list arg_ptr;
    va_start(arg_ptr, pszFormat);
    vsnprintf(szBuf, sizeof(szBuf)-1, pszFormat, arg_ptr);
    va_end(arg_ptr);

    g_Log.Unlock();
    return szBuf;
    }

///////////////////////////////////////////////////////////////////////////////

static char szSpaces[kMaxIndentSpaces + 1];

static const char * GetIndentString()
    {
    // If first use, generate indentation string
    if (!szSpaces[0])
        memset(szSpaces, ' ', kMaxIndentSpaces);

    if (!(g_Log.fFlags & kLogDisableIndent))
        return &szSpaces[kMaxIndentSpaces] - LogGetIndent() * kDebugIndentTextLen; // LogGetIndent is responsible for range verification (toml 04-30-96)
    else
        return &szSpaces[kMaxIndentSpaces];
    }

//
// Default debugger routine
//
void LGAPI _LogDefaultLogger(const char *pszMessage, const char *pszFile, unsigned uLine)
    {
    if (!(g_Log.fFlags & kLogToAll))
        return;

    g_Log.Lock();

    // Make all parameters safe
    if (!pszMessage)
        pszMessage = "";

    if (!pszFile)
        pszFile = "";

    // Pick an approptiate log string
    static const char * pszDefDbgMsg = "%12s(%4u): %s%s\n";
    static const char * pszTimedDbgMsg = "%12s(%4u): %s%-80s [%7u, %7u]\n";
    const char * pszDbgMsg = (g_Log.fFlags & kLogDisplayTime) ? pszTimedDbgMsg : pszDefDbgMsg;

    // Trim out any path from the file name
    const char * pszTrimmedFile = pszFile;
    if ((pszTrimmedFile = strrchr(pszFile, '\\')) != 0)
        pszTrimmedFile++;
    else
        pszTrimmedFile = pszFile;

    // Figure the timing values, if appropriate
    unsigned long ulTimeSinceStart;
    unsigned long ulTimeSinceLast;

    if (g_Log.fFlags & kLogDisplayTime)
        {
        static unsigned long ulStart = PlatformMilliseconds();
        static unsigned long ulLast = 0;

        unsigned long ulCurrent = PlatformMilliseconds();
        ulTimeSinceStart = ulCurrent - ulStart;
        ulTimeSinceLast = ulCurrent - ulLast;
        ulLast = ulCurrent;
        }

    // Make the string
    char szBuf[kLogScratchBufLen];
    szBuf[0] = 0;
    snprintf(szBuf, sizeof(szBuf)-1, pszDbgMsg, pszTrimmedFile, uLine, GetIndentString(), pszMessage, ulTimeSinceStart, ulTimeSinceLast);

    // Perform the specified outputs.  First to debugger...
    if (g_Log.fFlags & kLogToDebugger)
        PlatformDebugOutput(szBuf);

    // Then to the log file...
    if ((g_Log.fFlags & kLogToFile) && g_Log.szName[0])
        {
        if (!hLogFile)
            {
            hLogFile = fopen(g_Log.szName, "w");
            if (!hLogFile)
                {
                g_Log.szName[0] = 0;
                g_Log.fFlags &= ~kLogToFile;
                g_Log.Unlock();
                return;
                }
            setbuf(hLogFile, NULL);
            }
        fprintf(hLogFile, "%s", szBuf);
        fflush(hLogFile);
        }

    // Finally to the monochrome monitor...
    if (g_Log.fFlags & kLogToMono)
        {
        // For mono output, remove the file name and line, optionally remove indent, then truncate
        char * pszMonoOutput = strchr(szBuf, ':');

        if (pszMonoOutput && (g_Log.fFlags & kLogDisableMonoIndent))
            {
            pszMonoOutput++;
            while (*pszMonoOutput && isspace(*pszMonoOutput))
                pszMonoOutput++;
            }

        if (pszMonoOutput)
            {
            pszMonoOutput++;
            pszMonoOutput[78] = '\n';
            pszMonoOutput[79] = 0;
            mprintf(pszMonoOutput);
            }
        }

    g_Log.Unlock();
    }

///////////////////////////////////////////////////////////////////////////////

void LGAPI LogStackTrace(const char * pszTitle)
    {
    g_Log.Lock();
    // @TBD (toml 05-01-96): Dig up the stack crawl code
    LogMsg1("Stack trace unavailable (%s)", pszTitle);
    g_Log.Unlock();
    }

///////////////////////////////////////////////////////////////////////////////

void LGAPI _LogWriteMsgFileLine(const char *m, const char *f, unsigned l)
    {
    printf("%s in file %s:%d\n", m, f, l);
    if (g_Log.pfnLogger)
       {
       g_Log.Lock();
       (*g_Log.pfnLogger)(m, f, l);
       g_Log.Unlock();
       }
    }

///////////////////////////////////////

int LGAPI _LogWriteMsg(const char * m)
    {
    if (g_Log.pfnLogger)
       {
       g_Log.Lock();
       (*g_Log.pfnLogger)(m, 0, 0);
       g_Log.Unlock();
       }
    return 1; // Is this being used? (toml 04-30-96)
    }

///////////////////////////////////////

int _LogWriteFmtMsg(const char * pszFormat, ...)
    {
    int n = 0;
    if (g_Log.pfnLogger)
       {
       g_Log.Lock();
       char szBuf[kLogScratchBufLen];
       szBuf[0] = 0;

       va_list arg_ptr;
       va_start(arg_ptr, pszFormat);
       n = vsnprintf(szBuf, sizeof(szBuf)-1, pszFormat, arg_ptr);
       va_end(arg_ptr);

       (*g_Log.pfnLogger)(szBuf, 0, 0);

       g_Log.Unlock();
       }
    return n;
    }

///////////////////////////////////////////////////////////////////////////////

tLogMessageFunc LGAPI LogSetMessageFunc(tLogMessageFunc pfnNew)
    {
    tLogMessageFunc pfnOld = g_Log.pfnLogger;
    g_Log.pfnLogger = pfnNew;
    return pfnOld;
    }

///////////////////////////////////////////////////////////////////////////////

int LGAPI LogGetIndent()
    {
    return g_Log.iIndent;
    }

///////////////////////////////////////

void LGAPI LogIncIndent()
    {
    g_Log.Lock();
    g_Log.iIndent++;
    if (g_Log.iIndent > kMaxIndent)
        g_Log.iIndent = kMaxIndent;
    g_Log.Unlock();
    }

///////////////////////////////////////

void LGAPI LogDecIndent()
    {
    g_Log.Lock();
    g_Log.iIndent--;
    if (g_Log.iIndent < 0)
        g_Log.iIndent = 0;
    g_Log.Unlock();
    }

///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
//
// Diagnostic helpers
//
// These functions generate a string that represents
// given argument in the most readable form.
//

///////////////////////////////////////////////////////////////////////////////

//
// Construct a debug enclosure
//
cTraceMsgLevel::cTraceMsgLevel(const char *pszFN, long l, const char *pszMsg)
    : pszFileName(0),
      pszMessage(0),
      lLine(l)
    {
    if (lLine != -1)
        {
        pszFileName = strdup(pszFN);
        pszMessage = strdup(pszMsg);

        // If there is a tag...
        const char * pszTag = (pszMessage && *pszMessage) ? _LogFmt("%s {", pszMessage) : "{";
        ::_LogWriteMsgFileLine(pszTag, pszFileName, lLine);
        LogIncIndent();
        }
    }


//
// Destroy the debug enclosure
//
cTraceMsgLevel::~cTraceMsgLevel()
    {
    if (lLine != -1)
        {
        LogDecIndent();

        ::_LogWriteMsgFileLine("}", pszFileName, lLine);

        free(pszFileName);
        free(pszMessage);
        }
    }

///////////////////////////////////////////////////////////////////////////////

