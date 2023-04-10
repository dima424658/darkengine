/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkloot.cpp,v 1.15 2000/01/04 18:25:47 BFarquha Exp $

#include <appagg.h>
#include <drkloot.h>
#include <lootprop.h>
#include <contain.h>
#include <string.h>
#include <rendprop.h>
#include <mnamprop.h>
#include <invrndpr.h>
#include <propman.h>
#include <propbase.h>
#include <str.h>
#include <iobjsys.h>
#include <drkinvui.h>
#include <drkinvpr.h>
#include <drkuires.h>
#include <report.h>
#include <drkrphlp.h>
#include <questapi.h>
#include <drkldout.h>
#include <iobjsys.h>
#include <objquery.h>

#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>
#include <playrobj.h>

#include <objedit.h>
#include <sdestool.h>
#include <objpos.h>
#include <objdef.h>

#include <traitman.h>

// include these last
#include <initguid.h>
#include <drkloot_.h>

#include <dbmem.h>

ILootProperty* gpLootProp = NULL;
static IInvRenderProperty* gpInvProp = NULL;
static IObjectSystem* gpObjSys = NULL;

static void set_loot_name(ObjID loot, const sLootProp* lootval)
{
   AutoAppIPtr_(PropertyManager,pPropMan);
   cAutoIPtr<IProperty> prop ( pPropMan->GetPropertyNamed(PROP_GAME_NAME_NAME));
   cAutoIPtr<IStringProperty> nameprop(IID_IStringProperty,prop);

   cStr name = "@LOOT: \"";
   int total = 0;
   char buf[32];

   // copy in loot values
   for (int i = 0; i < kLootSpecial; i++)
   {
      char strname[8];
      sprintf(strname,"Loot_%d",i);
      cStr lootname = FetchUIString("misc",strname);

      sprintf(buf,"%s: %d\n",(const char*)lootname,lootval->count[i]);
      name += buf;
      total += lootval->count[i];
   }

   cStr totname = FetchUIString("misc","loot_total");
   sprintf(buf,"%s: %d\"",(const char*)totname,total);
   name += buf;
   nameprop->Set(loot,name);
}

static BOOL combine_loot(ObjID targ, ObjID src)
{
   sLootProp* srcprop;
   if (gpLootProp->Get(src,&srcprop))
   {
      sLootProp newprop;
      sLootProp* targprop;

      if (gpLootProp->Get(targ,&targprop))
         newprop = *targprop;
      else
         memset(&newprop,0,sizeof(newprop));

      // copy in loot values
      for (int i = 0; i < kLootSpecial; i++)
         newprop.count[i] += srcprop->count[i];
      newprop.count[kLootSpecial] |= srcprop->count[kLootSpecial];

      gpLootProp->Set(targ,&newprop);

      set_loot_name(targ,&newprop);

      // Copy render info too
      ObjSetRenderType(targ,ObjRenderType(src));

      char buf[256]; // this is dumb, but the api is ancient
      if (ObjGetModelName(src,buf))
         ObjSetModelName(targ,buf);

      sInvRenderType* rtype;

      if (gpInvProp->Get(src,&rtype))
         gpInvProp->Set(targ,rtype);
      {
         sInvRenderType blank = { kInvRenderDefault };
         gpInvProp->Set(targ,&blank);
      }

      return TRUE;

   }

   return FALSE;
}

static BOOL loot_combine_CB(eContainsEvent event, ObjID targ, ObjID src, eContainType ctype, ContainCBData /*data*/)
{
   if (event == kContainCombine)
   {
      combine_loot(targ,src);
   }
   else if (event == kContainAdd)
   {
      sLootProp* prop;
      if (gpLootProp->Get(src,&prop))
      {
         set_loot_name(src,prop);
         if (IsPlayerObj(targ))
            InvUIRefreshObj(src);
      }
   }

   return TRUE;
}

static void tabulate_player_loot(BOOL destroy)
{
   AutoAppIPtr(QuestData);

   gpObjSys->Lock();
   int total = pQuestData->Get(MONEY_QVAR);
   sLootProp* prop;
   AutoAppIPtr(ContainSys);
   sContainIter* iter = pContainSys->IterStart(PlayerObject());
   for (; !iter->finished; pContainSys->IterNext(iter))
      if (gpLootProp->Get(iter->containee,&prop))
      {
         // copy in loot values
         for (int i = 0; i < kLootSpecial; i++)
            total += prop->count[i];

         if (destroy)
            gpObjSys->Destroy(iter->containee);
      }
   pContainSys->IterEnd(iter);

   gpObjSys->Unlock();
   pQuestData->Set(MONEY_QVAR,total);

}

void DarkLootPrepMission(void)
{
   tabulate_player_loot(TRUE);
}

static void sim_cb(const sDispatchMsg* msg, const sDispatchListenerDesc* desc)
{
   switch(msg->kind)
   {
      case kSimInit:
         {
            AutoAppIPtr(QuestData);
            pQuestData->Set(MONEY_QVAR,0);
         }
         break;

      case kSimTerm:
         tabulate_player_loot(FALSE);
         break;
   }
}

static void setup_sim_cb()
{
   sDispatchListenerDesc desc =
   {
      &SIM_Loot,
      kSimInit|kSimTerm,
      sim_cb,
      NULL
   };

   AutoAppIPtr(SimManager);
   pSimManager->Listen(&desc);
}

// count all loot (not held by except)
int count_all_loot(ObjID except)
{
   sLootProp *val;
   int total=0;

   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(ContainSys);
   IObjectQuery *allObjQuery=pObjectSystem->Iter(kObjectConcrete);
   for ( ; !allObjQuery->Done(); allObjQuery->Next())
      if (gpLootProp->Get(allObjQuery->Object(),&val))
         if (except==OBJ_NULL || !pContainSys->Contains(except,allObjQuery->Object()))
            for (int i = 0; i < kLootSpecial; i++)
               total += val->count[i];
   SafeRelease(allObjQuery);
   return total;
}

//////////////////
// Report Functions
#ifdef REPORT

static void _loot_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   static sLootProp total;
   static int t_obj_count;
   sLootProp *pProp;
   char *p=buffer;
   int i;
   AutoAppIPtr_(StructDescTools,pTools);
   const sStructDesc* sdesc = pTools->Lookup("sLootProp");

   if (WarnLevel<kReportLevel_Info) return;  // nothing to say, no warnings or errors for loot yet

   switch (phase)
   {
      case kReportPhase_Init:
         for (i = 0; i < kNumLoot; i++)
            total.count[i] = 0;
         t_obj_count=0;
         if (loud)
         {
            rsprintf(&p,"\nLoot Report\n"); // no need to deal with p, since we are done
            DarkReportHelpersStartUse();
         }
         break;
      case kReportPhase_Loop:
         if (!gpLootProp->Get(obj,&pProp))
            break;
         for (i = 0; i < kLootSpecial; i++)
            total.count[i] += pProp->count[i];
         total.count[kLootSpecial] |= pProp->count[kLootSpecial];
         t_obj_count++;

         if (loud)
         {
            rsprintf(&p,"%s%s%s vals ",ObjWarnName(obj),RHLocS(obj),DRHDiffS(obj));
            if (sdesc)
            {
               pTools->UnparseFull(sdesc,pProp,p,128);
               p+=strlen(p);
            }
            rsprintf(&p,"\n");
         }
         break;
      case kReportPhase_Term:
         if (sdesc)
         {
            rsprintf(&p,"%sTotal loot: ",loud?"":"\n");
            pTools->UnparseFull(sdesc,&total,p,128);
            p+=strlen(p);
            rsprintf(&p," from %d objects\n",t_obj_count);
         }
         if (loud)
            DarkReportHelpersDoneWith();
         break;
   }
}
#endif

//////////////////
// Init/Term

void DarkLootInit(void)
{
   gpLootProp = InitLootProp();

   AutoAppIPtr_(PropertyManager,pPropMan);
   IProperty* invprop = pPropMan->GetPropertyNamed(PROP_INVREND_NAME);
   Verify(SUCCEEDED(invprop->QueryInterface(IID_IInvRenderProperty,(void**)&gpInvProp)));

   AutoAppIPtr(ContainSys);
   pContainSys->Listen(OBJ_NULL, loot_combine_CB, NULL);

   gpObjSys = AppGetObj(IObjectSystem);

   ReportRegisterObjCallback(_loot_report,"Loot System",NULL);
   setup_sim_cb();
}

void DarkLootTerm(void)
{
   ReportUnRegisterObjCallback(_loot_report,NULL);
   SafeRelease(gpLootProp);
   SafeRelease(gpInvProp);
   SafeRelease(gpObjSys);
}

