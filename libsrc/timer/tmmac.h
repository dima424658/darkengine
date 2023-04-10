
/* external interface to timer library -- easier than renaming all the tm_
 * functions
 */

/* see comment in tmdecl.h

#define tm_add_process \
     ((int (*) ()) tm_ftab[TMC_ADD_PROCESS])
*/

#define tm_remove_process \
     ((int (*) ()) tm_ftab[TMC_REM_PROCESS])

#define tm_activate_process \
     ((int (*) ()) tm_ftab[TMC_ACT_PROCESS])

#define tm_deactivate_process \
     ((int (*) ()) tm_ftab[TMC_DEACT_PROCESS])

#define tm_set_process_denom \
     ((void  (*) ()) tm_ftab[TMC_SET_PROC_DENOM])

#define tm_set_process_freq \
     ((void  (*) ()) tm_ftab[TMC_SET_PROC_FREQ])

#define tm_set_process_period \
     ((void  (*) ()) tm_ftab[TMC_SET_PROC_PER])



