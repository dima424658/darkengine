/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brquery.c,v 1.3 2000/02/19 12:27:46 toml Exp $
// some misc brush query/list control functions

#include <editbr.h>
#include <editbr_.h>
#include <brlist.h>
#include <vbrush.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////////////
// brush finder - returns the brush (if there is one) of brush id br_id
editBrush *brFind(int br_id)
{
   editBrush *us;
   int hIter;

   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if (us->br_id==br_id)
      {
         blistIterDone(hIter);
         break;             // found it
      }
      us=blistIterNext(hIter);
   }
   return us;
}

/////////////////
// generic brush cycler - takes a "is brush ok" callback which you provide
BOOL brCycle(int dir, BOOL (*br_ok)(editBrush *br))
{
   editBrush *initial_br=vBrush_GetSel(), *cur;
   int seek_fix=dir>0?SEEK_SET:SEEK_END;
   BOOL rv=TRUE;
   
   do {
      if (!blistSeek(dir,SEEK_CUR))
         blistSeek(0,seek_fix);
      if (br_ok==NULL) break;     // if no criterion, first found brush is it
      cur=blistGet();
      if (cur==initial_br) rv=FALSE; // back at front, you lose.  rv=FALSE will cause us to break out
   } while (rv && (!(*br_ok)(cur))); // we can just call br_ok, cause if NULL we already left loop
   if (rv)
      we_switched_brush_focus();
   return rv;
}

/////////////////
// brush finder - returns the brush (if there is one) of brush id br_id
int brFilter(BOOL (*br_ok)(editBrush *br), BOOL (*br_run)(editBrush *br))
{
   editBrush *us;
   int hIter, cnt=0;

   us=blistIterStart(&hIter);
   while (us!=NULL)
   {
      if ((*br_ok)(us))
         if ((*br_run)(us))
            cnt++;
      us=blistIterNext(hIter);
   }
   return cnt;
}
