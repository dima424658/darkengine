/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Misc Deep Cover Specific report functions

#include <dpcreprt.h>

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
#include <dpcplayr.h>
#include <dpcplprp.h>
#include <dpchazpr.h>
#include <dpcspawn.h>

#include <drkdiff.h>

#include <hashpp.h>
#include <hashfunc.h>
#include <hshpptem.h>

#include <dpcprop.h>

#include <dbmem.h>

typedef cHashTableFunctions<ObjID> ObjCountHashFunctions;
typedef cHashTable<ObjID, int, ObjCountHashFunctions> cObjCountTable;

#ifdef _MSC_VER
template cObjCountTable;
#endif

static void _DPCinv_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
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

static void _DPCqb_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
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

static void _DPClog_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
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

static void _DPCmonster_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Monsters",obj,WarnLevel,data,buffer,phase);
}

static void _DPCgoodies_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Goodies",obj,WarnLevel,data,buffer,phase);
}

static void _DPCweapon_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Weapon",obj,WarnLevel,data,buffer,phase);
}

static void _DPCkey_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
{
   core_loot_report("Keys",obj,WarnLevel,data,buffer,phase);
}

static void _DPCfunctional_report(ObjID obj, int WarnLevel, void *data, char *buffer, eReportPhase phase)
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
static void _DPCexp_report(int WarnLevel, void *data, char *buffer)
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

static void _DPCrespawn_report(int WarnLevel, void *data, char *buffer)
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

static void _DPCeco_report(int WarnLevel, void *data, char *buffer)
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

static void _DPChack_report(int WarnLevel, void *data, char *buffer)
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

static void _DPCnanite_report(int WarnLevel, void *data, char *buffer)
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

void DPCReportInit()
{
   ReportRegisterObjCallback(_DPCinv_report,"Monster Inv.",NULL);   
   ReportRegisterObjCallback(_DPCqb_report,"QuestData",NULL);   
   ReportRegisterGenCallback(_DPCrespawn_report,kReportGame,"Respawn",NULL);
   ReportRegisterObjCallback(_DPClog_report,"LogData",NULL);   
   ReportRegisterObjCallback(_DPCmonster_report,"Monsters",NULL);   
   ReportRegisterObjCallback(_DPCgoodies_report,"Goodies",NULL);   
   ReportRegisterObjCallback(_DPCweapon_report,"Weapon",NULL);   
   ReportRegisterObjCallback(_DPCkey_report,"Keys",NULL);   
   ReportRegisterObjCallback(_DPCfunctional_report,"Functional",NULL);   
   ReportRegisterGenCallback(_DPCexp_report,kReportGame,"EXP",NULL);
   ReportRegisterGenCallback(_DPCeco_report,kReportGame,"Ecology",NULL);
   ReportRegisterGenCallback(_DPChack_report,kReportGame,"Hack",NULL);
   ReportRegisterGenCallback(_DPCnanite_report,kReportGame,"Nanite",NULL);
}

void DPCReportTerm(void)
{
   ReportUnRegisterObjCallback(_DPCinv_report,NULL);
   ReportUnRegisterObjCallback(_DPCqb_report,NULL);
   ReportUnRegisterGenCallback(_DPCrespawn_report,kReportGame,NULL);
   ReportUnRegisterObjCallback(_DPClog_report,NULL);
   ReportUnRegisterObjCallback(_DPCmonster_report,NULL);
   ReportUnRegisterObjCallback(_DPCgoodies_report,NULL);
   ReportUnRegisterObjCallback(_DPCweapon_report,NULL);
   ReportUnRegisterObjCallback(_DPCkey_report,NULL);
   ReportUnRegisterObjCallback(_DPCfunctional_report,NULL);
   ReportUnRegisterGenCallback(_DPCexp_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_DPCeco_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_DPChack_report,kReportGame,NULL);
   ReportUnRegisterGenCallback(_DPCnanite_report,kReportGame,NULL);
}
#else 

void DPCReportInit()
{
}

void DPCReportTerm(void)
{
}

#endif
////////////////////////
