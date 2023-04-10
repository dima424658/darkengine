/*
 * $Header: x:/prj/tech/libsrc/timer/RCS/tmwin32.c 1.11 1997/06/18 02:13:36 TOML Exp $
 *
 */

#ifdef __WATCOMC__
#pragma message("**************************")
#pragma message("*** Ignore warning 203 ***")
#pragma message("**************************")
#endif

#ifdef _WIN32
#include <windows.h>

#ifndef __INTTYPES_H
#include <inttype.h>
#endif

#include <tmdecl.h>
#include <tmftab.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <gshelapi.h>
#include <recapi.h>

static int          g_TimerInitCount;
static IGameShell * g_pTmGameShell;
static IRecorder *  g_pTmRecorder;

EXTERN long LGAPI tm_get_millisec_unrecorded(void)
{
    AssertMsg(g_pTmGameShell, "Must call tm_init to use tm_get_millisec");

    if (g_pTmGameShell)
        return IGameShell_GetTimerTick(g_pTmGameShell);
    return 0;
}

EXTERN long LGAPI tm_get_millisec(void)
{
   long retVal = tm_get_millisec_unrecorded();
   RecStreamAddOrExtract(g_pTmRecorder, &retVal, sizeof(long), "time_msec");
   return retVal;
}

EXTERN fix LGAPI tm_get_fix(void)
{
    return fix_make(tm_get_millisec(), 0);
}


/* win32 to timer function mappings */

#pragma off (unreferenced)

typedef struct {
   long     period;
   pfv      proc;
   uint32	resolution;
   uint32   id;
} tm_wproc;

#define TM_BEST_RESOLUTION 1
#define TM_PERIODIC_TIMER  1

#define MAX_WIN_TIMERS 16
static tm_wproc tm_win32_procs[MAX_WIN_TIMERS];
static TIMECAPS tm_win32_caps;

// here is the callback that we get from windows.
// the dwUser parameter is our fuction so we do the cast and call it

void CALLBACK tm_win32_callback(UINT nTimerID, UINT uReserved,
                                 DWORD dwUser,
                                 DWORD dwReserved1, DWORD
                                 dwReserved2)
{
   (*(pfv)dwUser)();
}

static int tm_add_process_win32 (void (*prot_func)(), int real_func, long denom)
{
   int i;

   for(i = 0; i < MAX_WIN_TIMERS; i++)
   {
      if(tm_win32_procs[i].proc == NULL)
      {
         tm_win32_procs[i].proc = prot_func;
         tm_win32_procs[i].period = (1000*denom) / 1193181L;
         return i;
      }
   }
   return -1;
}

#pragma on (unreferenced)

static int tm_remove_process_win32 (int id)
{
   int res = (timeKillEvent(tm_win32_procs[id].id) == 0 &&
               timeEndPeriod(tm_win32_procs[id].resolution) == 0);

   tm_win32_procs[id].proc = NULL;
   tm_win32_procs[id].id = -1;
   return res;
}

static int tm_activate_process_win32 (int id)
{

   // check for valid period....

   timeBeginPeriod(tm_win32_procs[id].period);

   tm_win32_procs[id].resolution = min(max(tm_win32_caps.wPeriodMin, TM_BEST_RESOLUTION), tm_win32_caps.wPeriodMax);
   timeBeginPeriod(tm_win32_procs[id].resolution);

   tm_win32_procs[id].id = timeSetEvent(tm_win32_procs[id].period, tm_win32_procs[id].resolution,
                  tm_win32_callback, (DWORD)(tm_win32_procs[id].proc), TM_PERIODIC_TIMER);

   return !tm_win32_procs[id].id;
}

static int tm_deactivate_process_win32 (int id)
{
   return (timeKillEvent(tm_win32_procs[id].id) == 0 &&
               timeEndPeriod(tm_win32_procs[id].id) == 0);
}

static void tm_set_process_denom_win32 (int id, int denom)
{
   tm_win32_procs[id].period = (1000L*denom) / 1193181L;
   tm_deactivate_process_win32(id);
   tm_activate_process_win32(id);
}

static void tm_set_process_freq_win32 (int id, int freq)
{
   tm_win32_procs[id].period = 1000/freq;
   tm_deactivate_process_win32(id);
   tm_activate_process_win32(id);
}

static void tm_set_process_period_win32 (int id, int period)
{
   tm_win32_procs[id].period = period;
   tm_deactivate_process_win32(id);
   tm_activate_process_win32(id);
}

pfv tm_ftab_win32 [TMC_TIMER_FNS] = {
  tm_add_process_win32,
  tm_remove_process_win32,
  tm_activate_process_win32,
  tm_deactivate_process_win32,
  tm_set_process_denom_win32,
  tm_set_process_freq_win32,
  tm_set_process_period_win32
};

/* application main() is repsonsible for starting/stopping AIL
   this assumes that AIL has already been init'ed
*/

int tm_init_win32 ()
{
    g_TimerInitCount++;
    if (g_TimerInitCount == 1)
    {
        g_pTmGameShell = AppGetObj(IGameShell);
        g_pTmRecorder = AppGetObj(IRecorder);

        // see if the windows timer exists
        if (timeGetDevCaps(&tm_win32_caps, sizeof(TIMECAPS)) ==  TIMERR_NOCANDO)
        {
        // Warning(("corrupt system timer\n"));
            return 0;
        }

        tm_ftab = &tm_ftab_win32[0];
    }
    return 0;
}

// shut down the real time timer
extern int tm_rt_id;

int tm_close_win32 ()
{
    g_TimerInitCount--;
    AssertMsg(g_TimerInitCount >= 0, "tm_close_win32() called too many times!");
    if (g_TimerInitCount == 0)
    {
        SafeRelease(g_pTmGameShell);
        SafeRelease(g_pTmRecorder);

        if (tm_rt_id > 0)
            tm_remove_process_win32 (tm_rt_id);
    }
    return 0;
}
#endif
