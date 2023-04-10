/*
 *
 * $Source: x:/prj/tech/libsrc/timer/RCS/tmdecl.h $
 * $Revision: 1.5 $
 * $Author
 * $Date: 1997/06/18 02:13:25 $
 *
 * External declarations for timer functions.
 *
 * This file is part of the timer library.
 *
 * $Log: tmdecl.h $
 * Revision 1.5  1997/06/18  02:13:25  TOML
 * time unrecorded
 * 
 * Revision 1.4  1996/07/19  14:25:50  TOML
 * *** empty log message ***
 * 
 * Revision 1.3  1994/11/09  00:30:13  lmfeeney
 * added variable names to prototype
 *
 *
 * Revision 1.2  1994/10/17  17:45:24  lmfeeney
 * removed old functions added new ones
 *
 * Revision 1.1  1993/05/20  15:53:03  kaboom
 * Initial revision
 *
 */

#include <fix.h>

typedef void (*pfv)();

extern volatile long tm_millisec;
extern volatile fix tm_fix;

#ifndef _WIN32
#define tm_get_millisec() (tm_millisec)
#define tm_get_fix() (tm_fix)
#else
EXTERN long LGAPI tm_get_millisec(void);
EXTERN long LGAPI tm_get_millisec_unrecorded(void);
EXTERN fix LGAPI tm_get_fix(void);
#endif


extern int tm_init_realtime (unsigned long rt_freq_in, unsigned long spec_freq, pfv fn);
extern long tm_get_rt_freq ();

 /* add_process is not a macro, because it  checks that the ftab
   is non-null and warns if it is.  this is mostly an aid for catching
   initialization order errors, not a useful check
   all the rest of the tm_functions are #defines in tmmac.h
   (which is not included as an internal tm header file!)
 */

extern int tm_add_process (pfv prot_func, int real_func, long denom);

