/*
 * $Header: x:/prj/tech/libsrc/timer/RCS/tmail.c 1.3 1996/10/16 16:08:07 TOML Exp $
 *
 */

#include <ail.h>
#include <tmdecl.h>
#include <tmftab.h>

/* AIL to timer function mappings */

#pragma off (unreferenced)

typedef void  (__cdecl * tAILTimerCallback)();

static int tm_add_process_AIL (void (*prot_func)(), int real_func, long denom)
{
   int r;

   if ((r=AIL_register_timer ((tAILTimerCallback)prot_func)) != -1)
     AIL_set_timer_divisor (r, (unsigned long) denom);
   return r;
}

#pragma on (unreferenced)

/* these are necessary because AIL functions are cdecl'ed and we need to
   hide this _in the timer library_
   also there are some minor inconsistencies in signatures
 */

static int tm_remove_process_AIL (int id)
{
  AIL_release_timer_handle (id);
  return 0;
}

static int tm_activate_process_AIL (int id)
{
  AIL_start_timer (id);
  return 0;
}

static int tm_deactivate_process_AIL (int id)
{
  AIL_stop_timer (id);
  return 0;
}

static void tm_set_process_denom_AIL (int id, int denom)
{
  AIL_set_timer_divisor (id, (unsigned) denom);
}

static void tm_set_process_freq_AIL (int id, int freq)
{
  AIL_set_timer_frequency (id, (unsigned long) freq);
}

static void tm_set_process_period_AIL (int id, int period)
{
  AIL_set_timer_period (id, (unsigned long) period);
}


pfv tm_ftab_ail [TMC_TIMER_FNS] = {
  tm_add_process_AIL,
  tm_remove_process_AIL,
  tm_activate_process_AIL,
  tm_deactivate_process_AIL,
  tm_set_process_denom_AIL,
  tm_set_process_freq_AIL,
  tm_set_process_period_AIL
};

/* application main() is repsonsible for starting/stopping AIL
   this assumes that AIL has already been init'ed
*/

int tm_init_AIL ()
{
  /* set function pointers to AIL equivalents of timer lib functions */
  tm_ftab = &tm_ftab_ail[0];
  return 0;
}

/* need to manually shutdown real time timer, in case we're leaving AIL up */
extern int tm_rt_id;

int tm_close_AIL ()
{
  if (tm_rt_id > 0)
    tm_remove_process_AIL (tm_rt_id);
  return 0;
}

