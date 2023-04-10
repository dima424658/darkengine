///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/thrdtool.h $
// $Author: TOML $
// $Date: 1997/01/07 13:05:41 $
// $Revision: 1.11 $
//
// Tools for thread synchronization and management.
//
//

#ifndef __THRDTOOL_H
#define __THRDTOOL_H

#ifdef _WIN32

#if defined(DEBUG) || defined(ONEOPT)
#define THRDTOOL_DEBUG
#endif

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadLock
//
// Wrapper for critical sections management
//

class cThreadLock
{
public:
    cThreadLock();
    ~cThreadLock();

    //
    // Get/wait for ownership of the critical section
    //
    void Lock();

    //
    // Release ownership of the critical section
    //
    void Unlock();

    //
    // To make deadlocks easier to track it is useful to insert in
    // the code an assertion that says whether we own a critical
    // section or not.
    //
    BOOL AssertOwnedByCurrentThread();

    //
    // In addition we provide a routine that allows usage of specific
    // critical sections to be traced.  This is NOT on by default.
    //
    void SetTrace(BOOL);


private:
    // make copy constructor and assignment operator inaccessible
    cThreadLock(const cThreadLock &refCritSec);
    cThreadLock &operator=(const cThreadLock &refCritSec);

    CRITICAL_SECTION m_CritSec;

#ifdef THRDTOOL_DEBUG
    DWORD   m_currentOwnerID;
    DWORD   m_lockCount;
    BOOL    m_fTrace;        // Trace this one
#endif
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAutoLock
//
// Locks a critical section, and unlocks it automatically
// when the lock goes out of scope
//

class cAutoLock
{
public:
    cAutoLock(cThreadLock & lock);
    ~cAutoLock();

private:
    cThreadLock & m_lock;

    // make copy constructor and assignment operator inaccessible
    cAutoLock(const cAutoLock &);
    cAutoLock &operator=(const cAutoLock &);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadSyncObject
//
// Base class for event, semaphore and mutex objects.
//
// @TBD (toml 08-19-96): named objects? ability to "attach" an existing object?
//

class cThreadSyncObject
{
public:
    ~cThreadSyncObject();

    //
    // Query if object is useful
    //
    BOOL operator!() const;

    //
    // Access handle
    //
    operator HANDLE ();

    //
    // Wait for a signal from the object
    //
    BOOL Wait(DWORD dwTimeout = INFINITE);

protected:
    cThreadSyncObject();
    #ifdef THRDTOOL_DEBUG
    void AssertUseable();
    #endif

    HANDLE m_hSyncObject;

private:
    // make copy constructor and assignment operator inaccessible
    cThreadSyncObject(const cThreadSyncObject &);
    cThreadSyncObject &operator=(const cThreadSyncObject &);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadEvent
//
// Wrapper for unnamed event objects
//

class cThreadEvent : public cThreadSyncObject
{
public:
    cThreadEvent(BOOL fManualReset = FALSE);

    //
    // Set the state to signaled
    //
    BOOL Set();


    //
    // Set the state to nonsignaled
    //
    BOOL Reset();

    //
    // A single operation that sets (to signaled) the state of
    // the object and then resets it (to nonsignaled) after
    // releasing the appropriate number of waiting threads.
    //
    BOOL Pulse();

    //
    // Check if the event is signaled
    //
    BOOL Check();

private:
    // make copy constructor and assignment operator inaccessible
    cThreadEvent(const cThreadEvent &);
    cThreadEvent &operator=(const cThreadEvent &);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadSemaphore
//

class cThreadSemaphore : public cThreadSyncObject
{
public:
    cThreadSemaphore(long initialValue, long maxValue);

    //
    // Increases the count of the semaphore object by a specified
    // amount.  Wait() decreases the count by one on return.
    //
    BOOL Release(long releaseCount = 1, long * pPreviousCount = NULL);

private:
    // make copy constructor and assignment operator inaccessible
    cThreadSemaphore(const cThreadSemaphore &);
    cThreadSemaphore &operator=(const cThreadSemaphore &);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cThreadMutex
//

class cThreadMutex : public cThreadSyncObject
{
public:
    cThreadMutex(BOOL fEstablishInitialOwnership = FALSE);

    //
    // Release ownership of the mutex
    //
    BOOL Release();

private:
    // make copy constructor and assignment operator inaccessible
    cThreadMutex(const cThreadMutex &);
    cThreadMutex &operator=(const cThreadMutex &);
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cWorkerThread
//
// Support for a worker thread
//
// Simple thread class supports creation of worker thread, synchronization
// and simpler communication. Must derived to use.
//

class cWorkerThread
{
public:
    cWorkerThread();
    virtual ~cWorkerThread();

    ///////////////////////////////////
    //
    // Functions for the master thread
    //

    //
    // Start thread running  - error if already running
    //
    BOOL Create();

    //
    // Accessor thread calls this when done with thread (having told thread
    // to exit)
    //
    void WaitForClose(DWORD dwErrorTimeout = 10000);

    //
    // Returns TRUE if thread has been created and hasn't yet exited
    //
    BOOL ThreadExists();


    ///////////////////////////////////
    //
    // Functions for both master and worker
    //

    //
    // Get the priority
    //
    int GetPriority() const;

    //
    // Set the priority
    //
    BOOL SetPriority(int);

    //
    // Suspend a thread
    //
    DWORD Suspend();

    //
    // Resume a suspended thread
    //
    DWORD Resume();

    //
    // Force hard-termination of thread.  Used for critical failures.
    //
    BOOL Terminate(DWORD dwExitCode);


    ///////////////////////////////////
    //
    // Inter-thread communication
    //
    // Calls in either direction take place on the same "channel."
    // Seperate functions are specified to make indents obvious
    //

    //
    // Master: Signal the thread, and block for a response
    //
    DWORD CallWorker(DWORD, BOOL fBoostWorkerPriorityToMaster = TRUE);

    //
    // Master: Signal the thread, and block for a response
    //
    DWORD CallMaster(DWORD);

    //
    // Wait for the next request
    //
    DWORD WaitForCall();

    //
    // Is there a request?
    //
    BOOL PeekCall(DWORD * pParam = NULL);

    //
    // Reply to the request
    //
    void Reply(DWORD);

    //
    // If you want to do WaitForMultipleObjects you'll need to include
    // this handle in your wait list or you won't be responsive
    //
    HANDLE GetCallHandle();

    //
    // Find out what the request was
    //
    DWORD GetCallParam() const;

protected:

    enum
    {
        kInvalidCallParam = 0xffffffff
    };

    //
    // Thread will run this function on startup
    // must be supplied by derived class
    //
    virtual DWORD ThreadProc() = 0;

    cThreadLock m_Lock;

private:

    DWORD Call(DWORD, BOOL fBoost);

    //
    // Thread initially runs this. param is actually 'this'. function
    // just gets this and calls ThreadProc
    //
    static unsigned __stdcall InitialThreadProc(LPVOID pv);

    // make copy constructor and assignment operator inaccessible
    cWorkerThread(const cWorkerThread &);
    cWorkerThread &operator=(const cWorkerThread &);

    HANDLE          m_hThread;

    cThreadEvent    m_EventSend;
    cThreadEvent    m_EventComplete;

    DWORD           m_dwParam;
    DWORD           m_dwReturnVal;

};


///////////////////////////////////////////////////////////////////////////////
//
// Inline functions
//

#ifndef THRDTOOL_DEBUG

///////////////////////////////////////
//
// CLASS: cThreadLock
//

inline cThreadLock::cThreadLock()
{
    InitializeCriticalSection(&m_CritSec);
}

///////////////////////////////////////

inline cThreadLock::~cThreadLock()
{
    DeleteCriticalSection(&m_CritSec);
}

///////////////////////////////////////

inline void cThreadLock::Lock()
{
    EnterCriticalSection(&m_CritSec);
}

///////////////////////////////////////

inline void cThreadLock::Unlock()
{
    LeaveCriticalSection(&m_CritSec);
}

///////////////////////////////////////

inline BOOL cThreadLock::AssertOwnedByCurrentThread()
{
    return TRUE;
}

///////////////////////////////////////

inline void cThreadLock::SetTrace(BOOL)
{
}

#endif /* !THRDTOOL_DEBUG */

///////////////////////////////////////
//
// CLASS: cAutoLock
//

inline
cAutoLock::cAutoLock(cThreadLock & lock)
  : m_lock(lock)
{
    m_lock.Lock();
}

///////////////////////////////////////

inline
cAutoLock::~cAutoLock()
{
    m_lock.Unlock();
}

///////////////////////////////////////
//
// CLASS: cThreadSyncObject
//

inline cThreadSyncObject::cThreadSyncObject()
  : m_hSyncObject(NULL)
{
}

///////////////////////////////////////

inline cThreadSyncObject::~cThreadSyncObject()
{
    if (m_hSyncObject)
        Verify(CloseHandle(m_hSyncObject));
}

///////////////////////////////////////

inline BOOL cThreadSyncObject::operator!() const
{
    return !m_hSyncObject;
}

///////////////////////////////////////

#ifdef THRDTOOL_DEBUG
inline void cThreadSyncObject::AssertUseable()
{
    AssertMsg(m_hSyncObject, "Thread synchronization object is unuseable");
}
#endif

///////////////////////////////////////

inline cThreadSyncObject::operator HANDLE ()
{
    return m_hSyncObject;
}

///////////////////////////////////////

inline BOOL cThreadSyncObject::Wait(DWORD dwTimeout)
{
#ifdef THRDTOOL_DEBUG
    AssertUseable();
#endif
    return (WaitForSingleObject(m_hSyncObject, dwTimeout) == WAIT_OBJECT_0);
}

///////////////////////////////////////
//
// CLASS: cThreadEvent
//

inline BOOL cThreadEvent::Set()
{
#ifdef THRDTOOL_DEBUG
    AssertUseable();
#endif
    return SetEvent(m_hSyncObject);
}

///////////////////////////////////////

inline BOOL cThreadEvent::Reset()
{
#ifdef THRDTOOL_DEBUG
    AssertUseable();
#endif
    return ResetEvent(m_hSyncObject);
}

///////////////////////////////////////

inline BOOL cThreadEvent::Pulse()
{
#ifdef THRDTOOL_DEBUG
    AssertUseable();
#endif
    return PulseEvent(m_hSyncObject);
}

///////////////////////////////////////

inline BOOL cThreadEvent::Check()
{
#ifdef THRDTOOL_DEBUG
    AssertUseable();
#endif
    return Wait(0);
}

///////////////////////////////////////
//
// CLASS: cWorkerThread
//

inline cWorkerThread::cWorkerThread()
  : m_EventSend(TRUE),                           // must be manual-reset for PeekCall()
    m_hThread(NULL)
{
}

///////////////////////////////////////

inline int cWorkerThread::GetPriority() const
{
    return GetThreadPriority(m_hThread);
}

///////////////////////////////////////

inline BOOL cWorkerThread::SetPriority(int priority)
{
    return SetThreadPriority(m_hThread, priority);
}

///////////////////////////////////////

inline DWORD cWorkerThread::Suspend()
{
    return SuspendThread(m_hThread);
}

///////////////////////////////////////

inline DWORD cWorkerThread::Resume()
{
    return ResumeThread(m_hThread);
}

///////////////////////////////////////

inline BOOL cWorkerThread::Terminate(DWORD dwExitCode)
{
    return TerminateThread(m_hThread, dwExitCode);
}

///////////////////////////////////////

inline DWORD cWorkerThread::CallWorker(DWORD dw, BOOL fBoostWorkerPriorityToMaster)
{
    return Call(dw, fBoostWorkerPriorityToMaster);
}

///////////////////////////////////////

inline DWORD cWorkerThread::CallMaster(DWORD dw)
{
    return Call(dw, FALSE);
}

///////////////////////////////////////

inline HANDLE cWorkerThread::GetCallHandle()
{
    return m_EventSend;
}

///////////////////////////////////////

inline DWORD cWorkerThread::GetCallParam() const
{
    return m_dwParam;
}

///////////////////////////////////////

#else

class cThreadLock
{
public:
    cThreadLock()   {};
    ~cThreadLock()  {};
    void Lock()     {};
    void Unlock()   {};
    BOOL AssertOwnedByCurrentThread() { return TRUE;}
    void SetTrace(BOOL) {};
};

class cAutoLock
{
public:
    cAutoLock(cThreadLock & lock)   {};
    ~cAutoLock()                    {};
};

#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__THRDTOOL_H */
