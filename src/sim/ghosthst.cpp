/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghosthst.cpp,v 1.1 1999/08/05 18:07:38 Justin Exp $
// histogram/analysis for ghost bandwidth/etc

#ifdef PLAYTEST

#include <lg.h>

#include <string.h>

#include <ghost.h>
#include <ghosthst.h>

#include <hshsttem.h>

#include <dbmem.h>

static int _histo_min_samples  = 2;
static int _histo_reason_count = 1;
static int _histo_short_time   = 5000;

typedef struct {
   int count;
   int time_between_sum;
   int count_long;
   int last_time;   // this wants to be a histo too, i guess
} sGhostHistoEntry;

// wildly un-rehost-savvy, this is somewhat of a problem....
typedef struct {
   ObjID obj;
   char  obj_name[128];
   sGhostHistoEntry *data;
} sGhostHisto;

// little hash by ObjID, each entry will be an array of structs, time of/reason for send
typedef cHashSet<sGhostHisto *, ObjID, cHashFunctions> cGhostHistoTableBase;

class cGhostHistoHash : public cGhostHistoTableBase
{
   virtual tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey)(((sGhostHisto *)(node))->obj);
   }
};

#ifdef _MSC_VER
template cGhostHistoTableBase;
#endif

cGhostHistoHash gGhostHisto;

// add to the histogram
void _ghost_histo_add(ObjID ghost, int reason, int time)
{
   sGhostHisto *pGH=gGhostHisto.Search(ghost);
   if (pGH==NULL)
   {  // add an entry
      pGH=new sGhostHisto;
      pGH->obj=ghost;
      strncpy(pGH->obj_name,ObjWarnName(ghost),120);
      pGH->obj_name[120]='\0';
      pGH->data=(sGhostHistoEntry *)malloc(sizeof(sGhostHistoEntry)*_histo_reason_count);
      memset(pGH->data,0,sizeof(sGhostHistoEntry)*_histo_reason_count);
      gGhostHisto.Insert(pGH);
   }
   sGhostHistoEntry *pEntry=&pGH->data[reason];
   pEntry->count++;
   int dt=time-pEntry->last_time;
   if (dt>_histo_short_time)
      pEntry->count_long++;
   else
      pEntry->time_between_sum+=dt;
   pEntry->last_time=time;
}

void _ghost_histo_clear(void)
{
   tHashSetHandle local_iter;    // go through local ghosts
   sGhostHisto *pGH=gGhostHisto.GetFirst(local_iter);
   while (pGH)
   {
      free(pGH->data);
      delete pGH;
      pGH=gGhostHisto.GetNext(local_iter);
   }
}

// @TBD:
//   should add a "by type" dump to only do AIs, or whatever
//   should add sorting into types, and sort within by "reason"
//   should show number of samples which were "long"
static void _ghost_histo_dump_obj(sGhostHisto *pHisto)
{
   int printed=0;
   for (int i=0; i<_histo_reason_count; i++)
   {
      sGhostHistoEntry *pEntry=&pHisto->data[i];
      if (pEntry->count>_histo_min_samples)
      {
         char buf[80];
         if (pEntry->count_long*5<pEntry->count*2) // at least 60% are "real"
            sprintf(buf,"%d x%d %1.2f ",i,pEntry->count,
                    ((float)(pEntry->time_between_sum/(float)(pEntry->count-pEntry->count_long)))/1000.0);
         else
            sprintf(buf,"%d x%d ",i,pEntry->count);
         if (printed==0)
            mprintf("%s: ",pHisto->obj_name);  // havent printed yet, so show name
         mprint(buf);
         printed++;
      }
   }
   if (printed>0)
      mprintf("\n");
}

void _ghost_histo_show(char *cmd)
{
   tHashSetHandle local_iter;    // go through local ghosts
   sGhostHisto *pGH=gGhostHisto.GetFirst(local_iter);
   if (pGH)
      mprintf("\nHistogram of send reasons\n");
   while (pGH)
   {
      _ghost_histo_dump_obj(pGH);
      pGH=gGhostHisto.GetNext(local_iter);
   }
}

void _ghost_histo_init(int num_reasons, int short_time)
{
   _histo_reason_count=num_reasons;
   if (short_time!=0)
      _histo_short_time=short_time;
}

void _ghost_histo_term(void)
{
   if (config_is_defined("ghostspew_histo"))
      _ghost_histo_show("");
   _ghost_histo_clear();
}

#endif  // PLAYTEST
