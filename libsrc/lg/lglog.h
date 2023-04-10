///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lglog.h $
// $Author: TOML $
// $Date: 1996/06/03 16:43:45 $
// $Revision: 1.4 $
//

#ifndef __LGLOG_H
#define __LGLOG_H

#ifdef __cplusplus
extern "C"  {
#endif

//
// LOG CONFIGURATION
//
enum eLoggingOptions
    {
    // Destination
    kLogToFile          = 0x01,
    kLogToMono          = 0x02,
    kLogToDebugger      = 0x04,
    kReserved1          = 0x08,
    kLogToAll           = (kLogToFile + kLogToMono + kLogToDebugger + kReserved1),

    // Options
    kLogDisableMonoIndent  = 0x10,
    kLogDisableIndent      = (0x20 + kLogDisableMonoIndent),

    kLogDisplayTime        = 0x40
    };

EXTERN void         LGAPI LogSetOptions(int fOptions);
EXTERN void         LGAPI LogSetLogFile(const char * pszFile);
EXTERN int          LGAPI LogGetOptions();
EXTERN const char * LGAPI LogGetLogFile();

//
// LOG UTILITY FUNCTIONS
//

// These help quiet noisy warnings about expressions always being true/false
// without forcing clients to turn off warnings
#if defined(__WATCOMC__)
    EXTERN int LGDATA __Debug_True;
    EXTERN int LGDATA __Debug_False;
    #define __Debug_QuietEval(expr)     (((expr) || __Debug_False) && __Debug_True)
#else
    #define __Debug_True                1
    #define __Debug_False               0
    #define __Debug_QuietEval(expr)     (expr)
#endif
// Set up a scope type that is both if-else safe and requires a semi-colon
#define __Debug_BeginScope          do {
#define __Debug_EndScope            } while (__Debug_False)

EXTERN const char * __cdecl _LogFmt(const char *, ...);


//
// LOGGING FUNCTIONS
//

// High-level log functions
#define LogMsg(Msg)                                 _LogWriteMsgFileLine(Msg, __FILE__, __LINE__)
#define LogMsg1(Msg, p1)                            LogMsg(_LogFmt(Msg, p1))
#define LogMsg2(Msg, p1, p2)                        LogMsg(_LogFmt(Msg, p1, p2))
#define LogMsg3(Msg, p1, p2, p3)                    LogMsg(_LogFmt(Msg, p1, p2, p3))
#define LogMsg4(Msg, p1, p2, p3, p4)                LogMsg(_LogFmt(Msg, p1, p2, p3, p4))
#define LogMsg5(Msg, p1, p2, p3, p4, p5)            LogMsg(_LogFmt(Msg, p1, p2, p3, p4, p5))
#define LogMsg6(Msg, p1, p2, p3, p4, p5, p6)        LogMsg(_LogFmt(Msg, p1, p2, p3, p4, p5, p6))
#define LogMsg7(Msg, p1, p2, p3, p4, p5, p6, p7)    LogMsg(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7))
#define LogStr(Msg)                                 LogMsg(Msg)

#if 0
// @TBD (toml 04-30-96): reroute mprintf to here?
#ifdef NO_LOG_MPRINTF
    #define mprintf                                     _LogWriteFmtMsg
    #define mprint                                      _LogWriteMsg
#else
    #define mprintf                                     _mprintf
    #define mprint                                      _mprint
#endif
#endif

// Do a stack dump
EXTERN void LGAPI LogStackTrace(const char * pszTitle);

// Debug log write function
EXTERN void LGAPI _LogWriteMsgFileLine(const char *, const char *, unsigned);
EXTERN int  LGAPI _LogWriteMsg(const char *);
EXTERN int  __cdecl _LogWriteFmtMsg(const char *, ...);

// Primal control of logging (not generally used directly)
typedef void (LGAPI * tLogMessageFunc)(const char *, const char *, unsigned line);
EXTERN tLogMessageFunc LGAPI LogSetMessageFunc(tLogMessageFunc);

EXTERN int  LGAPI LogGetIndent();
EXTERN void LGAPI LogIncIndent();
EXTERN void LGAPI LogDecIndent();

//
// DIAGNOSTIC HELPERS
//
// These functions generate a string that represents
// given argument in the most readable form. Be aware that
// these use static buffers.
//

#if defined(_WIN32) && defined(_INC_WINDOWS)
EXTERN const char * LGAPI LogStrWinMsg(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#define LogStrWinRect(rc)   _LogStrWinRect( #rc, &(rc) )
#define LogStrWinPt(pt)     _LogStrWinPt( #pt, &(pt) )

EXTERN const char * LGAPI _LogStrWinRect(const char *, const RECT *);
EXTERN const char * LGAPI _LogStrWinPt(const char *, const POINT *);
#endif


#ifdef __cplusplus
}
#endif

#endif /* !__LGLOG_H */
