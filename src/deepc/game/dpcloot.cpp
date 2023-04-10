/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <mprintf.h>
#include <appagg.h>

#include <dpcloot.h>

#include <iobjsys.h>
#include <objdef.h>
#include <contain.h>

#include <dmgbase.h>
#include <playrobj.h>
#include <objpos.h>
#include <rand.h>
#include <rendprop.h>
#include <traitman.h>
#include <questapi.h>
#include <drkdiff.h>
#include <physapi.h>

#include <dpctrcst.h>
#include <dpcplayr.h>
#include <dpcplcst.h>
#include <dpcprop.h>
#include <dpcparam.h>

// must be last header
#include <dbmem.h>

// add a particular piece of loot
static void AddLootNamed(Label *name, int slot, ObjID *archs)
{
   if (slot >= MAX_LOOT_PICKS)
      return;

   IObjectSystem *objsys;

   objsys = AppGetObj(IObjectSystem);
   archs[slot] = objsys->GetObjectNamed(name->text);
}

//
// Choose one item of loot randomly among possibilities
//
static BOOL ChooseLoot(sLootInfo *pLoot, int slot, ObjID *archs, float *values)
{
   if (slot >= MAX_LOOT_PICKS)
      return(FALSE);

   // figure out total so we can randomize within
   int total = 0;
   for (int i = 0; i < MAX_LOOT_ITEMS; i++)
   {
      total = total + pLoot->m_rarity[i];
   }

   int randval = Rand() % total;
   //mprintf("randval = %d (%d %d %d %d %d %d)\n",randval,pLoot->m_rarity[0],pLoot->m_rarity[1],
   //   pLoot->m_rarity[2],pLoot->m_rarity[3],pLoot->m_rarity[4],pLoot->m_rarity[5]);
   int count = 0;
   int choose = 0;
   for (; choose < MAX_LOOT_ITEMS; choose++)
   {
      count = count + pLoot->m_rarity[choose];
      if (randval < count)
         break;
   }

   //mprintf("choosing %d\n",choose);
   if (choose < MAX_LOOT_ITEMS)
   {
      IObjectSystem *objsys;

      objsys = AppGetObj(IObjectSystem);
      if (strlen(pLoot->m_items[choose])> 0)
      {
         sDiffParams *diff;
         int rv2;
         AutoAppIPtr(QuestData);
         int g_diff = pQuestData->Get(DIFF_QVAR); 
         diff = GetDiffParams();

         // okay, every time a non-blank item comes up, we have an X percent
         // chance of just getting nothing instead.  har har.
         rv2 = Rand() % 100;
         if (rv2 <= diff->m_loothose[g_diff])
         {
            // BZZZZZT!  Thank you for playing
            archs[slot] = OBJ_NULL;
            values[slot] = 0;
         }
         else
         {
            // yay, we get to keep it
            archs[slot] = objsys->GetObjectNamed(pLoot->m_items[choose]);
            values[slot] = pLoot->m_value[choose];
         }
      }
      else
      {
         // getting nothing legitimately is always acceptable under this sadistic regime
         archs[slot] = OBJ_NULL;
         values[slot] = 0;
      }
      //mprintf("Generating arch %d (%s)\n",archs[slot],pLoot->m_items[choose]);
      return(TRUE);
   }
   return(FALSE);
}

//
//
// Put all the treasure on a corpse
//
void GenerateLoot(ObjID corpse)
{
   sLootInfo *pLoot;
   ObjID loot_archs[MAX_STAT_VAL][MAX_LOOT_PICKS];
   float loot_values[MAX_STAT_VAL][MAX_LOOT_PICKS];
   int i,n;
   ObjID oid;
   ObjPos *pos;
   float val, bestval;
   int bestset;

   AutoAppIPtr(ContainSys);
   AutoAppIPtr(DPCPlayer);

   if (!gPropLoot->Get(corpse,&pLoot))
      return;

   //statval = pDPCPlayer->GetStat(kStatCyber);
   int statval = 1; // disabled "luck" treasure system
   for (n=0; n < statval; n++)
   {
      for (i=0; i < MAX_LOOT_ITEMS; i++)
      {  
         loot_archs[n][i] = OBJ_NULL;
         loot_values[n][i] = 0;
      }

      for (i=0; i < pLoot->m_numpicks; i++)
      {
         ChooseLoot(pLoot,i,loot_archs[n],loot_values[n]);
      }

      AutoAppIPtr(DPCPlayer);
      if (gPropGuaranteedLoot->IsRelevant(corpse) && pDPCPlayer->HasTraitMulti(kTraitBorg))
      {
         Label *pLabel;
         gPropGuaranteedLoot->Get(corpse,&pLabel);
         AddLootNamed(pLabel,i,loot_archs[n]);
      }

      if (gPropReallyGuaranteedLoot->IsRelevant(corpse))
      {
         Label *pLabel;
         gPropReallyGuaranteedLoot->Get(corpse,&pLabel);
         AddLootNamed(pLabel,i,loot_archs[n]);
      }
   }
   bestset = -1;
   bestval = -1;
   for (n=0; n < statval; n++)
   {
      // sum up our value
      val = 0;
      for (i=0; i < MAX_LOOT_ITEMS; i++)
      {
         //mprintf("%g ",loot_values[n][i]);
         val = val + loot_values[n][i];
      }
      // is it better?
      //mprintf("set %d has value %g\n",n,val);
      if (val > bestval)
      {
         bestval = val;
         bestset = n;
      }
   }
   //mprintf("choosing set %d\n",bestset);
   if (bestset == -1)
   {
      Warning(("GenerateLoot: couldn't find best set!\n"));
      return;
   }

   for (i=0; i < MAX_LOOT_PICKS; i++)
   {
      if (loot_archs[bestset][i] != OBJ_NULL)
      {
         IObjectSystem *objsys;
         objsys = AppGetObj(IObjectSystem);
         oid = objsys->BeginCreate(loot_archs[bestset][i],kObjectConcrete);
         //mprintf("Creating obj id %d\n",oid);

         pos = ObjPosGet(corpse);
         ObjPosCopyUpdate(oid,pos);
         ObjSetHasRefs(oid,FALSE);
         objsys->EndCreate(oid);

         pContainSys->Add(corpse,oid,0,CTF_NONE);
         // Make sure that the loot gets created non-physical:
         if (PhysObjHasPhysics(oid))
            PhysDeregisterModel(oid);
      }
   }
}

//
// Damage Listener
//
eDamageResult LGAPI DPCLootDamageListener(const sDamageMsg* pMsg, tDamageCallbackData)
{
   switch(pMsg->kind)
   {
   case kDamageMsgTerminate:
      {
         // pMsg->victim
         if (gPropLoot->IsRelevant(pMsg->victim))
         {
            sContainIter *scip;
            AutoAppIPtr(ContainSys);
            AutoAppIPtr(TraitManager);
            AutoAppIPtr(ObjectSystem);
            // snap all existing projectile links
            scip = pContainSys->IterStart(pMsg->victim);
            while (!scip->finished)   
            {
               ObjID obj,projarch;

               pContainSys->IterNext(scip);
               
               obj = scip->containee;
               projarch = pObjectSystem->GetObjectNamed("Projectiles"); // shoot me now
               if (pTraitManager->ObjHasDonor(obj,projarch))
               {
                  pContainSys->Remove(pMsg->victim,obj);   
               }
            }
            pContainSys->IterEnd(scip);

            // add some TREASURE!!
            GenerateLoot(pMsg->victim);
         }
      }
      break;
   }
   return kDamageNoOpinion;
}

