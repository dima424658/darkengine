/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibassns.cpp,v 1.95 2000/02/28 11:30:48 toml Exp $
//
// Core AI sense module. Looking for a place to start? Try cAISenses::Update()
//
// @TBD (toml 06-23-98): body noticing needs to be rethought
//
// Note: sense links from the AI to itself denote events in the world the AI
// cares about but where there is no real object, or the object is transient
//

#include <lg.h>
#include <hashset.h>
#include <config.h>
#include <mprintf.h>

#include <appsfx.h>
#include <dmgbase.h>
#include <filevar.h>
#include <link.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <objpos.h>
#include <physapi.h>
#include <phmoapi.h>
#include <playrobj.h>
#include <plyrspd.h>
#include <plyrmode.h>
#include <psndinfo.h>
#include <relation.h>
#include <rendprop.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <objedit.h>
#include <propface.h>
#include <propbase.h>
#include <property.h>

#include <aiapiinf.h>
#include <aiapinet.h>
#include <aiaware.h>
#include <aibassns.h>
#include <aibasctm.h>
#include <aicblink.h>
#include <aidebug.h>
#include <aipathdb.h>
#include <aiprattr.h>
#include <aiprcore.h>
#include <aipthloc.h>
#include <aisgtcst.h>
#include <aisndtyp.h>
#include <aiteams.h>
#include <aiutils.h>
#include <aivision.h>
#include <aiwr.h>
#include <crjoint.h>
#include <weapon.h>
#include <weapprop.h>

#include <filevar.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <hshsttem.h>

//selflit prop
#include <objslit.h>

// Must be last header
#include <dbmem.h>

#define ClearVision(pAwareness) { pSense->flags &= ~(kAIAF_Seen | kAIAF_HaveLOS);  }
#define ClearAllSense(pAwareness) { pAwareness->flags &= ~(kAIAF_Sensed | kAIAF_CanRaycast | kAIAF_HaveLOS); }

#define kLowLightNorm       25
#define kMedLightNorm       50
#define kHighLightNorm      75

#define kAIPlayerUpdate    200
#define kAINonPlayerUpdate 500

#define kFreeKnowledgePeriod 1500

#define kAIDamageResonanceTime 10000

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc g_AIFreeKnowledgePropertyDesc =
{
   "AI_FreeKnow",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Free sense knowledge" },
   kPropertyChangeLocally, // net_flags
};

IIntProperty * g_pAIFreeKnowledgeProperty;

#define AIGetFreeKnowledgePeriod(obj) AIGetProperty(g_pAIFreeKnowledgeProperty, (obj), (int)kFreeKnowledgePeriod)

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc g_AISeesProjectilePropertyDesc =
{
   "AI_SeesPrj",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Sees projectiles" },
   kPropertyChangeLocally, // net_flags
};

IBoolProperty * g_pAISeesProjectileProperty;

#define AIGetSeesProjectile(obj) AIGetProperty(g_pAISeesProjectileProperty, (obj), (BOOL)TRUE)

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc g_AINoticesDamagePropertyDesc =
{
   "AI_NoticeDmg",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Notices Damage" },
   kPropertyChangeLocally, // net_flags
};

IBoolProperty * g_pAINoticesDamageProperty;

#define AIGetNoticesDamage(obj) AIGetProperty(g_pAINoticesDamageProperty, (obj), (BOOL)TRUE)

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc g_AINoticesBodiesPropertyDesc =
{
   "AI_NtcBody",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Notices Bodies" },
   kPropertyChangeLocally, // net_flags
};

IBoolProperty * g_pAINoticesBodiesProperty;

#define AIGetNoticesBodies(obj) AIGetProperty(g_pAINoticesBodiesProperty, (obj), (BOOL)TRUE)

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc g_AILaunchVisiblePropertyDesc =
{
   "AI_LaunchVis",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Projectile: Visible launch" },
   kPropertyChangeLocally, // net_flags
};

IBoolProperty * g_pAILaunchVisibleProperty;

#define AIGetLaunchVisible(obj) AIGetProperty(g_pAILaunchVisibleProperty, (obj), (BOOL)FALSE)

///////////////////////////////////////////////////////////////////////////////

static sPropertyDesc g_AIOnlyNoticesPlayerPropertyDesc =
{
   "AI_OnlyPlayer",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Only Notices Player" },
   kPropertyChangeLocally, // net_flags
};

IBoolProperty * g_pAIOnlyNoticesPlayerProperty;

#define AIGetOnlyNoticesPlayer(obj) AIGetProperty(g_pAIOnlyNoticesPlayerProperty, (obj), (BOOL)FALSE)

///////////////////////////////////////////////////////////////////////////////

#define SOUND_RAD_MUL      1.3
#define SOUND_DB_MUL       1.7

static sAISoundTweaks g_AIDefaultSoundTweaks =
{
   // Ranges by type
   {
      20*SOUND_RAD_MUL,
      25*SOUND_RAD_MUL,
      18*SOUND_RAD_MUL,
      25*SOUND_RAD_MUL,
      30*SOUND_RAD_MUL,
      35*SOUND_RAD_MUL
   }
};

///////////////////

// the DB values which match the radii
static int g_aAISoundDBVals[kAIST_Num];
// the actual structure with the data
sAISoundTweaks * g_pAISoundTweaks;

class cAISoundTweak : public cFileVar2<sAISoundTweaks>
{
public:
   cAISoundTweak()
      : cFileVar2<sAISoundTweaks> (kGameSysVar,
                                   "AISNDTWK",
                                   "AI Sound Tweaks",
                                   FILEVAR_TYPE(sAISoundTweaks),
                                   1, 3,
                                   1, 3,
                                   NULL,
                                   &g_pAISoundTweaks,
                                   &g_AIDefaultSoundTweaks)
   {
   }

   void Update()
   {
      int i;
      for (i=(int)kAIST_None; i<(int)kAIST_Num; i++)
         g_aAISoundDBVals[i]=SFX_Attenuate(0,defaultRanges[i]*SOUND_DB_MUL);
   }
};

cAISoundTweak g_AISoundTweaks;  // here is the class itself, woo woo

///////////////////////////////////////

struct sHearingStats
{
   float dist_muls[6];  // @TODO: should be Number of ability levels
   int   db_adds[6];
};

#define _one_hear_stat(name, offset) \
   { name": DistMul",    kFieldTypeFloat, FieldLocation(sHearingStats, dist_muls[offset]), }, \
   { "         DB Add",  kFieldTypeInt,   FieldLocation(sHearingStats, db_adds[offset]), }

static sFieldDesc hear_stat_fields[] =
{
   _one_hear_stat("VeryLow",1),
   _one_hear_stat("Low",2),
   _one_hear_stat("Normal",3),
   _one_hear_stat("High",4),
   _one_hear_stat("VeryHigh",5),
};

static sStructDesc _g_AIHearStatStructDesc = StructDescBuild(sHearingStats,kStructFlagNone,hear_stat_fields);

sFileVarDesc gHearStatDesc =
  { kGameSysVar,"AIHearStat", "AIHearingStats",FILEVAR_TYPE(sHearingStats),{1,0},{1,0} };

static sHearingStats def_hear_stats =
{
  { 0, 0.25, 0.65, 1.0, 1.5, 3.0 },
  { 1000000, 1000, 200, 0, -200, -1000 }
};

class cHearingStats : public cFileVar<sHearingStats,&gHearStatDesc>
{
   void Reset()
   {
      sHearingStats& vars = *this;
      vars = def_hear_stats;
   }
};

static cHearingStats gHearingStats;

///////////////////////////////////////

static sFieldDesc _g_AISoundTweaksFieldDesc[] =
{
   { "Default Untyped Range",        kFieldTypeInt,  FieldLocation(sAISoundTweaks, defaultRanges[0]) },
   { "Default Inform Range",         kFieldTypeInt,  FieldLocation(sAISoundTweaks, defaultRanges[1]) },
   { "Default Minor Anomoly Range",  kFieldTypeInt,  FieldLocation(sAISoundTweaks, defaultRanges[2]) },
   { "Default Major Anomoly Range",  kFieldTypeInt,  FieldLocation(sAISoundTweaks, defaultRanges[3]) },
   { "Default Non-combat High Range", kFieldTypeInt, FieldLocation(sAISoundTweaks, defaultRanges[4]) },
   { "Default Combat Range",         kFieldTypeInt,  FieldLocation(sAISoundTweaks, defaultRanges[5]) },
};

static sStructDesc _g_AISoundTweaksStructDesc = StructDescBuild(sAISoundTweaks, kStructFlagNone, _g_AISoundTweaksFieldDesc);

///////////////////////////////////////////////////////////////////////////////

struct sAcuitySet
{
   float toLighting;
   float toMovement;
   float toExposure;
};

typedef int eAIAcuitySetIdx;

enum eAIAcuitySetIdx_
{
   kAcuityNormalIdx =   0,
   kAcuityPeriphIdx =   1,
   kAcuityOmniIdx =     2,
   kAcuityLightIdx =    3,
   kAcuityMoveIdx =     4,
   kAcuityLowLightIdx = 5,
   kAcuityIdxNum,
};

struct sAIAcuitySets
{
   sAcuitySet set[kAcuityIdxNum];
};

sAIAcuitySets _g_AIDefaultConeAcuities =
{
   {
      {   1,   1,   1 },      // Normal
      { 0.3,   3,   1 },      // Periph
      { 0.8, 1.4, 1.2 },      // Omni
      // @TODO: make these really work, then figure them out, then use them!!
      {   1,   0,   0 },      // Light only
      {   0,   5,   0 },      // Move only -- how do we normalize these
      { 6.0,   1,   1 },      // Low light powerful vision
   }
};

///////////////////////////////////////////////////////////////////////////////

sAIAcuitySets *g_pAIConeAcuities;

class cAIAcuitySets: public cFileVar2<sAIAcuitySets>
{
public:
   cAIAcuitySets()
      : cFileVar2<sAIAcuitySets> (kGameSysVar,
                                   "AIACS",
                                   "AI Acuity Set",
                                   FILEVAR_TYPE(sAIAcuitySets),
                                   1, 0,
                                   1, 0,
                                   NULL,
                                   &g_pAIConeAcuities,
                                   &_g_AIDefaultConeAcuities)
   {
   }

};

cAIAcuitySets g_AIAcuitySets;

#define AI_ACUITY_FIELDS(name,index) \
   { name": Lighting",    kFieldTypeFloat, FieldLocation(sAIAcuitySets, set[index].toLighting) }, \
   { name": Movement",  kFieldTypeFloat, FieldLocation(sAIAcuitySets, set[index].toMovement) }, \
   { name": Exposure",  kFieldTypeFloat, FieldLocation(sAIAcuitySets, set[index].toExposure) }

static sFieldDesc _g_AIAcuitySetsFieldDesc[] =
{
      AI_ACUITY_FIELDS("Normal",       kAcuityNormalIdx),
      AI_ACUITY_FIELDS("Peripheral",   kAcuityPeriphIdx),
      AI_ACUITY_FIELDS("Omni",         kAcuityOmniIdx),
      AI_ACUITY_FIELDS("Light Only",   kAcuityLightIdx),
      AI_ACUITY_FIELDS("Movement Only",kAcuityMoveIdx),
      AI_ACUITY_FIELDS("Low Light",    kAcuityLowLightIdx),
};

static sStructDesc _g_AIAcuitySetsStructDesc = StructDescBuild(sAIAcuitySets, kStructFlagNone, _g_AIAcuitySetsFieldDesc);

///////////////////////////////////////////////////////////////////////////////

static eAIAwareLevel g_SoundTypeToAware[kAIST_Num] =
{
   kAIAL_Lowest,
   kAIAL_Lowest,
   kAIAL_Low,
   kAIAL_Moderate,
   kAIAL_High,
   kAIAL_High,
};

///////////////////////////////////////////////////////////////////////////////

// Variables set by cheat commands.
// @TBD: Currently only useful if PlayerObject() is hosting the AI (change this?)
BOOL g_fAIAwareOfPlayer = TRUE;
BOOL g_fAIForgetPlayer = FALSE;

///////////////////////////////////////

sAIVisibilityControl g_AIDefVisCtrl;

///////////////////////////////////////

sAIAwareCapacitor g_AIDefAwareCap =
{
   4000,
   8000,
   45000
};

///////////////////////////////////////

sAIVisionDesc g_AIDefVisionDesc =
{
   // Reserved
   AI_VISDESC_RESERVED,

   // Vision cones, in the order that they should be evaluated
   {
      // flags          angle    zAngle   range acuity

      // Close up near perfect vision, high alert
      {  kAIVC_Active|kAIVC_LowLight|kAIVC_NoAlert0|kAIVC_NoAlert1,
                        170,     170,     5,    1500,    },

      // Close up near perfect vision, low alert
      {  kAIVC_Active|kAIVC_LowLight|kAIVC_NoAlert2|kAIVC_NoAlert3,
                        50,     100,     5,    1500,    },

      {  kAIVC_Active|kAIVC_NoAlert2|kAIVC_NoAlert3,
                        170,     170,     5,    1500,    },

      // Near cone perfect vision
      {  kAIVC_Active,  60,      90,     10,    200,     },

#define OMNI_ALLOWED
#ifdef OMNI_ALLOWED
      // Round the back magic vision
      {  kAIVC_Active|kAIVC_NoAlert0|kAIVC_NoAlert1|kAIVC_Omni,
                        320,     90,      6,    70,      },
#endif

      // would be cool to get rid of one of these...

#define NORMAL_BIN
#ifdef NORMAL_BIN
      // want to focus this at higher alertness
      // Normal near binocular vision1
      {  kAIVC_Active,  120,     90,      22,   120,     },
#endif

//#define FANCY_BIN
#ifdef FANCY_BIN
      // want to focus this at higher alertness
      // Normal near binocular vision
      {  kAIVC_Active|kAIVC_NoAlert2|kAIVC_NoAlert3,
                        130,     90,      20,   115,     },
      {  kAIVC_Active|kAIVC_NoAlert0|kAIVC_NoAlert1,
                         90,     80,      25,   150,     },
#endif

      // Mid-range sight
      {  kAIVC_Active,  150,     70,      35,   80,      },

      // Long range and wide range peripheral vision
      {  kAIVC_Active|kAIVC_Periph,
                        230,     70,      60,   60,      },

      // Long range and high Z
      {  kAIVC_Active|kAIVC_Periph,
                        230,     110,     80,   40,      },
   },

   // Z Offset
   3,
};

///////////////////////////////////////////////////////////////////////////////

sAIAlertSenseMults g_AIDefAlertSenseMults =
{
   {
   //   XYMult   ZMult    RMult    KMult
      {  1.0,     1.0,     1.0,     1.0  },     // Alert 0
      {  1.0,     1.0,     1.0,     1.0  },     // Alert 1
      {  1.15,    1.15,    1.1,     1.25 },     // Alert 2
      {  1.30,    1.25,    1.25,    2.0  },     // Alert 3
   },

   // CombatKMult
   1.666666
};

///////////////////////////////////////////////////////////////////////////////

sAIVisibilityMods g_AIDefVisibilityMods = {1, 1, 1, 1, 1, 1,};

///////////////////////////////////////////////////////////////////////////////

void AIInitSenses()
{
   g_AIDefVisCtrl.lowVisibility  = 13;
   g_AIDefVisCtrl.midVisibility  = 19;
   g_AIDefVisCtrl.highVisibility = 44;

   g_AIDefVisCtrl.lowSpeed      = SLOW_MOVE_SPEED - 0.01;
   g_AIDefVisCtrl.highSpeed     = SLOW_MOVE_SPEED + (MOVE_SPEED - SLOW_MOVE_SPEED) / 2;

   g_AIDefVisCtrl.lowSpeedMod   = 0;
   g_AIDefVisCtrl.normSpeedMod  = 5;
   g_AIDefVisCtrl.highSpeedMod  = 10;

   g_AIDefVisCtrl.wallDist      = 1.75;
   g_AIDefVisCtrl.crouchMod     = -3;
   g_AIDefVisCtrl.wallMod       = -1;

   g_AIDefVisCtrl.cornerLeanMod = 0.65;

   StructDescRegister(&_g_AIAcuitySetsStructDesc);
   StructDescRegister(&_g_AISoundTweaksStructDesc);
   StructDescRegister(&_g_AIHearStatStructDesc);

   g_pAIFreeKnowledgeProperty = CreateIntProperty(&g_AIFreeKnowledgePropertyDesc, kPropertyImplVerySparse);
   g_pAISeesProjectileProperty = CreateBoolProperty(&g_AISeesProjectilePropertyDesc, kPropertyImplVerySparse);
   g_pAINoticesDamageProperty = CreateBoolProperty(&g_AINoticesDamagePropertyDesc, kPropertyImplVerySparse);
   g_pAIOnlyNoticesPlayerProperty = CreateBoolProperty(&g_AIOnlyNoticesPlayerPropertyDesc, kPropertyImplVerySparse);
   g_pAINoticesBodiesProperty = CreateBoolProperty(&g_AINoticesBodiesPropertyDesc, kPropertyImplVerySparse);
   g_pAILaunchVisibleProperty = CreateBoolProperty(&g_AILaunchVisiblePropertyDesc, kPropertyImplVerySparse);
}

///////////////////////////////////////

void AITermSenses()
{
   SafeRelease(g_pAIFreeKnowledgeProperty);
   SafeRelease(g_pAISeesProjectileProperty);
   SafeRelease(g_pAINoticesDamageProperty);
   SafeRelease(g_pAIOnlyNoticesPlayerProperty);
   SafeRelease(g_pAINoticesBodiesProperty);
   SafeRelease(g_pAILaunchVisibleProperty);
}

///////////////////////////////////////////////////////////////////////////////

struct sAIQueuedSound
{
   sSoundInfo   info;
   sAISoundType type;
};

///////////////////////////////////////

class cAIObjectSounds : public cHashSet<sAIQueuedSound *, ObjID, cHashFunctions>
{
public:
   cAIObjectSounds()
    : cHashSet<sAIQueuedSound *, ObjID, cHashFunctions>(kHS_Tiny) {}

   ~cAIObjectSounds()
   {
      DestroyAll();
   }

   tHashSetKey GetKey(tHashSetNode node) const
   {
      return (tHashSetKey)(((sAIQueuedSound *)node)->info.SrcObject);
   }
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISenses
//

#define kFrameBudget 4
BOOL cAISenses::gm_FrameBudget;

///////////////////////////////////////

cAISenses::cAISenses()
 : m_Timer(AICustomTime(kAIPlayerUpdate)),
   m_OthersTimer(AICustomTime(2000)),
   m_MaxInformTimer(kAIT_1Sec),
   m_pObjectSounds(new cAIObjectSounds),
   m_pPositionSound(NULL),
   m_LastPlayerDamage(0),
   m_LastPlayerProjectile(0),
   m_pInform(NULL),
   m_flags(0),
   m_SenseCombatTimer(AICustomTime(kAIPlayerUpdate * 4))
{

}

///////////////////////////////////////

cAISenses::~cAISenses()
{
   delete m_pObjectSounds;
   delete m_pPositionSound;
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cAISenses::GetName()
{
   return "Standard senses";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::Init()
{
   SetNotifications(kAICN_BeginFrame |
                    kAICN_ModeChange |
                    kAICN_GoalChange |
                    kAICN_Sound |
                    kAICN_FoundBody |
                    kAICN_Damage |
                    kAICN_FastObj);

   m_pInform = AIGetInform(m_pAI);
   // We're part of the same object...
   if (m_pInform)
      m_pInform->Release();
   m_MaxInformTimer.Force();
   m_FreshTimer.Force();
}

///////////////////////////////////////

#define kSenseVer 7

STDMETHODIMP_(BOOL) cAISenses::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Senses, 0, kSenseVer, pTagFile))
   {
      SaveNotifications(pTagFile);

      m_Timer.Save(pTagFile);
      m_OthersTimer.Save(pTagFile);
      m_MaxInformTimer.Save(pTagFile);
      m_SenseCombatTimer.Save(pTagFile);
      AITagMove(pTagFile, &m_LastPlayerDamage);
      AITagMove(pTagFile, &m_LastPlayerProjectile);
      AITagMove(pTagFile, &m_LastProjectileLoc);
      AITagMove(pTagFile, &m_flags);

      m_FreshTimer.Save(pTagFile);

      m_ModerateDelay.Save(pTagFile);
      m_ModerateDelayReuse.Save(pTagFile);
      m_HighDelay.Save(pTagFile);
      m_HighDelayReuse.Save(pTagFile);

      int num;
      tHashSetHandle  handle;
      sAIQueuedSound * pSound;

      num = m_pObjectSounds->GetCount();
      AITagMove(pTagFile, &num);
      pSound = m_pObjectSounds->GetFirst(handle);
      while (pSound != NULL)
      {
         AITagMove(pTagFile, pSound);
         pSound = m_pObjectSounds->GetNext(handle);
      }

      if (m_pPositionSound)
      {
         num = 1;
         AITagMove(pTagFile, &num);
         AITagMove(pTagFile, m_pPositionSound);
      }
      else
      {
         num = 0;
         AITagMove(pTagFile, &num);
      }

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAISenses::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_Senses, 0, kSenseVer, pTagFile))
   {
      LoadNotifications(pTagFile);

      m_Timer.Load(pTagFile);
      m_OthersTimer.Load(pTagFile);
      m_MaxInformTimer.Load(pTagFile);
      m_SenseCombatTimer.Load(pTagFile);
      AITagMove(pTagFile, &m_LastPlayerDamage);
      AITagMove(pTagFile, &m_LastPlayerProjectile);
      AITagMove(pTagFile, &m_LastProjectileLoc);
      AITagMove(pTagFile, &m_flags);

      m_FreshTimer.Load(pTagFile);

      m_ModerateDelay.Load(pTagFile);
      m_ModerateDelayReuse.Load(pTagFile);
      m_HighDelay.Load(pTagFile);
      m_HighDelayReuse.Load(pTagFile);

      sAIQueuedSound * pSound;

      int num;
      AITagMove(pTagFile, &num);
      for (; num; num--)
      {
         pSound = new sAIQueuedSound;
         AITagMove(pTagFile, pSound);
         m_pObjectSounds->Insert(pSound);
      }

      AITagMove(pTagFile, &num);
      if (num)
      {
         m_pPositionSound = new sAIQueuedSound;
         AITagMove(pTagFile, m_pPositionSound);
      }
      else
         m_pPositionSound = NULL;

      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnBeginFrame()
{
   gm_FrameBudget = kFrameBudget;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnModeChange(eAIMode previous, eAIMode mode)
{
   if (mode == kAIM_Dead)
   {
      LinkDestroyMany(GetID(), LINKOBJ_WILDCARD, g_pAIAwarenessLinks->GetID());
      LinkDestroyMany(LINKOBJ_WILDCARD, GetID(), g_pAIAwarenessLinks->GetID()); // force other AIs to resense the dead AI
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnGoalChange(const cAIGoal * pPrevious, const cAIGoal * pGoal)
{
   m_FreshTimer.Force();
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnSound(const sSoundInfo * pSoundInfo, const sAISoundType * pType)
{
   if (pSoundInfo->SrcObject==GetID())
      return;  // ignore myself

   AIWatch4(Hear, GetID(),"Heard %d (%s) from %s (d %d)", pType->type, ObjWarnName(pSoundInfo->SchemaID), ObjWarnName(pSoundInfo->SrcObject), (int)pSoundInfo->Distance);

   if (!pType->type)
      return;

   if (pType->type == kAIST_Combat)
   {
      m_flags |= kSensedCombat;
   }

   sAIQueuedSound * p;
   BOOL fObjIsAI = ObjIsAI(pSoundInfo->SrcObject);
   BOOL fObject = (IsAPlayer(pSoundInfo->SrcObject) || fObjIsAI);

   int teamRelation = (fObject) ? AITeamCompare(AIGetTeam(pSoundInfo->SrcObject), AIGetTeam(GetID())) :
                                  kAI_Indifferent;

   static int doInform[kAIST_Num] =
   {
      FALSE,
      TRUE,
      TRUE,
      TRUE,
      FALSE,
      FALSE,
   };

   // Deal with "inform" sounds generating borrowed awareness
   if (fObject &&
       m_MaxInformTimer.Expired() &&
       doInform[pType->type] &&
       m_pInform && m_pInform->WantsInforms() &&
       fObjIsAI &&
       AITeamCompare(pSoundInfo->SrcObject, GetID()) == kAI_Teammates)
   {
      InformFrom(pSoundInfo->SrcObject);
   }

   // Deal with intrinsic awareness generators
   BOOL fTreatAsPos = (!fObject || (fObjIsAI && pType->type == kAIST_Combat));

   // @TBD: recode this to be more straightforward, probably
   //   the "parallelism" between the 2 cases (on fTreatAsPos)
   //   is a problem with the central loop, see comment below on

   if (!fTreatAsPos)
      p = m_pObjectSounds->Search(pSoundInfo->SrcObject);
   else
      p = m_pPositionSound;

   if (p)
   {
      if (p->type.type > pType->type)
         return;

      if (p->type.type == pType->type && FunctionallySofter(&p->info,pSoundInfo))
         return;

      // need to test fTreatAsPos too
      // otherwise, if AI case, we Remove a p which is really m_pPositionSound
      // however, if there is an objsound from that ObjID also,
      //   we remove it, since remove is by p.key, not p itself
      // thus we remove something which isnt us, which is also never dealloced
      if (fObject && !fTreatAsPos)
         m_pObjectSounds->Remove(p);
   }
   else
      p = new sAIQueuedSound;

   p->info = *pSoundInfo;
   p->type = *pType;

   if (!fTreatAsPos)
      m_pObjectSounds->Insert(p);
   else
      m_pPositionSound = p;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnFoundBody(ObjID body)
{
   m_flags |= kSensedCombat;

   eAIAwareLevel level = kAIAL_High;

   LinkID         linkID;
   sAIAwareness * pAwareness;

   linkID = GetAwarenessLink(GetID(), &pAwareness);

   if (pAwareness->level <= level)
   {
      mxs_vector loc;

      GetBestInWorld(*GetObjLocation(body), &loc);

      Pulse(pAwareness, level, loc, kAIAL_Lowest, loc);
      SetAwareness(GetID(), linkID, pAwareness);
   }
}
///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnFoundSuspicious(ObjID suspobj)
{
//do nothing.
}

///////////////////////////////////////

#define kProjectileTime 2500
#define kRedamageBonusTime 10000

STDMETHODIMP_(void) cAISenses::OnDamage(const sDamageMsg * pMsg, ObjID realCulpritID)
{
   if (!AIGetNoticesDamage(GetID()))
      return;

   if (pMsg->kind != kDamageMsgDamage)
      return;

   BOOL fPlayer = IsAPlayer(realCulpritID);

   if (!fPlayer &&
       (!m_LastPlayerProjectile || AIGetTime() - m_LastPlayerProjectile > kProjectileTime))
   {
      return;
   }

   // Determine damage source..
   mxs_vector suspectedOrigin;
   if (fPlayer)
      GetObjLocation(realCulpritID, &suspectedOrigin);
   else
      suspectedOrigin = m_LastProjectileLoc;

   eAIAwareLevel level = kAIAL_High;

   LinkID         linkID;
   LinkID         playerLinkID;
   sAIAwareness * pPlayerAwareness;
   sAIAwareness * pAwareness;

   linkID = GetAwarenessLink(GetID(), &pAwareness);

#ifdef NEW_NETWORK_ENABLED
   // When in a multi-player game, the pMsg->culprit must be a player in order
   // to set the awareness to that player.
   AutoAppIPtr(AINetServices);
   if (!pAINetServices->Networking())
      playerLinkID = GetAwarenessLink(PlayerObject(), &pPlayerAwareness);
   else if (fPlayer)
      playerLinkID = GetAwarenessLink(realCulpritID, &pPlayerAwareness);
   else
   {
      pPlayerAwareness = NULL;
      playerLinkID = NULL;
   }
#else
   playerLinkID = GetAwarenessLink(PlayerObject(), &pPlayerAwareness);
#endif

   if (pPlayerAwareness != NULL && g_fAIAwareOfPlayer && !g_fAIForgetPlayer)
   {
      if (pPlayerAwareness->level >= kAIAL_Moderate || AIGetTime() - m_LastPlayerProjectile > kRedamageBonusTime)
      {
         pAwareness = pPlayerAwareness;
         linkID = playerLinkID;
      }
   }

#if 0 // wsf: uh, this is always true, right?
   if (pAwareness->level <= level)
   {
#endif
      mxs_vector loc;

      GetBestInWorld(suspectedOrigin, &loc);

      // wsf: Added this condition to fix T2 blackjack bug where blackjacking caused temporary blindess, thus making
      // it the ultimate weapon.
      if (pAwareness->flags & kAIAF_Seen)
         Pulse(pAwareness, pAwareness->level, pAwareness->lastPos, level, loc);
      else
         Pulse(pAwareness, kAIAL_Lowest, loc, level, loc);

      SetAwareness(pAwareness->object, linkID, pAwareness);
      m_LastPlayerDamage = AIGetTime();
#if 0
   }
#endif
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::OnFastObj(ObjID firer, ObjID projectile, const mxs_vector & velocity)
{
   // @TOM: I think this is the right thing to do? (Jon)
   if (!AIGetSeesProjectile(GetID()))
      return;

   if (IsAPlayer(firer))
   {
      const mxs_vector * pFirerLoc = GetObjLocation(firer);
      BOOL               we_care = FALSE;

#define kICareDist (3.9)
      // we have a line from pFirerLoc(x,y) along velocity(x,y)
      // we want to know how far from that line we (GetLocation(x,y)) are
      mxs_vector delta;
      mx_sub_vec(&delta,m_pAIState->GetLocation(),pFirerLoc);
      if (mx_dot_vec(&delta,&velocity)>0)
      {
         float num1=(velocity.z*delta.y-velocity.y*delta.z);
         float num2=(velocity.x*delta.z-velocity.z*delta.x);
         float num3=(velocity.y*delta.x-velocity.x*delta.y);
         float num=(num1*num1)+(num2*num2)+(num3*num3);
         float den=(velocity.x*velocity.x)+(velocity.y*velocity.y)+(velocity.z*velocity.z);
         if (den!=0)
         {
            float dist_squared=num/den;
            if (dist_squared<(kICareDist*kICareDist))
            {
               we_care=TRUE;
               // mprintf("Yo, dist %g\n",dist_squared);
            }
         }
         else
            Warning(("Ummm... vel (0 0 0) in OnFastObj\n"));
      }

      if (we_care)
      {
         m_LastPlayerProjectile = AIGetTime();
         m_LastProjectileLoc = *pFirerLoc;

#if 0
         if (!g_fAIAwareOfPlayer || g_fAIForgetPlayer)
            return;

         sAIAwareness * pAwareness;
         LinkID         linkID;
         linkID = GetAwarenessLink(firer, &pAwareness);
         if (pAwareness->level >= kAIAL_Low && pAwareness->level < kAIAL_High && (pAwareness->flags & kAIAF_HaveLOS))
         {
            m_pAI->NotifyWitnessCombat();
            if (m_pAIState->DistSq(*pFirerLoc) < pAwareness->level >= kAIAL_Moderate)
            {
               Pulse(pAwareness, kAIAL_High, *pFirerLoc, kAIAL_High, *pFirerLoc);
               SetAwareness(firer, linkID, pAwareness);
            }
            else
            {
               Pulse(pAwareness, kAIAL_Moderate, *pFirerLoc, kAIAL_Moderate, *pFirerLoc);
               SetAwareness(firer, linkID, pAwareness);
            }
         }
#endif
      }
   }
}

///////////////////////////////////////

#define kDistSenseOtherAIs sq(45.0)

STDMETHODIMP cAISenses::Update()
{
   HRESULT result = S_FALSE;
   if (gm_FrameBudget && m_Timer.Expired())
   {
      m_flags |= kInUpdate;

      AutoAppIPtr(AINetServices);
      tAIPlayerList players;

      pAINetServices->GetPlayers(&players);

      ObjID * pPlayerID = &players[0];

      while (*pPlayerID)
      {
         Update(*pPlayerID);
         pPlayerID++;
      }

      if (m_OthersTimer.Expired())
      {
         IInternalAI ** AIs;
         int            nAIs;

         m_pAI->AccessAIs(&AIs, &nAIs);

         for (int i = 0; i < nAIs; i++)
         {
            if (AIs[i] != m_pAI &&
                m_pAIState->DistSq(*AIs[i]->GetState()->GetLocation()) < kDistSenseOtherAIs)
            {
               if (ObjHasRefs(AIs[i]->GetObjID()))
                  Update(AIs[i]->GetObjID());
            }
         }
         m_OthersTimer.Reset();
      }

      ProcessPositionSounds();

      ILinkQuery * pQuery = g_pAIAwarenessLinks->Query(GetID(), LINKOBJ_WILDCARD);
      for (; !pQuery->Done(); pQuery->Next())
      {
         TestForget(pQuery->ID(), (const sAIAwareness *)pQuery->Data());
      }
      SafeRelease(pQuery);

      #define kDistDelayLongerSq sq(40.0)
      if (m_pAIState->DistSq(*GetObjLocation(PlayerObject())) > kDistDelayLongerSq)
         m_Timer.Delay(kAIPlayerUpdate * 2.5);
      else
         m_Timer.Reset();

      gm_FrameBudget--;

      if (m_flags & kSensedCombat)
      {
         m_pAI->NotifyWitnessCombat();
         m_flags &= ~kSensedCombat;
      }
      result = S_OK;
      m_flags &= ~kInUpdate;
   }

   return result;
}

///////////////////////////////////////

LinkID cAISenses::GetAwarenessLink(ObjID objId, sAIAwareness ** ppAwareness)
{
   //
   // Grab any existing sense data, or use temporary
   //
   static sAIAwareness awarenessBuf;
   LinkID              linkID       = (objId) ? g_pAIAwarenessLinks->GetSingleLink(GetID(), objId) : NULL;
   int                 time         = AIGetTime();

   if (linkID)
      *ppAwareness = (sAIAwareness *)g_pAIAwarenessLinks->GetData(linkID);

   else
   {
      *ppAwareness = &awarenessBuf;

      memset(&awarenessBuf, 0, sizeof(awarenessBuf));

      (*ppAwareness)->lastContact = INT_MAX;

      // We need to generate a baseline, and simply assume the AI has sensed
      // the object on average for some period, for the purposes of "awareness"
      if (time > kAIT_20Hz)
         (*ppAwareness)->updateTime = time - kAIT_20Hz;
   }
   (*ppAwareness)->object = objId;
   (*ppAwareness)->linkID = linkID;
   return linkID;
}

///////////////////////////////////////
//
// Central sensing function
//

#define kMaxDistSenseSq sq(90.0)
#define kMaxZSense      45.0

STDMETHODIMP cAISenses::Update(ObjID objId, unsigned flags)
{
   int            time = AIGetTime();
   int            updatePeriod;
   LinkID         linkID;
   sAIAwareness * pAwareness;

   if (!objId || !ObjPosGet(objId))
      return S_FALSE;

   linkID = GetAwarenessLink(objId, &pAwareness);

   //
   //
   //
   updatePeriod = time - pAwareness->updateTime;

   if (!updatePeriod)
      return S_FALSE;

   if (linkID &&
       ((!IsAPlayer(objId) && updatePeriod < kAINonPlayerUpdate) ||
        updatePeriod < kAIPlayerUpdate))
      return S_FALSE;

   pAwareness->updateTime = time;

   //
   // Sense object, if not too distant
   //
   eAIAwareLevel      visionPulse = kAIAL_Lowest;
   eAIAwareLevel      soundPulse  = kAIAL_Lowest;
   const mxs_vector & locAI       = *m_pAIState->GetLocation();
   cMxsVector         locObject;
   mxs_vector         soundLoc;

   GetObjLocation(objId, &locObject);

   sVisionArgs args;
   float zDiff;

   if ((!IsAPlayer(objId) || m_pAIState->GetMode() >= kAIM_Normal) &&
       (zDiff = m_pAIState->DiffZ(locObject)) < kMaxZSense &&
       (args.distSq = m_pAIState->DistSq(locObject)) < kMaxDistSenseSq )
   {
      args.obj         = objId;
      args.flags       = flags;
      args.fromLoc     = *m_pAIState->GetPortalLocation();
      args.pTargetLoc  = &ObjPosGet(objId)->loc;
      args.lookLoc     = *args.pTargetLoc;
      args.pVisionDesc = AIGetVisionDesc(GetID());

      if (args.pVisionDesc != NULL)
      {
         args.fromLoc.vec.z += args.pVisionDesc->zOffset;
         UpdateChangedLocation(&args.fromLoc);
      }

      if (IsAPlayer(objId))
      {
         PhysGetSubModLocation(objId, PLAYER_HEAD, &args.lookLoc.vec);
         UpdateChangedLocation(&args.lookLoc);
      }

      if (UpdateSightcast(args, pAwareness))
         visionPulse = Look(args, pAwareness);

      soundPulse = Listen(objId, pAwareness, &soundLoc);
   }
   else
   {
      ClearAllSense(pAwareness);
   }

   //
   // Maybe this isn't the best way to handle this. Basically, the
   // AI is allowed to cheat if it can raycast to both the player
   // and a friend in combat with the player. (toml 10-09-98)
   //
   if (IsAPlayer(objId) &&
       m_SenseCombatTimer.Expired() &&
       pAwareness->level < kAIAL_Moderate &&
       (pAwareness->flags & kAIAF_CanRaycast) &&
       visionPulse < kAIAL_Moderate &&
       soundPulse < kAIAL_Moderate &&
       args.distSq < sq(25.0))
   {
      ILinkQuery * pQuery = g_pAIAttackLinks->Query(LINKOBJ_WILDCARD, objId);
      sLink attackLink;
      sAIAwareness * pAwarenessOfTeammate;
      sAIAwareness * pOtherAwareness;

      for (; !pQuery->Done(); pQuery->Next())
      {
         pQuery->Link(&attackLink);
         if (GetID() != attackLink.source &&
             AITeamCompare(GetID(), attackLink.source) == kAI_Teammates)
         {
            GetAwarenessLink(attackLink.source, &pAwarenessOfTeammate);
            if (pAwarenessOfTeammate->flags & kAIAF_CanRaycast)
            {
               LinkID otherLinkID = g_pAIAwarenessLinks->GetSingleLink(attackLink.source, objId);

               if (otherLinkID)
               {
                  pOtherAwareness = (sAIAwareness *)g_pAIAwarenessLinks->GetData(otherLinkID);

                  if (pOtherAwareness &&
                      pOtherAwareness->level == kAIAL_Moderate &&
                      (pAwareness->flags & kAIAF_Seen))
                  {
                     if (args.distSq < sq(10.0) && (pAwareness->flags & kAIAF_HaveLOS))
                        soundPulse = kAIAL_High;
                     else
                        soundPulse = kAIAL_Moderate;
                     soundLoc = locObject;
                     m_flags |= kSensedCombat;
                     break;
                  }
               }
            }
         }
      }
      pQuery->Release();
      m_SenseCombatTimer.Reset();
   }

#ifndef SHIP
   //
   // Bail if unaware of player
   //
   if (g_fAIForgetPlayer && objId == PlayerObject())
   {
      delete m_pObjectSounds->RemoveByKey(objId);
      ClearSense(objId);
      return S_OK;
   }
#endif

   //
   // Update the awareness and awareness links
   //
   Pulse(pAwareness, visionPulse, locObject, soundPulse, soundLoc);
   SetAwareness(objId, linkID, pAwareness);

   return S_OK;
}

///////////////////////////////////////
//
// Update the line of sight to an object from the AI. Returns TRUE if there is LOS.
//

BOOL cAISenses::UpdateSightcast(const sVisionArgs & args, sAIAwareness * pAwareness)
{
   #define kTimeReuseLOS 333
   #define kMaxDistLOSSq sq(80.0)
   #define kVisionCone   DEGREES(330)

   ulong timeSinceLos = AIGetTime() - pAwareness->LOSTime;

   if (timeSinceLos > kTimeReuseLOS || ((args.flags & kAISU_Force) && timeSinceLos))
   {
      pAwareness->LOSTime = AIGetTime();
      pAwareness->flags &= ~(kAIAF_HaveLOS | kAIAF_CanRaycast | kAIAF_Seen);

      if (args.distSq < kMaxDistLOSSq && AIGetVision(GetID()))
      {
#ifndef SHIP
         // Bail if unaware of player
         if (!g_fAIAwareOfPlayer && args.obj == PlayerObject())
            ;
         else
#endif
         if (AISightCast((Location *)&args.fromLoc, (Location *)&args.lookLoc))
            pAwareness->flags |= kAIAF_CanRaycast;
      }
   }
   return !!(pAwareness->flags & kAIAF_CanRaycast);
}

///////////////////////////////////////
//
// Update the intrinsic "visibility" of an object
//
// The "general visibility" or "unhiddeness" of the AI.  It does not strictly
// lighting cues to the asker however, it also includes concepts of
// exposure, movement, etc.  The rating is purely in terms of this AI, not the
// viewer.  Thus, this rating should only apply if the viewer already
// has line of sight with the AI.  0 means fully hidden, 100 means fully
// visible.
//

// @TBD (toml 05-04-98): real numbers here.
const long kVisibilityUpdateRate = 200;

int cAISenses::CalculateLightRating(ObjID objId, const sAIVisibilityControl * pVisCtrl)
{
   int rawLighting = 100 * AIGetObjectLighting(objId);
   int preNormBase;
   int preNormRange;
   int normBase;
   int normRange;

#ifdef DOUG_IS_TESTING
   if (IsAPlayer(objId))
      mprintf("Got player lighting %d\n",rawLighting);
#endif

   if (rawLighting > 100)
      rawLighting = 100;
   else if (rawLighting == 0)
      rawLighting = 1;

   if (rawLighting < pVisCtrl->lowVisibility)
   {
      preNormBase = 0;
      preNormRange = pVisCtrl->lowVisibility;
      normBase = 0;
      normRange = kLowLightNorm;
   }
   else if (rawLighting < pVisCtrl->midVisibility)
   {
      preNormBase  = pVisCtrl->lowVisibility;
      preNormRange = pVisCtrl->midVisibility - pVisCtrl->lowVisibility;
      normBase     = kLowLightNorm;
      normRange    = kMedLightNorm - kLowLightNorm;
   }
   else if (rawLighting < pVisCtrl->highVisibility)
   {
      preNormBase  = pVisCtrl->midVisibility;
      preNormRange = pVisCtrl->highVisibility - pVisCtrl->midVisibility;
      normBase     = kMedLightNorm;
      normRange    = kHighLightNorm - kMedLightNorm;
   }
   else
   {
      preNormBase  = pVisCtrl->highVisibility;
      preNormRange = 100 - pVisCtrl->highVisibility;
      normBase     = kHighLightNorm;
      normRange    = 100 - kHighLightNorm;
   }

   return normBase + ((float)(rawLighting - preNormBase) / (float)preNormRange) * normRange;
}

///////////////////////////////////////

int cAISenses::GetVisibility(const sVisionArgs & args, const sAIVisionCone * pCone)
{
   float visibilityToViewer;
   int visionType;

   sAIVisibilityControl * pVisCtrl = AIGetVisCtrl(args.obj);

   if (IsAPlayer(args.obj))
   {
      sAIVisibility * pVisibility = AIGetVisibility(args.obj);

      if (!pVisibility)
      {
         AIWatch1(Sight,GetID(),"Can't see %s, as it has no Vis Prop\n",ObjWarnName(args.obj));
         return 0;
      }

      AIWatchRaw3(Sight,GetID(),"vis (%d;%d;%d) ",
                  pVisibility->lighting, pVisibility->movement, pVisibility->exposure);

      sAcuitySet *pAcuity;

      if (pCone->flags&kAIVC_LowLight)
      {
         pAcuity=&g_pAIConeAcuities->set[kAcuityLowLightIdx];
      }
      else if (pCone->flags&kAIVC_Periph)
         pAcuity=&g_pAIConeAcuities->set[kAcuityPeriphIdx];
      else if (pCone->flags&kAIVC_Omni)
         pAcuity=&g_pAIConeAcuities->set[kAcuityOmniIdx];
      else
         pAcuity=&g_pAIConeAcuities->set[kAcuityNormalIdx];

      visibilityToViewer = (pVisibility->lighting * pAcuity->toLighting) +
                           (pVisibility->movement * pAcuity->toMovement) +
                           (pVisibility->exposure * pAcuity->toExposure);

      AIWatchRaw2(Sight,GetID(),"raw %d %sCone ",(int)visibilityToViewer,
                  (pCone->flags&kAIVC_LowLight)?"Low":
                   (pCone->flags&kAIVC_Periph)?"Periph":
                   (pCone->flags&kAIVC_Omni)?"Omni":"Normal");

      if (visibilityToViewer <= 0)
         visibilityToViewer = 1;
      visibilityToViewer = visibilityToViewer * ((float)pCone->acuity / 100.0);

      // Attempt to factor in sneaky corner leaning
      if (args.obj == PlayerObject() && IsLeaning())  //! @TODO: multiplayer
      {
         // If we can see the players head, but not body, the player gets the bonus
         if (!AISightCast((Location *)&args.fromLoc, (Location *)args.pTargetLoc))
         {
            visibilityToViewer = visibilityToViewer * pVisCtrl->cornerLeanMod;
            AIWatchRaw(Sight,GetID()," [corner bonus] ");
         }
      }

      AIWatchRaw2(Sight,GetID(),"acu %d => %d\n",(int)pCone->acuity,(int)visibilityToViewer);
   }
   else if (ObjHasRefs(args.obj))
   {
      visibilityToViewer = CalculateLightRating(args.obj, pVisCtrl) * ((float)pCone->acuity / 100.0);
   }
   else
      return 100;

   // Apply visibility modification if the AI has a vision type
   if (g_pAIVisionTypeProperty->Get(m_pAIState->GetID(), &visionType) &&
      (visionType>=0) && (visionType<kAIMaxVisibilityMods))
   {  // get target's visibility modifiers
      sAIVisibilityMods *pVisibilityMods = AIGetVisibilityMod(args.obj);
      visibilityToViewer *= pVisibilityMods->m_mods[visionType];
   }

   if (visibilityToViewer < 0)
      visibilityToViewer = 0;
   else if (visibilityToViewer > 100)
      visibilityToViewer = 100;

   return visibilityToViewer;
}

///////////////////////////////////////

STDMETHODIMP cAISenses::ClearSense(ObjID objId)
{
   LinkID linkId = (objId) ? g_pAIAwarenessLinks->GetSingleLink(GetID(), objId) : NULL;

   if (linkId != LINKID_NULL)
      g_pAIAwarenessLinks->Remove(linkId);

   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP_(const sAIAwareness *) cAISenses::GetHighAwareness(unsigned flags, unsigned maxLastContact)
{
   sAIAwareness * pHighest = NULL;
   sAIAwareness * pCurrent;
   sLink          link;
   ulong          time = AIGetTime();

   ILinkQuery * pQuery = g_pAIAwarenessLinks->Query(GetID(), LINKOBJ_WILDCARD);

   for (; !pQuery->Done(); pQuery->Next())
   {
      pQuery->Link(&link);

      if (GetID() != link.dest) // reserved "self" link is for alarming sounds -- we always assume they are from objects we care about
      {
         int teamRel = AITeamCompare(GetID(), link.dest);

         if ((flags & kAISGH_OpponentsOnly) && teamRel != kAI_Opponents)
         {
            continue;
         }

         if (flags & kAISGH_Alerting)
         {
            if (teamRel == kAI_Indifferent || (AIGetOnlyNoticesPlayer(GetID()) && !IsAPlayer(link.dest)) ||
                (teamRel == kAI_Teammates && (AIGetMode(link.dest) != kAIM_Dead || !AIGetNoticesBodies(GetID()))))
               continue;
         }

         if (teamRel == kAI_Opponents && AIGetMode(link.dest) == kAIM_Dead)
         {
            continue;
         }
      }

      pCurrent = (sAIAwareness *)pQuery->Data();
      pCurrent->linkID = pQuery->ID();
      pCurrent->object = link.dest;

      if (time - pCurrent->lastContact > maxLastContact)
         continue;

      if ((flags & kAISGH_FirstHand) && !(pCurrent->flags & kAIAF_FirstHand))
         continue;

      if (!pHighest || pCurrent->level > pHighest->level)
         pHighest = pCurrent;
      else
      {
         if (pHighest->level == pCurrent->level)
         {
            switch (pHighest->level)
            {
               case kAIAL_Lowest:
               case kAIAL_Low:
               case kAIAL_High:
                  if (IsAPlayer(pHighest->object))
                     continue;
                  if (IsAPlayer(pCurrent->object))
                  {
                     pHighest = pCurrent;
                     continue;
                  }

               // fall through
               case kAIAL_Moderate:
                  if (pHighest->flags & kAIAF_Highest)
                     continue;
                  if (pCurrent->flags & kAIAF_Highest)
                  {
                     pHighest = pCurrent;
                     continue;
                  }
                  if (pCurrent->lastContact > pHighest->lastContact)
                  {
                     pHighest = pCurrent;
                     continue;
                  }
                  if (m_pAIState->DistSq(pCurrent->lastPos) < m_pAIState->DistSq(pHighest->lastPos))
                  {
                     pHighest = pCurrent;
                     continue;
                  }
            }
         }
      }
   }

   SafeRelease(pQuery);

   return pHighest;
}

///////////////////////////////////////

STDMETHODIMP_(const sAIAwareness *) cAISenses::GetAwareness(ObjID target)
{
   if (!IsAPlayer(target) && target != GetID())
      Update(target);                            // players are automatically sensed at a regular interval, self is a secret token for heard object-less sounds

   LinkID linkID = (target) ? g_pAIAwarenessLinks->GetSingleLink(GetID(), target) : NULL;
   sAIAwareness * pResult = NULL;

   if (!linkID)
      return AINullAware(target);

   pResult = (sAIAwareness *)g_pAIAwarenessLinks->GetData(linkID);
   pResult->object = target;
   pResult->linkID = linkID;

   return pResult;
}

///////////////////////////////////////

STDMETHODIMP_(int) cAISenses::GetAllAwareness(cAIAwareArray * pResult)
{
   sLink          link;
   sAIAwareness * pCurrent;
   ILinkQuery *   pQuery = g_pAIAwarenessLinks->Query(GetID(), LINKOBJ_WILDCARD);

   for (; !pQuery->Done(); pQuery->Next())
   {
      pQuery->Link(&link);
      pCurrent = (sAIAwareness *)pQuery->Data();
      pCurrent->object = link.dest;
      pCurrent->linkID = pQuery->ID();
      pResult->Append(pCurrent);
   }
   SafeRelease(pQuery);

   return pResult->Size();
}


///////////////////////////////////////

void cAISenses::SetAwareness(ObjID objId, LinkID linkId, const sAIAwareness * pAwareness)
{
   if (linkId != LINKID_NULL)
      g_pAIAwarenessLinks->SetData(linkId, (void *)pAwareness);
   else if (pAwareness->level != kAIAL_Lowest)
      g_pAIAwarenessLinks->AddFull(GetID(), objId, (void *)pAwareness);
}

///////////////////////////////////////

// @TBD (toml 08-13-98): hook these up
#define kPlayerForget    (2*60*1000)
#define kNonPlayerForget (60*1000)
#define kForget kPlayerForget

void cAISenses::TestForget(LinkID linkId, const sAIAwareness * pAwareness)
{
   BOOL forget = (pAwareness->TimeSinceContact() > kForget &&
                  !(pAwareness->flags & kAIAF_CanRaycast));
   if (forget)
      g_pAIAwarenessLinks->Remove(linkId);
}

///////////////////////////////////////

eAIAwareLevel cAISenses::Look(const sVisionArgs & args, sAIAwareness * pAwareness)
{
   // Pick vision cone/update LOS
   const sAIVisionCone * pVisionCone;

   if (!(pAwareness->flags & kAIAF_CanRaycast) ||
       args.pVisionDesc == NULL ||
       (pVisionCone = FindVisionCone(args)) == NULL)
   {
      pAwareness->flags &= ~kAIAF_HaveLOS;
      return kAIAL_Lowest;
   }

   pAwareness->flags |= kAIAF_HaveLOS;

   // Target is potenially visible, update visibility
   int visibility = GetVisibility(args, pVisionCone);

   // Determine pulse
   if (visibility < kLowLightNorm)
      return kAIAL_Lowest;

   if (visibility < kMedLightNorm)
      return kAIAL_Low;

   if (visibility < kHighLightNorm)
      return kAIAL_Moderate;

   return kAIAL_High;
}

///////////////////////////////////////

eAIAwareLevel cAISenses::Listen(ObjID objId, sAIAwareness * pAwareness, mxs_vector * pSoundPos)
{
   if (!AIGetHearing(GetID()))
      return kAIAL_Lowest;

#ifndef SHIP
   // Bail if unaware of player
   if (!g_fAIAwareOfPlayer && objId == PlayerObject())
   {
      delete m_pObjectSounds->RemoveByKey(objId);
      return kAIAL_Lowest;
   }
#endif

   sAIQueuedSound * pQueuedSound = m_pObjectSounds->Search(objId);

   if (!pQueuedSound)
      return kAIAL_Lowest;

   GetBestInWorld(pQueuedSound->info.Position, pSoundPos);

   eAIAwareLevel level = g_SoundTypeToAware[pQueuedSound->type.type];
   delete m_pObjectSounds->Remove(pQueuedSound);
   return level;
}

///////////////////////////////////////

HRESULT cAISenses::Pulse(sAIAwareness * pAwareness, eAIAwareLevel visionPulse, const mxs_vector & visionPos, eAIAwareLevel soundPulse, const mxs_vector & soundPos)
{
   BOOL player = IsAPlayer(pAwareness->object);
   AIWatchTrue9(player, SensePulse, GetID(),
                "cAISenses::Pulse(%d, %d, [%d, %d, %d], %d, [%d, %d, %d]) {",
                pAwareness->object,
                visionPulse, (int)visionPos.x, (int)visionPos.y, (int)visionPos.z,
                soundPulse, (int)soundPos.x, (int)soundPos.y, (int)soundPos.z);

   pAwareness->flags &= ~(kAIAF_Seen | kAIAF_Heard);

   eAIAwareLevel      pulse = kAIAL_Lowest;
   const mxs_vector * pPulseLoc = &visionPos;
   unsigned           time = AIGetTime();

   if (visionPulse || soundPulse)
   {
      pAwareness->flags |= kAIAF_FirstHand;
      if (visionPulse >= soundPulse)
      {
         pAwareness->flags |= kAIAF_Seen;
         pulse = visionPulse;
         AIWatchTrue(player, SensePulse, GetID(), "  Seen");
      }
      else if (soundPulse)
      {
         pAwareness->flags |= kAIAF_Heard;
         pulse = soundPulse;
         pPulseLoc = &soundPos;
         AIWatchTrue(player, SensePulse, GetID(), "  Heard");
      }
   }

   AwarenessDelay(pAwareness, &pulse, *pPulseLoc);

   BOOL fKeepFresh = (player) ? !m_FreshTimer.Expired() : FALSE;

   AIWatchTrue1(player && fKeepFresh, SensePulse, GetID(), "  Keeping link fresh (%d remaining)", m_FreshTimer.Remaining());

   if (fKeepFresh && !(pAwareness->flags & kAIAF_Freshened))
   {
      pAwareness->flags |= kAIAF_Freshened;
      pAwareness->freshness = time - pAwareness->lastContact;
   }

   // Update last contact
   if ((pAwareness->flags & kAIAF_Sensed) || (time - pAwareness->trueLastContact < GetFreeKnowledgePeriod()))
   {
      if ((pAwareness->flags & kAIAF_Sensed))
      {
         AIWatchTrue(player, SensePulse, GetID(), "  Setting last contact to current");
         pAwareness->lastContact = time;
         pAwareness->trueLastContact = time;
      }
      AIWatchTrue(player, SensePulse, GetID(), "  Setting last position to current");
      pAwareness->lastPos = *pPulseLoc;
   }

   if (fKeepFresh)
   {
      AIWatchTrue(player, SensePulse, GetID(), "  Setting last contact to current for freshness");
      if (pAwareness->lastContact < time - pAwareness->freshness)
         pAwareness->lastContact = time - pAwareness->freshness;
   }
   else if (pAwareness->flags & kAIAF_Freshened)
   {
      pAwareness->flags &= ~kAIAF_Freshened;
      pAwareness->freshness = INT_MAX;
   }

   pAwareness->lastPulse = pulse;

   // If the AI is fully aware, and still has some perception of the target
   // remain aware
   if (pulse && pAwareness->level == kAIAL_High)
   {
      AIWatchTrue(player, SensePulse, GetID(), "  Have high, ignoring pulse level");
      pAwareness->time = time;
   }
   // Otherwise, if new level is less than current awareness...
   else if (pulse < pAwareness->level)
   {
      if (!fKeepFresh)
      {
         const sAIAwareCapacitor * pCapacitor    = GetCapacitor();
         int                       timeDelta     = time - pAwareness->time;
         int                       dischargeTime = pCapacitor->dischargeTimes[pAwareness->level - 1];

         // And we've been at this level without equal or greater pulses
         // for greater than the capacity...
         if (timeDelta >= dischargeTime)
         {
            // ...discharge to next lower level
            pAwareness->DecLev();
            pAwareness->time = time - (timeDelta - dischargeTime);
            AIWatchTrue1(player, SensePulse, GetID(), "  Degrading link (%d)", pAwareness->level);
         }
      }
      else
      {
         AIWatchTrue1(player, SensePulse, GetID(), "  Keeping fresh at %d", pAwareness->level);
         pAwareness->time = time;
      }
   }
   else
   {
      // Otherwise, the level is greater than or equal to current, set level
      // and reset capacitor
      pAwareness->level = pulse;

      if (pAwareness->level > pAwareness->peak)
      {
         if (pAwareness->level == kAIAL_High)
            pAwareness->bDidHighAlert = TRUE;
         pAwareness->peak = pAwareness->level;
      }
      pAwareness->time = time;

      AIWatchTrue1(player, SensePulse, GetID(), "  Upgrading link (%d)", pAwareness->level);
   }

   AIWatchTrue(IsAPlayer(pAwareness->object), SensePulse, GetID(), "}");

   return S_OK;
}

///////////////////////////////////////

void cAISenses::AwarenessDelay(sAIAwareness * pAwareness, eAIAwareLevel * pPulse, const mxs_vector & pulseLoc)
{
   if (!(m_flags & kInUpdate) || !IsAPlayer(pAwareness->object) ||
       *pPulse < kAIAL_Moderate || *pPulse <= pAwareness->level)
   {
      return;
   }

   //
   // Check to see if we already we should ignore this pulse, or if currently
   // disallowing delays
   //
   switch (*pPulse)
   {
      case kAIAL_Moderate:
      {
         if (!m_ModerateDelay.Expired())
         {
            AIWatch1(Alert, GetID(), "Reaction delay -- Ignoring moderate pulse, %d to go", m_ModerateDelay.Remaining());
            *pPulse = pAwareness->level;
            m_Timer.Delay(m_ModerateDelay.Remaining());
            return;
         }
         else if (!m_ModerateDelayReuse.Expired())
         {
            AIWatch1(Alert, GetID(), "No moderate reaction delay, %d to go", m_ModerateDelayReuse.Remaining());
            return;
         }

         break;
      }

      case kAIAL_High:
      {
         if (!m_HighDelay.Expired())
         {
            AIWatch2(Alert, GetID(), "Reaction delay -- Ignoring high pulse, returning %d, %d to go", pAwareness->level, m_HighDelay.Remaining());
            *pPulse = (m_ModerateDelay.Expired()) ?
                        kAIAL_Moderate :
                        pAwareness->level;
            if (m_HighDelay.Remaining() < m_ModerateDelay.Remaining())
               m_Timer.Delay(m_HighDelay.Remaining());
            else
               m_Timer.Delay(m_ModerateDelay.Remaining());
            return;
         }
         else if (!m_HighDelayReuse.Expired())
         {
            AIWatch1(Alert, GetID(), "No high reaction delay, %d to go", m_HighDelayReuse.Remaining());
            return;
         }

         break;
      }
   }

   //
   // Begin reaction delay (maybe)
   //
   sAIAwareDelay * pDelay = AIGetAwareDelay(GetID());

   if (m_pAIState->DistSq(pulseLoc) < (float)sq(pDelay->ignoreRange))
   {
      return;
   }

   switch (*pPulse)
   {
      case kAIAL_Moderate:
      {
         m_ModerateDelay.Set(AICustomTime(pDelay->toTwo));
         m_ModerateDelayReuse.Set(AICustomTime(pDelay->twoReuse));
         m_HighDelay.Set(AICustomTime(pDelay->toThree));
         m_HighDelayReuse.Set(AICustomTime(pDelay->threeReuse));

         *pPulse = kAIAL_Low;

         AIWatch(Alert, GetID(), "Starting moderate reaction delay");
         break;
      }

      case kAIAL_High:
      {
         if (pAwareness->level < kAIAL_Moderate)
            m_ModerateDelay.Set(AICustomTime(pDelay->toTwo));

         if (!m_ModerateDelay.Expired())
            *pPulse = kAIAL_Low;
         else
            *pPulse = kAIAL_Moderate;

         m_HighDelay.Set(AICustomTime(pDelay->toThree));
         m_HighDelayReuse.Set(AICustomTime(pDelay->threeReuse));
         m_ModerateDelayReuse.Set(AICustomTime(pDelay->twoReuse));

         AIWatch1(Alert, GetID(), "Starting high reaction delay, returning %d", *pPulse);
         break;
      }
   }
}

///////////////////////////////////////
//
// Finds the shortest distance between the AI and closest wall.
//

float cAISenses::FindDistanceToClosestWallSq(ObjID objId)
{
   #define kMaxWallScan (5.0)

   const Location & location = ObjPosGet(objId)->loc;
   Location         toLoc,
                    hitLoc;

   int const        TRIES = 8;
   int              i;
   float            distanceSq,
                    thisDistanceSq;

   floatang         direction;

   distanceSq = sq(kMaxWallScan);
   toLoc      = location;

   for (i = 0; i < TRIES; i++)
   {
      direction.value = TWO_PI * i / TRIES;

      ProjectFromLocationOnZPlane(location.vec, kMaxWallScan, direction, &toLoc.vec);
      UpdateChangedLocation(&toLoc);

      if (!AIRaycast(&location, &toLoc, &hitLoc, 0))
      {
         thisDistanceSq = AIXYDistanceSq(location.vec, hitLoc.vec);
         if (thisDistanceSq < distanceSq)
            distanceSq = thisDistanceSq;
      }
   }

   return distanceSq;
}

///////////////////////////////////////

const sAIVisionCone * cAISenses::FindVisionCone(const sVisionArgs & args) const
{
   float                 xyAng;
   float                 zAng;
   float                 range;
   eAIAlertLevel         alertness = m_pAIState->GetAlertness();
   int                   distSq    = (int)args.distSq;
   const sAIVisionCone * cones     = args.pVisionDesc->cones;
   const mxs_vector &    viewPos   = args.fromLoc.vec;
   const mxs_vector &    lookPos   = args.lookLoc.vec;
   sAIAlertSenseMults *  pMults    = AIAlertSenseMults(GetID());
   sAISenseMult &        mults     = pMults->mults[alertness];

   // Ah yes, let's perpetuate a spelling error!
   zAng  = AICompueZAngle(viewPos, lookPos);
   xyAng = Delta(GetVisionFacing(), floatang(viewPos.x, viewPos.y, lookPos.x, lookPos.y)).value;

   for (int i = 0; i < kAIMaxVisionCones; i++)
   {
      if (!(cones[i].flags & kAIVC_Active))
         continue;

      if (cones[i].flags & kAIVC_AlertnessRestricted)
      {
         static unsigned restriction[] =
         {
            kAIVC_NoAlert0,
            kAIVC_NoAlert1,
            kAIVC_NoAlert2,
            kAIVC_NoAlert3,
         };
         if (cones[i].flags & restriction[alertness])
            continue;
      }

      range = mults.RMult * cones[i].range;
      if (distSq > sq(range))
         continue;

      if (DEGREES(cones[i].zAngle * mults.ZMult) / 2.0 < zAng)
         continue;

      float useAng = xyAng;
      if (cones[i].flags & kAIVC_Behind)
         useAng = DEGREES(180) - xyAng;

      if (DEGREES(cones[i].angle * mults.XYMult) / 2.0 < useAng)
         continue;

      // hmmm, want this to be not on its on line...
      AIWatchRaw3(Sight,GetID(),"[AI(%3d) Watch:%5s] cone %d ",GetID(),"Sight",i);
      return &cones[i];
   }
   return NULL;
}

///////////////////////////////////////

#ifdef PLAYTEST
//#define NOTE_DIFFS
#endif
#define PI_AGAIN 3.14159265358979323846

floatang cAISenses::GetVisionFacing() const
{  // @TBD (toml 06-18-98): should vision facing be on the IInternalAI?
   if (m_pAIState->IsCreature())
   {
      ObjID our_id=m_pAIState->GetID();
      int visionJoint = AIGetVisionJoint(our_id);
      if (visionJoint != kCJ_Invalid)
         if (GetCreatureJointID(our_id,visionJoint)!=-1)
         {
            mxs_matrix head_mat=GetCreatureJointOrient(our_id,visionJoint);
            if (fabs(head_mat.vec[0].el[2])<0.8) // if you are not mostly looking up or down
            {
#ifdef PLAYTEST
               if ((fabs(head_mat.vec[0].el[1])<0.01)&&(fabs(head_mat.vec[0].el[0])<0.01))
                  mprintf("ARGH! GetVisionFacing has %g %g %g for %s\n",
                  head_mat.vec[0].el[0],head_mat.vec[0].el[1],head_mat.vec[0].el[2],
                  ObjWarnName(our_id));
               else
#endif
               {
                  float tan=atan2(head_mat.vec[0].el[1],head_mat.vec[0].el[0]);
#ifdef NOTE_DIFFS
                  //               if (AIIsWatched(Sight,our_id))
                  {
                     float diff=fabs(tan-(float)m_pAIState->GetFacingAng().value);
                     if (diff>PI_AGAIN) diff=2*PI_AGAIN-diff;
                     if (diff>PI_AGAIN/4)
                        mprintf("AISight: Large Angle Difference: %s saw %g and %g diff %g\n",ObjEditName(GetID()),tan,(float)m_pAIState->GetFacingAng().value,diff);
                  }
#endif
                  if (tan<0)
                     tan+=2*PI_AGAIN;
                  return tan;
               }
            }
         }
         else
            Warning(("GetVisionFacing: Cant find head joint for %s\n",ObjWarnName(m_pAIState->GetID())));
   }
   return m_pAIState->GetFacingAng();
}

///////////////////////////////////////

const sAIAwareCapacitor * cAISenses::GetCapacitor()
{
   return AIGetAwareCap(GetID());
}

///////////////////////////////////////

BOOL cAISenses::FunctionallySofter(const sSoundInfo * pSoundInfoNew, const sSoundInfo * pSoundInfoOld)
{
   // for now, lets ignore DB
   // @TODO: really want to ratio dist/dist and vol/vol and pick
   //   but im not gonna do that right now, sorry
   return pSoundInfoNew->Distance <= pSoundInfoOld->Distance;
}

///////////////////////////////////////

STDMETHODIMP_(int) cAISenses::Hears(const sSoundInfo * pSoundInfo, const sAISoundType * pType)
{
   int hear_lvl=AIGetHearing(GetID());
   if (hear_lvl)
   {
      if (pSoundInfo->Distance < g_pAISoundTweaks->defaultRanges[pType->type]*gHearingStats.dist_muls[hear_lvl])
         if (pSoundInfo->Volume > g_aAISoundDBVals[pType->type] + gHearingStats.db_adds[hear_lvl])
            return 100;
   }
#ifndef SHIP
   // only do the watch if the thing is reasonably close
   if (hear_lvl)
      if (pSoundInfo->Distance < 3*g_pAISoundTweaks->defaultRanges[pType->type]*gHearingStats.dist_muls[hear_lvl])
         if (pSoundInfo->Volume > g_aAISoundDBVals[pType->type] + gHearingStats.db_adds[hear_lvl] + 750)
            AIWatch5(Hear, GetID(),"Can't hear %d [%s] from %s (d %d v %d)", pType->type, ObjEditName(pSoundInfo->SchemaID), ObjEditName(pSoundInfo->SrcObject), (int)pSoundInfo->Distance, pSoundInfo->Volume);
#endif
   return 0;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISenses::KeepFresh(ObjID obj, unsigned duration)
{
   if (IsAPlayer(obj))
   {
      if (duration)
         m_FreshTimer.Set(AICustomTime(duration));
      else
      {
         LinkID         linkID;
         sAIAwareness * pAwareness;

         linkID = GetAwarenessLink(GetID(), &pAwareness);

         if (pAwareness->flags & kAIAF_Freshened)
         {
            pAwareness->flags &= ~kAIAF_Freshened;
            pAwareness->freshness = INT_MAX;
            SetAwareness(GetID(), linkID, pAwareness);
         }

         m_FreshTimer.Force();
      }
   }
}

///////////////////////////////////////

void cAISenses::GetBestInWorld(const mxs_vector & original, mxs_vector * pResult)
{
   tAIPathCellID cell = AILocateBestLocoCell(*m_pAIState->GetLocation(), m_pAIState->GetPathCell(), original);
   if (!cell)
   {
#ifndef SHIP
      mprintf("ERROR: AI %d could not approximate location of sound in the AI world\n", GetID());
#endif
      cell = 1;
   }

   if (IsPtInCellXYPlane(original, g_AIPathDB.GetCell(cell)))
   {
      pResult->x = original.x;
      pResult->y = original.y;
      pResult->z = AIGetZAtXYOnCell(original, cell) + 0.25;

   }
   else
   {
      *pResult = g_AIPathDB.GetCenter(cell);
   }
}

///////////////////////////////////////

void cAISenses::ProcessPositionSounds()
{
   if (m_LastPlayerDamage && AIGetTime() - m_LastPlayerDamage < kAIDamageResonanceTime)
      return;

   if (AIGetHearing(GetID()) && m_pPositionSound)
   {
      eAIAwareLevel level = g_SoundTypeToAware[m_pPositionSound->type.type];

      LinkID         linkID;
      sAIAwareness * pAwareness;

      linkID = GetAwarenessLink(GetID(), &pAwareness);

      if (pAwareness->level <= level)
      {
         mxs_vector loc;
         GetBestInWorld(m_pPositionSound->info.Position, &loc);

         Pulse(pAwareness, kAIAL_Lowest, loc, level, loc);
         SetAwareness(GetID(), linkID, pAwareness);
      }
      delete m_pPositionSound;
      m_pPositionSound = NULL;
   }
}

///////////////////////////////////////

void cAISenses::InformFrom(ObjID from)
{
   IInternalAI * pOtherAI     = m_pAI->AccessOtherAI(from);
   IAISenses *   pOtherSenses = pOtherAI->AccessSenses();
   if (pOtherSenses)
   {
      const sAIAwareness * pOtherAwareness = pOtherSenses->GetHighAwareness(kAISGH_OpponentsOnly | kAISGH_FirstHand);

      if (pOtherAwareness)
      {
         Location fromLoc, toLoc;
         fromLoc = *m_pAIState->GetPortalLocation();
         toLoc   = ObjPosGet(from)->loc;
         if (AISightCast(&fromLoc, &toLoc))
         {
            sAIAwareness newAwareness;
            const sAIAwareness * pAwareness = GetAwareness(pOtherAwareness->object);

            eAIAwareLevel otherUrgency;
            #define kAgeInheritAlert 15000
            if (AIGetTime() - pOtherAwareness->lastContact < kAgeInheritAlert)
               otherUrgency = max(pOtherAwareness->level, pOtherAI->GetState()->GetAlertness());
            else
               otherUrgency = pOtherAwareness->level;

            if (!pAwareness->linkID)
            {
               memcpy(&newAwareness, pOtherAwareness, sizeof(sAIAwareness));
               newAwareness.flags  = 0;
               newAwareness.linkID = 0;
               newAwareness.level  = otherUrgency;
            }
            else
            {
               memcpy(&newAwareness, pAwareness, sizeof(sAIAwareness));
               if (pAwareness->lastContact < pOtherAwareness->lastContact)
               {
                  newAwareness.lastPos = pOtherAwareness->lastPos;
                  newAwareness.lastContact = pOtherAwareness->lastContact;
               }

               eAIAwareLevel thisUrgency = pAwareness->level;
               newAwareness.level = max(thisUrgency, otherUrgency);
            }

            newAwareness.lastContact = AIGetTime();

            m_MaxInformTimer.Reset();

            LinkID         linkID;
            sAIAwareness * pMiscAwareness;

            linkID = GetAwarenessLink(GetID(), &pMiscAwareness);

            if (pMiscAwareness->level <= newAwareness.level)
            {
               // We've borrowed from a specific link to freshen our
               // general link. However, there are flags on the
               // specific link we definitely don't want.
               newAwareness.flags &= ~(kAIAF_Seen | kAIAF_CanRaycast | kAIAF_HaveLOS | kAIAF_Highest | kAIAF_FirstHand);
               newAwareness.flags |= kAIAF_Heard;
               newAwareness.object = GetID();
               SetAwareness(GetID(), linkID, &newAwareness);
            }
         }
      }
   }
}

///////////////////////////////////////

int cAISenses::GetFreeKnowledgePeriod()
{
   sAIAlertSenseMults * pMults = AIAlertSenseMults(GetID());
   int   base = AIGetFreeKnowledgePeriod(GetID());
   float mult = pMults->mults[m_pAIState->GetAlertness()].KnowledgeMult;

   if (m_pAIState->GetMode() != kAIM_Combat)
      return base * mult;
   return base * mult * pMults->CombatKnowledgeMult;

}

///////////////////////////////////////////////////////////////////////////////

void AIUpdateVisibility(ObjID objId)
{
   Assert_(IsAPlayer(objId));

   sAIVisibility * pVisibility = AIGetVisibility(objId);

   int timeSinceLast = (pVisibility) ? AIGetTime() - pVisibility->updateTime : 0;

   if (!pVisibility || timeSinceLast > kVisibilityUpdateRate)
   {
      sAIVisibility visibility;
      sAIVisibilityControl * pVisCtrl = AIGetVisCtrl(objId);

      visibility.updateTime = AIGetTime();

      //
      // Calculate light rating
      //
      // darker he is, the less visible he is.
      //
      visibility.lighting = cAISenses::CalculateLightRating(objId, pVisCtrl);

      if (visibility.lighting <= 0)
         visibility.lighting = 1;

      int selflitval = 0;
      if (ObjGetSelfLit(objId,&selflitval))
      {
         visibility.lighting += selflitval;
      }

      //
      // Calculate movement rating
      //
      // the faster the object moves, the more visible it is,
      // we ignore the z velocity part, because it's
      // probably cooler in gameplay.  but maybe not.
      //
      mxs_vector velocity;
      float v;

      GetObjVelocity(objId, &velocity);
      v = AIDistanceSq(0, 0, velocity.x, velocity.y);

      if (v < pVisCtrl->lowSpeed)
         visibility.movement = pVisCtrl->lowSpeedMod;
      else if (v > pVisCtrl->highSpeed)
         visibility.movement = pVisCtrl->highSpeedMod;
      else
         visibility.movement = pVisCtrl->normSpeedMod;

      //
      // Calculate exposure rating
      //
      if (cAISenses::FindDistanceToClosestWallSq(objId) < sq(pVisCtrl->wallDist))
         visibility.exposure = pVisCtrl->wallMod;
      else
         visibility.exposure = 0;

      //!@TODO: multiplayer: maintain crouch state for networked players (mtr).
      if (objId==PlayerObject())
      {
         switch (PlayerMotionGetActive())
         {
            case kMoCrouch:
            case kMoCrawlLeft:
            case kMoCrawlRight:
               visibility.exposure += pVisCtrl->crouchMod;
               break;
         }
      }

      ObjID weapObj = GetWeaponObjID(objId);
      if (weapObj)
         if (IsWeaponSwinging(weapObj)!=0)
            visibility.exposure += GetWeapSwingExposure(weapObj);
         else
            visibility.exposure += GetWeaponExposure(weapObj);

      // Compute the final composite rating
      visibility.level = visibility.lighting + visibility.movement + visibility.exposure;

      if (visibility.level < 0)
         visibility.level = 0;
      else if (visibility.level > 100)
         visibility.level = 100;

      g_pAIVisibilityProperty->Set(objId, &visibility);
   }
}

///////////////////////////////////////////////////////////////////////////////
