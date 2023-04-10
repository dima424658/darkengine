// Miscellaneous Res Utilities, all off in their own private module...

#include <dbg.h>
#include <res.h>
#include <res_.h>

long ResViewCache(bool bOnlyLocks)
{
   long cache_mem = 0;
   Id curr_id = ID_MIN;

   Spew(DSRC_RES_CumStat, ("Cache contents:\n"));

   while (curr_id < resDescMax)
   {
      if (ResInUse(curr_id) && ResPtr(curr_id) && ((!bOnlyLocks) || ResLocked(curr_id)))
      {
         cache_mem += ResSize(curr_id);
         Spew(DSRC_RES_CumStat, ("%d(%x) l:%d sz:%d\n", curr_id, curr_id, ResLocked(curr_id), ResSize(curr_id)));
      }
      curr_id++;
   }
#ifdef DBG_ON
   Spew(DSRC_RES_CumStat, ("Total memory in cache = %d\n", cache_mem));
#endif
   return (cache_mem);
}

void ResCheckCache(int maxLocks)
{
   Id curr_id = ID_MIN;

   while (curr_id < resDescMax)
   {
      if (ResInUse(curr_id) && ResPtr(curr_id))
         if (ResLocked(curr_id)>maxLocks)
            Warning(("ResCheckCache: res %d is locked %d times\n", curr_id, ResLocked(curr_id)));
      curr_id++;
   };
}









