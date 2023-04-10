///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lgassert.h $
// $Author: TONY $
// $Date: 1997/03/27 13:54:54 $
// $Revision: 1.11 $
//
// @TBD (toml 04-30-96): Move assertions from dbg.h to here

#ifndef __LGASSERT_H
#define __LGASSERT_H

#include <assert.h>
#include <lglog.h>

//
// Control over whether to show dialog box on critical error
//
EXTERN BOOL g_fQuietAssert;

//
// Critical messages.  Unconditionally blow-up, always enabled.  Often used for
// switches and if-else clauses where suitable path cannot be found
//
// Made terse when not debug
//

#define CriticalMsg(Msg)                                 _CriticalMsg(_MakeCriticalStr(Msg), __FILE__, __LINE__)
#define CriticalMsg1(Msg, p1)                            CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1)))
#define CriticalMsg2(Msg, p1, p2)                        CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2)))
#define CriticalMsg3(Msg, p1, p2, p3)                    CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3)))
#define CriticalMsg4(Msg, p1, p2, p3, p4)                CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4)))
#define CriticalMsg5(Msg, p1, p2, p3, p4, p5)            CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5)))
#define CriticalMsg6(Msg, p1, p2, p3, p4, p5, p6)        CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6)))
#define CriticalMsg7(Msg, p1, p2, p3, p4, p5, p6, p7)    CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7)))
#define CriticalMsg8(Msg, p1, p2, p3, p4, p5, p6, p7, p8) CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7, p8)))
#define CriticalMsg9(Msg, p1, p2, p3, p4, p5, p6, p7, p8, p9) CriticalMsg(_MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7, p8, p9)))
#define CriticalStr(Msg)                                 CriticalMsg(Msg)

#ifdef SHIP
    #define TERSE_ASSERT 1
#endif

#ifdef TERSE_ASSERT
    #define _MakeCriticalStr(s) 0
#else
    #define _MakeCriticalStr(s) s
#endif


//
// Assertions.  Conditionally blow-up.  Often used for
// switches and if-else clauses where suitable path cannot be found
//
// Made terse when not debug, disabled for shipping code
//

#if !defined(NO_ASSERTS) && !defined(SHIP)

    #define AssertMsg(Test, Msg) \
            __Debug_BeginScope \
                { \
                if (__Debug_QuietEval(Test)) \
                    ; \
                else \
                    CriticalMsg(Msg); \
                } \
            __Debug_EndScope

    #define Assert_(Test)                                        AssertMsg(Test, #Test)
    #define AssertMsg1(Test, Msg, p1)                            AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1)))
    #define AssertMsg2(Test, Msg, p1, p2)                        AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2)))
    #define AssertMsg3(Test, Msg, p1, p2, p3)                    AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3)))
    #define AssertMsg4(Test, Msg, p1, p2, p3, p4)                AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4)))
    #define AssertMsg5(Test, Msg, p1, p2, p3, p4, p5)            AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5)))
    #define AssertMsg6(Test, Msg, p1, p2, p3, p4, p5, p6)        AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6)))
    #define AssertMsg7(Test, Msg, p1, p2, p3, p4, p5, p6, p7)    AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7)))
    #define AssertMsg8(Test, Msg, p1, p2, p3, p4, p5, p6, p7, p8) AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7, p8)))
    #define AssertMsg9(Test, Msg, p1, p2, p3, p4, p5, p6, p7, p8, p9) AssertMsg(Test, _MakeCriticalStr(_LogFmt(Msg, p1, p2, p3, p4, p5, p6, p7, p8, p9)))
    #define AssertStr(Test, Msg)                                 AssertMsg(Test, Msg)

#else

    #define Assert_(Test)
    #define AssertMsg(Test, Msg)
    #define AssertMsg1(Test, Msg, p1)
    #define AssertMsg2(Test, Msg, p1, p2)
    #define AssertMsg3(Test, Msg, p1, p2, p3)
    #define AssertMsg4(Test, Msg, p1, p2, p3, p4)
    #define AssertMsg5(Test, Msg, p1, p2, p3, p4, p5)
    #define AssertMsg6(Test, Msg, p1, p2, p3, p4, p5, p6)
    #define AssertMsg7(Test, Msg, p1, p2, p3, p4, p5, p6, p7)
    #define AssertMsg8(Test, Msg, p1, p2, p3, p4, p5, p6, p7, p8)
    #define AssertMsg9(Test, Msg, p1, p2, p3, p4, p5, p6, p7, p8, p9)
    #define AssertStr(Test, Msg)

#endif

//
// Verifications: Like assertions, but the expression will still expand even
// when not debugging.
//

#if !defined(NO_VERIFYS)
    #if defined(DEBUG)
        #define Verify(Test)                                        AssertMsg(Test, #Test)
        #define VerifyMsg(Test, Msg)                                AssertMsg(Test, Msg)
        #define VerifyMsg1(Test, Msg, p1)                           AssertMsg1(Test, Msg, p1)
        #define VerifyMsg2(Test, Msg, p1, p2)                       AssertMsg2(Test, Msg, p1, p2)
        #define VerifyMsg3(Test, Msg, p1, p2, p3)                   AssertMsg3(Test, Msg, p1, p2, p3)
        #define VerifyMsg4(Test, Msg, p1, p2, p3, p4)               AssertMsg4(Test, Msg, p1, p2, p3, p4)
        #define VerifyMsg5(Test, Msg, p1, p2, p3, p4, p5)           AssertMsg5(Test, Msg, p1, p2, p3, p4, p5)
        #define VerifyMsg6(Test, Msg, p1, p2, p3, p4, p5, p6)       AssertMsg6(Test, Msg, p1, p2, p3, p4, p5, p6)
        #define VerifyMsg7(Test, Msg, p1, p2, p3, p4, p5, p6, p7)   AssertMsg7(Test, Msg, p1, p2, p3, p4, p5, p6, p7)
    #else
        #define Verify(Test)                                        ((void)(Test))
        #define VerifyMsg(Test, Msg)                                ((void)(Test))
        #define VerifyMsg1(Test, Msg, p1)                           ((void)(Test))
        #define VerifyMsg2(Test, Msg, p1, p2)                       ((void)(Test))
        #define VerifyMsg3(Test, Msg, p1, p2, p3)                   ((void)(Test))
        #define VerifyMsg4(Test, Msg, p1, p2, p3, p4)               ((void)(Test))
        #define VerifyMsg5(Test, Msg, p1, p2, p3, p4, p5)           ((void)(Test))
        #define VerifyMsg6(Test, Msg, p1, p2, p3, p4, p5, p6)       ((void)(Test))
        #define VerifyMsg7(Test, Msg, p1, p2, p3, p4, p5, p6, p7)   ((void)(Test))
    #endif
#endif

//
// Critical message handler
//
EXTERN void LGAPI _CriticalMsg(const char * pszMsg, const char * pszFile, unsigned uLine);


//
// Optional hook to handle special requirements. In general, this is only
// used by low-level technology libraries -- it is not indended as a general
// notification mechanism
//
enum eCritMsgNotification
    {
    //
    // Starting/Ending assertion display
    //
    kCritMsgEnter,
    kCritMsgExit,

    //
    // User response (terminate means no exit and debug does not imply this)
    //
    kCritMsgIgnoring,
    kCritMsgTerminating,
    kCritMsgDebugging
    };

typedef void (LGAPI * tCritMsgNotificationHandler)(enum eCritMsgNotification);
EXTERN tCritMsgNotificationHandler LGAPI CritMsgSetHandler(tCritMsgNotificationHandler);

#endif /* !__LGASSERT_H */
