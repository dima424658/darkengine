
#include <lg.h>
#include <tmdecl.h>
#include <tmftab.h>

/* array of pointer to fn, points to tm_ftab_libt or to
   tm_ftab_AIL or tm_ftab_win32
 */

pfv *tm_ftab;

/* real tm_add_process, tm_add_process is special, since it detects
   uninitialized ftab -- maybe this should go away, since it's mostly
   a conversion problem, but no problem if it stays

 */

int tm_add_process (void (*prot_fn)(), int real_fn, long demon )
{
  int r;

  if (tm_ftab) {
    r = ((int (*) ()) tm_ftab[TMC_ADD_PROCESS]) (prot_fn, real_fn, demon);
    return r;
  }
  else {
    Warning(("NULL tm_ftab! Have you called tm_init yet?\n"));
    return -1;
  }
}
