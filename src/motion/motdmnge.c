/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdmnge.c,v 1.11 2000/02/19 12:29:52 toml Exp $

#include <lg.h>
#include <appagg.h>
#include <string.h>
#include <motdmng_.h>
#include <motdmnge.h>
#include <motdesc.h>

#include <resapi.h>
#include <binrstyp.h>

#include <mprintf.h>
#include <multiped.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

MotDataHandle *motDataHandles=NULL;
static int    *motDataLockCnt=NULL;
static int g_nMotHandles=0;

void MotDmngeInit(int num_motions)
{
   int i;
   MotDataHandle *md;

   motDataHandles=(MotDataHandle *)Malloc(num_motions*sizeof(MotDataHandle));
   motDataLockCnt=(int *)Malloc(num_motions*sizeof(int));
   memset(motDataLockCnt,0,sizeof(int)*num_motions);
   
   md=motDataHandles;
   for(i=0;i<num_motions;i++,md++)
   {
      *md=NULL;
   }
   g_nMotHandles=num_motions;
}

void MotDmngeClose()
{
   if (motDataHandles)
   {
      // free any data that is currently locked
      int i;
      MotDataHandle *md=motDataHandles;

      for(i=0;i<g_nMotHandles;i++,md++)
      {
         if (*md) {
            IRes_Unlock(*md);
            SafeRelease(*md);
            *md=NULL;
         }
      }
      Free(motDataHandles);
      Free(motDataLockCnt);
      motDataHandles=NULL;
      motDataLockCnt=NULL;
   }
   g_nMotHandles=0;
}

// assumes mot_num is valid
// only use for non-virtual motions
void MotDmngeLock(int mot_num)
{
   if( ((mps_motion *)mp_motion_list+mot_num)->info.type!=MT_CAPTURE)
      return;
   if (motDataLockCnt[mot_num]==0)
   {
      IResMan *pResMan = AppGetObj(IResMan);
      char name[30];
      strcpy(name,MotDescGetName(mot_num));
      strcat(name,"_.mc");
      motDataHandles[mot_num]=IResMan_Bind(pResMan,
                                           name,
                                           RESTYPE_BINARY,
                                           NULL,
                                           "motions\\",
                                           0);
      // @TBD (justin 6-26-98): For now, we're maintaining a global lock, for
      // safety. This should get replaced by more concise locks, by someone
      // who groks motset better...
      SafeRelease(pResMan);
      if (motDataHandles[mot_num]) {
         IRes_Lock(motDataHandles[mot_num]);
      } else {
         Warning(("MotDmngeLock: no such resource %s.\n", name));
         return;
      }
   }
   motDataLockCnt[mot_num]++;
   // mprintf("loading %d\n",mot_num);
}

void MotDmngeUnlock(int mot_num)
{
   if( ((mps_motion *)mp_motion_list+mot_num)->info.type!=MT_CAPTURE)
      return;

   // I'd like to make this an assert, but it seems that we are not guaranteed that this
   // case won't occur. In fact, it definitely does occur when an attempt is made to lock
   // a motion that doesn't exist.
   if (motDataLockCnt[mot_num]<=0)
   {
      Warning(("MotDmngeUnlock: Ref count for motion %d already zero\n", mot_num));
      return;
   }
   motDataLockCnt[mot_num]--;
   if (motDataLockCnt[mot_num]==0)
   {
      MotDataHandle *md=&motDataHandles[mot_num];
      if (*md) {
         IRes_Unlock(*md);
         SafeRelease(*md);
         *md=NULL;
      }
   }
   // mprintf("freeing %d\n",mot_num);
}
