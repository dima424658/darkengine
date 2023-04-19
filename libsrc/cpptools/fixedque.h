///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/fixedque.h $
// $Author: TOML $
// $Date: 1997/09/22 22:29:35 $
// $Revision: 1.6 $
//
// Fixed size queues of fixed size things...
//
// Very simple overflowing circular queue structures, with thread-safe and
// normal versions.  Designed with systems-level stuff in mind -- Not meant for
// use with high copy overhead -- or any complex use really.  Input devices,
// network protocols, multithreaded communication...
//
// Not a lot of error checking!
//
// Tries to remain consistent if in interrupt situation, but not verified! Make
// sure to declare instance volatile if using under interrupt
//
// Size MUST be a power of 2, because we implement the wrap-around logic
// (index = (index + 1) % kMaxElems) with only powers of 2 in mind
// (index = (index + 1) & (kMaxElems - 1)) for performance
//
// We use temporaries so that values are never totally invalid (i.e., we can never
// be interrrupted or preempted with either insert or remove index pointing
// off the end of the array)
//

#ifndef __FIXEDQUE_H
#define __FIXEDQUE_H

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cFixedQueue
//
// A simple queue for things like device managers, network handling, etc.
//

template <class T, unsigned kMaxElems>
class cFixedQueue
{
    // A macro is used for index advancement to _ensure_ this
    // is inlined -- Watcom sometimes decides not to inline
    // this tiny code fragment!

    #define cFixedQueue_Advance(index) \
    { \
        unsigned temp = index; \
        temp++; \
        temp &= unsigned(kMaxElems - 1); \
        index = temp; \
    }

public:
    cFixedQueue()
    {
        #ifdef DEBUG
        unsigned test = ((kMaxElems) & (kMaxElems - 1));
        AssertMsg(test == 0, "Must be power of 2");
        #endif
        m_iRemove = m_iInsert = 0;
    }

    //
    // Queries for queue state
    //
    BOOL IsEmpty()
    {
        return (m_iRemove == m_iInsert);
    }

    BOOL IsFull()
    {
        return (((m_iInsert + 1) & unsigned(kMaxElems - 1)) == m_iRemove);
    }

    unsigned GetSize()
    {
        return kMaxElems;
    }

    //
    // Empty the queue
    //
    void Flush()
    {
        m_iRemove = 0;
        m_iInsert = 0;
    }

    //
    // Add to the queue, discarding oldest if overflown.  TRUE is no overflow
    //
    BOOL Add(const T * pAdd)
    {
        const BOOL fOverflow = IsFull();

        // If overflowing, discard oldest queue item...
        if (fOverflow)
        {
            cFixedQueue_Advance(m_iRemove);
        }

        m_Entries[m_iInsert] = *pAdd;

        cFixedQueue_Advance(m_iInsert);

        return fOverflow;
    }

    //
    // Add to the queue, getting pointer to added item.
    // TRUE is no overflow
    //
    BOOL Add(const T * pAdd, const T ** ppAdded)
    {
        const BOOL fOverflow = IsFull();

        if (fOverflow)
        {
            cFixedQueue_Advance(m_iRemove);
        }

        *ppAdded = &m_Entries[m_iInsert];
        m_Entries[m_iInsert] = *pAdd;

        cFixedQueue_Advance(m_iInsert);

        return fOverflow;
    }

    //
    // Add to the queue, discarding if overflown.  TRUE is no overflow
    //
    BOOL AddUnlessFull(const T * pAdd)
    {
        if (!IsFull())
        {
            m_Entries[m_iInsert] = *pAdd;
            cFixedQueue_Advance(m_iInsert);
            return TRUE;
        }
        return FALSE;
    }

    //
    // Add to the queue, getting pointer to added item.
    // TRUE is no overflow
    //
    BOOL AddUnlessFull(const T * pAdd, const T ** ppAdded)
    {
        if (!IsFull())
        {
            *ppAdded = &m_Entries[m_iInsert];
            m_Entries[m_iInsert] = *pAdd;
            cFixedQueue_Advance(m_iInsert);
            return TRUE;
        }
        *ppAdded = NULL;
        return FALSE;
    }

    //
    // Get the next available item from the queue. TRUE if
    // there is any.
    //
    BOOL GetNext(T * pReturn)
    {
        if (!IsEmpty())
        {
            *pReturn = m_Entries[m_iRemove];
            cFixedQueue_Advance(m_iRemove);
            return TRUE;
        }
        return FALSE;
    }

    //
    // Check for the next available item from the queue. TRUE if
    // there is any.
    //
    BOOL PeekNext(T * pReturn)
    {
        if (!IsEmpty())
        {
            *pReturn = m_Entries[m_iRemove];
            return TRUE;
        }
        return FALSE;
    }

    //
    // Check for the next available item from the stack.
    // Results are undefined if queue is actually empty
    //
    const T & PeekNext()
    {
        return m_Entries[m_iRemove];
    }

    //
    // Threading support
    //
    BOOL WaitForData(ulong msecTimeout = 0)
    {
        return !IsEmpty();
    }
    HANDLE GetAvailabilitySignalHandle()    { return NULL; }
    void Lock()                             {}
    void Unlock()                           {}


private:
    unsigned    m_iRemove;
    unsigned    m_iInsert;

    T           m_Entries[kMaxElems];

    #undef cFixedQueue_Advance
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cFixedMTQueue
//
// A thread-safe version of cFixedQueue
//

#ifdef __THRDTOOL_H

template <class T, unsigned kMaxElems>
class cFixedMTQueue : private cFixedQueue<T, kMaxElems>
{
public:
    BOOL IsEmpty()
    {
        cAutoLock lock(m_Lock);
        return cFixedQueue<T, kMaxElems>::IsEmpty();
    }

    BOOL IsFull()
    {
        cAutoLock lock(m_Lock);
        return cFixedQueue<T, kMaxElems>::IsFull();
    }

    unsigned GetSize()
    {
        return cFixedQueue<T, kMaxElems>::GetSize();
    }

    void Flush()
    {
        cAutoLock lock(m_Lock);
        cFixedQueue<T, kMaxElems>::Flush();
    }

    BOOL Add(const T * pAdd)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedQueue<T, kMaxElems>::Add(pAdd);
    }

    BOOL Add(const T * pAdd, const T ** ppAdded)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedQueue<T, kMaxElems>::Add(pAdd, ppAdded);
    }

    BOOL AddUnlessFull(const T * pAdd)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedQueue<T, kMaxElems>::AddUnlessFull(pAdd);
    }

    BOOL AddUnlessFull(const T * pAdd, const T ** ppAdded)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedQueue<T, kMaxElems>::AddUnlessFull(pAdd, ppAdded);
    }

    BOOL GetNext(T * pReturn)
    {
        cAutoLock lock(m_Lock);
        BOOL result = cFixedQueue<T, kMaxElems>::GetNext(pReturn);
        if (IsEmpty())
            m_AvailabilitySignal.Reset();
        return result;
    }

    BOOL PeekNext(T * pReturn)
    {
        cAutoLock lock(m_Lock);
        return cFixedQueue<T, kMaxElems>::PeekNext(pReturn);
    }

    BOOL WaitForData(ulong msecTimeout = INFINITE)
    {
        return m_AvailabilitySignal.Wait(msecTimeout);
    }

    HANDLE GetAvailabilitySignalHandle()
    {
        return m_AvailabilitySignal;
    }

    void Lock()
    {
        m_Lock.Lock();
    }

    void Unlock()
    {
        m_Lock.Unlock();
    }

private:
    cThreadLock  m_Lock;
    cThreadEvent m_AvailabilitySignal;
};

#endif

///////////////////////////////////////////////////////////////////////////////

#endif /* !__FIXEDQUE_H */
