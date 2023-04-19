///////////////////////////////////////////////////////////////////////////////
// $Source: x:/prj/tech/libsrc/lg/RCS/timings.h $
// $Author: kate $
// $Date: 1998/05/22 14:17:32 $
// $Revision: 1.8 $
//

#ifndef __TIMINGS_H
#define __TIMINGS_H

///////////////////////////////////////////////////////////////////////////////
//
// Performance Timing Classes
// ==========================
//
// KINDS
// -----
// There are two kinds of timer class:
//
//     Diff:               Low precision point-to-point timing...
//                             start
//                             how long since start
//                             how long since last
//                             how long since last
//                             .
//                             .
//                             .
//                             stop, how long since start
//
//     Average:            Timing of loop or multiple-use code from outside...
//                             start
//                             begin loop
//                                 mark iteration
//                             stop, what's average?
//
// USAGE
// -----
//
// The classes here may be used directly, but macros are provided for
// convenience and quick enable/disable.
//
// Single instances
// ----------------
//
// While the classes cDiffTimer and cAverageTimer may be used
// directly, the accepted usage is through macros for conditional
// compilation on PROFILE_ON:
//
//     DECLARE_TIMER(name, kind)
//
// where kind is Diff or Average.
//
// Example usage:
//     //...
//     DECLARE_TIMER(MyTimer, Average);
//     TIMER_Start(MyTimer);
//     for (;;)
//         {
//         TIMER_Mark(MyTimer);
//         if (TodayIsWednesday())
//             break;
//         }
//     TIMER_Stop_(MyTimer, "Done example timer");
//
// If PROFILE_ON is defined, this declares a timer with the identifier
// name, sets the name string. Use the timer class instance.
//
// A special macro/class combination provides automatic average
// support:
//
//     DECLARE_AUTO_TIMER(name)
//
// If PROFILE_ON is not defined, this declares nothing
//
// Example usage:
//     //...
//     foo()
//     {
//         DECLARE_AUTO_TIMER(FooTimer);
//         //...
//     }
//
// Timer sets
// ----------
//
// A timer set is a (suprise) collection of timers. While the
// cPerformanceTimerSet template can be used directly, the
// accepted usage is through macros for conditional compilation
// on PROFILE_ON.
//
//     DECLARE_TSET(name, kind, number)
//
// Example usage:
//     //...
//     DECLARE_TSET(MySet, Average, 3);
//     TSET_ItemStart(MySet, 2);
//     for (;;)
//         {
//         TSET_ItemMark(MySet, 2);
//         if (TodayIsWednesday())
//             break;
//         }
//     TSET_ItemStop_(MySet, 2, "Done example timer");
//
// --------------------------------------------
// @Note (toml 07-01-97): To add: 1) verbosity levels, 2) global clear, enable, quiet
//


///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <mprintf.h>

#ifdef _WIN32
EXTERN unsigned long __declspec(dllimport) __stdcall timeGetTime(void);
#define TimerRawTimeValue() timeGetTime()
#else
#define TimerRawTimeValue() 1L
#endif

///////////////////////////////////////////////////////////////////////////////

#ifdef PROFILE_ON
    #pragma message ("Profiling enabled...")

    #define DECLARE_TIMER(name, kind) \
        static c##kind##Timer name ( #name )

    #define DECLARE_TSET(name, kind, number) \
        static cTimerSet < c##kind##Timer , number > name (#kind " timer set " #name)

    #define DECLARE_AUTO_TIMER(name) \
        static cAverageTimer name ( #name ); \
        cAverageTimerMarker name##Marker (name)

    #define AUTO_TIMER(timer)                         cAverageTimerMarker timer##Marker (timer)

    #define TIMER_SetName(timer, name)                timer.SetName(name)
    #define TIMER_GetName(timer)                      timer.GetName()
    #define TIMER_IsActive(timer)                     timer.IsActive()
    #define TIMER_GetResult(timer)                    timer.GetResult()
    #define TIMER_OutputResult(timer)                 timer.OutputResult()
    #define TIMER_Clear(timer)                        timer.Clear()
    #define TIMER_Start(timer)                        timer.Start()
    #define TIMER_Mark(timer)                         timer.Mark()
    #define TIMER_Mark_(timer, comment)               timer.Mark(comment)
    #define TIMER_Stop(timer)                         timer.Stop()
    #define TIMER_Stop_(timer, comment)               timer.Stop(comment)
    #define TIMER_MarkStop(timer)                     timer.MarkStop()
    #define TIMER_MarkStop_(timer, comment)           timer.MarkStop(comment)

    #define TSET_SetName(set, name)                   set.SetName(name)
    #define TSET_GetName(set)                         set.GetName()
    #define TSET_Size(set)                            set.Size()
    #define TSET_StopAll(set)                         set.StopAll()
    #define TSET_ClearAll(set)                        set.ClearAll()
    #define TSET_OutputResultAll(set)                 set.OutputResultAll()
    #define TSET_GetSumResult()                       set.GetSumResult()
    #define TSET_OutputSumResult()                    set.OutputSumResult()
    #define TSET_TimerSetName(set, timer, name)       set[timer].SetName(name)
    #define TSET_TimerGetName(set, timer)             set[timer].GetName()
    #define TSET_TimerIsActive(set, timer)            set[timer].IsActive()
    #define TSET_TimerGetResult(set, timer)           set[timer].GetResult()
    #define TSET_TimerOutputResult(set, timer)        set[timer].OutputResult()
    #define TSET_TimerClear(set, timer)               set[timer].Clear()
    #define TSET_TimerStart(set, timer)               set[timer].Start()
    #define TSET_TimerMark(set, timer)                set[timer].Mark()
    #define TSET_TimerMark_(set, timer, comment)      set[timer].Mark(comment)
    #define TSET_TimerStop(set, timer)                set[timer].Stop()
    #define TSET_TimerStop_(set, timer, comment)      set[timer].Stop(comment)
    #define TSET_TimerMarkStop(set, timer)            set[timer].MarkStop()
    #define TSET_TimerMarkStop_(set, timer, comment)  set[timer].MarkStop(comment)

#else

    #define DECLARE_TIMER(name, kind)
    #define DECLARE_TSET(name, kind, number)
    #define DECLARE_AUTO_TIMER(name)
    
    #define AUTO_TIMER(timer)

    #define TIMER_SetName(timer, name)
    #define TIMER_GetName(timer)                    ""
    #define TIMER_IsActive(timer)                   FALSE
    #define TIMER_GetResult(timer)                  (-1.0)
    #define TIMER_OutputResult(timer)
    #define TIMER_Clear(timer)
    #define TIMER_Start(timer)
    #define TIMER_Mark(timer)
    #define TIMER_Mark_(timer, comment)
    #define TIMER_Stop(timer)
    #define TIMER_Stop_(timer, comment)
    #define TIMER_MarkStop(timer)
    #define TIMER_MarkStop_(timer, comment)

    #define TSET_SetName(set, name)
    #define TSET_GetName(set)                       ""
    #define TSET_Size(set)                          0
    #define TSET_StopAll(set)
    #define TSET_ClearAll(set)
    #define TSET_TimerGetResult(set, timer)         (-1.0)
    #define TSET_OutputResultAll(set)
    #define TSET_GetSumResult()                     (-1.0)
    #define TSET_OutputSumResult()
    #define TSET_TimerSetName(set, timer, name)
    #define TSET_TimerGetName(set, timer)           ""
    #define TSET_TimerIsActive(set, timer)          FALSE
    #define TSET_TimerOutputResult(set, timer)
    #define TSET_TimerClear(set, timer)
    #define TSET_TimerStart(set, timer)
    #define TSET_TimerMark(set, timer)
    #define TSET_TimerMark_(set, timer, comment)
    #define TSET_TimerStop(set, timer)
    #define TSET_TimerStop_(set, timer, comment)
    #define TSET_TimerMarkStop(set, timer)
    #define TSET_TimerMarkStop_(set, timer, comment)

#endif

#ifndef SHIP

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTimerBase
//

class cTimerBase
{
public:
    void         SetName(const char * p);
    const char * GetName() const;
    BOOL         IsActive() const;

protected:
    cTimerBase();
    cTimerBase(const char * p);

    enum eFlags
    {
        kActive = 0x01
    };

    char m_szName[64];
    BOOL m_flags;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSimpleTimer
//

class cSimpleTimer : public cTimerBase
{
public:
    cSimpleTimer();
    cSimpleTimer(const char * p);
    ~cSimpleTimer();

    double GetResult();
    void   OutputResult();

    void Clear();
    void Start();
    void Stop();

private:
    unsigned long m_StartTime;
    unsigned long m_TotalTime;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDiffTimer
//

class cDiffTimer : public cTimerBase
{
public:
    cDiffTimer();
    cDiffTimer(const char * p);
    ~cDiffTimer();

    double GetResult();
    void   OutputResult();

    void Clear();
    void Start();

    void Mark();
    void Mark(const char * pcszComment);

    void Stop(); // output status
    void Stop(const char * pcszComment);

private:
    unsigned long m_StartTime;
    unsigned long m_LastTime;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAverageTimer
//

class cAverageTimer : public cTimerBase
{
public:
    cAverageTimer();
    cAverageTimer(const char * p);
    ~cAverageTimer();

    ulong GetIters();
    ulong GetTotalTime();
    ulong GetMinTime();
    ulong GetMaxTime();

    double GetResult();
    void   OutputResult();

    void Clear();
    void Start();

    void Mark();

    void Stop();
    void Stop(const char * pcszComment);

    void MarkStop();
    void MarkStop(const char * pcszComment);

private:
    unsigned long m_StartTime;
    unsigned long m_TotalTime;
    unsigned long m_Iterations;
    unsigned long m_Depth;
    unsigned long m_MinTime;
    unsigned long m_MaxTime;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAverageTimerMarker
//

class cAverageTimerMarker
{
public:
    cAverageTimerMarker(cAverageTimer &);
    ~cAverageTimerMarker();

private:
    cAverageTimer & m_AverageTimer;
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTimerSet
//

template <class TIMER_TYPE, int SIZE>
class cTimerSet
{
public:
    cTimerSet();
    cTimerSet(const char *);
    ~cTimerSet();

    int Size() const;

    void         SetName(const char * p);
    const char * GetName() const;

    TIMER_TYPE & operator[](int i);

    void StopAll();
    void ClearAll();
    void OutputResultAll();

    double GetSumResult();
    void   OutputSumResult();

private:
    TIMER_TYPE m_PerformanceTimers[SIZE];
    char       m_szName[64];
};

///////////////////////////////////////

template <int SIZE>
class cDiffTimerSet : public cTimerSet<cDiffTimer, SIZE>
{
};

///////////////////////////////////////

template <int SIZE>
class cAverageTimerSet : public cTimerSet<cAverageTimer, SIZE>
{
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTimerBase, inline functions
//

inline void cTimerBase::SetName(const char * p)
{
    strncpy(m_szName, p, sizeof(m_szName)-1);
    m_szName[sizeof(m_szName)-1] = 0;
}

///////////////////////////////////////

inline const char * cTimerBase::GetName() const
{
    return m_szName;
}

///////////////////////////////////////

inline BOOL cTimerBase::IsActive() const
{
    return m_flags & kActive;
}

///////////////////////////////////////

inline cTimerBase::cTimerBase()
  : m_flags(0)
{
    m_szName[0] = 0;
}

///////////////////////////////////////

inline cTimerBase::cTimerBase(const char * p)
  : m_flags(0)
{
    SetName(p);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cSimpleTimer, inline functions
//

inline cSimpleTimer::cSimpleTimer()
  : m_TotalTime(0)
{
}

///////////////////////////////////////

inline cSimpleTimer::cSimpleTimer(const char * p)
  : cTimerBase(p), m_TotalTime(0)
{
}

///////////////////////////////////////

inline double cSimpleTimer::GetResult()
{
    if (m_TotalTime && !IsActive())
        return  (double)(m_TotalTime);
    return -1.0;
}

///////////////////////////////////////

void SimpleTimerOutputResult(const char *, ulong);

inline void cSimpleTimer::OutputResult()
{
    if (m_TotalTime && !IsActive())
        SimpleTimerOutputResult(m_szName, m_TotalTime);
        mprintf("Total time for %s: %lu ms\n", m_szName, m_TotalTime);
}

///////////////////////////////////////

inline void cSimpleTimer::Clear()
{
    m_flags &= ~cTimerBase::kActive;
    m_TotalTime = 0L;
}

///////////////////////////////////////

inline void cSimpleTimer::Start()
{
    m_flags |= cTimerBase::kActive;
    m_StartTime = TimerRawTimeValue();
}

///////////////////////////////////////

inline void cSimpleTimer::Stop()
{
    unsigned long MarkTime = TimerRawTimeValue();
    m_TotalTime += MarkTime - m_StartTime;
    m_flags &= ~cTimerBase::kActive;
}

///////////////////////////////////////

inline cSimpleTimer::~cSimpleTimer()
{
    if (IsActive())
    {
        Stop();
        OutputResult();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cDiffTimer, inline functions
//

inline cDiffTimer::cDiffTimer()
  : m_LastTime(0)
{
}

///////////////////////////////////////

inline cDiffTimer::cDiffTimer(const char * p)
  : cTimerBase(p), m_LastTime(0)
{
}

///////////////////////////////////////

inline double cDiffTimer::GetResult()
{
    if (m_LastTime && !IsActive())
        return  (double)(m_LastTime - m_StartTime);
    return -1.0;
}

///////////////////////////////////////

void DiffTimerOutputResult(const char *, ulong);

inline void cDiffTimer::OutputResult()
{
    if (m_LastTime && !IsActive())
        DiffTimerOutputResult(m_szName, m_LastTime - m_StartTime);
}

///////////////////////////////////////

inline void cDiffTimer::Clear()
{
    m_flags &= ~cTimerBase::kActive;
    m_LastTime = 0L;
}

///////////////////////////////////////

inline void cDiffTimer::Start()
{
    m_flags |= cTimerBase::kActive;
    m_StartTime = m_LastTime = TimerRawTimeValue();
}

///////////////////////////////////////

inline void cDiffTimer::Mark()
{
    unsigned long MarkTime = TimerRawTimeValue();
    mprintf("%s: Time elapsed %lu ms (%lu ms)\n", m_szName, MarkTime - m_LastTime, MarkTime - m_StartTime);
    m_LastTime = TimerRawTimeValue();
}

///////////////////////////////////////

inline void cDiffTimer::Mark(const char * pcszComment)
{
    unsigned long MarkTime = TimerRawTimeValue();
    mprintf("%s: Time elapsed %lu ms (%lu ms); (%s)\n", m_szName, MarkTime - m_LastTime, MarkTime - m_StartTime, pcszComment);
    m_LastTime = TimerRawTimeValue();
}

///////////////////////////////////////

inline void cDiffTimer::Stop()
{
    unsigned long MarkTime = TimerRawTimeValue();
    mprintf("Stop %s: Time elapsed %lu ms (%lu ms)\n", m_szName, MarkTime - m_LastTime, MarkTime - m_StartTime);
    m_LastTime = MarkTime;
    m_flags &= ~cTimerBase::kActive;
}

///////////////////////////////////////

inline void cDiffTimer::Stop(const char * pcszComment)
{
    unsigned long MarkTime = TimerRawTimeValue();
    mprintf("Stop %s: Time elapsed %lu ms (%lu ms); (%s)\n", m_szName, MarkTime - m_LastTime, MarkTime - m_StartTime, pcszComment);
    m_LastTime = MarkTime;
    m_flags &= ~cTimerBase::kActive;
}

///////////////////////////////////////

inline cDiffTimer::~cDiffTimer()
{
    if (IsActive())
    {
        Stop();
        OutputResult();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAverageTimer, inline functions
//

inline cAverageTimer::cAverageTimer()
  : m_StartTime(0L), m_TotalTime(0L), m_Iterations(0L), m_Depth(0), m_MinTime(0xffffffffL), m_MaxTime(0L)
{
}

///////////////////////////////////////

inline cAverageTimer::cAverageTimer(const char * p)
  : cTimerBase(p), m_StartTime(0L), m_TotalTime(0L), m_Iterations(0L), m_Depth(0), m_MinTime(0xffffffffL), m_MaxTime(0L)
{
}

///////////////////////////////////////

inline ulong cAverageTimer::GetIters()
{
    return m_Iterations;
}

///////////////////////////////////////

inline ulong cAverageTimer::GetTotalTime()
{
    return m_TotalTime;
}

///////////////////////////////////////

inline ulong cAverageTimer::GetMinTime()
{
    return m_MinTime;
}

///////////////////////////////////////

inline ulong cAverageTimer::GetMaxTime()
{
    return m_MaxTime;
}

///////////////////////////////////////

inline double cAverageTimer::GetResult()
{
    if (m_Iterations && !IsActive())
        return double(m_TotalTime)/double(m_Iterations);
    return -1.0;
}

///////////////////////////////////////

void AverageTimerOutputResult(const char *, double, ulong, ulong, ulong, ulong);

inline void cAverageTimer::OutputResult()
{
    if (m_Iterations && !IsActive())
        AverageTimerOutputResult(m_szName, double(m_TotalTime)/double(m_Iterations), m_MaxTime, m_MinTime, m_TotalTime, m_Iterations);
}

///////////////////////////////////////

inline void cAverageTimer::Clear()
{
    m_flags = 0;
    m_Iterations = 0L;
    m_TotalTime = 0L;
    m_MinTime = 0xffffffffL;
    m_MaxTime = 0L;
}

///////////////////////////////////////

inline void cAverageTimer::Start()
{
    m_Depth++;
    if (m_Depth == 1)
    {
        m_flags |= cTimerBase::kActive;
        m_StartTime = TimerRawTimeValue();
    }
}

///////////////////////////////////////

inline void cAverageTimer::Mark()
{
    m_Iterations++;
}

///////////////////////////////////////

inline void cAverageTimer::Stop()
{
    m_Depth--;
    if (m_Depth == 0)
    {
        unsigned long delta=TimerRawTimeValue()-m_StartTime;
        m_TotalTime += delta;
        if(delta>m_MaxTime) m_MaxTime=delta;
        if(delta<m_MinTime) m_MinTime=delta;
        m_flags &= ~cTimerBase::kActive;
    }
}

///////////////////////////////////////

inline void cAverageTimer::Stop(const char * pcszComment)
{
    Stop();
    mprintf("Stop %s (%s)\n", m_szName, pcszComment);
}

///////////////////////////////////////

inline void cAverageTimer::MarkStop()
{
    Mark();
    Stop();
}

///////////////////////////////////////

inline void cAverageTimer::MarkStop(const char * pcszComment)
{
    Mark();
    Stop(pcszComment);
}

///////////////////////////////////////

inline cAverageTimer::~cAverageTimer()
{
    if (IsActive())
        Stop();

    OutputResult();
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAverageTimerMarker
//

inline cAverageTimerMarker::cAverageTimerMarker(cAverageTimer & timer)
  : m_AverageTimer(timer)
{
    m_AverageTimer.Start();
}

///////////////////////////////////////

inline cAverageTimerMarker::~cAverageTimerMarker()
{
    m_AverageTimer.MarkStop();
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cTimerSet, inline functions
//

template <class TIMER_TYPE, int NUM_TIMERS>
inline int cTimerSet<TIMER_TYPE, NUM_TIMERS>::Size() const
{
    return NUM_TIMERS;
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline void cTimerSet<TIMER_TYPE, NUM_TIMERS>::SetName(const char * p)
{
    strncpy(m_szName, p, sizeof(m_szName)-1);
    m_szName[sizeof(m_szName)-1] = 0;
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline const char * cTimerSet<TIMER_TYPE, NUM_TIMERS>::GetName() const
{
    return m_szName;
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline TIMER_TYPE & cTimerSet<TIMER_TYPE, NUM_TIMERS>::operator[](int i)
{
    return m_PerformanceTimers[i];
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline void cTimerSet<TIMER_TYPE, NUM_TIMERS>::StopAll()
{
    for (int i = 0; i < NUM_TIMERS; i++)
        m_PerformanceTimers[i].Stop();
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline void cTimerSet<TIMER_TYPE, NUM_TIMERS>::ClearAll()
{
    for (int i = 0; i < NUM_TIMERS; i++)
        m_PerformanceTimers[i].Clear();
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline void cTimerSet<TIMER_TYPE, NUM_TIMERS>::OutputResultAll()
{
    for (int i = 0; i < NUM_TIMERS; i++)
        m_PerformanceTimers[i].OutputResult();
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline double cTimerSet<TIMER_TYPE, NUM_TIMERS>::GetSumResult()
{
    double result;
    double sum = 0;

    for (int i = 0; i < NUM_TIMERS; i++)
    {
        result = m_PerformanceTimers[i].GetResult();
        if (result < 0)
            return -1.0;
        sum += result;
    }
    return sum;
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline void cTimerSet<TIMER_TYPE, NUM_TIMERS>::OutputSumResult()
{
    double sum = GetSumResult();
    if (sum > 0)
        mprintf("Sum time for %s: %g ms\n", m_szName, sum);
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline cTimerSet<TIMER_TYPE, NUM_TIMERS>::cTimerSet()
{
    for (int i = 0; i < Size(); i++)
    {
        sprintf(m_szName, "Timer %d", i);
        m_PerformanceTimers[i].SetName(m_szName);
    }
    m_szName[0] = 0;
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline cTimerSet<TIMER_TYPE, NUM_TIMERS>::cTimerSet(const char * p)
{
    for (int i = 0; i < Size(); i++)
    {
        sprintf(m_szName, "%s timer %d", p, i);
        m_PerformanceTimers[i].SetName(m_szName);
    }
    SetName(p);
}

///////////////////////////////////////

template <class TIMER_TYPE, int NUM_TIMERS>
inline cTimerSet<TIMER_TYPE, NUM_TIMERS>::~cTimerSet()
{
    StopAll();
}

///////////////////////////////////////////////////////////////////////////////
#endif /* !SHIP */

#endif /* !__TIMINGS_H */
