/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkreprt.cpp,v 1.21 2000/02/27 18:52:03 adurant Exp $
// Misc Dark Specific report functions

#ifdef REPORT

#include <mprintf.h>
#include <appagg.h>

#include <traitman.h>
#include <traitbas.h>

#include <command.h>

#include <drkreprt.h>
#include <report.h>
#include <drkrphlp.h>
#include <reprthlp.h>

#include <objquery.h>
#include <objedit.h>
#include <objpos.h>
#include <contain.h>
#include <playrobj.h>

#include <drkinvpr.h>   // for the store
#include <property.h>
#include <questprp.h>
#include <drkdiff.h>

#include <contain.h>        // for belt report
#include <invtype.h>
#include <crattach.h>
#include <rendprop.h>

#include <linkman.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
#include <iobjsys.h>
#include <propbase.h>
#include <creatext.h>
#include <autolink.h>
#include <frobprop.h>
#include <traitman.h>

#include <physapi.h> //climbable
#include <phmod.h>
#include <phcore.h>
#include <phmods.h>
#include <phmodobb.h>

#include <drkstats.h> //secrets
#include <drkfixpr.h> //fixtures

#include <lockprop.h> // for test

#include <dbmem.h>

////////////////////////

static void _zerozero_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   mxs_vector *loc;
   char *p=buffer;
   static int cnt;

   switch (phase)
   {
      case kReportPhase_Init:
         cnt=0;
         break;
      case kReportPhase_Loop:
         loc=ObjPosGetLocVector(obj);
         if (loc!=NULL)  // if we have a loc
            if (((*(int *)(&loc->x))|(*(int *)(&loc->y))|(*(int *)(&loc->z)))==0)
            {
               if (cnt==0)
                  rsprintf(&p,"\nZeroZero Report\n");
               rsprintf(&p,"Object %s at ZERO ZERO ZERO\n",ObjWarnName(obj));
               cnt++;
            }
         break;
      case kReportPhase_Term:
         if (cnt)
            rsprintf(&p,"Found %d ZeroZero objects\n",cnt);
         break;
   }
}

////////////////////////

static void _climbable_report(int WarnLevel, void *data, char *buffer)
{

  char *p=buffer;

  DarkReportHelpersStartUse();
  rsprintf(&p, "\nReport on Climbable objects\n");

  AutoAppIPtr(ObjectSystem);

  IObjectQuery* objquery = pObjectSystem->Iter(kObjectConcrete);
  for (; !objquery->Done(); objquery->Next())
    {
      ObjID targ = objquery->Object();
      if (PhysObjHasPhysics(targ))
	{
	  cPhysModel *pModel = g_PhysModels.GetActive(targ);
	  if (pModel->GetType(0) == kPMT_OBB)
	    {
	      if ((((cPhysOBBModel *)pModel)->GetClimbableSides())>0)
		{
		  rsprintf(&p,"Object %s is climbable.\n",ObjWarnName(targ));
		}
	    }
	}
    }
  objquery->Release();
  DarkReportHelpersDoneWith();
}

/////////////////////

static void _fixtures_report(int WarnLevel, void *data, char *buffer)
{

  char *p=buffer;

  DarkReportHelpersStartUse();
  rsprintf(&p, "\nReport on Fixtures\n");

  AutoAppIPtr(ObjectSystem);

  IObjectQuery* objquery = pObjectSystem->Iter(kObjectConcrete);
  for (; !objquery->Done(); objquery->Next())
    {
      ObjID targ = objquery->Object();
      if (is_fixture(targ))
	{
	  rsprintf(&p,"Object %s at %s is a fixture.\n",ObjWarnName(targ),RHLocS(targ));
	}
    }
  objquery->Release();
  DarkReportHelpersDoneWith();
}

////////////////////////

static void _frobinertdoor_report(int WarnLevel, void *data, char *buffer)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   char *p=buffer;
   int bad=0;

   DarkReportHelpersStartUse();
   rsprintf(&p,"\nReport on FrobInert doors\n");
   AutoAppIPtr_(TraitManager,pTraitMan);
   ObjID arch_obj=EditGetObjNamed("Door");
   IObjectQuery *pDescQuery=pTraitMan->Query(arch_obj,kTraitQueryAllDescendents);
   for (; !pDescQuery->Done(); pDescQuery->Next())
   {
      ObjID obj = pDescQuery->Object();
      if (OBJ_IS_CONCRETE(obj))
      {  // want to know if part of loadout or in world...
         sFrobInfo *frobData;
         if (!pFrobInfoProp->Get(obj,&frobData)||
             (frobData->actions[kFrobLocWorld]==0))
         {  // for now we assume if actions NULL, you are FrobInert - so find control devices
            int cnt=0;
            cAutoLinkQuery query("ControlDevice",LINKOBJ_WILDCARD,obj);
            for (; !query->Done(); query->Next())     // find everything that locks me
               cnt++;
            if (cnt!=1)
            {  // are we lazy, or what - lets requery
               rsprintf(&p,"WARNING: zero or multilever (%d) door at %s%s%s\n",
                        cnt,ObjWarnName(obj),RHLocS(obj),cnt?" controlled by":"");
               if (cnt)
               {
                  cAutoLinkQuery printQuery("ControlDevice",LINKOBJ_WILDCARD,obj);
                  for (int items=0; !printQuery->Done(); printQuery->Next(), items++)
                     rsprintf(&p,"%s %s%s",items?",":" ",ObjWarnName(printQuery.GetSource()),RHLocS(printQuery.GetSource()));
                  rsprintf(&p,"\n");
               }
               bad++;
            }
         }
      }
   }
   if (bad)
      rsprintf(&p,"Found %d doors which could get out of synch\n",bad);
   SafeRelease(pDescQuery);
   DarkReportHelpersDoneWith();
}

////////////////////////////
// report to check belt items attach/contain links

// iterate over all contains links, if of type belt, make sure no crattach relation
static void _belt_report(int WarnLevel, void *data, char *buffer)
{
   char *p=buffer;
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pContRel ( pLinkMan->GetRelationNamed("Contains") ); // i cheat more
   ILinkQuery *contLinks = pContRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   rsprintf(&p,"\nChecking BeltContains/CreatureAttach links\n");
   for (; !contLinks->Done(); contLinks->Next())
   {
      sLink containLink;
      contLinks->Link(&containLink);
      if (*(int *)contLinks->Data() == kContainTypeBelt)
      {  // found a belt link
         LinkID attachLink = g_pCreatureAttachRelation->GetSingleLink(containLink.source,containLink.dest);
         if (attachLink!=LINKID_NULL)
            rsprintf(&p,"ERROR: contains and creature attach between %s and %s\n",
                     ObjWarnName(containLink.source),ObjWarnName(containLink.dest));
      }
      if (ObjHasRefs(containLink.dest))
         rsprintf(&p,"ERROR: contained obj %s (in %s) has refs\n",
                  ObjWarnName(containLink.dest),ObjWarnName(containLink.source));
   }
   SafeRelease(contLinks);
   // wed like to somehow check for creature attach relations on their own too
   // but im not sure what the right semantic is for that?
}

////////////////////////////
// report to check alternate items attach/contain links

// iterate over all contains links, if of type alt, make sure no crattach relation
static void _alt_report(int WarnLevel, void *data, char *buffer)
{
   char *p=buffer;
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pContRel ( pLinkMan->GetRelationNamed("Contains") ); // i cheat more
   ILinkQuery *contLinks = pContRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   rsprintf(&p,"\nChecking AltContains/CreatureAttach links\n");
   for (; !contLinks->Done(); contLinks->Next())
   {
      sLink containLink;
      contLinks->Link(&containLink);
      if (*(int *)contLinks->Data() == kContainTypeAlt)
      {  // found an alt link
         LinkID attachLink = g_pCreatureAttachRelation->GetSingleLink(containLink.source,containLink.dest);
         if (attachLink!=LINKID_NULL)
            rsprintf(&p,"ERROR: contains and creature attach between %s and %s\n",
                     ObjWarnName(containLink.source),ObjWarnName(containLink.dest));
      }
      if (ObjHasRefs(containLink.dest))
         rsprintf(&p,"ERROR: contained obj %s (in %s) has refs\n",
                  ObjWarnName(containLink.dest),ObjWarnName(containLink.source));
   }
   SafeRelease(contLinks);
   // wed like to somehow check for creature attach relations on their own too
   // but im not sure what the right semantic is for that?
}

////////////////////////
// report on all powerups in the game

#define kAllowedInvStoreOnly 0x1
#define kAllowedWorldOnly    0x2
#define kAllowedNoStack      0x4

typedef struct {
   char name[32];
   int flags;
} powerup_info;

static powerup_info powerup_archs[]=
{
   { "FireCrystal", kAllowedWorldOnly },
   { "WaterCrystal", kAllowedWorldOnly },
   { "AirCrystal", kAllowedWorldOnly },
   { "EarthCrystal", kAllowedWorldOnly },
   { "GasMine", 0 },
   { "Flashbomb", 0 },
   { "Mine", 0 },
   { "HolyH2O", 0 },
   { "AirPotion", 0 },
   { "HealingPotion", 0 },
   { "HealingFruit", 0 },
   { "firearr", kAllowedInvStoreOnly },
   { "noise", 0 },
   { "broadhead", 0 },
   { "water", kAllowedInvStoreOnly },
   { "GasArrow", kAllowedInvStoreOnly },
   { "EarthArrow", kAllowedInvStoreOnly },
   { "RopeArrow", 0 },
   { "LockPick", kAllowedInvStoreOnly },
};

static const char *_ErrStr(BOOL error)
{
   return error ? "ERROR" : "     ";
}

#define InvNoAllow(flg)       ((flg&kAllowedWorldOnly)!=0)
#define WorldNoAllow(flg)     ((flg&kAllowedInvStoreOnly)!=0)
#define StackNoAllow(flg,obj)  (flg&kAllowedNoStack)
#define StackErr(flg,obj)     (StackNoAllow(flg,obj)?FALSE:!RHStakB(obj))
#define IsCritter(obj)         (CreatureExists(obj))

#define LootInvErr(flg,obj)   (InvNoAllow(flg)||StackErr(flg,obj))
#define LootWorldErr(flg,obj) (WorldNoAllow(flg)||StackErr(flg,obj))
#define LootContainedErr(flg,obj,container) \
                              (WorldNoAllow(flg)||((!IsCritter(container))&&StackErr(flg,obj)))

static ObjID _find_store(char **rep_str)
{
   IBoolProperty *_pStoreProp=NULL;
   GetPropertyInterfaceNamed(PROP_STORE_NAME,IBoolProperty,&_pStoreProp);
   
   ObjID store_obj=OBJ_NULL;
   sPropertyObjIter iter;
   BOOL in_store;
   ObjID obj;
   _pStoreProp->IterStart(&iter);
   while (_pStoreProp->IterNextValue(&iter,&obj,&in_store))
      if (OBJ_IS_CONCRETE(obj) && in_store)
      {
         if (store_obj!=OBJ_NULL)
            rsprintf(rep_str,"Warning, found another store %s\n",ObjEditName(obj));
         else
            store_obj=obj;
      }
   _pStoreProp->IterStop(&iter);
   SafeRelease(_pStoreProp);
   return store_obj;
}

// modular check error level and return calls
static void LootObjInvTalk(char **pp, ObjID obj, int WarnLevel, BOOL err, const char *loc)
{
   if ((WarnLevel>=kReportLevel_Info)||err)
      rsprintf(pp,"%s %s%s in %s%s\n",
               _ErrStr(err),ObjEditName(obj),RHStakS(obj),loc,DRHDiffS(obj));
}

static void LootObjWorldTalk(char **pp, ObjID obj, int WarnLevel, BOOL err)
{
   if ((WarnLevel>=kReportLevel_Info)||err)
      rsprintf(pp,"%s %s%s%s%s\n",
               _ErrStr(err),ObjEditName(obj),RHStakS(obj),RHLocS(obj),DRHDiffS(obj));
}

// report on powerups/setup
static void _powerup_report(int WarnLevel, void *data, char *buffer)
{
   ObjID virtual_player_obj=PlayerObject();
   char *p=buffer;
   int i;

   if (virtual_player_obj==OBJ_NULL)
   {
      AutoAppIPtr_(LinkManager,pLinkMan);
      cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("PlayerFactory") );
      LinkID id = pRel->GetSingleLink(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
      if (id!=LINKID_NULL)
      {
         sLink link;
         pRel->Get(id,&link);
         virtual_player_obj=link.source;
      }
   }
   AutoAppIPtr_(TraitManager,pTraitMan);
   AutoAppIPtr_(ContainSys,pContainSys);
   DarkReportHelpersStartUse();
   rsprintf(&p,"\nReport on Powerups\n");
   ObjID store_obj=_find_store(&p);
   for (i=0; i<sizeof(powerup_archs)/sizeof(powerup_archs[0]); i++)
   {
      ObjID arch_obj=EditGetObjNamed(powerup_archs[i].name);
      int flags=powerup_archs[i].flags;
      IObjectQuery *pDescQuery=pTraitMan->Query(arch_obj,kTraitQueryAllDescendents);
      for (; !pDescQuery->Done(); pDescQuery->Next())
      {
         ObjID obj = pDescQuery->Object(), container_obj;
         if (OBJ_IS_CONCRETE(obj))
         {  // want to know if part of loadout or in world...
            if ((virtual_player_obj!=OBJ_NULL)&&pContainSys->Contains(virtual_player_obj,obj))
               LootObjInvTalk(&p,obj,WarnLevel,LootInvErr(flags,obj),"Inventory");
            else if ((store_obj!=OBJ_NULL)&&pContainSys->Contains(store_obj,obj))
               LootObjInvTalk(&p,obj,WarnLevel,LootInvErr(flags,obj),"Store");
            else if ((container_obj=pContainSys->GetContainer(obj))!=OBJ_NULL)
               LootObjInvTalk(&p,obj,WarnLevel,LootContainedErr(flags,obj,container_obj),ObjWarnName(container_obj));
            else
               LootObjWorldTalk(&p,obj,WarnLevel,LootWorldErr(flags,obj));
         }
      }
      SafeRelease(pDescQuery);
   }
   DarkReportHelpersDoneWith();
}

////////////////////////
// report on secrets
static void _secret_report(int WarnLevel, void *data, char *buffer)
{
  int total=0;
  char *p = buffer;
  ObjID o;
  
  AutoAppIPtr(ObjectSystem);
  AutoAppIPtr(ContainSys);
  IObjectQuery *allObjQuery=pObjectSystem->Iter(kObjectConcrete);
  for (; !allObjQuery->Done(); allObjQuery->Next())
    if (DarkStatCheckBit(allObjQuery->Object(),kDarkStatBitHidden))
      {
	total++;
        o=allObjQuery->Object();
        rsprintf(&p,"Found secret on object %s\n",ObjWarnName(o));
      }
  SafeRelease(allObjQuery);
  rsprintf(&p,"Total secrets found: %d\n",total);

}

////////////////////////
// helper functions

static IQuestVarProperty *_gpDiffDestroyProp=NULL;

void DarkReportHelpersStartUse(void)
{
   if (!_gpDiffDestroyProp)   
      GetPropertyInterfaceNamed(PROP_DIFF_DESTROY_NAME,IQuestVarProperty,&_gpDiffDestroyProp);
   ReportHelpersStartUse();
}

void DarkReportHelpersDoneWith(void)
{
   if (_gpDiffDestroyProp)
   {
      SafeRelease(_gpDiffDestroyProp);
      _gpDiffDestroyProp=NULL;
   }
   ReportHelpersDoneWith();
}

char *DarkReportHelperDiffString(ObjID obj, char *buffer)
{
   static char our_static_buf[64];
   char *p=buffer?buffer:our_static_buf;
   int diffs=0;
   if (_gpDiffDestroyProp->Get(obj,&diffs))
      if (diffs)
      {
         sprintf(p," Diff %s%s%s",(diffs&1)?"":"N",(diffs&2)?"":"H",(diffs&4)?"":"E");
         return p;
      }

   strcpy(p, "");

   return p;   // if we are here, we have nothing to say
}

////////////////////////
// commands

// iterate over all contains links, if of type belt, make sure no crattach relation
static void DarkReportDerefContainees(void)
{
   AutoAppIPtr_(LinkManager,pLinkMan);
   cAutoIPtr<IRelation> pContRel ( pLinkMan->GetRelationNamed("Contains") ); // i cheat more
   ILinkQuery *contLinks = pContRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !contLinks->Done(); contLinks->Next())
   {
      sLink containLink;
      contLinks->Link(&containLink);
      if (ObjHasRefs(containLink.dest))
      {
         mprintf("Derefing %s (contained in %s)\n",
                  ObjWarnName(containLink.dest),ObjWarnName(containLink.source));
         ObjSetHasRefs(containLink.dest,FALSE);
      }
   }
   SafeRelease(contLinks);
}

static void _testAccess(char *str)
{
   sObjAccess aList[MAX_OBJ_ACCESS];
   int x, y, cnt, i, all=0;
   sscanf(str,"%d %d %d",&x,&y,&all);
   cnt=CheckObjectAccess(x,y,aList,all);
   if (cnt)
      for (i=0; i<cnt; i++)
         mprintf("Use %s on %s (Flags %x)\n",ObjWarnName(aList[i].frober),ObjWarnName(aList[i].frobee),aList[i].flags);
   else
      mprintf("No Access to %s for %s\n",ObjWarnName(y),ObjWarnName(x));
}

Command drkreport_keys[] =
{
   { "deref_containees", FUNC_VOID, DarkReportDerefContainees, "Remove Refs from all contained objs" },
   { "test_access", FUNC_STRING, _testAccess },
};

////////////////////////
// init/term

void DarkReportInit(void)
{
   COMMANDS(drkreport_keys, HK_BRUSH_EDIT);
   ReportRegisterGenCallback(_powerup_report,kReportAllObj,"Dark Powerups",NULL);   
   ReportRegisterGenCallback(_belt_report,kReportAllObj,"Belt System",NULL);
   ReportRegisterGenCallback(_alt_report,kReportAllObj,"Alternate Attach System",NULL);
   ReportRegisterGenCallback(_frobinertdoor_report,kReportAllObj,"FrobInertDoors",NULL);
   ReportRegisterGenCallback(_climbable_report,kReportAllObj,"ClimbableObjects",NULL);
   ReportRegisterGenCallback(_fixtures_report,kReportAllObj,"Fixtures",NULL);
   ReportRegisterObjCallback(_zerozero_report,"ZeroZero",NULL);
   ReportRegisterGenCallback(_secret_report,kReportAllObj,"Secrets",NULL);
}

void DarkReportTerm(void)
{
   ReportUnRegisterGenCallback(_powerup_report,kReportAllObj,NULL);
   ReportUnRegisterGenCallback(_belt_report,kReportAllObj,NULL);
   ReportUnRegisterGenCallback(_alt_report,kReportAllObj,NULL);
   ReportUnRegisterGenCallback(_frobinertdoor_report,kReportAllObj,NULL);
   ReportUnRegisterGenCallback(_climbable_report,kReportAllObj,NULL);
   ReportUnRegisterGenCallback(_fixtures_report,kReportAllObj,NULL);
   ReportUnRegisterObjCallback(_zerozero_report,NULL);
   ReportUnRegisterGenCallback(_secret_report,kReportAllObj,NULL);
   DarkReportHelpersDoneWith();  // just to be safe, in case someone spaced
}

#endif /* REPORT */
