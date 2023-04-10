/* timer real-time routines */

/* This is a rather lame implementation, much more efficient to have two specialized
 * routines, one for timer and one for AIL, which hook in to the lower level
 * There are some issues, though and I'm a little hurried.
 */

#include <fix.h>
#include <timer.h>


#define FIX_INCR 131           /* 65536 / 500 , sort of */

volatile long tm_millisec = 0;

volatile fix tm_fix = fix_make(0,0);
volatile fix tm_fix_incr = fix_make(0,0);

static pfv spec_fn = NULL;

static long rt_freq = -1;
static fix  rt_ms_incr;

static fix  rt_ms_frac = fix_make(0,0);      /* running frac part of ms timer */

void tm_incr_tm ()
{
   rt_ms_frac += rt_ms_incr;
   tm_millisec += fix_int(rt_ms_frac);
   rt_ms_frac = fix_frac(rt_ms_frac);
   tm_fix += tm_fix_incr;

   if (spec_fn)
      (*spec_fn)();

}

int tm_rt_id = -1;

int tm_init_realtime (unsigned long rt_freq_in, unsigned long spec_freq, pfv fn)
{
  int spec_id;

  /* If the base freq and the realtime counter run at the same freq, 
   * then chain them.  This should be the usual case (?).
   */

  /* just a spec fn */   
  if (rt_freq_in == 0) {
    if (spec_freq > 0 && fn != NULL) {
      if ((spec_id = tm_add_process (fn, 0, TMD_FREQ/spec_freq)) < 0)
        return -1;
      tm_activate_process (spec_id);
    } 
  }

  /* get ready to start the rt timer */
  else {
    rt_freq = rt_freq_in;
    rt_ms_incr = fix_div((1000<<16), (rt_freq<<16));
    tm_fix_incr = fix_div((1<<16), (rt_freq<<16));

    if ((tm_rt_id = tm_add_process (tm_incr_tm, 0, TMD_FREQ/rt_freq)) < 0)
      return -1;

    /* no spec fn, run rt timer */
    if (spec_freq == 0 || fn == NULL)
      tm_activate_process (tm_rt_id);

    /* rt timer and spec fn at same frequency so chain */
    else if (spec_freq == rt_freq_in) {
      spec_fn = fn;
      tm_activate_process (tm_rt_id);
    }

    /* start the rt timer, run special fn separately */
    else {
      tm_activate_process (tm_rt_id);
      if ((spec_id = tm_add_process (fn, 0, TMD_FREQ/spec_freq)) < 0)
        return -1;
      tm_activate_process (spec_id);
    }
  }
  return 0;
}

long tm_get_rt_freq()
{
   return rt_freq;
}
