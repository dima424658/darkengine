/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/preload.c,v 1.9 2000/02/19 13:11:13 toml Exp $
// stupid preloading system
//
// @TBD (justin 6-26-98): This can probably go away entirely once the
// resource system is fully functional. As of this writing, AsyncPreload()
// fails on two counts -- the ARQ is currently non-funct in the resource
// system, and the shared cache (essential to preloading) isn't there.
// However, both should be in before Dark ships, so AsyncPreload() and/or
// plain Preload() should be able to deal with this functionality better.
//

#include <string.h>

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <preload.h>

#include <resapi.h>
#include <binrstyp.h>
#include <sndrstyp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static int  hand_cnt=0, hand_sz=0;
static IRes **hand_pre=NULL;

static int  preload_size=0;
static BOOL preload_unknown=FALSE;

BOOL preload_data(int type_code, char *name, char *aux)
{
   char *lr_type;
   char pre_path[256];
   IRes *pRes;
   IResMan *pResMan = AppGetObj(IResMan);

   pre_path[0] = '\0';

   switch (type_code)
   {
      case PRELOAD_SOUND: 
         lr_type=RESTYPE_SOUND; 
         strcpy(pre_path,"snd/"); 
         break;

      case PRELOAD_BIN:
         lr_type=RESTYPE_BINARY;
         strcpy(pre_path,aux);
         break;

      default: 
         return FALSE;
   }
   pRes = IResMan_Bind(pResMan, name, lr_type, NULL, pre_path, 0);
   if (!pRes)
   {
      Warning(("preload failed for %s (type %d - auxpath %s)\n",name,type_code,aux?aux:"NULL"));
      return FALSE;
   }
   if (hand_cnt==hand_sz)
   {
      hand_sz+=128;
      if (hand_cnt==0)
         hand_pre=(IRes **)Malloc(sizeof(IRes *)*hand_sz);
      else
         hand_pre=(IRes **)Realloc(hand_pre,sizeof(IRes *)*hand_sz);
   }
   if (hand_pre==NULL)
   {
      SafeRelease(pRes);
      return FALSE;
   }
   IRes_Lock(pRes);
   hand_pre[hand_cnt++]=pRes;
   if (IRes_GetSize(pRes)!=0)
      preload_size+=IRes_GetSize(pRes);
   else
      preload_unknown=TRUE;
   return TRUE;
}

void preload_free_all(void)
{
   if (hand_pre)
   {
      int i;
      for (i=0; i<hand_cnt; i++) {
         IRes_Unlock(hand_pre[i]);
         SafeRelease(hand_pre[i]);
      }
      Free(hand_pre);
   }
   hand_cnt=hand_sz=0;
   hand_pre=NULL;
   preload_size=0;
   preload_unknown=FALSE;
}

void preload_dump_stats(void)
{
   mprintf("Cur count %d, table size %d - using %s %d bytes of mem\n",
      hand_cnt,hand_sz,preload_unknown?"At least":"Exactly",preload_size);
}
