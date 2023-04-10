/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisound.cpp,v 1.53 2000/03/06 22:36:32 bfarquha Exp $
//
//

// #define PROFILE_ON 1

#include <lg.h>
#include <mprintf.h>
#include <str.h>
#include <timings.h>
#include <cfgdbg.h>

#include <label.h>
#include <speech.h>
#include <tagdbin.h>
#include <playrobj.h>
#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <ctagset.h>
#include <ctagnet.h>

#include <drkplinv.h>

#include <aiapisns.h>
#include <aiaware.h>
#include <aidebug.h>
#include <aiprcore.h>
#include <aisndtyp.h>
#include <aisound.h>
#include <aiteams.h>
#include <aiutils.h>

#include <aicnvapi.h>

#include <aibasctm.h>

#include <schtype.h>

#include <iobjsys.h>
#include <traitman.h>
#include <memall.h>
#include <dbmem.h>   // must be last header!


F_DECLARE_INTERFACE(IAIBroadcastProperty);

///////////////////////////////////////////////////////////////////////////////

static IAIAppServices *       g_pAIAppServices;
static IAIBroadcastProperty * g_pAIBroadcastProperty;
static tNetMsgHandlerID       g_NetMsgHandlerID;

///////////////////////////////////////////////////////////////////////////////

static const char * g_ppszAISoundConcepts[kAISC_CoreNum] =
{
   "",

   "Sleeping",
   "Alert zero broadcast",
   "Alert one broadcast",
   "Alert two broadcast",
   "Alert three broadcast",

   "Alert to one",
   "Alert to two",
   "Alert to three",
   "Spot player",

   "Alert down from one",
   "Lost contact",

   "Charge",
   "Shoot",
   "Flee",
   "Friend",
   "Alarm",

   "Attack",
   "Attack success",
   "Block success",
   "Detect block",
   "Blocked",
   "Hit no damage",
   "Hit high damage",
   "Hit low damage",
   "Hit ambush",

   "Die loud",
   "Die soft",

   "Found body",
   "Something mising",
   "Security breach",
   "Small anomaly",
   "Large anomaly",
   "Found robot",
   "Saw intruder",
   "Saw body",
   "Saw missing",
   "Saw something unspecific",
   "Saw recent dead robot",

   "Out of reach"
};

///////////////////////////////////////////////////////////////////////////////

enum eAIBroadcastType
{
   kAIBT_Normal,
   kAIBT_NoBroadcast,
   kAIBT_CustomConcept,

   kAIBT_Num,

   kAIBT_IntMax = 0xffffffff
};

///////////////////

struct sAIBroadcastSetting
{
   eAISoundConcept  concept;
   eAIBroadcastType type;
   union
   {
      char          szConcept[16];
      Label         conceptLabel;
   };
   char             szTags[64];
};

///////////////////

#define kMaxBroadcastSettings 8

struct sAIBroadcastSettings
{
   BOOL                disabled;
   sAIBroadcastSetting settings[kMaxBroadcastSettings];
};


///////////////////

static const char * _g_ppszAIBroadcastTypes[kAIBT_Num] =
{
   "Normal",
   "No broadcast",
   "Custom concept"
};

///////////////////

#define BROADCAST_FIELDS(i) \
   { "Custom " #i ": Broadcast",         kFieldTypeEnum,   FieldLocation(sAIBroadcastSettings, settings[i - 1].concept), kFieldFlagNone, 0, FieldNames(g_ppszAISoundConcepts) }, \
   { "               Type",              kFieldTypeEnum,   FieldLocation(sAIBroadcastSettings, settings[i - 1].type), kFieldFlagNone, 0, FieldNames(_g_ppszAIBroadcastTypes) }, \
   { "               Custom concept",    kFieldTypeString, FieldLocation(sAIBroadcastSettings, settings[i - 1].szConcept)                                                    }, \
   { "               Additional tags",   kFieldTypeString, FieldLocation(sAIBroadcastSettings, settings[i - 1].szTags)                                                       }

static sFieldDesc _g_AIBroadcastFieldDesc[] =
{
   { "Broadcasts disabled",    kFieldTypeBool, FieldLocation(sAIBroadcastSettings, disabled)                                                            },
   BROADCAST_FIELDS(1),
   BROADCAST_FIELDS(2),
   BROADCAST_FIELDS(3),
   BROADCAST_FIELDS(4),
   BROADCAST_FIELDS(5),
   BROADCAST_FIELDS(6),
   BROADCAST_FIELDS(7),
   BROADCAST_FIELDS(8)
};

static sStructDesc _g_AIBroadcastStructDesc = StructDescBuild(sAIBroadcastSettings, kStructFlagNone, _g_AIBroadcastFieldDesc);

///////////////////////////////////////
//
// Broadcast settings property
//

#undef INTERFACE
#define INTERFACE IAIBroadcastProperty
DECLARE_PROPERTY_INTERFACE(IAIBroadcastProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIBroadcastSettings *);
};

///////////////////

static sPropertyDesc _g_AIBroadcastPropertyDesc =
{
   "AI_BcstSet",
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CORE_CAT, "Broadcast customization" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////

typedef cSpecificProperty<IAIBroadcastProperty, &IID_IAIBroadcastProperty, sAIBroadcastSettings *, cHashPropertyStore< cClassDataOps<sAIBroadcastSettings> > > cAIBroadcastPropertyBase;

class cAIBroadcastProperty : public cAIBroadcastPropertyBase
{
public:
   cAIBroadcastProperty()
      : cAIBroadcastPropertyBase(&_g_AIBroadcastPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(sAIBroadcastSettings);
};

///////////////////

#define AIGetCustomBroadcasts(obj) AIGetProperty(g_pAIBroadcastProperty, (obj), ((sAIBroadcastSettings *)NULL))

const sAIBroadcastSetting * AIGetSettings(ObjID id, eAISoundConcept concept)
{
   static sAIBroadcastSetting defaultSetting;
   const sAIBroadcastSettings * pSettings = AIGetCustomBroadcasts(id);

   if (pSettings)
   {
      if (pSettings->disabled)
      {
         defaultSetting.concept = concept;
         defaultSetting.type    = kAIBT_NoBroadcast;
         return &defaultSetting;
      }

      for (int i = 0; i < kMaxBroadcastSettings; i++)
      {
         if (pSettings->settings[i].concept == concept)
            return &pSettings->settings[i];
      }
   }

   defaultSetting.concept = concept;
   defaultSetting.type    = kAIBT_Normal;
   return &defaultSetting;
}

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSoundEnactor(IAIManager *)
{
   StructDescRegister(&_g_AIBroadcastStructDesc);
   g_pAIBroadcastProperty = new cAIBroadcastProperty;
   g_pAIAppServices = AppGetObj(IAIAppServices);

   return TRUE;
}

///////////////////////////////////////

BOOL AITermSoundEnactor()
{
   SafeRelease(g_pAIBroadcastProperty);
   SafeRelease(g_pAIAppServices);
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

struct sAISoundConceptDesc
{
   eAISoundConcept concept;
   const char *    pszConcept;
   Label           conceptLabel;

   const char *    pszTags;

   unsigned        minDelay;
   unsigned        maxDelay;
   unsigned        flags;
};

enum eAISoundConceptFlags
{
   kTagSense      = 0x01,
   kTagFriends    = 0x02
};

///////////////////////////////////////

#define CONCEPT(enumval)   enumval, g_ppszAISoundConcepts[enumval]

sAISoundConceptDesc g_AISoundConceptDescs[] =
{
   { CONCEPT(kAISC_Sleeping),             "sleeping",        NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_AlertZeroBroadcast),   "atlevelzero",  NULL, 14900,  30000, 0 },
   { CONCEPT(kAISC_AlertOneBroadcast),    "atlevelone",   NULL, 10000,  20000, 0 },
   { CONCEPT(kAISC_AlertTwoBroadcast),    "atleveltwo",   NULL, 11333,  22666, 0 },
   { CONCEPT(kAISC_AlertThreeBroadcast),  "atlevelthree", NULL, 11333,  22666, 0 },


   { CONCEPT(kAISC_AlertToOne),           "tolevelone",    NULL, 2000,  3000, 0 },
   { CONCEPT(kAISC_AlertToTwo),           "toleveltwo",    NULL, 2000,  3000, 0 },
   { CONCEPT(kAISC_AlertToThree),         "tolevelthree",  NULL, 2000,  3000, 0 },
   { CONCEPT(kAISC_SpotPlayer),           "spotplayer",    NULL, 10000,  20000, 0 },

   { CONCEPT(kAISC_AlertDownToZero),      "backtozero",     NULL, 2000,  3000, 0 },
   { CONCEPT(kAISC_LostContact),          "lostcontact",    NULL, 10666,  21333, 0 },


   { CONCEPT(kAISC_ReactCharge),          "reactcharge",     NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_ReactShoot),           "reactshoot",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_ReactRun),             "reactrun",        NULL, 5000,    20000, 0 },
   { CONCEPT(kAISC_ReactFriend),          "reactfriend",     NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_ReactAlarm),           "reactalarm",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_ReactFindFriend),      "getbowman",       NULL, 5000,    20000, 0 },


   { CONCEPT(kAISC_CombatAttack),         "comattack",       NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_CombatSuccHit),        "comsucchit",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_CombatSuccBlock),      "comsuccblock",    NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_CombatDetBlock),       "comdetblock",     NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_CombatBlocked),        "comblocked",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_CombatHitNoDam),       "comhitnodam",     NULL, 3000,    5000, 0 },
   // note: damagehigh means much damage, ie. comhitlow for low hit points left
   { CONCEPT(kAISC_CombatHitDamageHigh),  "comhitlow",       NULL, 1000,    2000, 0 },
   { CONCEPT(kAISC_CombatHitDamageLow),   "comhithigh",      NULL, 1000,    2000, 0 },
   { CONCEPT(kAISC_CombatHitAmbush),      "comhitamb",       NULL, 1000,    2000, 0 },

   { CONCEPT(kAISC_DieLoud),              "comdieloud",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_DieSoft),              "comdiesoft",      NULL, 3000,    5000, 0 },


   { CONCEPT(kAISC_FoundBody),            "foundbody",       NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_FoundMissing),         "foundmissing",    NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_FoundBreach),          "foundbreach",     NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_FoundSmall),           "foundsmall",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_FoundLarge),           "foundlarge",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_FoundRobot),           "foundrobot",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_RecentIntruder),       "recentintruder",  NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_RecentBody),           "recentbody",      NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_RecentMissing),        "recentmissing",   NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_RecentOther),          "recentother",     NULL, 3000,    5000, 0 },
   { CONCEPT(kAISC_RecentRobot),           "recentrobot",      NULL, 3000,    5000, 0 },

   { CONCEPT(kAISC_OutOfReach),           "outofreach",     NULL, 3000,    5000, 0 },
};

///////////////////////////////////////

sAISoundConceptDesc * AIGetConceptDesc(eAISoundConcept concept)
{
   for (int i = 0; i < sizeof(g_AISoundConceptDescs); i++)
   {
      if (g_AISoundConceptDescs[i].concept == concept)
         return &g_AISoundConceptDescs[i];
   }
   return NULL;
}

///////////////////////////////////////

sAISoundConceptDesc * AIGetConceptDesc(const char * pszConceptLabelText)
{
   for (int i = 0; i < sizeof(g_AISoundConceptDescs); i++)
   {
      if (stricmp(g_AISoundConceptDescs[i].conceptLabel.text, pszConceptLabelText) == 0)
         return &g_AISoundConceptDescs[i];
   }

   static sAISoundConceptDesc customDesc;

   customDesc.concept    = kAISC_GameBase;
   customDesc.pszConcept = pszConceptLabelText;
   strncpy(customDesc.conceptLabel.text, pszConceptLabelText, sizeof(customDesc.conceptLabel.text) - 1);
   customDesc.conceptLabel.text[sizeof(customDesc.conceptLabel.text) - 1] = 0;

   return &customDesc;
}

//////////////////////////////////////

const char * AIGetConceptName(eAISoundConcept concept)
{
   static cStr customStr;

   sAISoundConceptDesc * pDesc = AIGetConceptDesc(concept);

   if (pDesc)
      return pDesc->pszConcept;

   const char * pszNoDescMsg = (concept >= kAISC_GameBase) ? "Game concept" : "Unknown concept";

   customStr.FmtStr("%s %d", pszNoDescMsg, concept);

   return customStr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISoundEnactor
//

STDMETHODIMP_(const char *) cAISoundEnactor::GetName()
{
   return "Sound enactor";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISoundEnactor::Init()
{
   cAISoundEnactorBase::Init();
   SetNotifications(kAICN_ModeChange);
   IAIComponent * pConverseComponent;
   m_pAI->GetComponent("Converse", &pConverseComponent);
   if (pConverseComponent)
   {
      pConverseComponent->QueryInterface(IID_IAIConverse, (void **)&m_pConverse);
      pConverseComponent->Release();
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISoundEnactor::End()
{
   cAISoundEnactorBase::End();
   SafeRelease(m_pConverse);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISoundEnactor::OnModeChange(eAIMode previous, eAIMode mode)
{
   if (mode < kAIM_Normal)
      SpeechHalt(GetID());
}

///////////////////////////////////////

STDMETHODIMP_(void) cAISoundEnactor::HaltCurrent()
{
   SpeechHalt(GetID());
}

///////////////////////////////////////

STDMETHODIMP cAISoundEnactor::RequestConcept(eAISoundConcept concept, const cTagSet * pTags)
{
   sAISoundConceptDesc * pDesc;

   // @HACK for E3
   if (m_fSpeechStop)
      return S_OK;

   if (concept > kAISC_CoreNum)
   {
      Warning(("No support for custom game concepts right now\n"));
      return E_FAIL;
   }

   pDesc = AIGetConceptDesc(concept);

   if (!pDesc)
   {
      AIWatch1(Sound, GetID(), "requested unknown sound concept %d", concept);
      Warning(("AI %d requested unknown sound concept %d\n", GetID(), concept));
      return E_FAIL;
   }

   return DoRequestConcept(pDesc, pTags);
}

///////////////////////////////////////

STDMETHODIMP cAISoundEnactor::RequestConcept(const char * pszConcept, const cTagSet * pTags)
{
   sAISoundConceptDesc * pDesc;

   // @HACK for E3
   if (m_fSpeechStop)
      return S_OK;

   pDesc = AIGetConceptDesc(pszConcept);

   if (!pDesc)
   {
      AIWatch1(Sound, GetID(), "requested unknown sound concept \"%s\"", pszConcept);
      Warning(("AI %d requested unknown sound concept \"%s\"\n", GetID(), pszConcept));
      return E_FAIL;
   }

   return DoRequestConcept(pDesc, pTags);
}

///////////////////////////////////////

DECLARE_TIMER(AI_SE_DRC, Average);

static BOOL IsARobot(ObjID id)
{
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);

   ObjID Robot = pObjectSystem->GetObjectNamed("Robot");
   if (Robot == OBJ_NULL)
      return FALSE;

   return pTraitManager->ObjHasDonor(id, Robot);
}


#define IsAlertnessBroadcast(concept) ((concept) <= kAISC_AlertThreeBroadcast)

HRESULT cAISoundEnactor::DoRequestConcept(sAISoundConceptDesc * pDesc, const cTagSet * pTags)
{
   int                   time = AIGetTime();
   int                   hSchema;

#ifndef SHIP
   if (!g_bSpeechDatabaseLoaded)
   {
      Warning(("No speech database loaded\n"));
      return E_FAIL;
   }
#endif

   if (m_pAIState->GetMode() == kAIM_Asleep)
      return S_FALSE;

#ifdef CONCEPT_PLAYBACK_RESTRICT
   if (pDesc->concept < kAISC_CoreNum && time < m_ConceptTimes[pDesc->concept])
      return S_FALSE;
#endif

   AUTO_TIMER(AI_SE_DRC);

   cTagSet                     tags;
   const char *                pszTagsProp = AIGetSoundTags(GetID());
   const sAIBroadcastSetting * pSetting    = AIGetSettings(GetID(), pDesc->concept);

   if (pSetting->type == kAIBT_NoBroadcast)
      return S_FALSE;

   //
   // If continuing an alertness broadcast with different input tags, kill
   // the previous loop
   //
   if (pSetting->type != kAIBT_CustomConcept &&
       m_LastConceptPlayed == pDesc->concept &&
       IsAlertnessBroadcast(m_LastConceptPlayed))
   {


      if (((!pTags || !pTags->Size()) && m_LastTagsPlayed.Size()) ||
          (pTags && !(m_LastTagsPlayed == *pTags)))
      {
         AIWatch(Sound, GetID(), "Was playing same alertness broadcast with different tags. Halting previous");
         SpeechHalt(GetID());
      }
   }

   //
   // Set up tags
   //
   if (pTags)
      tags.Append(*pTags);

   // Reacquire tag, if spot player
   if (pDesc->concept == kAISC_SpotPlayer)
   {
      if ((m_flags & kPlayedSpotPlayer) && !m_ReacquireTimer.Expired())
         tags.Append("Reacquire true");
      else
      {
         // @TBD (toml 11-02-98): HACK -- clean up later
         AutoAppIPtr(Inventory);
         if (pInventory != NULL)
         {
            ObjID item = pInventory->Selection(kCurrentItem);
            if (ObjIsAI(item) &&
                AIGetMode(item) == kAIM_Dead &&
                AITeamCompare(AIGetTeam(item), AIGetTeam(GetID())) == kAI_Teammates)
            {
               tags.Append("CarryBody true");
            }

         }
         m_ReacquireTimer.Reset();
      }
      m_flags |= kAISC_SpotPlayer;

   }

   // Property tags
   if (pszTagsProp)
      tags.Append(pszTagsProp);

   // Override property tags
   if (pSetting->szTags[0])
      tags.Append(pSetting->szTags);

   // Sight/sound tags
   if (m_pAI->AccessSenses())
   {
      const sAIAwareness * pAwareness = m_pAI->AccessSenses()->GetHighAwareness();
      if (pAwareness && (pAwareness->flags & kAIAF_FirstHand))
      {
         if (pAwareness->flags & kAIAF_Seen)
            tags.Add(cTag("Sense", "Sight"));
         else if (pAwareness->flags & kAIAF_Heard)
            tags.Add(cTag("Sense", "Sound"));
      }

      // Nearby friends
      cAIAwareArray awarenesses;
      #define kFriendClosenessSq sq(25.0)
      ObjID         friendObj  = OBJ_NULL;
      float         friendDist = FLT_MAX;

      m_pAI->AccessSenses()->GetAllAwareness(&awarenesses);
      for (int i = 0; i < awarenesses.Size(); i++)
      {
         if (GetID() != awarenesses[i]->object &&
             (awarenesses[i]->flags & kAIAF_CanRaycast) &&
             AIGetMode(awarenesses[i]->object) != kAIM_Dead &&
             AITeamCompare(GetID(), awarenesses[i]->object) == kAI_Teammates)
         {
            float dist = m_pAIState->DistSq(*GetObjLocation(awarenesses[i]->object));
            // Humans don't speak to robots, robots speak to humans and other robots:
            if ((dist < kFriendClosenessSq && dist < friendDist) && (IsARobot(GetID()) || !IsARobot(awarenesses[i]->object)))
            {
               friendDist = dist;
               friendObj = awarenesses[i]->object;
            }
            break;
         }
      }

      if (friendObj)
         tags.Add(cTag("NearbyFriends", (int)sqrt(friendDist)));

   }

   if (g_pAIAppServices)
      g_pAIAppServices->AddSoundTags(&tags);

#ifndef SHIP
   if (AIIsWatched(Sound, GetID()))
   {
      cStr tagStr;
      tags.ToString(&tagStr);
      AIWatch3(Sound, GetID(),
               "want \"%s\" concept \"%s\" tags \"%s\"",
               pDesc->pszConcept,
               (pSetting->type == kAIBT_CustomConcept) ? pSetting->conceptLabel.text : pDesc->conceptLabel.text,
               (tagStr[0]) ? tagStr.operator const char *() : "None");
   }
#endif

   AssertMsg2(tags.Size() < 32, "AI %d made a sound request with an unusually high number of tags (%d)", GetID(), tags.Size());

   // Speak!
   const Label *conceptLabel = (pSetting->type == kAIBT_CustomConcept) ? &pSetting->conceptLabel
                                                                       : &pDesc->conceptLabel;

   cTagDBInput           tagDBInput;
   tagDBInput.AppendTagSet(&tags);

   hSchema = SpeechSpeak(GetID(), conceptLabel, &tagDBInput, (void *)pDesc->concept);

   if (hSchema == SCH_HANDLE_NULL)
   {
      AIWatch(Sound, GetID(), "Speech system didn't want to speak");
      return S_FALSE;
   }

   if (m_pConverse)
      m_pConverse->OnSpeechStart(hSchema);

#ifdef CONCEPT_PLAYBACK_RESTRICT
   if (pDesc->concept < kAISC_CoreNum)
      m_ConceptTimes[pDesc->concept] = time + AIRandom(pDesc->minDelay, pDesc->maxDelay);

   AIWatch2(Sound,
            GetID(),
            "played concept \"%s\"; new delay is %d.", pDesc->pszConcept, (pDesc->concept < kAISC_CoreNum) ? (m_ConceptTimes[pDesc->concept] - AIGetTime()) : 0);
#else
   AIWatch1(Sound,
            GetID(),
            "played concept \"%s\"", pDesc->pszConcept);
#endif

   if (pSetting->type != kAIBT_CustomConcept)
   {
      m_LastConceptPlayed = pDesc->concept;
      if (pTags)
         m_LastTagsPlayed = *pTags;
      else
         m_LastTagsPlayed.Clear();
   }
   else
   {
       m_LastConceptPlayed = kAISC_CoreBase;
       m_LastTagsPlayed.Clear();
   }

   return S_OK;
}

///////////////////////////////////////

#ifdef CONCEPT_PLAYBACK_RESTRICT
#pragma message("Need save load of concept times")
#endif

STDMETHODIMP_(BOOL) cAISoundEnactor::Save(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_SoundEnactor, 0, 0, pTagFile))
   {
      SaveNotifications(pTagFile);

      AITagMove(pTagFile, &m_fSpeechStop);
      m_ReacquireTimer.Save(pTagFile);
      AITagMove(pTagFile, &m_flags);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAISoundEnactor::Load(ITagFile * pTagFile)
{
   if (AIOpenTagBlock(GetID(), kAISL_SoundEnactor, 0, 0, pTagFile))
   {
      LoadNotifications(pTagFile);

      AITagMove(pTagFile, &m_fSpeechStop);
      m_ReacquireTimer.Load(pTagFile);
      AITagMove(pTagFile, &m_flags);
      AICloseTagBlock(pTagFile);
   }
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
