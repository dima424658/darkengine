/*
 *  Portable source level profiling
 *
 *    To port, update the macro RAW_TIME_VALUE
 *    which is used to get timer values.
 *
 *
 *  Profiling requires three steps:
 *
 *    Instrumenting the source code to be profiled
 *    Adding runtime invocation of the profiling
 *       analysis routines
 *    Setting preprocessor flags to the appropriate
 *       profiling configuration
 *
 *  (1) Code Instrumentation
 *
 *    Example usage:
 *
 *        int foobar(void)
 *        {  PROF
 *           int x;
 *           x = bozbar();
 *           if (x)
 *              RETURN_PROF(x);
 *           bozbar();
 *           RETURN_PROF(-1);
 *        }
 *
 *        void bozbar(void)
 *        {  PROF
 *           ...
 *           END_PROF;
 *        }
 *
 *    PROF
 *       Must appear in every routine to be profiled, in
 *       the declaration section.  Generally should be first
 *       so initialization of automatics will be profiled.
 *       No trailing ; because otherwise I couldn't let it
 *       become null when profiling is turned off.
 *
 *    END_PROF
 *       Marks the end of the routine for the profiler.
 *       Must appear before every possible path that exits
 *       from the routine (or use one of the below).
 *
 *    RETURN_PROF(val)
 *       Marks the end of the routine and returns a value.
 *         i.e.  { END_PROF; return (val); }
 *
 *    EXIT_PROF
 *       Marks the end of a void routine and returns.
 *         i.e.  { END_PROF; return; }
 *
 *
 *  (2) Run-time interface
 *
 *    initProfiler(void)
 *       Performs various initializations
 *
 *    writeProfile(char *filename)
 *       Write out profile samples to the file
 *
 *  (3) Configuration
 *
 *    The profiler is enabled and disabled by setting a number
 *    of preprocessor flags.  All of them require recompilation
 *    of all affected source code, so that unused features do
 *    not affect performance.
 *
 *    PROFILE_ON
 *       This enables profiling.  With no other flags, the default
 *       profiling only records cumulative execution times (time
 *       spent in descendents accumulates to each module).
 *
 *    PROFILE_SELF
 *       Record how much time is actually spent in a given routine
 *       without counting descendents.
 *
 *    PROFILE_CHILD
 *       Figure out exactly how much time is spent in each child
 *       of a module.  Implies PROFILE_SELF.  Adds a lot of overhead
 *       for every PROF / END_PROF pair.
 *
 *  $Header: x:/prj/tech/libsrc/prof/RCS/prof.h 1.7 1997/01/28 11:55:38 MAT Exp $
 *  $Log: prof.h $
 * Revision 1.7  1997/01/28  11:55:38  MAT
 * relocated trailing #endif to end of file
 * /
 * 
 * Revision 1.6  1997/01/27  16:19:07  TOML
 * changed time function to use higher resolution timer
 * 
 * Revision 1.5  1996/07/19  14:08:16  TOML
 * Added extern "C" for C++
 *
 * Revision 1.4  1996/07/16  13:15:38  TOML
 * Added Windows support
 *
 * Revision 1.3  1995/03/07  11:07:38  buzzard
 * *** empty log message ***
 *
 * Revision 1.2  1995/01/17  07:54:58  buzzard
 * PROFILE_CHILD support
 *
 * Revision 1.1  1995/01/16  14:12:37  buzzard
 * Initial revision
 *
 */

#ifdef __cplusplus
extern "C"  {
#endif

#ifndef _WIN32

    #define RAW_TIME_VALUE     tm_get_millisec()

    #ifndef _TIMER_H
    #include <timer.h>
    #endif

#else

    #define RAW_TIME_VALUE     timeGetTime()

    #ifndef _INC_WINDOWS
    __declspec(dllimport) DWORD __stdcall timeGetTime(void);
    #endif

#endif

#ifdef PROFILE_ON

  // maintain invariant of PROFILE_CHILD implying PROFILE_SELF

  #ifdef PROFILE_CHILD
    #ifndef PROFILE_SELF
    #define PROFILE_SELF
    #endif
  #endif

  // determine whether we need the child stack wackiness

  #ifdef PROFILE_SELF
    #define PROFILE_STACK
  #endif

#else

#endif



enum {
  PROFILE_MODE_NONE,
  PROFILE_MODE_BASIC,
  PROFILE_MODE_SELF,
  PROFILE_MODE_CHILD
};

// Yay ANSI

void initProfiler(int);

#ifndef PROFILE_PROTOTYPES

  #ifdef PROFILE_ON
    #if defined(PROFILE_CHILD)
      #define initProfiler()    initProfiler(PROFILE_MODE_CHILD)
    #elif defined(PROFILE_SELF)
      #define initProfiler()    initProfiler(PROFILE_MODE_SELF)
    #else
      #define initProfiler()    initProfiler(PROFILE_MODE_BASIC)
    #endif
  #else
    #define initProfiler()      initProfiler(PROFILE_MODE_NONE)
  #endif

#endif // PROFILE_PROTOTYPES

void writeProfile(char *name);
void profileClearData(void);

typedef struct {
   long time;
   long count;
   long child_time;
} lPfData;
long profileRecordSource(char *name, int line, lPfData *profile);

typedef struct {
   ushort hashCode;
   ushort parentHash;
   long *time_ptr;
} lPfDataHash;
long profileRecordHashInit(char *name, int line, lPfDataHash *profile);
long profileRecordHash(lPfDataHash *profile);

// The meat of the implementation

#ifndef PROFILE_ON

  #define PROF
  #define END_PROF
  #define RETURN_PROF(x)   return (x)
  #define EXIT_PROF        return

#else

  #define PROF_RECORD(x) profileRecordSource(__FILE__, __LINE__, &x)
  #define PROF_REC_HASH(x) profileRecordHash(__FILE__, __LINE__, &x)

  #define PROF_DECLARE_BASE      \
     static lPfData lPfInfo;

  #define PROF_REGISTER_AND_TIME \
     (lPfInfo.count++ ? RAW_TIME_VALUE : PROF_RECORD(lPfInfo))

  #if !defined(PROFILE_SELF)

    // the simple one

    #define PROF                 \
      PROF_DECLARE_BASE          \
      long lPfTime = PROF_REGISTER_AND_TIME;

    #define END_PROF             \
      (lPfInfo.time += RAW_TIME_VALUE - lPfTime)

  #elif !defined(PROFILE_CHILD)

    // profile "self"
    //   To do this, we have to subtract out time accumulated to us
    //   from our parent.  To do this, we have to keep a stack on which
    //   we have a pointer to our parent's time.  We subtract out our time from it
    //   when we're done.  The fact that this works even up multiple
    //   levels is just a hack.  Really it's just luck, although I could've
    //   made a more sophisticated system if it hadn't.

    extern long **lPfStack;

    #define PROF                 \
      PROF_DECLARE_BASE          \
      long lPfTime = (*++lPfStack = &lPfInfo.child_time, PROF_REGISTER_AND_TIME);

    #define END_PROF                       \
      (lPfTime = RAW_TIME_VALUE - lPfTime, \
       lPfInfo.time += lPfTime,            \
       **--lPfStack += lPfTime)

  #else   //  PROFILE_CHILD

     // We push most of the work off into a function call for this one

     extern ushort *lPfHashStack;

     #define PROF_FIRST_TIME              \
       profileRecordHashInit(__FILE__, __LINE__, &lPfInfo)
     #define PROF_OTHER_TIMES             \
       profileRecordHash(&lPfInfo)

     #define PROF                         \
       static lPfDataHash lPfInfo;        \
       static uchar lPfInit;              \
       long lPfTime = (lPfInit ? PROF_OTHER_TIMES : (lPfInit=1, PROF_FIRST_TIME));

     #define END_PROF                        \
       (lPfTime = RAW_TIME_VALUE - lPfTime,  \
        --lPfHashStack,                      \
        *lPfInfo.time_ptr += lPfTime)

  #endif


  #define RETURN_PROF(x)           \
    if (1) { END_PROF; return (x); } else

  #define EXIT_PROF                \
    if (1) { END_PROF; return; } else

#endif

#ifdef __cplusplus
}
#endif

