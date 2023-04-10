/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkstats.cpp,v 1.10 2000/02/09 19:29:13 bfarquha Exp $
// dark specific stat gathering

#include <mprintf.h>

#include <appagg.h>
#include <config.h>

#include <objtype.h>

// stats/quest
#include <drkstats.h>
#include <questapi.h>

// props
#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <sdesbase.h>
#include <sdesc.h>

// links
#include <linkbase.h>
#include <lnkquery.h>
#include <relation.h>
#include <autolink.h>

// misc stats we want to get
#include <simtime.h>
#include <contain.h>
#include <invtype.h> // this is in /dark
#include <drkloot.h>
#include <picklock.h>

#include <dbmem.h>

///////////////////

IIntProperty* pDarkStatProp = NULL;

///////////////////

#ifdef PLAYTEST
#define stat_track(x) do { if (config_is_defined("StatTrack")) mprintf x; } while (0)
#else
#define stat_track(x)
#endif

///////////////////
// core stuff

void _DarkStatIntAdd(char *StatName, int val, BOOL campaign)
{
   AutoAppIPtr(QuestData);
   if (pQuestData->Exists(StatName))
      pQuestData->Set(StatName,pQuestData->Get(StatName)+val);
   else
      pQuestData->Create(StatName,val,campaign?kQuestDataCampaign:kQuestDataMission);
   stat_track(("Stat %s had %d added, now %d\n",StatName,val,pQuestData->Get(StatName)));
}

int _DarkStatIntGet(char *StatName)
{
   AutoAppIPtr(QuestData);
   if (pQuestData->Exists(StatName))
      return pQuestData->Get(StatName);
   return 0;
}

void _DarkStatIntSet(char *StatName, int val, BOOL campaign)
{
   AutoAppIPtr(QuestData);
   if (pQuestData->Exists(StatName))
      pQuestData->Set(StatName,val);
   else
      pQuestData->Create(StatName,val,campaign?kQuestDataCampaign:kQuestDataMission);
   stat_track(("Stat %s set to %d, now %d\n",StatName,val,pQuestData->Get(StatName)));
}

void DarkStatIntAdd(char *StatName, int val)
{
   _DarkStatIntAdd(StatName,val,FALSE);
}

void DarkStatIntSet(char *StatName, int val)
{
   _DarkStatIntSet(StatName,val,FALSE);
}

///////////////////
// start and end analysis/tally/etc

void DarkStatFinishMission(void)
{
   stat_track(("DarkStat FinishMission\n"));

   int total_loot=count_all_loot(OBJ_NULL);
   int sim_time=GetSimTime();

   DarkStatIntSet(kDarkStatLootTotal,total_loot);
   DarkStatIntSet(kDarkStatTime,sim_time);

   // move everything out to the campaign
   _DarkStatIntAdd(kDarkStatCampTime,sim_time,TRUE);
   _DarkStatIntAdd(kDarkStatCampLoot,_DarkStatIntGet(kDarkStatLoot),TRUE);
   _DarkStatIntAdd(kDarkStatCampDamageDealt,_DarkStatIntGet(kDarkStatDamageDealt),TRUE);
   _DarkStatIntAdd(kDarkStatCampDamageTaken,_DarkStatIntGet(kDarkStatDamageTaken),TRUE);
   _DarkStatIntAdd(kDarkStatCampKills,_DarkStatIntGet(kDarkStatKills),TRUE);

   // just to test
#ifdef PLAYTEST
   if (config_is_defined("force_stats"))
   {
      DarkStatInc(kDarkStatLoot);
      DarkStatInc(kDarkStatPickPocket);
      DarkStatInc(kDarkStatPickPocketFail);
      DarkStatInc(kDarkStatPickPocketAble);
      DarkStatInc(kDarkStatLockPick);
      DarkStatInc(kDarkStatLockPickAble);
      DarkStatInc(kDarkStatDamageDealt);
      DarkStatInc(kDarkStatDamageTaken);
      DarkStatInc(kDarkStatDamageSelf);
      DarkStatInc(kDarkStatHealingTaken);
      DarkStatInc(kDarkStatObjDmgDealt);
      DarkStatInc(kDarkStatObjsKilled);
      DarkStatInc(kDarkStatKills);
      DarkStatInc(kDarkStatInnocents);
      DarkStatInc(kDarkStatSuicides);
      DarkStatInc(kDarkStatBackStabs);
      DarkStatInc(kDarkStatKnockouts);
      DarkStatInc(kDarkStatAerials);
      DarkStatInc(kDarkStatGassed);
      DarkStatInc(kDarkStatDiscovered);
      DarkStatInc(kDarkStatLootTotal);
      DarkStatInc(kDarkStatBodiesFound);
      DarkStatInc(kDarkStatReloads);
      DarkStatInc(kDarkStatGameCode);
      DarkStatInc(kDarkStatTime);
      DarkStatInc(kDarkStatRobotsKilled);
      DarkStatInc(kDarkStatRobotsDeactivated);
      _DarkStatIntAdd(kDarkStatCampTime,1,TRUE);
      _DarkStatIntAdd(kDarkStatCampLoot,1,TRUE);
      _DarkStatIntAdd(kDarkStatCampDamageDealt,1,TRUE);
      _DarkStatIntAdd(kDarkStatCampDamageTaken,1,TRUE);
      _DarkStatIntAdd(kDarkStatCampKills,1,TRUE);
      _DarkStatIntAdd(kDarkStatCampReloads,1,TRUE);
   }
#endif
}


// Helper function to return total # of secrets in mission
static int GetTotalSecrets()
   {
   int nNumSecrets = 0;
   sPropertyObjIter It;
   ObjID ThisObject;
   int bits;

   pDarkStatProp->IterStart(&It);
   while (pDarkStatProp->IterNext(&It, &ThisObject))
      if (pDarkStatProp->Get(ThisObject, &bits) && OBJ_IS_CONCRETE(ThisObject) && (bits & kDarkStatBitHidden))
         nNumSecrets++;
   pDarkStatProp->IterStop(&It);

   return nNumSecrets;
   }


void DarkStatInitMission(void)
{
   stat_track(("DarkStat StartMission\n"));
   AutoAppIPtr(QuestData);
   if (!pQuestData->Exists("InitStatsBuilt"))
   {
      pQuestData->Create("InitStatsBuilt",1,kQuestDataMission);

      cAutoLinkQuery query("Contains",LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
      int belt_cnt=0;
      for (; !query->Done(); query->Next())
	{
         if ((*(eContainType *)query->Data())==kContainTypeBelt)
            belt_cnt++;
	 if ((*(eContainType *)query->Data())==kContainTypeAlt)
	   belt_cnt++;
       }
      DarkStatIntSet(kDarkStatPickPocketAble,belt_cnt);

      DarkStatIntSet(kDarkStatLockPickAble,PickLockCountPickableLockedDoors());

      DarkStatIntSet(kDarkStatTotalSecrets,GetTotalSecrets());
   }
   else
      stat_track(("Mission thinks it is already started\n"));
}

///////////////////
// property

static sPropertyDesc DarkStatPropDesc={"DarkStat", 0, NULL, 0, 0, { "Dark GameSys", "Stats" }};
static char* darkstat_names[] = { "Innocent", "Enemy", "Hidden", "FoundBody", "Robot" };

#define NUM_STAT_STRINGS (sizeof(rend_type_strings)/sizeof(rend_type_strings[0]))
#define DARKSTAT_TYPENAME "tDarkStat"

static sFieldDesc darkstat_field[] =
{
   { "", kFieldTypeBits, sizeof(int), 0, FullFieldNames(darkstat_names) },
};

static sStructDesc darkstat_sdesc =
{
   DARKSTAT_TYPENAME,
   sizeof(int),
   kStructFlagNone,
   sizeof(darkstat_field)/sizeof(darkstat_field[0]),
   darkstat_field,
};

static sPropertyTypeDesc darkstat_tdesc = { DARKSTAT_TYPENAME, sizeof(int) };

BOOL DarkStatCheckBit(ObjID obj, int bit)
{
   int val=0;
   if (pDarkStatProp->Get(obj,&val))
      return (val&bit)!=0;
   return 0;
}

void DarkStatSetBit(ObjID obj, int bit, BOOL val)
{
   int old=0;
   pDarkStatProp->Get(obj,&old);
   if (val)
      old|=bit;
   else
      old&=~bit;
   pDarkStatProp->Set(obj,old);
}

///////////////////
// init/term

void DarkStatInit(void)
{
   StructDescRegister(&darkstat_sdesc);
   pDarkStatProp = CreateIntegralProperty(&DarkStatPropDesc,&darkstat_tdesc,kPropertyImplSparseHash);
}

void DarkStatTerm(void)
{
   SafeRelease(pDarkStatProp);
}
