///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/thrdtool.cpp $
// $Author: TOML $
// $Date: 1997/01/03 14:17:45 $
// $Revision: 1.11 $
//

#ifdef _WIN32

#include <windows.h>
#include <lg.h>
#include <thrdtool.h>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadLock
//

#ifdef THRDTOOL_DEBUG

cThreadLock::cThreadLock()
{
    InitializeCriticalSection(&m_CritSec);
    m_currentOwnerID = m_lockCount = 0;
    m_fTrace = FALSE;
}

///////////////////////////////////////

cThreadLock::~cThreadLock()
{
    DeleteCriticalSection(&m_CritSec);
}

///////////////////////////////////////

void cThreadLock::Lock()
{
    DWORD thisThreadID = GetCurrentThreadId();

    DebugMsgTrue3(m_currentOwnerID && (m_currentOwnerID != thisThreadID) && m_fTrace, "Thread %u about to wait for lock %x owned by %u", GetCurrentThreadId(), &m_CritSec, m_currentOwnerID);

    EnterCriticalSection(&m_CritSec);

    if (m_lockCount == 0)
    {
        // we now own it for the first time.  Set owner information
        m_currentOwnerID = thisThreadID;
        DebugMsgTrue2(m_fTrace, "Thread %u now owns lock 0x%x", m_currentOwnerID, &m_CritSec);
    }

    m_lockCount++;
}

///////////////////////////////////////

void cThreadLock::Unlock()
{
    AssertMsg(m_lockCount >= 1, "Invalid unlock of thread lock");
    m_lockCount--;
    if (m_lockCount == 0)
    {
        DebugMsgTrue2(m_fTrace, "Thread %u releasing lock 0x%x", m_currentOwnerID, &m_CritSec);
        m_currentOwnerID = 0;
    }
    LeaveCriticalSection(&m_CritSec);
}

///////////////////////////////////////

BOOL cThreadLock::AssertOwnedByCurrentThread()
{
    if (GetCurrentThreadId() == m_currentOwnerID)
        return TRUE;
    CriticalMsg3("Expected thread %u as owner of lock 0x%x, but %u owns", GetCurrentThreadId(), &m_CritSec, m_currentOwnerID);
    return TRUE;
}

///////////////////////////////////////

void cThreadLock::SetTrace(BOOL fTrace)
{
    m_fTrace = fTrace;
}

///////////////////////////////////////

#endif /* DEBUG */

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadEvent
//

cThreadEvent::cThreadEvent(BOOL fManualReset)
{
    m_hSyncObject = CreateEvent(NULL, fManualReset, FALSE, NULL);

    AssertMsg1(m_hSyncObject, "Failed to create event (error 0x%x)", GetLastError());
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadSemaphore
//

cThreadSemaphore::cThreadSemaphore(long initialValue, long maxValue)
{
    AssertMsg(maxValue > 0, "Invalid max value for semaphore");
    AssertMsg(initialValue >= 0 && initialValue <= maxValue, "Invalid initial value for semaphore");

    m_hSyncObject = CreateSemaphore(NULL, initialValue, maxValue, NULL);

    AssertMsg1(m_hSyncObject, "Failed to create semaphore (error 0x%x)", GetLastError());
}

///////////////////////////////////////

BOOL cThreadSemaphore::Release(long releaseCount, long * pPreviousCount)
{
    #ifdef THRDTOOL_DEBUG
    AssertUseable();
    #endif
    return ReleaseSemaphore(m_hSyncObject, releaseCount, pPreviousCount);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadMutex
//

cThreadMutex::cThreadMutex(BOOL fEstablishInitialOwnership)
{
    m_hSyncObject = CreateMutex(NULL, fEstablishInitialOwnership, NULL);

    AssertMsg1(m_hSyncObject, "Failed to create mutex (error 0x%x)", GetLastError());
}

///////////////////////////////////////

BOOL cThreadMutex::Release()
{
    #ifdef THRDTOOL_DEBUG
    AssertUseable();
    #endif
    return ReleaseMutex(m_hSyncObject);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWorkerThread
//

cWorkerThread::~cWorkerThread()
{
    if (m_hThread)
    {
        AssertMsg(!ThreadExists(),
                  "Thread still exists even though object supporting it has been destroyed!\n"
                  "Classes derived from cWorkerThread must negotiate an end to the worker\n"
                  "prior to destruction of the thread management object");
        WaitForClose();
    }
}

///////////////////////////////////////
//
// Return TRUE if the thread exists. FALSE otherwise
//

BOOL cWorkerThread::ThreadExists(void)
{
    DWORD dwExitCode;

    return (m_hThread &&
            GetExitCodeThread(m_hThread, &dwExitCode) &&
            dwExitCode == STILL_ACTIVE);
}

///////////////////////////////////////
//
// when the thread starts, it calls this function. We unwrap the 'this'
// pointer and call ThreadProc.
//

unsigned __stdcall cWorkerThread::InitialThreadProc(LPVOID pv)
{
    cWorkerThread * pThread = (cWorkerThread *) pv;
    pThread->m_EventComplete.Set();
    return pThread->ThreadProc();
}

///////////////////////////////////////

BOOL cWorkerThread::Create()
{
    unsigned    threadid;
    cAutoLock   lock(m_Lock);

    if (ThreadExists())
    {
        CriticalMsg("Tried to create a thread that has already been created!");
        return FALSE;
    }

    // @Note (toml 08-21-96): We use _beginthreadex() instead of CreateThread()
    // because it's the correct thing to do when using the C runtime (like
    // if someone uses errno, for example)
    m_hThread = (HANDLE) _beginthreadex(NULL,
                                        0,
                                        cWorkerThread::InitialThreadProc,
                                        this,
                                        0,
                                        &threadid);

    AssertMsg1(m_hThread, "Failed to create thread (error 0x%x)", GetLastError());

    // Force serialized thread creation...
    if (!m_EventComplete.Wait(10000))
    {
        CriticalMsg("Timeout waiting for worker thread to initialize. Probable deadlock or creation failure!");
    }

    return !!m_hThread;
}

///////////////////////////////////////

void cWorkerThread::WaitForClose(DWORD dwErrorTimeout)
{
    if (m_hThread)
    {
        if (WaitForSingleObject(m_hThread, dwErrorTimeout) == WAIT_TIMEOUT)
        {
            CriticalMsg("Timeout waiting for worker thread to close. Probable deadlock!");
        }
        CloseHandle(m_hThread);
        m_hThread = 0;
    }
}

///////////////////////////////////////

DWORD cWorkerThread::Call(DWORD dwParam, BOOL fBoostPriority)
{
    AssertMsg(!m_EventSend.Check(), "Cannot nest inter-thread calls (line is busy)");

    cAutoLock lock(m_Lock);

    if (!ThreadExists())
    {
        return E_FAIL;
    }

    int iInitialPriority;
    if (fBoostPriority)
    {
        iInitialPriority = GetPriority();
        const int iNewPriority = GetThreadPriority(GetCurrentThread());
        if (iNewPriority > iInitialPriority)
            SetPriority(iNewPriority);
    }

    // set the parameter, signal the worker thread, wait for the completion to be signalled
    m_dwParam = dwParam;

    m_EventSend.Set();
    m_EventComplete.Wait();

    if (fBoostPriority)
    {
        SetPriority(iInitialPriority);
    }

    return m_dwReturnVal;
}

///////////////////////////////////////
//
// Wait for a request from the client
//

DWORD cWorkerThread::WaitForCall()
{
    m_EventSend.Wait();
    return m_dwParam;
}

///////////////////////////////////////
//
// is there a request?
//

BOOL cWorkerThread::PeekCall(DWORD * pParam)
{
    if (!m_EventSend.Check())
    {
        return FALSE;
    }
    else
    {
        if (pParam)
        {
            *pParam = m_dwParam;
        }
        return TRUE;
    }
}

///////////////////////////////////////
//
// Reply to the request
//

void cWorkerThread::Reply(DWORD dw)
{
    m_dwParam = kInvalidCallParam;
    m_dwReturnVal = dw;

    // The request is now complete so PeekCall() should fail from
    // now on
    //
    // This event should be reset BEFORE we signal the client or
    // the client may Set it before we reset it and we'll then
    // reset it (!)

    m_EventSend.Reset();

    // Tell the client we're finished
    m_EventComplete.Set();
}

///////////////////////////////////////////////////////////////////////////////

#endif /* _WIN32 */
