///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/gameshel/RCS/wgshelmt.h $
// $Author: TOML $
// $Date: 1997/09/10 14:01:42 $
// $Revision: 1.12 $
//
// Multithreaded Game Shell
//

#ifndef __WGSHELMT_H
#define __WGSHELMT_H

#include <thrdtool.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cMTWinGameShell
//
// In general, I'm not a huge fan of MI.  In this case, however, it seems to
// improve the code in such a way to outweigh the negatives (toml 09-20-96)
//

//
// Messages to worker
//
enum eGSWTMessage
{
    // Exit the thread
    kGSWTExit,

#if 0
    // Suspend
    kGSWTSuspend,
#endif

    // Create the window
    kGSWTCreateWindow,

};

///////////////////////////////////////

class cMTWinGameShell : public cWinGameShell, public cWorkerThread
{
public:
    // Construct an instance...
    cMTWinGameShell(int fFlags, IUnknown * pOuterUnknown);
    virtual ~cMTWinGameShell();

protected:

    STDMETHOD_(void, BeginFrame)();
    STDMETHOD_(void, PumpEvents)(int fPumpFlags = kPumpAll);
    // Handle a message dispatched to our window
    virtual long WndProc(UINT msg, WPARAM wParam, LPARAM lParam);
    virtual void CreateGameWindow();
    virtual void DestroyGameWindow();
    virtual void NotifyDisplayOnActivateApp(BOOL fActive);
    virtual void OnQuit();
    virtual void OnActivateApp(BOOL fActive);

    ///////////////////////////////////

    virtual DWORD ThreadProc();

    ///////////////////////////////////

    BOOL OkToBlock();

    ///////////////////////////////////

    HANDLE  m_hMasterThread;
    BOOL    m_fMasterBlockedFromDisplay;

    void BlockDisplay();
    void UnblockDisplay();

    ///////////////////////////////////

    enum eState
    {
        kStartup,
        kActive,
        kClosed
    };

    eState  m_fState;
    BOOL    m_fInGame;

    ///////////////////////////////////

    DWORD   m_WorkerThreadId;
    DWORD   m_MasterThreadId;

    void    AssertInWorker()    { AssertMsg(!m_WorkerThreadId || m_WorkerThreadId == GetCurrentThreadId(), "Expected to be in worker thread"); }
    void    AssertInMaster()    { AssertMsg(!m_MasterThreadId || m_MasterThreadId == GetCurrentThreadId(), "Expected to be in master thread"); }

    static BOOL gm_fInAssert;
    static tCritMsgNotificationHandler gm_pfnPreviousCritMsgNotificationHandler;
    static void LGAPI CritMsgNotificationHandler(eCritMsgNotification);
};

///////////////////////////////////////////////////////////////////////////////

#endif /* !__WGSHELMT_H */
