///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/lglocmsg.h $
// $Author: TOML $
// $Date: 1996/04/30 18:22:49 $
// $Revision: 1.1 $
//
// This header must notbe precompiled
//
// (c) Copyright 1996 Tom Leonard. All Rights Reserved. Unlimited license granted to Looking Glass Technologies Inc.
//
// @TBD (toml 04-30-96): The most important thing to do here is make local
// messages aware of banks, and make bank-spew aware of common primitive
// logger
// @TBD (toml 04-30-96): should rename DebugMsg (etc) to LocalMsg
//

#ifndef __LGLOCMSG_H
#define __LGLOCMSG_H

//
// Local debug messages.  They differ from banked debug messages
// in that they are completely preprocessed out unless
// intentionally enabled. They are especially useful when you
// don't have a source-level debugger.
//
// Must define LDEBUG in when compiling client module. ...Ex
// versions become active when DEBUG_TAG is defined as non-zero,
// The only predefined tag is DEBUG_VERBOSE.
//
// Define DEBUGEX to show only ...Ex elements, not general LDEBUG
//

// For some compilers, this can be used to place debug strings in a special
// location for efficiency (often in the code segment)
#ifndef DEBUG_STRING_PLACEMENT
    #define DEBUG_STRING_PLACEMENT
#endif

#define LDEBUG
#if defined(LDEBUG) && defined(DEBUG)

    #pragma message ("Local debugging is enabled")

    // LDEBUG implies EX enabled
    #define DEBUGEX 1

    #define DBG_LEVEL_STR(s)            __LevelMsg( __FILE__, __LINE__, s)
    #define DBG_LEVEL_STR_TRUE(b, s)    __LevelMsg( __FILE__, (b ? __LINE__ : -1), s)
    #define DBG_LEVEL_MSG(s, m)         char DEBUG_STRING_PLACEMENT __Msg[] = s; DBG_LEVEL_STR(m)
    #define DBG_LEVEL_MSG_TRUE(b, s, m) char DEBUG_STRING_PLACEMENT __Msg[] = s; DBG_LEVEL_STR_TRUE(b, m)

    #define DebugMsg(s)                                 LogMsg(s)
    #define DebugMsg1(s, p1)                            LogMsg1(s, p1)
    #define DebugMsg2(s, p1, p2)                        LogMsg2(s, p1, p2)
    #define DebugMsg3(s, p1, p2, p3)                    LogMsg3(s, p1, p2, p3)
    #define DebugMsg4(s, p1, p2, p3, p4)                LogMsg4(s, p1, p2, p3, p4)
    #define DebugMsg5(s, p1, p2, p3, p4, p5)            LogMsg5(s, p1, p2, p3, p4, p5)
    #define DebugMsg6(s, p1, p2, p3, p4, p5, p6)        LogMsg6(s, p1, p2, p3, p4, p5, p6)
    #define DebugMsg7(s, p1, p2, p3, p4, p5, p6, p7)    LogMsg7(s, p1, p2, p3, p4, p5, p6, p7)
    #define DebugStr(s)                                 LogStr(s)

    #define DebugMsgTrue(t, s)                              if (__Debug_QuietEval(!(t))) ; else LogMsg(s)
    #define DebugMsgTrue1(t, s, p1)                         if (__Debug_QuietEval(!(t))) ; else LogMsg1(s, p1)
    #define DebugMsgTrue2(t, s, p1, p2)                     if (__Debug_QuietEval(!(t))) ; else LogMsg2(s, p1, p2)
    #define DebugMsgTrue3(t, s, p1, p2, p3)                 if (__Debug_QuietEval(!(t))) ; else LogMsg3(s, p1, p2, p3)
    #define DebugMsgTrue4(t, s, p1, p2, p3, p4)             if (__Debug_QuietEval(!(t))) ; else LogMsg4(s, p1, p2, p3, p4)
    #define DebugMsgTrue5(t, s, p1, p2, p3, p4, p5)         if (__Debug_QuietEval(!(t))) ; else LogMsg5(s, p1, p2, p3, p4, p5)
    #define DebugMsgTrue6(t, s, p1, p2, p3, p4, p5, p6)     if (__Debug_QuietEval(!(t))) ; else LogMsg6(s, p1, p2, p3, p4, p5, p6)
    #define DebugMsgTrue7(t, s, p1, p2, p3, p4, p5, p6, p7) if (__Debug_QuietEval(!(t))) ; else LogMsg7(s, p1, p2, p3, p4, p5, p6, p7)
    #define DebugStrTrue(t, s)                              if (__Debug_QuietEval(!(t))) ; else LogStr(s)

    // Hand-forced indentation.  Value limited usually to focusing on order of
    // creation/destruction, else not very important
    #ifdef DOINDENTMSG

        #pragma message ("IncDebugMsg() and DecDebugMsg() are enabled")

        #define IncDebugMsg(s)                              __Debug_BeginScope { LogMsg(s " {"); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg1(s, p1)                         __Debug_BeginScope { LogMsg1(s " {", p1); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg2(s, p1, p2)                     __Debug_BeginScope { LogMsg2(s " {", p1, p2); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg3(s, p1, p2, p3)                 __Debug_BeginScope { LogMsg3(s " {", p1, p2, p3); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg4(s, p1, p2, p3, p4)             __Debug_BeginScope { LogMsg4(s " {", p1, p2, p3, p4); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg5(s, p1, p2, p3, p4, p5)         __Debug_BeginScope { LogMsg5(s " {", p1, p2, p3, p4, p5); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg6(s, p1, p2, p3, p4, p5, p6)     __Debug_BeginScope { LogMsg6(s " {", p1, p2, p3, p4, p5, p6); LogIncIndent(); } __Debug_EndScope
        #define IncDebugMsg7(s, p1, p2, p3, p4, p5, p6, p7) __Debug_BeginScope { LogMsg7(s " {", p1, p2, p3, p4, p5, p6, p7); LogIncIndent(); } __Debug_EndScope
        #define IncDebugStr(s)                              __Debug_BeginScope { LogStr(s); IncDebugMsg(""); } __Debug_EndScope

        #define DecDebugMsg(s)                              __Debug_BeginScope { LogDecIndent(); LogMsg("} " s); } __Debug_EndScope
        #define DecDebugMsg1(s, p1)                         __Debug_BeginScope { LogDecIndent();  LogMsg1("} " s, p1); } __Debug_EndScope
        #define DecDebugMsg2(s, p1, p2)                     __Debug_BeginScope { LogDecIndent(); LogMsg2("} " s, p1, p2); } __Debug_EndScope
        #define DecDebugMsg3(s, p1, p2, p3)                 __Debug_BeginScope { LogDecIndent(); LogMsg3("} " s, p1, p2, p3); } __Debug_EndScope
        #define DecDebugMsg4(s, p1, p2, p3, p4)             __Debug_BeginScope { LogDecIndent(); LogMsg4("} " s, p1, p2, p3, p4); } __Debug_EndScope
        #define DecDebugMsg5(s, p1, p2, p3, p4, p5)         __Debug_BeginScope { LogDecIndent(); LogMsg5("} " s, p1, p2, p3, p4, p5);  } __Debug_EndScope
        #define DecDebugMsg6(s, p1, p2, p3, p4, p5, p6)     __Debug_BeginScope { LogDecIndent();  LogMsg6("} " s, p1, p2, p3, p4, p5, p6); } __Debug_EndScope
        #define DecDebugMsg7(s, p1, p2, p3, p4, p5, p6, p7) __Debug_BeginScope { LogDecIndent(); LogMsg7("} " s, p1, p2, p3, p4, p5, p6, p7); } __Debug_EndScope
        #define DecDebugStr(s)                              __Debug_BeginScope { LogDecIndent(); LogMsg("} (cont'd.)"); LogStr(s);  } __Debug_EndScope

    #else

        #define IncDebugMsg(s)                              DebugMsg(s)
        #define IncDebugMsg1(s, p1)                         DebugMsg1(s, p1)
        #define IncDebugMsg2(s, p1, p2)                     DebugMsg2(s, p1, p2)
        #define IncDebugMsg3(s, p1, p2, p3)                 DebugMsg3(s, p1, p2, p3)
        #define IncDebugMsg4(s, p1, p2, p3, p4)             DebugMsg4(s, p1, p2, p3, p4)
        #define IncDebugMsg5(s, p1, p2, p3, p4, p5)         DebugMsg5(s, p1, p2, p3, p4, p5)
        #define IncDebugMsg6(s, p1, p2, p3, p4, p5, p6)     DebugMsg6(s, p1, p2, p3, p4, p5, p6)
        #define IncDebugMsg7(s, p1, p2, p3, p4, p5, p6, p7) DebugMsg7(s, p1, p2, p3, p4, p5, p6, p7)
        #define IncDebugStr(s)                              DebugStr(s)

        #define DecDebugMsg(s)                              DebugMsg(s)
        #define DecDebugMsg1(s, p1)                         DebugMsg1(s, p1)
        #define DecDebugMsg2(s, p1, p2)                     DebugMsg2(s, p1, p2)
        #define DecDebugMsg3(s, p1, p2, p3)                 DebugMsg3(s, p1, p2, p3)
        #define DecDebugMsg4(s, p1, p2, p3, p4)             DebugMsg4(s, p1, p2, p3, p4)
        #define DecDebugMsg5(s, p1, p2, p3, p4, p5)         DebugMsg5(s, p1, p2, p3, p4, p5)
        #define DecDebugMsg6(s, p1, p2, p3, p4, p5, p6)     DebugMsg6(s, p1, p2, p3, p4, p5, p6)
        #define DecDebugMsg7(s, p1, p2, p3, p4, p5, p6, p7) DebugMsg7(s, p1, p2, p3, p4, p5, p6, p7)
        #define DecDebugStr(s)                              DebugStr(s)

    #endif

    #define LAssertMsg(b, s) AssertMsg(b, s)
    #define LAssertStr(b, s) AssertStr(b, s)

#else

    #define DBG_LEVEL_STR(s)
    #define DBG_LEVEL_STR_TRUE(b, s)
    #define DBG_LEVEL_MSG(s, m)
    #define DBG_LEVEL_MSG_TRUE(b, s, m)

    #define DebugMsg(s)
    #define DebugStr(s)
    #define DebugMsg1(s, p1)
    #define DebugMsg2(s, p1, p2)
    #define DebugMsg3(s, p1, p2, p3)
    #define DebugMsg4(s, p1, p2, p3, p4)
    #define DebugMsg5(s, p1, p2, p3, p4, p5)
    #define DebugMsg6(s, p1, p2, p3, p4, p5, p6)
    #define DebugMsg7(s, p1, p2, p3, p4, p5, p6, p7)

    #define DebugMsgTrue(t, s)
    #define DebugStrTrue(t, s)
    #define DebugMsgTrue1(t, s, p1)
    #define DebugMsgTrue2(t, s, p1, p2)
    #define DebugMsgTrue3(t, s, p1, p2, p3)
    #define DebugMsgTrue4(t, s, p1, p2, p3, p4)
    #define DebugMsgTrue5(t, s, p1, p2, p3, p4, p5)
    #define DebugMsgTrue6(t, s, p1, p2, p3, p4, p5, p6)
    #define DebugMsgTrue7(t, s, p1, p2, p3, p4, p5, p6, p7)

    #define IncDebugMsg(s)
    #define IncDebugMsg1(s, p1)
    #define IncDebugMsg2(s, p1, p2)
    #define IncDebugMsg3(s, p1, p2, p3)
    #define IncDebugMsg4(s, p1, p2, p3, p4)
    #define IncDebugMsg5(s, p1, p2, p3, p4, p5)
    #define IncDebugMsg6(s, p1, p2, p3, p4, p5, p6)
    #define IncDebugMsg7(s, p1, p2, p3, p4, p5, p6, p7)
    #define IncDebugStr(s)

    #define DecDebugMsg(s)
    #define DecDebugMsg1(s, p1)
    #define DecDebugMsg2(s, p1, p2)
    #define DecDebugMsg3(s, p1, p2, p3)
    #define DecDebugMsg4(s, p1, p2, p3, p4)
    #define DecDebugMsg5(s, p1, p2, p3, p4, p5)
    #define DecDebugMsg6(s, p1, p2, p3, p4, p5, p6)
    #define DecDebugMsg7(s, p1, p2, p3, p4, p5, p6, p7)
    #define DecDebugStr(s)

    #define LAssertMsg(b, s)
    #define LAssertStr(b, s)
#endif

#if defined(DEBUGEX) && defined(DEBUG)

    #ifndef DEBUG_VERBOSE
        #define DEBUG_VERBOSE 0
    #else
        #pragma message ("DEBUG_VERBOSE is enabled")
    #endif


    #ifndef LDEBUG
        #pragma message ("DebugEx is enabled")
    #endif

    #define DBG_LEVEL_STR_TRUE_EX(b, s)     cTraceMsgLevel __DebugEnclosure__( __FILE__, (__Debug_QuietEval(b) ? __LINE__ : -1), s)
    #define DBG_LEVEL_MSG_TRUE_EX(b, s, m)  char DEBUG_STRING_PLACEMENT __Msg[] = s; DBG_LEVEL_STR_TRUE_EX(b, m)

    #define DebugMsgEx(c, s)                                if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg(s)
    #define DebugMsgEx1(c, s, p1)                           if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg1(s, p1)
    #define DebugMsgEx2(c, s, p1, p2)                       if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg2(s, p1, p2)
    #define DebugMsgEx3(c, s, p1, p2, p3)                   if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg3(s, p1, p2, p3)
    #define DebugMsgEx4(c, s, p1, p2, p3, p4)               if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg4(s, p1, p2, p3, p4)
    #define DebugMsgEx5(c, s, p1, p2, p3, p4, p5)           if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg5(s, p1, p2, p3, p4, p5)
    #define DebugMsgEx6(c, s, p1, p2, p3, p4, p5, p6)       if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg6(s, p1, p2, p3, p4, p5, p6)
    #define DebugMsgEx7(c, s, p1, p2, p3, p4, p5, p6, p7)   if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogMsg7(s, p1, p2, p3, p4, p5, p6, p7)
    #define DebugStrEx(c, s)                                if (__Debug_QuietEval(!(DEBUG_ ## c))) ; else LogStr(s)

    #define DebugMsgTrueEx(c, t, s)                                 if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg(s)
    #define DebugMsgTrueEx1(c, t, s, p1)                            if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg1(s, p1)
    #define DebugMsgTrueEx2(c, t, s, p1, p2)                        if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg2(s, p1, p2)
    #define DebugMsgTrueEx3(c, t, s, p1, p2, p3)                    if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg3(s, p1, p2, p3)
    #define DebugMsgTrueEx4(c, t, s, p1, p2, p3, p4)                if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg4(s, p1, p2, p3, p4)
    #define DebugMsgTrueEx5(c, t, s, p1, p2, p3, p4, p5)            if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg5(s, p1, p2, p3, p4, p5)
    #define DebugMsgTrueEx6(c, t, s, p1, p2, p3, p4, p5, p6)        if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg6(s, p1, p2, p3, p4, p5, p6)
    #define DebugMsgTrueEx7(c, t, s, p1, p2, p3, p4, p5, p6, p7)    if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogMsg7(s, p1, p2, p3, p4, p5, p6, p7)
    #define DebugStrTrueEx(c, t, s)                                 if (__Debug_QuietEval(!(DEBUG_ ## c)) || !(t)) ; else LogStr(s)

#else

    #define DBG_LEVEL_STR_TRUE_EX(b, s)
    #define DBG_LEVEL_MSG_TRUE_EX(b, s, m)

    #define DebugMsgEx(c, s)
    #define DebugMsgEx1(c, s, p1)
    #define DebugMsgEx2(c, s, p1, p2)
    #define DebugMsgEx3(c, s, p1, p2, p3)
    #define DebugMsgEx4(c, s, p1, p2, p3, p4)
    #define DebugMsgEx5(c, s, p1, p2, p3, p4, p5)
    #define DebugMsgEx6(c, s, p1, p2, p3, p4, p5, p6)
    #define DebugMsgEx7(c, s, p1, p2, p3, p4, p5, p6, p7)
    #define DebugStrEx(c, s)

    #define DebugMsgTrueEx(c, t, s)
    #define DebugMsgTrueEx1(c, t, s, p1)
    #define DebugMsgTrueEx2(c, t, s, p1, p2)
    #define DebugMsgTrueEx3(c, t, s, p1, p2, p3)
    #define DebugMsgTrueEx4(c, t, s, p1, p2, p3, p4)
    #define DebugMsgTrueEx5(c, t, s, p1, p2, p3, p4, p5)
    #define DebugMsgTrueEx6(c, t, s, p1, p2, p3, p4, p5, p6)
    #define DebugMsgTrueEx7(c, t, s, p1, p2, p3, p4, p5, p6, p7)
    #define DebugStrTrueEx(c, t, s)

#endif

//
// Level debugging: Provides scoped indented debugging messages (under C++)
//
#define BEGIN_DEBUG_STR(s)                              { DBG_LEVEL_STR(s);
#define BEGIN_DEBUG_MSG(s)                              { DBG_LEVEL_MSG(s, __Msg);
#define BEGIN_DEBUG_MSG1(s, p1)                         { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1));
#define BEGIN_DEBUG_MSG2(s, p1, p2)                     { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1, p2));
#define BEGIN_DEBUG_MSG3(s, p1, p2, p3)                 { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1, p2, p3));
#define BEGIN_DEBUG_MSG4(s, p1, p2, p3, p4)             { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1, p2, p3, p4));
#define BEGIN_DEBUG_MSG5(s, p1, p2, p3, p4, p5)         { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1, p2, p3, p4, p5));
#define BEGIN_DEBUG_MSG6(s, p1, p2, p3, p4, p5, p6)     { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1, p2, p3, p4, p5, p6));
#define BEGIN_DEBUG_MSG7(s, p1, p2, p3, p4, p5, p6, p7) { DBG_LEVEL_MSG(s, _LogFmt(__Msg, p1, p2, p3, p4, p5, p6, p7));

#define BEGIN_DEBUG_STR_TRUE(b, s)                              { DBG_LEVEL_STR_TRUE(b, s);
#define BEGIN_DEBUG_MSG_TRUE(b, s)                              { DBG_LEVEL_MSG_TRUE(b, s, __Msg);
#define BEGIN_DEBUG_MSG_TRUE1(b, s, p1)                         { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1));
#define BEGIN_DEBUG_MSG_TRUE2(b, s, p1, p2)                     { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1, p2));
#define BEGIN_DEBUG_MSG_TRUE3(b, s, p1, p2, p3)                 { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1, p2, p3));
#define BEGIN_DEBUG_MSG_TRUE4(b, s, p1, p2, p3, p4)             { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1, p2, p3, p4));
#define BEGIN_DEBUG_MSG_TRUE5(b, s, p1, p2, p3, p4, p5)         { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1, p2, p3, p4, p5));
#define BEGIN_DEBUG_MSG_TRUE6(b, s, p1, p2, p3, p4, p5, p6)     { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1, p2, p3, p4, p5, p6));
#define BEGIN_DEBUG_MSG_TRUE7(b, s, p1, p2, p3, p4, p5, p6, p7) { DBG_LEVEL_MSG_TRUE(b, s, _LogFmt(__Msg, p1, p2, p3, p4, p5, p6, p7));

#define BEGIN_DEBUG_STR_EX(c, s)                                { DBG_LEVEL_STR_TRUE_EX(DEBUG_ ## c, s);
#define BEGIN_DEBUG_MSG_EX(c, s)                                { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, __Msg);
#define BEGIN_DEBUG_MSG_EX1(c, s, p1)                           { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1));
#define BEGIN_DEBUG_MSG_EX2(c, s, p1, p2)                       { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1, p2));
#define BEGIN_DEBUG_MSG_EX3(c, s, p1, p2, p3)                   { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1, p2, p3));
#define BEGIN_DEBUG_MSG_EX4(c, s, p1, p2, p3, p4)               { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1, p2, p3, p4));
#define BEGIN_DEBUG_MSG_EX5(c, s, p1, p2, p3, p4, p5)           { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1, p2, p3, p4, p5));
#define BEGIN_DEBUG_MSG_EX6(c, s, p1, p2, p3, p4, p5, p6)       { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1, p2, p3, p4, p5, p6));
#define BEGIN_DEBUG_MSG_EX7(c, s, p1, p2, p3, p4, p5, p6, p7)   { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c, s, _LogFmt(__Msg, p1, p2, p3, p4, p5, p6, p7));

#define BEGIN_DEBUG_STR_TRUE_EX(c, b, s) { DBG_LEVEL_STR_TRUE_EX(DEBUG_ ## c && (b), s);
#define BEGIN_DEBUG_MSG_TRUE_EX(c, b, s) { DBG_LEVEL_MSG_TRUE_EX(DEBUG_ ## c && (b), s, __Msg);

#define END_DEBUG }


#ifdef __cplusplus

///////////////////////////////////////
//
// CLASS: cTraceMsgLevel
//
// This class handles the automatic indent/outdent on entering/leaving scope
class cTraceMsgLevel
    {
public:
    cTraceMsgLevel(const char *pFN, long line, const char *pMsg);
    ~cTraceMsgLevel();

    const char *GetFileName() const
        {
        return pszFileName;
        }

    long GetLine() const
        {
        return lLine;
        }

private:
    char *pszFileName;
    long lLine;
    char *pszMessage;
    };

#define __LevelMsg(f, l, m) cTraceMsgLevel __DebugEnclosure__(f, l, m)
#else
#define __LevelMsg(f, l, m) DebugMsg(m)
#endif

#endif /* !__LGLOCMSG_H */
