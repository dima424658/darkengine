/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkreprt.cpp,v 1.5 1999/06/04 00:21:28 XEMU Exp $
// Misc Shock Specific report functions

#include <shkreprt.h>

#ifdef REPORT

#include <mprintf.h>
#include <appagg.h>

#include <traitman.h>
#include <traitbas.h>

#include <command.h>
#include <propbase.h>

#include <iobjsys.h>

#include <report.h>
#include <reprthlp.h>

#include <objquery.h>
#include <objedit.h>
#include <objpos.h>
#include <contain.h>
#include <combprop.h>
#include <playrobj.h>
#include <shkplayr.h>
#include <shkplprp.h>
#include <shkhazpr.h>
#include <shkspawn.h>
#include <shkrep.h>

#include <drkdiff.h>

#include <hashpp.h>
#include <hashfunc.h>
#include <hshpptem.h>

#include <shkprop.h>

#include <dbmem.h>

typedef cHashTableFunctions<ObjID> ObjCountHashFunctions;
typedef cHashTable<ObjID, int, ObjCountHashFunctions> cObjCountTable;

#ifdef _MSC_VER
template cObjCountTable;
#endif

static void _shkinv_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   char *p=buffer;

   if (WarnLevel<kReportLevel_Info) return;  

   switch (phase)
   {
      case kReportPhase_Init:
         rsprintf(&p, "Analyzing contain links:\n");
         break;
      case kReportPhase_Loop:
         {
            // check for contain links from the object 
            // if we have both, badness 10000
            AutoAppIPtr(ContainSys);
            sContainIter *scip;

            scip = pContainSys->IterStart(obj); 
            while (!scip->finished)
            {
               if (!OBJ_IS_CONCRETE(scip->containee))
                  rsprintf(&p,"object %d is linked to abstract obj %d!\n",obj, scip->containee);
               pContainSys->IterNext(scip);
            }
            pContainSys->IterEnd(scip);
         }
         break;
      case kReportPhase_Term:         
         break;
   }
}

static void _shkqb_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   char *p=buffer;

   if (WarnLevel<kReportLevel_Info) return;  

   switch (phase)
   {
      case kReportPhase_Init:
         rsprintf(&p, "Objects with QB Name properties:\n");
         break;
      case kReportPhase_Loop:
         {
            const char *name;
            int val;
            if (gPropQBName->Get(obj,&name))
            {
               if (gPropQBVal->Get(obj,&val))
                  rsprintf(&p,"object %d has qb name %s, qb val %d\n",obj, name, val);
               else
                  rsprintf(&p,"object %d has qb name %s, qb val not set.\n",obj, name);
            }
         }
         break;
      case kReportPhase_Term:         
         rsprintf(&p, "---------------\n");
         break;
   }
}

static void _shklog_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   char *p=buffer;

   if (WarnLevel<kReportLevel_Warning) return;  

   switch (phase)
   {
      case kReportPhase_Init:
         rsprintf(&p, "Objects with Log / Email properties:\n");
         break;
      case kReportPhase_Loop:
         {
            int i,j;
            sLogData *ldp;
            BOOL set = FALSE;

            for (i=0; i < NUM_PDA_LEVELS; i++)
            {
               if (g_LogProperties[i]->Get(obj,&ldp))
               {
                  for (j=0; j < 32; j++)
                  {
                     if (ldp->m_data[0] & (1 << j))
                     {
                        if (!set)
                        {
                           if (WarnLevel>=kReportLevel_Info) 
                              rsprintf(&p,"object %d has level %d, email %d\n",obj,i+1,j);
                           set = TRUE;
                        }
                        else
                           rsprintf(&p,"WARNING: object %d has multiple email/log bits set!\n",obj);
                     }
                     if (ldp->m_data[1] & (1 << j))
                     {
                        if (!set)
                        {
                           if (WarnLevel>=kReportLevel_Info) 
                              rsprintf(&p,"object %d has level %d, log %d\n",obj,i+1,j);
                           set = TRUE;
                        }
                        else
                           rsprintf(&p,"WARNING: object %d has multiple email/log bits set!\n",obj);
                     }
                  }
               }
            }
         }
         break;
      case kReportPhase_Term:         
         rsprintf(&p, "---------------\n");
         break;
   }
}

// 0-5 for difficulty
// 6 for "all diffs"
static cObjCountTable count_tables[7];

static void AddByDifficulty(ObjID obj)
{
   BOOL permitted[6];
   BOOL inall = TRUE;
   int i,count;
   ObjID myarch;

   AutoAppIPtr(TraitManager);

   // since 6 is the fake difficulty
   for (i=0; i < 6; i++)
   {
      if (DarkDifficultyIsAllowed(obj,i))
         permitted[i] = TRUE;
      else
      {
         permitted[i] = FALSE;
         inall = FALSE;
      }
   }

   myarch = pTraitManager->GetArchetype(obj);

   // are we available to all?
   if (inall)
   {
      count = count_tables[6].Search(myarch);
      count_tables[6].Set(myarch,count+1);
   }
   else
   {
      // otherwise, note all difficulty levels that we exist at
      for (i=0; i < 6; i++)
      {
         if (permitted[i])
         {
            count = count_tables[i].Search(myarch);
            count_tables[i].Set(myarch,count+1);
         }
      }
   }
}

static void SpewTable(int i, char **p, const char *title)
{
   cObjCountTable::cIter hashiter;
   AutoAppIPtr(ObjectSystem);
   int count;

   hashiter = count_tables[i].Iter();
   count = count_tables[i].nElems();
   if (count > 0)
      rsprintf(p,"%s\n",title);
   while (!hashiter.Done())
   {
      rsprintf(p, "%s (%d)\n",pObjectSystem->GetName(hashiter.Key()),hashiter.Value());
      hashiter.Next();
   }
}

static void core_loot_report(char *name, ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   BOOL loud=(WarnLevel==kReportLevel_DumpAll);
   char *p=buffer;
   int i;
   ObjID arch;

   if (WarnLevel<kReportLevel_Info) return;  

   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);

   switch (phase)
   {
      case kReportPhase_Init:
         for (i=0; i < 7; i++)
            count_tables[i].Clear();
         break;
      case kReportPhase_Loop:
         arch = pObjectSystem->GetObjectNamed(name);
         if (pTraitManager->ObjHasDonor(obj,arch))
         {
            AddByDifficulty(obj);
         }
         break;
      case kReportPhase_Term:         
         {
            // first describe all the base creations
            char temp[255];
            sprintf(temp,"%s at all difficulties:",name);
            SpewTable(6,&buffer,temp);
            for (i=0; i < 6; i++)
            {
               sprintf(temp,"\nExtra %s at difficulty %d:",name,i);
               SpewTable(i,&buffer,temp);
            }
            rsprintf(&buffer, "---------------\n");
         }
         break;
   }
}

static void _shkmonster_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Monsters",obj,WarnLevel,data,buffer,phase);
}

static void _shkgoodies_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Goodies",obj,WarnLevel,data,buffer,phase);
}

static void _shkweapon_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Weapon",obj,WarnLevel,data,buffer,phase);
}

static void _shkkey_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Keys",obj,WarnLevel,data,buffer,phase);
}

static void _shkfunctional_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Functional",obj,WarnLevel,data,buffer,phase);
}

   // okay, sift through every object in the world and collect stats
   /*
   pQuery = pObjectSystem->Iter(kObjectAbstract);
   while (!pQuery->Done())
   {
      obj = pQuery->Object();
      pQuery->Next();
   }
   */

// Report prints out all models used by abstract objects
static void _shkexp_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);

   IObjectQuery* pQuery;
   ObjID obj,arch,arch2;
   int total = 0;
   int expval;
   sPropertyObjIter iter;

   if (WarnLevel<kReportLevel_Info) return;  

   rsprintf(&buffer,"\nEXP Report\n");

   gPropExp->IterStart(&iter);
   while (gPropExp->IterNext(&iter,&obj))
   {
      if (OBJ_IS_CONCRETE(obj))
      {
         gPropExp->Get(obj,&expval);
         arch = pTraitManager->GetArchetype(obj);
         rsprintf(&buffer,"Obj %d (%s) provides %d EXP.\n",obj,pObjectSystem->GetName(arch),expval);
         total = total + expval;
      }
   }
   gPropExp->IterStop(&iter);

   // now iterate over all of the physical cookies
   pQuery = pObjectSystem->Iter(kObjectConcrete);
   arch = pObjectSystem->GetObjectNamed("EXP Cookies");
   while (!pQuery->Done())
   {
      obj = pQuery->Object();
      if (pTraitManager->ObjHasDonor(obj,arch))
      {
         // use "normal" difficulty
         if (DarkDifficultyIsAllowed(obj,2))
         {
            if (!gStackCountProp->Get(obj,&expval))
               expval = 0;
            arch2 = pTraitManager->GetArchetype(obj);
            rsprintf(&buffer,"Obj %d (%s) provides %d EXP.\n",obj,pObjectSystem->GetName(arch2),expval);
            total = total + expval;
         }
      }
      pQuery->Next();
   }

   // now print out final results
   rsprintf(&buffer, "Total EXP on level: %d\n",total);
   rsprintf(&buffer, "-----------\n");
}

static void _shkrespawn_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(ObjectSystem);
   ObjID obj, obj2;
   int count = 0;

   rsprintf(&buffer,"\nRespawn Markers\n");

   obj = pObjectSystem->GetObjectNamed("Multiplayer Respawn");
   if (obj == OBJ_NULL)
      rsprintf(&buffer,"ERROR: No multiplayer respawn marker!\n");

   if (WarnLevel >= kReportLevel_Warning)
   {
      obj = pObjectSystem->GetObjectNamed("Respawn Marker");
      obj2 = pObjectSystem->GetObjectNamed("Hack Respawn Marker");
      if ((obj == OBJ_NULL) && (obj2 == OBJ_NULL))
      {
         rsprintf(&buffer,"WARNING: No respawn point on level!\n");
      }
   }

   // now print out
   rsprintf(&buffer, "-----------\n");
}

static void _shkrad_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(ObjectSystem);
   ObjID obj;
   sPropertyObjIter iter;
   float rad,absorb;

   if (WarnLevel<kReportLevel_Info) return;  

   rsprintf(&buffer,"\nRadiation Report\n");

   gPropRadLevel->IterStart(&iter);
   while (gPropRadLevel->IterNext(&iter,&obj))
   {
      if (OBJ_IS_CONCRETE(obj))
      {
         gPropRadLevel->Get(obj,&rad);
         if (gPropRadAbsorb->Get(obj,&absorb))
            rsprintf(&buffer,"Obj %d (%s) has radiation level %.02f, absorb %.02f.\n",obj,pObjectSystem->GetName(obj),rad, absorb);
         else
            rsprintf(&buffer,"Hey! Obj %d (%s) has radiation level %.02f, but no absorb!.\n",obj,pObjectSystem->GetName(obj),rad);
      }
   }
   gPropRadLevel->IterStop(&iter);

   // now print out
   rsprintf(&buffer, "-----------\n");
}

static void _shkeco_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);
   ObjID obj,arch;
   sPropertyObjIter iter;
   sEcologyInfo *eco;
   int ecotype;

   if (WarnLevel<kReportLevel_Info) return;  

   rsprintf(&buffer,"\nEcology Report\n");

   gPropEcology->IterStart(&iter);
   while (gPropEcology->IterNext(&iter,&obj))
   {
      gPropEcology->Get(obj,&eco);
      gPropEcoType->Get(obj,&ecotype);
      rsprintf(&buffer,"Obj %d has ecology data: type = %d, period = %.02f\n",obj,
         ecotype,eco->m_period);
      rsprintf(&buffer,"    Normal: min/max = %d / %d, rand = %d, recover = %.02f\n",eco->m_mincount[0],eco->m_maxcount[0],
         eco->m_randval[0],eco->m_recovery[0]);
      rsprintf(&buffer,"    Alert: min/max  = %d / %d, rand = %d, recover = %.02f\n",eco->m_mincount[2],eco->m_maxcount[2],
         eco->m_randval[2],eco->m_recovery[2]);
      /*
      if (gPropScripts->IsSimplyRelevant(obj))
         rsprintf(&buffer,"Obj %d is difficulty-sensitive, probably.\n",obj);
      */
   }
   gPropEcology->IterStop(&iter);

   gPropEcoType->IterStart(&iter);
   while (gPropEcoType->IterNext(&iter,&obj))
   {
      if (OBJ_IS_CONCRETE(obj))
      {
         gPropEcoType->Get(obj,&ecotype);
         arch = pTraitManager->GetArchetype(obj);
         rsprintf(&buffer,"Obj %d (%s) has ecotype %d\n",obj,pObjectSystem->GetName(arch),ecotype);
      }
   }
   gPropEcoType->IterStop(&iter);

   // now print out
   rsprintf(&buffer, "-----------\n");
}

static void _shkrep_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);
   ObjID obj;
   sPropertyObjIter iter;
   sRepContents *contents;
   int i;

   if (WarnLevel<kReportLevel_Warning) return;  

   BOOL loud = (WarnLevel >= kReportLevel_Info);

   rsprintf(&buffer,"\nReplicator Report\n");

   g_RepContentsProperty->IterStart(&iter);
   while (g_RepContentsProperty->IterNext(&iter,&obj))
   {
      if (!OBJ_IS_CONCRETE(obj))
         continue;

      g_RepContentsProperty->Get(obj,&contents);
      if (loud)
         rsprintf(&buffer,"\nObj %d is a replicator, contents:\n",obj);
      for (i=0; i < 4; i++)
      {
         if (loud && (strlen(contents->m_objs[i]) > 0))
            rsprintf(&buffer,"Slot %d: %s for %d nanites.\n",i,contents->m_objs[i],contents->m_costs[i]);
      }
      if (loud)
         rsprintf(&buffer,"Hacked contents:\n");
      if (!g_RepContentsHackedProperty->Get(obj,&contents))
      {
         rsprintf(&buffer,"WARNING: Replicator %d has no hacked contents!\n",obj);
      }
      else
      {
         for (i=0; i < 4; i++)
         {
            if (loud && (strlen(contents->m_objs[i]) > 0))
               rsprintf(&buffer,"Slot %d: %s for %d nanites.\n",i,contents->m_objs[i],contents->m_costs[i]);
         }
      }
   }
   g_RepContentsProperty->IterStop(&iter);

   // now print out terminator
   rsprintf(&buffer, "-----------\n");
}

static void _shkhack_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);
   ObjID obj,arch;
   sPropertyObjIter iter;
   sTechInfo *tech;

   if (WarnLevel<kReportLevel_Info) return;  

   rsprintf(&buffer,"\nHack Difficulty Report\n");

   gPropHackDiff->IterStart(&iter);
   while (gPropHackDiff->IterNext(&iter,&obj))
   {
      if (OBJ_IS_CONCRETE(obj))
      {
         gPropHackDiff->Get(obj,&tech);
         arch = pTraitManager->GetArchetype(obj);
         rsprintf(&buffer,"Obj %d (%s) has hack success %d, fail %d, cost %.02f.\n",obj,pObjectSystem->GetName(arch),
            tech->m_success,tech->m_critfail,tech->m_cost);
      }
   }
   gPropHackDiff->IterStop(&iter);

   // now print out
   rsprintf(&buffer, "-----------\n");
}

static void _shknanite_report(int WarnLevel, void *data, char *buffer)
{
   AutoAppIPtr(TraitManager);
   AutoAppIPtr(ObjectSystem);
   IObjectQuery* pQuery;
   ObjID obj,arch,arch2;
   int val,total;

   if (WarnLevel<kReportLevel_Info) return;  

   rsprintf(&buffer,"\nNanite Report\n");

   pQuery = pObjectSystem->Iter(kObjectConcrete);
   arch = pObjectSystem->GetObjectNamed("Nanites");
   total = 0;
   while (!pQuery->Done())
   {
      obj = pQuery->Object();
      if (pTraitManager->ObjHasDonor(obj,arch))
      {
         // use "normal" difficulty
         if (DarkDifficultyIsAllowed(obj,2))
         {
            if (!gStackCountProp->Get(obj,&val))
               val = 0;
            arch2 = pTraitManager->GetArchetype(obj);
            rsprintf(&buffer,"Obj %d (%s) provides %d Nanites.\n",obj,pObjectSystem->GetName(arch2),val);
            total = total + val;
         }
      }
      pQuery->Next();
   }

   // now print out
   rsprintf(&buffer, "Total Nanites on level: %d\n",total);
   rsprintf(&buffer, "-----------\n");
}

void ShockReportInit()
{
   ReportRegisterObjCallback(_shkinv_report,"Monster Inv.",NULL);   
   ReportRegisterObjCallback(_shkqb_report,"QuestData",NULL);   
   ReportRegisterGenCallback(_shkrespawn_report,kReportGame,"Respawn",NULL);
   ReportRegisterObjCallback(_shklog_report,"LogData",NULL);   
   ReportRegisterObjCallback(_shkmonster_report,"Monsters",NULL);   
   ReportRegisterObjCallback(_shkgoodies_report,"Goodies",NULL);   
   ReportRegisterObjCallback(_shkweapon_report,"Weapon",NULL);   
   ReportRegisterObjCallback(_shkkey_report,"Keys",NULL);   
   ReportRegisterObjCallback(_shkfunctional_report,"Functional",NULL);   
   ReportRegisterGenCallback(_shkexp_report,kReportGame,"EXP",NULL);
   ReportRegisterGenCallback(_shkrad_report,kReportGame,"Radiation",NULL);
   ReportRegisterGenCallback(_shkeco_report,kReportGame,"Ecology",NULL);
   ReportRegisterGenCallback(_shkrep_report,kReportGame,"Replicator",NULL);
   ReportRegisterGenCallback(_shkhack_report,kReportGame,"Hack",NULL);
   ReportRegisterGenCallback(_shknanite_report,kReportGame,"Nanite",NULL);
}

void ShockReportTerm(void)
{
   ReportUnRegisterObjCallback(_shkinv_report,NULL);
   ReportUnRegisterObjCallback(_shkqb_report,NULL);
   ReportUnRegisterGenCallback(_shkrespawn_report,kReportGame,NULL);
   ReportUnRegisterObjCallback(_shklog_report,NULL);
   ReportUnRegisterObjCallback(_shkmonster_report,NULL);
   ReportUnRegisterObjCallback(_shkgoodies_report,NULL);
   ReportUnRegisterObjCallback(_shkweapon_report,NULL);
   ReportUnRegisterObjCallback(_shkkey_report,NULL);
   ReportUnRegisterObjCallback(_shkfunctional_report,NULL);
   ReportUnRegisterGenCallback(_shkexp_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_shkrad_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_shkeco_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_shkrep_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_shkhack_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_shknanite_report,kReportGame,NULL);
}
#else 

void ShockReportInit()
{
}

void ShockReportTerm(void)
{
}

#endif
////////////////////////
