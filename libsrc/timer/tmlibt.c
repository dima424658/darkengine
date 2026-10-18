#include <tmdecl.h>
#include <tmftab.h>

/* these are "extern" only within the timer library, the
   outside world sees them as #define'd accesses through
   the function table -- the fact that they have the same
   names (except for add_process, which is a true function)
   is a historical artcifact and testimonial to my laziness
*/

extern int TF_tm_add_process (void (*prot_func)(), int real_func, long denom);

extern int tm_remove_process (int id);
extern int tm_activate_process (int id);
extern int tm_deactivate_process (int id);

extern void tm_set_process_denom (int id, int denom);
extern void tm_set_process_freq (int id, int freq);
extern void tm_set_process_period (int id, int period);
 

pfv tm_ftab_libt [TMC_TIMER_FNS] = {
  (pfv)TF_tm_add_process,
  (pfv)tm_remove_process,
  (pfv)tm_activate_process,
  (pfv)tm_deactivate_process,
  (pfv)tm_set_process_denom,
  (pfv)tm_set_process_freq,
  (pfv)tm_set_process_period
};




