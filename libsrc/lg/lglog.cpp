///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lglog.cpp $
// $Author: TOML $
// $Date: 1998/02/11 09:29:11 $
// $Revision: 1.13 $
//
// To avoid reentrancy issues, this module has to be almost completely
// independent of almost everything else
//

#ifdef _WIN32
#include <windows.h>
#endif
#include <lg.h>

///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <stdarg.h>
#include <stdio.h>

#include <io.h>

#include <mprintf.h>
#include <coremutx.h>

///////////////////////////////////////////////////////////////////////////////

#if defined (__WATCOMC__)
    #define snprintf    _bprintf
    #define vsnprintf   _vbprintf
#elif defined (__SC__) || defined (_MSC_VER)
    #define snprintf    _snprintf
    #define vsnprintf   _vsnprintf
#else
#error ("Need size-limited printf functions");
#endif

#define kMaxPath            260
#define kLogScratchBufLen   1024

///////////////////////////////////////

inline BOOL FileExists(const char * pszFile)
    {
    return (access(pszFile, 0) == 0);
    }

///////////////////////////////////////////////////////////////////////////////

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

static const char * pszOptionsIniFileName = "lg.ini";
static const char * pszDebugSection = "DebugOptions";

///////////////////////////////////////////////////////////////////////////////

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

#ifdef _WIN32
    void Lock()
        {
        CoreThreadLock();
        }

    void Unlock()
        {
        CoreThreadUnlock();
        }
#else
    void Lock()
        {
        }

    void Unlock()
        {
        }
#endif
    };

///////////////////////////////////////

static sLGLog g_Log;

sLGLog::sLGLog()
    {
#ifdef _WIN32
    pfnLogger = _LogDefaultLogger;
    iIndent = 0;
    fFlags = 0;

    char szDevIniFile[kMaxPath + 1];
    char szDefualtLog[kMaxPath + 1];

    ::GetModuleFileName(GetModuleHandle(0), szDefualtLog, kMaxPath);
    char * pEndPath = strrchr(szDefualtLog, '.');
    if (pEndPath)
        *pEndPath = 0;
    strcat(szDefualtLog, ".log");

    // Load options from .ini file, checking for lg.ini in CWD, then Windows directory
    strcpy(szDevIniFile, szDefualtLog);
    pEndPath = strrchr(szDevIniFile, '\\');
    if (pEndPath)
        *pEndPath = 0;
    else
        szDevIniFile[0] = 0;

    strcat(szDevIniFile, "\\");
    strcat(szDevIniFile, pszOptionsIniFileName);

    if (!FileExists(szDevIniFile))
        {
        strcpy(szDevIniFile, pszOptionsIniFileName);
        }

    fFlags = (GetPrivateProfileInt(pszDebugSection, "LogDestinations", 0, szDevIniFile) & kLogToAll);
    fFlags |= (GetPrivateProfileInt(pszDebugSection, "LogTimeStamp", FALSE, szDevIniFile)) ? kLogDisplayTime : 0;

    GetPrivateProfileString(pszDebugSection, "LogFile", szDefualtLog, szName, sizeof(szName), szDevIniFile);

#endif
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
        unlink(g_Log.szName);
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

#if defined(_WIN32)
    if (g_Log.fFlags & kLogDisplayTime)
        {
        static unsigned long ulStart = GetTickCount();
        static unsigned long ulLast = 0;

        unsigned long ulCurrent = GetTickCount();
        ulTimeSinceStart = ulCurrent - ulStart;
        ulTimeSinceLast = ulCurrent - ulLast;
        ulLast = ulCurrent;
        }
#endif

    // Make the string
    char szBuf[kLogScratchBufLen];
    szBuf[0] = 0;
    snprintf(szBuf, sizeof(szBuf)-1, pszDbgMsg, pszTrimmedFile, uLine, GetIndentString(), pszMessage, ulTimeSinceStart, ulTimeSinceLast);

#if defined(_WIN32)
    // Perform the specified outputs.  First to debugger...
    if (g_Log.fFlags & kLogToDebugger)
        OutputDebugString(szBuf);
#endif

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
        fprintf(hLogFile, szBuf);
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

#if defined(_WIN32) && defined(_INC_WINDOWS)

/////////////////////////////////////////

struct sStrULongEquivalence
    {
    const char *    pszValue;
    unsigned long   ulValue;
    };

static BOOL SearchEquivalenceTable(const sStrULongEquivalence * pStrULongEquivalence, unsigned long ulLookup, char * pszReturn)
    {
    // This is crude, but since this is for debugging, a simple linear search should suffice...
    while (pStrULongEquivalence->pszValue)
        {
        if (pStrULongEquivalence->ulValue == ulLookup)
            {
            strcpy(pszReturn, pStrULongEquivalence->pszValue);
            return TRUE;
            }
        pStrULongEquivalence++;
        }

    sprintf(pszReturn, "0x%lx", ulLookup);
    return FALSE;
    }

/////////////////////////////////////////
//
// Windows message tables
//
static sStrULongEquivalence aWindowMessages[] =
    {
        { "WM_NULL",                        0x0000 },
        { "WM_CREATE",                      0x0001 },
        { "WM_DESTROY",                     0x0002 },
        { "WM_MOVE",                        0x0003 },
        { "WM_SIZE",                        0x0005 },
        { "WM_ACTIVATE",                    0x0006 },
        { "WM_SETFOCUS",                    0x0007 },
        { "WM_KILLFOCUS",                   0x0008 },
        { "WM_ENABLE",                      0x000A },
        { "WM_SETREDRAW",                   0x000B },
        { "WM_SETTEXT",                     0x000C },
        { "WM_GETTEXT",                     0x000D },
        { "WM_GETTEXTLENGTH",               0x000E },
        { "WM_PAINT",                       0x000F },
        { "WM_CLOSE",                       0x0010 },
        { "WM_QUERYENDSESSION",             0x0011 },
        { "WM_QUIT",                        0x0012 },
        { "WM_QUERYOPEN",                   0x0013 },
        { "WM_ERASEBKGND",                  0x0014 },
        { "WM_SYSCOLORCHANGE",              0x0015 },
        { "WM_ENDSESSION",                  0x0016 },
        { "WM_SHOWWINDOW",                  0x0018 },
        { "WM_WININICHANGE/WM_SETTINGCHANGE", 0x001A },
        { "WM_DEVMODECHANGE",               0x001B },
        { "WM_ACTIVATEAPP",                 0x001C },
        { "WM_FONTCHANGE",                  0x001D },
        { "WM_TIMECHANGE",                  0x001E },
        { "WM_CANCELMODE",                  0x001F },
        { "WM_SETCURSOR",                   0x0020 },
        { "WM_MOUSEACTIVATE",               0x0021 },
        { "WM_CHILDACTIVATE",               0x0022 },
        { "WM_QUEUESYNC",                   0x0023 },
        { "WM_GETMINMAXINFO",               0x0024 },
        { "WM_PAINTICON",                   0x0026 },
        { "WM_ICONERASEBKGND",              0x0027 },
        { "WM_NEXTDLGCTL",                  0x0028 },
        { "WM_SPOOLERSTATUS",               0x002A },
        { "WM_DRAWITEM",                    0x002B },
        { "WM_MEASUREITEM",                 0x002C },
        { "WM_DELETEITEM",                  0x002D },
        { "WM_VKEYTOITEM",                  0x002E },
        { "WM_CHARTOITEM",                  0x002F },
        { "WM_SETFONT",                     0x0030 },
        { "WM_GETFONT",                     0x0031 },
        { "WM_SETHOTKEY",                   0x0032 },
        { "WM_GETHOTKEY",                   0x0033 },
        { "WM_QUERYDRAGICON",               0x0037 },
        { "WM_COMPAREITEM",                 0x0039 },
        { "WM_COMPACTING",                  0x0041 },
        { "WM_COMMNOTIFY",                  0x0044 },
        { "WM_WINDOWPOSCHANGING",           0x0046 },
        { "WM_WINDOWPOSCHANGED",            0x0047 },
        { "WM_POWER",                       0x0048 },
        { "WM_COPYDATA",                    0x004A },
        { "WM_CANCELJOURNAL",               0x004B },
#if (WINVER >= 0x0400)
        { "WM_NOTIFY",                      0x004E },
        { "WM_INPUTLANGCHANGEREQUEST",      0x0050 },
        { "WM_INPUTLANGCHANGE",             0x0051 },
        { "WM_TCARD",                       0x0052 },
        { "WM_HELP",                        0x0053 },
        { "WM_USERCHANGED",                 0x0054 },
        { "WM_NOTIFYFORMAT",                0x0055 },
        { "WM_CONTEXTMENU",                 0x007B },
        { "WM_STYLECHANGING",               0x007C },
        { "WM_STYLECHANGED",                0x007D },
        { "WM_DISPLAYCHANGE",               0x007E },
        { "WM_GETICON",                     0x007F },
        { "WM_SETICON",                     0x0080 },
#endif /* WINVER >= 0x0400 */
        { "WM_NCCREATE",                    0x0081 },
        { "WM_NCDESTROY",                   0x0082 },
        { "WM_NCCALCSIZE",                  0x0083 },
        { "WM_NCHITTEST",                   0x0084 },
        { "WM_NCPAINT",                     0x0085 },
        { "WM_NCACTIVATE",                  0x0086 },
        { "WM_GETDLGCODE",                  0x0087 },
        { "WM_NCMOUSEMOVE",                 0x00A0 },
        { "WM_NCLBUTTONDOWN",               0x00A1 },
        { "WM_NCLBUTTONUP",                 0x00A2 },
        { "WM_NCLBUTTONDBLCLK",             0x00A3 },
        { "WM_NCRBUTTONDOWN",               0x00A4 },
        { "WM_NCRBUTTONUP",                 0x00A5 },
        { "WM_NCRBUTTONDBLCLK",             0x00A6 },
        { "WM_NCMBUTTONDOWN",               0x00A7 },
        { "WM_NCMBUTTONUP",                 0x00A8 },
        { "WM_NCMBUTTONDBLCLK",             0x00A9 },
        { "WM_KEYDOWN",                     0x0100 },
        { "WM_KEYUP",                       0x0101 },
        { "WM_CHAR",                        0x0102 },
        { "WM_DEADCHAR",                    0x0103 },
        { "WM_SYSKEYDOWN",                  0x0104 },
        { "WM_SYSKEYUP",                    0x0105 },
        { "WM_SYSCHAR",                     0x0106 },
        { "WM_SYSDEADCHAR",                 0x0107 },
        { "WM_KEYLAST",                     0x0108 },
#if(WINVER >= 0x0400)
        { "WM_IME_STARTCOMPOSITION",        0x010D },
        { "WM_IME_ENDCOMPOSITION",          0x010E },
        { "WM_IME_COMPOSITION",             0x010F },
        { "WM_IME_KEYLAST",                 0x010F },
#endif /* WINVER >= 0x0400 */
        { "WM_INITDIALOG",                  0x0110 },
        { "WM_COMMAND",                     0x0111 },
        { "WM_SYSCOMMAND",                  0x0112 },
        { "WM_TIMER",                       0x0113 },
        { "WM_HSCROLL",                     0x0114 },
        { "WM_VSCROLL",                     0x0115 },
        { "WM_INITMENU",                    0x0116 },
        { "WM_INITMENUPOPUP",               0x0117 },
        { "WM_MENUSELECT",                  0x011F },
        { "WM_MENUCHAR",                    0x0120 },
        { "WM_ENTERIDLE",                   0x0121 },
        { "WM_CTLCOLORMSGBOX",              0x0132 },
        { "WM_CTLCOLOREDIT",                0x0133 },
        { "WM_CTLCOLORLISTBOX",             0x0134 },
        { "WM_CTLCOLORBTN",                 0x0135 },
        { "WM_CTLCOLORDLG",                 0x0136 },
        { "WM_CTLCOLORSCROLLBAR",           0x0137 },
        { "WM_CTLCOLORSTATIC",              0x0138 },
        { "WM_MOUSEMOVE",                   0x0200 },
        { "WM_LBUTTONDOWN",                 0x0201 },
        { "WM_LBUTTONUP",                   0x0202 },
        { "WM_LBUTTONDBLCLK",               0x0203 },
        { "WM_RBUTTONDOWN",                 0x0204 },
        { "WM_RBUTTONUP",                   0x0205 },
        { "WM_RBUTTONDBLCLK",               0x0206 },
        { "WM_MBUTTONDOWN",                 0x0207 },
        { "WM_MBUTTONUP",                   0x0208 },
        { "WM_MBUTTONDBLCLK",               0x0209 },
        { "WM_MOUSELAST",                   0x0209 },
        { "WM_PARENTNOTIFY",                0x0210 },
        { "WM_ENTERMENULOOP",               0x0211 },
        { "WM_EXITMENULOOP",                0x0212 },
#if(WINVER >= 0x0400)
        { "WM_NEXTMENU",                    0x0213 },
        { "WM_SIZING",                      0x0214 },
        { "WM_CAPTURECHANGED",              0x0215 },
        { "WM_MOVING",                      0x0216 },
        { "WM_POWERBROADCAST",              0x0218 },
        { "WM_DEVICECHANGE",                0x0219 },
        { "WM_IME_SETCONTEXT",              0x0281 },
        { "WM_IME_NOTIFY",                  0x0282 },
        { "WM_IME_CONTROL",                 0x0283 },
        { "WM_IME_COMPOSITIONFULL",         0x0284 },
        { "WM_IME_SELECT",                  0x0285 },
        { "WM_IME_CHAR",                    0x0286 },
        { "WM_IME_KEYDOWN",                 0x0290 },
        { "WM_IME_KEYUP",                   0x0291 },
#endif /* WINVER >= 0x0400 */
        { "WM_MDICREATE",                   0x0220 },
        { "WM_MDIDESTROY",                  0x0221 },
        { "WM_MDIACTIVATE",                 0x0222 },
        { "WM_MDIRESTORE",                  0x0223 },
        { "WM_MDINEXT",                     0x0224 },
        { "WM_MDIMAXIMIZE",                 0x0225 },
        { "WM_MDITILE",                     0x0226 },
        { "WM_MDICASCADE",                  0x0227 },
        { "WM_MDIICONARRANGE",              0x0228 },
        { "WM_MDIGETACTIVE",                0x0229 },
        { "WM_MDISETMENU",                  0x0230 },
        { "WM_ENTERSIZEMOVE",               0x0231 },
        { "WM_EXITSIZEMOVE",                0x0232 },
        { "WM_DROPFILES",                   0x0233 },
        { "WM_MDIREFRESHMENU",              0x0234 },
        { "WM_CUT",                         0x0300 },
        { "WM_COPY",                        0x0301 },
        { "WM_PASTE",                       0x0302 },
        { "WM_CLEAR",                       0x0303 },
        { "WM_UNDO",                        0x0304 },
        { "WM_RENDERFORMAT",                0x0305 },
        { "WM_RENDERALLFORMATS",            0x0306 },
        { "WM_DESTROYCLIPBOARD",            0x0307 },
        { "WM_DRAWCLIPBOARD",               0x0308 },
        { "WM_PAINTCLIPBOARD",              0x0309 },
        { "WM_VSCROLLCLIPBOARD",            0x030A },
        { "WM_SIZECLIPBOARD",               0x030B },
        { "WM_ASKCBFORMATNAME",             0x030C },
        { "WM_CHANGECBCHAIN",               0x030D },
        { "WM_HSCROLLCLIPBOARD",            0x030E },
        { "WM_QUERYNEWPALETTE",             0x030F },
        { "WM_PALETTEISCHANGING",           0x0310 },
        { "WM_PALETTECHANGED",              0x0311 },
        { "WM_HOTKEY",                      0x0312 },
#if(WINVER >= 0x0400)
        { "WM_PRINT",                       0x0317 },
        { "WM_PRINTCLIENT",                 0x0318 },
        { "WM_HANDHELDFIRST",               0x0358 },
        { "WM_HANDHELDLAST",                0x035F },
        { "WM_AFXFIRST",                    0x0360 },
        { "WM_AFXLAST",                     0x037F },
#endif /* WINVER >= 0x0400 */
        { "WM_PENWINFIRST",                 0x0380 },
        { "WM_PENWINLAST",                  0x038F },
#if(WINVER >= 0x0400)
        { "WM_APP",                         0x8000 },
#endif /* WINVER >= 0x0400 */
        { "WM_USER",                        0x0400 },
        { 0,                                0 }
    };

static const char * GetMessageName(unsigned uMessage)
    {
    static char szBuf[64];
    SearchEquivalenceTable(aWindowMessages, uMessage, szBuf);
    return szBuf;
    }

///////////////////////////////////////

static sStrULongEquivalence aVirtualKeys[] =
    {
        { "VK_LBUTTON",       0x01 },
        { "VK_RBUTTON",       0x02 },
        { "VK_CANCEL",        0x03 },
        { "VK_MBUTTON",       0x04 },
        { "VK_BACK",          0x08 },
        { "VK_TAB",           0x09 },
        { "VK_CLEAR",         0x0C },
        { "VK_RETURN",        0x0D },
        { "VK_SHIFT",         0x10 },
        { "VK_CONTROL",       0x11 },
        { "VK_MENU",          0x12 },
        { "VK_PAUSE",         0x13 },
        { "VK_CAPITAL",       0x14 },
        { "VK_ESCAPE",        0x1B },
        { "VK_SPACE",         0x20 },
        { "VK_PRIOR",         0x21 },
        { "VK_NEXT",          0x22 },
        { "VK_END",           0x23 },
        { "VK_HOME",          0x24 },
        { "VK_LEFT",          0x25 },
        { "VK_UP",            0x26 },
        { "VK_RIGHT",         0x27 },
        { "VK_DOWN",          0x28 },
        { "VK_SELECT",        0x29 },
        { "VK_PRINT",         0x2A },
        { "VK_EXECUTE",       0x2B },
        { "VK_SNAPSHOT",      0x2C },
        { "VK_INSERT",        0x2D },
        { "VK_DELETE",        0x2E },
        { "VK_HELP",          0x2F },
        /* VK_0 thru VK_9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
        /* VK_A thru VK_Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
        { "VK_LWIN",          0x5B },
        { "VK_RWIN",          0x5C },
        { "VK_APPS",          0x5D },
        { "VK_NUMPAD0",       0x60 },
        { "VK_NUMPAD1",       0x61 },
        { "VK_NUMPAD2",       0x62 },
        { "VK_NUMPAD3",       0x63 },
        { "VK_NUMPAD4",       0x64 },
        { "VK_NUMPAD5",       0x65 },
        { "VK_NUMPAD6",       0x66 },
        { "VK_NUMPAD7",       0x67 },
        { "VK_NUMPAD8",       0x68 },
        { "VK_NUMPAD9",       0x69 },
        { "VK_MULTIPLY",      0x6A },
        { "VK_ADD",           0x6B },
        { "VK_SEPARATOR",     0x6C },
        { "VK_SUBTRACT",      0x6D },
        { "VK_DECIMAL",       0x6E },
        { "VK_DIVIDE",        0x6F },
        { "VK_F1",            0x70 },
        { "VK_F2",            0x71 },
        { "VK_F3",            0x72 },
        { "VK_F4",            0x73 },
        { "VK_F5",            0x74 },
        { "VK_F6",            0x75 },
        { "VK_F7",            0x76 },
        { "VK_F8",            0x77 },
        { "VK_F9",            0x78 },
        { "VK_F10",           0x79 },
        { "VK_F11",           0x7A },
        { "VK_F12",           0x7B },
        { "VK_F13",           0x7C },
        { "VK_F14",           0x7D },
        { "VK_F15",           0x7E },
        { "VK_F16",           0x7F },
        { "VK_F17",           0x80 },
        { "VK_F18",           0x81 },
        { "VK_F19",           0x82 },
        { "VK_F20",           0x83 },
        { "VK_F21",           0x84 },
        { "VK_F22",           0x85 },
        { "VK_F23",           0x86 },
        { "VK_F24",           0x87 },
        { "VK_NUMLOCK",       0x90 },
        { "VK_SCROLL",        0x91 },
        { "VK_LSHIFT",        0xA0 },
        { "VK_RSHIFT",        0xA1 },
        { "VK_LCONTROL",      0xA2 },
        { "VK_RCONTROL",      0xA3 },
        { "VK_LMENU",         0xA4 },
        { "VK_RMENU",         0xA5 },
#if(WINVER >= 0x0400)
        { "VK_PROCESSKEY",    0xE5 },
#endif /* WINVER >= 0x0400 */
        { "VK_ATTN",          0xF6 },
        { "VK_CRSEL",         0xF7 },
        { "VK_EXSEL",         0xF8 },
        { "VK_EREOF",         0xF9 },
        { "VK_PLAY",          0xFA },
        { "VK_ZOOM",          0xFB },
        { "VK_NONAME",        0xFC },
        { "VK_PA1",           0xFD },
        { "VK_OEM_CLEAR",     0xFE },
        { 0,                  0 }
    };

static const char * InterpretWParam(unsigned uMessage, unsigned wParam)
    {
    // Do any special interpretation
    static char szBuf[64];

    if (uMessage >= WM_KEYFIRST && uMessage <= WM_KEYLAST && SearchEquivalenceTable(aVirtualKeys, wParam, szBuf))
        return szBuf;

    snprintf(szBuf, sizeof(szBuf) - 1, "0x%x", wParam);
    return szBuf;
    }

///////////////////////////////////////

#define GetPackedVal(num, high, low) (((num << (31 - high)) >> (31 - high)) >> low)

static const char * InterpretLParam(unsigned uMessage, unsigned long lParam)
    {
    static char szBuf[64];
    if (uMessage >= WM_KEYFIRST && uMessage <= WM_KEYLAST)
        {
        snprintf(szBuf, sizeof(szBuf) - 1,
                 "0x%x (tr %d, prev %d, cx %d, ext %d, scan %d, repeat %d)",
                 lParam,
                 GetPackedVal(lParam, 31, 31),
                 GetPackedVal(lParam, 30, 30),
                 GetPackedVal(lParam, 29, 29),
                 GetPackedVal(lParam, 24, 24),
                 GetPackedVal(lParam, 23, 16),
                 GetPackedVal(lParam, 15, 0));
        return szBuf;
        }

    snprintf(szBuf, sizeof(szBuf) - 1, "0x%x", lParam);
    return szBuf;
    }

///////////////////////////////////////
//
// Dump out a Windows message
//

const char * LGAPI LogStrWinMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
    static char szBuf[256];
    snprintf(szBuf, sizeof(szBuf) - 1, "0x%p, %s, %s, %s", hwnd, GetMessageName(msg), InterpretWParam(msg, wParam), InterpretLParam(msg, lParam));
    return szBuf;
    }


//
// Dump out a rectangle
//
const char * LGAPI _LogStrWinRect(const char* rectName, const RECT * r)
    {
    static char szBuf[64];
    snprintf(szBuf, sizeof(szBuf) - 1, "%s: %d,%d,%d,%d", rectName, r->left,r->top,r->right,r->bottom);
    return szBuf;
    }


//
// Dump out a point
//
const char * LGAPI _LogStrWinPt(const char* ptName, const POINT * pt)
    {
    static char szBuf[64];
    snprintf(szBuf, sizeof(szBuf) - 1, "%s: %d,%d",ptName, pt->x, pt->y);
    return szBuf;
    }

#endif

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

