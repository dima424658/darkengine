///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/cpptools/RCS/fixedstk.h $
// $Author: TOML $
// $Date: 1997/09/22 22:29:29 $
// $Revision: 1.1 $
//
// Fixed size stacks of fixed size things...
//
// Very simple overflowing circular stack structures, with thread-safe and
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

#ifndef __FIXEDSTK_H
#define __FIXEDSTK_H

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cFixedStack
//
// A simple stack for things like device managers, network handling, etc.
//

template <class T, unsigned kMaxElems>
class cFixedStack
{
    // A macro is used for index advancement to _ensure_ this
    // is inlined -- Watcom sometimes decides not to inline
    // this tiny code fragment!

    #define cFixedStack_Advance(index) \
    { \
        register unsigned temp = index; \
        temp++; \
        temp &= unsigned(kMaxElems - 1); \
        index = temp; \
    }

    #define cFixedStack_Retreat(index) \
    { \
        register unsigned temp = index; \
        temp--; \
        temp &= unsigned(kMaxElems - 1); \
        index = temp; \
    }

public:
    cFixedStack()
    {
        #ifdef DEBUG
        unsigned test = ((kMaxElems) & (kMaxElems - 1));
        AssertMsg(test == 0, "Must be power of 2");
        #endif
        m_iNext = m_iBottom = 0;
    }

    //
    // Queries for stack state
    //
    BOOL IsEmpty()
    {
        return (m_iNext == m_iBottom);
    }

    BOOL IsFull()
    {
        return (((m_iNext + 1) & unsigned(kMaxElems - 1)) == m_iBottom);
    }

    unsigned GetSize()
    {
        return kMaxElems;
    }

    //
    // Empty the stack
    //
    void Flush()
    {
        m_iNext = 0;
        m_iBottom = 0;
    }

    //
    // Push to the stack, discarding oldest if overflown.  TRUE is no overflow
    //
    BOOL Push(const T * pPush)
    {
        const BOOL fOverflow = IsFull();

        // If overflowing, discard oldest stack item...
        if (fOverflow)
        {
            cFixedStack_Advance(m_iBottom);
        }

        m_Entries[m_iNext] = *pPush;

        cFixedStack_Advance(m_iNext);

        return fOverflow;
    }

    //
    // Push to the stack, getting pointer to added item.
    // TRUE is no overflow
    //
    BOOL Push(const T * pPush, const T ** ppPushed)
    {
        const BOOL fOverflow = IsFull();

        if (fOverflow)
        {
            cFixedStack_Advance(m_iBottom);
        }

        *ppPushed = &m_Entries[m_iNext];
        m_Entries[m_iNext] = *pPush;

        cFixedStack_Advance(m_iNext);

        return fOverflow;
    }

    //
    // Push to the stack, discarding if overflown.  TRUE is no overflow
    //
    BOOL PushUnlessFull(const T * pPush)
    {
        if (!IsFull())
        {
            m_Entries[m_iNext] = *pPush;
            cFixedStack_Advance(m_iNext);
            return TRUE;
        }
        return FALSE;
    }

    //
    // Push to the stack, getting pointer to added item.
    // TRUE is no overflow
    //
    BOOL PushUnlessFull(const T * pPush, const T ** ppPushed)
    {
        if (!IsFull())
        {
            *ppPushed = &m_Entries[m_iNext];
            m_Entries[m_iNext] = *pPush;
            cFixedStack_Advance(m_iNext);
            return TRUE;
        }
        *ppPushed = NULL;
        return FALSE;
    }

    //
    // Get the next available item from the stack. TRUE if
    // there is any.
    //
    BOOL Pop(T * pReturn)
    {
        if (!IsEmpty())
        {
            cFixedStack_Retreat(m_iNext);
            *pReturn = m_Entries[m_iNext];
            return TRUE;
        }
        return FALSE;
    }

    //
    // Check for the next available item from the stack. TRUE if
    // there is any.
    //
    BOOL Peek(T * pReturn)
    {
        if (!IsEmpty())
        {
            *pReturn = m_Entries[((m_iNext - 1) & unsigned(kMaxElems - 1))];
            return TRUE;
        }
        return FALSE;
    }

    //
    // Check for the next available item from the stack.
    // Results are undefined if stack is actually empty
    //
    const T & Peek()
    {
        return m_Entries[((m_iNext - 1) & unsigned(kMaxElems - 1))];
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
    unsigned    m_iNext;
    unsigned    m_iBottom;

    T           m_Entries[kMaxElems];

    #undef cFixedStack_Advance
    #undef cFixedStack_Retreat
};

///////////////////////////////////////////////////////////////////////////////
//
// TEMPLATE: cFixedMTStack
//
// A thread-safe version of cFixedStack
//

#ifdef __THRDTOOL_H

template <class T, unsigned kMaxElems>
class cFixedMTStack : private cFixedStack<T, kMaxElems>
{
public:
    BOOL IsEmpty()
    {
        cAutoLock lock(m_Lock);
        return cFixedStack<T, kMaxElems>::IsEmpty();
    }

    BOOL IsFull()
    {
        cAutoLock lock(m_Lock);
        return cFixedStack<T, kMaxElems>::IsFull();
    }

    unsigned GetSize()
    {
        return cFixedStack<T, kMaxElems>::GetSize();
    }

    void Flush()
    {
        cAutoLock lock(m_Lock);
        cFixedStack<T, kMaxElems>::Flush();
    }

    BOOL Push(const T * pPush)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedStack<T, kMaxElems>::Push(pPush);
    }

    BOOL Push(const T * pPush, const T ** ppPushed)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedStack<T, kMaxElems>::Push(pPush, ppPushed);
    }

    BOOL PushUnlessFull(const T * pPush)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedStack<T, kMaxElems>::PushUnlessFull(pPush);
    }

    BOOL PushUnlessFull(const T * pPush, const T ** ppPushed)
    {
        cAutoLock lock(m_Lock);
        if (IsEmpty())
            m_AvailabilitySignal.Set();
        return cFixedStack<T, kMaxElems>::PushUnlessFull(pPush, ppPushed);
    }

    BOOL Pop(T * pReturn)
    {
        cAutoLock lock(m_Lock);
        BOOL result = cFixedStack<T, kMaxElems>::Pop(pReturn);
        if (IsEmpty())
            m_AvailabilitySignal.Reset();
        return result;
    }

    BOOL Peek(T * pReturn)
    {
        cAutoLock lock(m_Lock);
        return cFixedStack<T, kMaxElems>::Peek(pReturn);
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

#endif /* !__FIXEDSTK_H */
