/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiman.cpp,v 1.85 2000/03/25 22:10:40 adurant Exp $
//
//

// #define PROFILE_ON 1


#include <lg.h>
#include <mprintf.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <appagg.h>
#include <config.h>
#include <cfgdbg.h>
#include <timer.h>

#include <appsfx.h>
#include <brloop.h>
#include <dbasemsg.h>
#include <dispbase.h>
#include <dmgbase.h>
#include <dmgmodel.h>
#include <dspchdef.h>
#include <engfloop.h>
#include <iobjsys.h>
#include <objloop.h>
#include <phscrt.h>
#include <physapi.h>
#include <physloop.h>
#include <playrobj.h>
#include <propbase.h>
#include <property.h>
#include <propobsv.h>
#include <psndinfo.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <simdef.h>
#include <simflags.h>
#include <simman.h>
#include <simstate.h>
#include <simtime.h>
#include <tagfile.h>
#include <weapcb.h>

#include <ai.h>
#include <aialert.h>
#include <aiaware.h>
#include <aiapi.h>
#include <aiapimov.h>
#include <aiapisns.h>
#include <aibasbhv.h>
#include <aidebug.h>
#include <aicblink.h>
#include <aicnvman.h>
#include <aicpinit.h>
#include <ailoop.h>
#include <aiman.h>
#include <aimanbhv.h>
#include <aipathdb.h>
#include <aiphycst.h>
#include <aiprabil.h>
#include <aiprattr.h>
#include <aiprutil.h>
#include <aiprcore.h>
#include <aiprutil.h>
#include <aisched.h>
#include <aisgtcst.h>
#include <aisndtyp.h>
#include <aisuslnk.h>
#include <aireport.h>

// for the link stuff
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

// @TBD (toml 05-26-98): e3 hack...
#include <aie3bhv.h>
// @TBD (toml 05-26-98): ...e3 hack

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Helper functions/macros
//

BOOL ObjIsAI(ObjID obj)
{
   if (g_pAIManager)
      return !!g_pAIManager->AccessAI(obj);
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Globals
//

const cMxsVector kInvalidLoc(FLT_MAX, FLT_MAX, FLT_MAX);

///////////////////////////////////////

cAIManager * g_pAIManager;
const char * g_pszAIDefBehaviorSet;

///////////////////////////////////////

tResult LGAPI _AIManagerCreate(REFIID, IAIManager ** ppScriptMan, IUnknown * pOuter)
{
   g_pAIManager = new cAIManager(pOuter);
   g_AILoopClientDesc.pClient = g_pAIManager;
   return (g_pAIManager) ? S_OK : E_FAIL;

}

///////////////////////////////////////////////////////////////////////////////

const char * g_ppszAISpeed[kAIS_Num] =
{
   "Stopped",                                    // kAIS_Stopped
   "Very slow",                                  // kAIS_VerySlow
   "Slow",                                       // kAIS_Slow
   "Normal",                                     // kAIS_Normal
   "Fast",                                       // kAIS_Fast
   "Very fast",                                  // kAIS_VeryFast
};

///////////////////////////////////////

const char * AIGetSpeedName(eAISpeed speed)
{
   return g_ppszAISpeed[speed];
}

eAISpeed AIGetSpeedFromName(const char * pszSpeed)
{
   for (int i = 0; i < kAIP_Num; i++)
   {
      if (stricmp(pszSpeed, g_ppszAISpeed[i]) == 0)
         return (eAISpeed)i;
   }
   return kAIS_Normal;
}

///////////////////////////////////////////////////////////////////////////////

const char * g_ppszAIPriority[kAIP_Num] =
{
   "Default/None",                               // kAIP_None
   "Very low",                                   // kAIP_VeryLow
   "Low",                                        // kAIP_Low
   "Normal",                                     // kAIP_Normal
   "High",                                       // kAIP_High
   "Very high",                                  // kAIP_VeryHigh
   "Absolute",                                   // kAIP_Absolute
};

///////////////////////////////////////

const char * AIGetPriorityName(eAIPriority priority)
{
   return g_ppszAIPriority[priority];
}

///////////////////////////////////////

static sFieldDesc g_AIPriorityFieldDesc[] =
{
   { "", kFieldTypeEnum, sizeof(int), 0, kFieldFlagUnsigned, 0, kAIP_Num, kAIP_Num, g_ppszAIPriority },
};

static sStructDesc g_AIPriorityStructDesc =
{
   "eAIPriority",
   sizeof(int),
   kStructFlagNone,
   sizeof(g_AIPriorityFieldDesc)/sizeof(g_AIPriorityFieldDesc[0]),
   g_AIPriorityFieldDesc,
};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBehaviorSets
//

#ifdef _MSC_VER
template cHashTable<const char *, IAIBehaviorSet *, cHashTableCaselessFunctions<const char *> >;
#endif

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: cAIManager::sAIScheduleInfo
//

inline void cAIManager::sAIScheduleInfo::Init(cAI * pInitAI)
{
   pAI = pInitAI;
   lastRun = 0;
   lastRender = 0;
   lastRefusal = 0;
   //flags = 0;
}

///////////////////////////////////////

inline BOOL cAIManager::sAIScheduleInfo::IsRunRequired() const
{
   unsigned time = AIGetTime();
   return (time - lastRender < kST_MinimumRenderedRun ||
           time - lastRefusal < kST_MinimumRefusalRun);
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIManager
//


//
// Loop Client Hooks
//
sLoopClientDesc g_AILoopClientDesc =
{
   &LOOPID_AI,
   "AI Client",
   kPriorityNormal,
   kMsgEnd | kMsgDatabase | kMsgsMode | kMsgsFrame | kMsgsAppOuter,

   kLCF_Singleton,
   0,                                            // Set when AI manager is created

   NO_LC_DATA,

   {
     { kConstrainAfter, &LOOPID_BrushList,      kMsgDatabase },
     { kConstrainAfter, &LOOPID_ObjSys,         kMsgDatabase },
     { kConstrainAfter, &LOOPID_EngineFeatures, kMsgNormalFrame },
     { kConstrainAfter, &LOOPID_EngineFeatures, kMsgsAppOuter },
     { kNullConstraint }
   }
};

///////////////////////////////////////

cAIManager::cAIManager(IUnknown * pOuter)
 : cLoopClient<kCTU_NoSelfDelete>(&g_AILoopClientDesc),
   m_pBehaviorSets(new cAIBehaviorSets),
   m_flags(0),
   m_pConversationManager(new cAIConversationManager),
   m_ignoreSoundTime(1000)
{
   MI_INIT_AGGREGATION_3(pOuter, IAIManager, IAINetManager, IAINetServices, kPriorityLibrary, NULL);
}

///////////////////////////////////////

cAIManager::~cAIManager()
{
   g_pAIManager = NULL;
   delete m_pBehaviorSets;
   delete m_pConversationManager;
}

///////////////////////////////////////
//
// Expose a behavior factory to the AI
//

STDMETHODIMP cAIManager::InstallBehaviorSet(IAIBehaviorSet * pBehaviorSet)
{
   AssertMsg1(!m_pBehaviorSets->Get(pBehaviorSet->GetName()), "AI behavior set %s added more than once", pBehaviorSet->GetName());
   m_pBehaviorSets->Add(pBehaviorSet);
   pBehaviorSet->AddRef();
   return S_OK;
}

///////////////////////////////////////
//
// Find the AI for a given object
//

STDMETHODIMP_(IAI *) cAIManager::GetAI(ObjID objId)
{
   int i = GetAIIndex(objId);

   if (i != kAINoIndex)
   {
      m_AIs[i]->AddRef();
      return m_AIs[i];
   }

   return NULL;
}

///////////////////////////////////////
//
// Make an AI out of a given object
//

STDMETHODIMP_(BOOL) cAIManager::CreateAI(ObjID objId, const char * pszBehaviorSet)
{
   cAI * pAI;

   if ((pAI = (cAI *) GetAI(objId)) != NULL)
   {
      Warning(("Double create of AI %d\n", objId));
      return FALSE;
   }

   if (stricmp(pszBehaviorSet, "Null") == 0)
      return FALSE;

   pszBehaviorSet = NetSelectBehaviorSet(objId, pszBehaviorSet);

   IAIBehaviorSet * pBehaviorSet = m_pBehaviorSets->Get(pszBehaviorSet);

   if (!pBehaviorSet)
   {
      Warning(("AI behavior set \"%s\" not found when creating AI %d. Using \"%s\".\n", pszBehaviorSet, objId, g_pszAIDefBehaviorSet));
      pBehaviorSet = m_pBehaviorSets->Get(g_pszAIDefBehaviorSet);
   }

   pAI = pBehaviorSet->CreateAI(objId);

   if (!pAI)
      return FALSE;

   if (!g_pAIModeProperty->IsRelevant(objId))
      g_pAIModeProperty->Set(objId, kAIM_Normal);

   if (!g_pAIAlertnessProperty->IsRelevant(objId))
   {
      sAIAlertness temp;
      g_pAIAlertnessProperty->Set(objId, &temp);
   }

   m_AIs.Append(pAI);
   int i = m_Schedule.Grow();
   m_Schedule[i].Init(pAI);

   // initialize our ghost here
   NetSetupAIGhost(objId);

   return TRUE;
}

///////////////////////////////////////
//
// Destroy an AI
//

STDMETHODIMP_(BOOL) cAIManager::DestroyAI(ObjID objId)
{
   int i = GetAIIndex(objId);

   if (i != kAINoIndex)
   {
      if (!(m_flags & kDeferDestroy))
      {
         // This needs to happen before the deletion per se, since how it
         // works depends on proxy info in the AI:
         NetRemoveAIGhost(objId);

         g_pAIModeProperty->Delete(objId);
         g_pAIAlertnessProperty->Delete(objId);
         // @TBD (toml 05-22-98): should clear all AI links as well

         cAI * pAI = m_AIs[i];

         m_AIs.FastDeleteItem(i);
         m_Schedule.FastDeleteItem(i);

         pAI->End();

         int refs = pAI->Release();

         if (refs != 0)
         {
            mprintf("ERROR: Attempted to destroy an instance of cAI (%d) that may still be in use, %d refs outstanding\n", objId, refs);
            while (pAI->Release())
               ;
         }

      }
      else
      {
         m_Destroys.Append(m_AIs[i]);
      }
      return TRUE;
   }

   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIManager::SelfSlayAI(ObjID objId)
{
   m_SelfSlays.Append(objId);
   return TRUE;
}

///////////////////////////////////////
//
// Count the AIs
//

STDMETHODIMP_(unsigned) cAIManager::CountAIs()
{
   return m_AIs.Size();
}

///////////////////////////////////////
//
// Iteration protocol
//

STDMETHODIMP_(IAI *) cAIManager::GetFirst(tAIIter * pIter)
{
   if (!m_AIs.Size())
   {
      *pIter = (tAIIter)0xbadbad;
      return NULL;
   }
   *pIter = (tAIIter)0;
   m_AIs[0]->AddRef();
   return m_AIs[0];
}

///////////////////////////////////////

STDMETHODIMP_(IAI *) cAIManager::GetNext(tAIIter * pIter)
{
   unsigned i = (*((unsigned *)(pIter))) + 1;
   if (i == m_AIs.Size())
   {
      *pIter = (tAIIter)0xbadbad;
      return NULL;
   }

   *pIter = (tAIIter)i;
   m_AIs[i]->AddRef();
   return m_AIs[i];
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIManager::GetDone(tAIIter * /*pIter*/)
{
}

///////////////////////////////////////

int cAIManager::GetAIIndex(ObjID id)
{
   int i;
   for (i = 0; i < m_AIs.Size(); i++)
   {
      if (m_AIs[i]->GetID() == id)
         return i;
   }
   return kAINoIndex;
}

///////////////////////////////////////
//
// Motion hooks
//

STDMETHODIMP cAIManager::SetMotor(ObjID objId, IMotor * pMotor)
{
   HRESULT result = E_FAIL;
   int i = GetAIIndex(objId);

   if (i != kAINoIndex)
   {
      IAIMoveEnactor * pMoveEnactor = m_AIs[i]->AccessMoveEnactor();
      if (pMoveEnactor)
      {
         result = pMoveEnactor->SetMotor(pMotor);
      }
   }
   return S_OK;
}

///////////////////////////////////////

STDMETHODIMP cAIManager::ResetMotionTags(ObjID objId)
{
   HRESULT result = E_FAIL;
   IAIMoveEnactor * pMoveEnactor;

   if (objId != OBJ_NULL)
   {
      int i = GetAIIndex(objId);

      if (i != kAINoIndex)
      {
         pMoveEnactor = m_AIs[i]->AccessMoveEnactor();
         if (pMoveEnactor)
         {
            result = pMoveEnactor->ResetMotionTags();
         }
      }
   }
   else
   {
      result = S_OK; // @tbd (toml 04-28-98)
      for (int i = 0; i < m_AIs.Size(); i++)
      {
         pMoveEnactor = m_AIs[i]->AccessMoveEnactor();
         if (pMoveEnactor)
         {
            result = pMoveEnactor->ResetMotionTags();
         }
      }
   }

   return S_OK;
}

///////////////////////////////////////
//
// Projectile hook
//

#define kFastObjXYDistSq sq(60.0)
#define kFastObjZDiff    15.0

STDMETHODIMP cAIManager::NotifyFastObj(ObjID firer, ObjID projectile, const mxs_vector & velocity)
{
   for (int i = 0; i < m_AIs.Size(); i++)
   {
      if (m_AIs[i]->GetState()->InsideCylinder(*GetObjLocation(firer), kFastObjXYDistSq, kFastObjZDiff))
         m_AIs[i]->NotifyFastObj(firer, projectile, velocity);
   }
   return S_OK;
}

///////////////////////////////////////
//
// Sound hook
//

STDMETHODIMP cAIManager::NotifySound(const sSoundInfo * pInfo)
{
   // @Note (toml 09-21-98): hack to get around physics objects making sounds while settling
   if (AIGetTime() < m_ignoreSoundTime)
      return S_OK;

   if (pInfo->Object != pInfo->SrcObject)
   {
      const sAISoundType * pType = AIGetSoundType(pInfo->SchemaID);
      if (pType)
      {
         int i = GetAIIndex(pInfo->Object);

         if (i != kAINoIndex)
            m_AIs[i]->NotifySound(pInfo, pType);
      }
   }
   return S_OK;
}

///////////////////////////////////////
//
// Sound hook
//

STDMETHODIMP_(void) cAIManager::SetIgnoreSoundTime(tSimTime time)
{
   m_ignoreSoundTime = AIGetTime()+time;
}

///////////////////////////////////////
//
// Conversation Manager
//
STDMETHODIMP_(BOOL) cAIManager::StartConversation(ObjID conversationID)
{
   return m_pConversationManager->Start(conversationID);
}

///////////////////////////////////////
//
// Conversation Manager
//

STDMETHODIMP_(IAIConversationManager*) cAIManager::GetConversationManager(void)
{
   m_pConversationManager->AddRef();
   return (IAIConversationManager*)m_pConversationManager;
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIManager::ListenToProperty(IProperty * pProperty)
{
   for (int i = 0 ; i < m_ListenedProperties.Size(); i++)
   {
      if (m_ListenedProperties[i] == pProperty)
         return;
   }

   pProperty->Listen(kListenPropModify | kListenPropSet | kListenPropUnset | kListenPropLoad,
                     PropListener,
                     (PropListenerData)pProperty);

   m_ListenedProperties.Append(pProperty);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIManager::ListenForCollisions(ObjID objID)
{
   PhysListen(m_physListenerHandle, objID);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIManager::UnlistenForCollisions(ObjID objID)
{
   PhysUnlisten(m_physListenerHandle, objID);
}

///////////////////////////////////////
//
// Central point of control flow
//

DECLARE_TIMER(AI_AIMAN_ONF, Average);

STDMETHODIMP_(eLoopMessageResult) cAIManager::ReceiveMessage(eLoopMessage message, tLoopMessageData hData)
{
   eLoopMessageResult result = kLoopDispatchContinue;
   int i;
   static ITagFile * pMisTagFile;
   static BOOL EnteredSinceLoad = FALSE;

   switch (message)
   {
      case kMsgEnterMode:
         break;

      case kMsgExitMode:
         break;

      case kMsgAppInit:
      {
         AIInitCoreProps();
         AIInitAttrProps();
         AIInitAbilProps();
         AIInitUtilProps();
         AIReportInit();
         g_AIPathDB.Init();
         AIInitStandardComponents(this);
         AIInstallCommands();
         AIInitAwareness();
         AIInitSuspiciousLinks();
         AIInitWeaponCallbacks();
         AIInitSightCast();

         // Set up physics listener
         m_physListenerHandle = PhysCreateListener("AI", kCollisionMsg, &PhysicsListener, NULL);

         if (config_is_defined("ai_schedule"))
         {
            config_get_int("ai_schedule", &g_AIScheduleSettings.budget);
            g_AIScheduleSettings.fActive = !!g_AIScheduleSettings.budget;
         }

         break;
      }

      case kMsgAppTerm:
      {
         SafeRelease(pMisTagFile);
         AITermAwareness();
         AITermSuspiciousLinks();
         AIReportTerm();
         AITermUtilProps();
         AITermAbilProps();
         AITermAttrProps();
         AITermCoreProps();
         AITermStandardComponents();
         g_AIPathDB.Term();
         AITermWeaponCallbacks();
         AITermSightCast();
         break;
      }

      case kMsgDatabase:
      {
         DispatchData * pDispatchData = (DispatchData *)hData;

         switch (DB_MSG(pDispatchData->subtype))
         {
            case kDatabaseReset:
               Assert_(!pMisTagFile);
               DatabaseReset();
               m_pConversationManager->Reset();
               //AIDBResets:AMSD 3/25/00
               AIDBResetUtilProps();
               AIDBResetStandardComponents();
               AIDBResetSightCast();
               //AISoundReset();
               break;

            case kDatabaseSave:
               Assert_(!pMisTagFile);
               if (pDispatchData->subtype & kDBMap)
                  AIPathFindWrite((ITagFile *)(pDispatchData->data));

#ifndef NO_AI_SAVELOAD
               if (!config_is_defined("ai_no_save_load"))
               {
                  if (pDispatchData->subtype & kObjPartConcrete)
                  {
                     AutoAppIPtr (ObjectSystem);

                     for (i = 0; i < m_AIs.Size(); i++)
                     {
                        if (pObjectSystem->IsObjSavePartition (m_AIs[i]->GetObjID(), (eObjPartition) pDispatchData->subtype))
                           if (!m_AIs[i]->PreSave())
                              goto io_error;
                     }

                     for (i = 0; i < m_AIs.Size(); i++)
                     {
                        if (pObjectSystem->IsObjSavePartition (m_AIs[i]->GetObjID(), (eObjPartition) pDispatchData->subtype))
                           if (!m_AIs[i]->Save((ITagFile *)(pDispatchData->data)))
                              goto io_error;
                     }

                     for (i = 0; i < m_AIs.Size(); i++)
                     {
                        if (pObjectSystem->IsObjSavePartition (m_AIs[i]->GetObjID(), (eObjPartition) pDispatchData->subtype))
                           if (!m_AIs[i]->PostSave())
                              goto io_error;
                     }

                     // Save out ongoing conversations.
                     m_pConversationManager->Save((ITagFile *)(pDispatchData->data));
                  }
               }
#endif
               break;

            case kDatabaseLoad:
               if (pDispatchData->subtype & kDBMap)
                  AIPathFindRead((ITagFile *)(pDispatchData->data));
#ifndef NO_AI_SAVELOAD
               if (!config_is_defined("ai_no_save_load"))
               {
                  if (pDispatchData->subtype & kObjPartConcrete)
                  {
                     Assert_(!pMisTagFile);
                     pMisTagFile = (ITagFile *)(pDispatchData->data);
                     pMisTagFile->AddRef();
                  }
               }
#endif
               break;

            case kDatabasePostLoad:
#ifndef NO_AI_SAVELOAD
               if (!config_is_defined("ai_no_save_load"))
               {
                  if (pMisTagFile && (pDispatchData->subtype & kObjPartConcrete))
                  {
                     for (i = 0; i < m_AIs.Size(); i++)
                     {
                        if (!m_AIs[i]->PreLoad())
                           goto io_error;
                     }

                     for (i = 0; i < m_AIs.Size(); i++)
                     {
                        if (!m_AIs[i]->Load(pMisTagFile))
                           goto io_error;
                     }

                     for (i = 0; i < m_AIs.Size(); i++)
                     {
                        if (!m_AIs[i]->PostLoad())
                           goto io_error;
                     }

                     // Load in ongoing conversations.
                     m_pConversationManager->Load(pMisTagFile);

                     SafeRelease(pMisTagFile);
                  }
               }
#endif
               break;
         }
         break;
      }

      case kMsgNormalFrame:
      {
         AIPathFindUpdate();
         OnNormalFrame();
         break;
      }

      case kMsgSuspendMode:
         break;

      case kMsgResumeMode:
         break;

      case kMsgEnd:
         break;
   }
io_error:
   return result;
}

///////////////////////////////////////
//
// Initialize the AI
//

STDMETHODIMP cAIManager::Init()
{
   StructDescRegister(&g_AIPriorityStructDesc);

   // Install the default AI behavior sets
   IAIBehaviorSet * pBehaviorSet = new cAIBehaviorSet;
   g_pszAIDefBehaviorSet = pBehaviorSet->GetName();
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

#ifndef SHIP
   pBehaviorSet = new cAIMotionTestBehaviorSet;
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   pBehaviorSet = new cAIMotionTagsTestBehaviorSet;
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);
#endif

   pBehaviorSet = new cAIRangedBehaviorSet;
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   pBehaviorSet = new cAIWatcherBehaviorSet;
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   pBehaviorSet = new cAISimpleBehaviorSet;
   InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   // Hook up for sim start/end messages
   static sDispatchListenerDesc desc =
   {
      &IID_IAIManager,
      (kSimInit | kSimTerm | kSimSuspend | kSimResume),
      DispatchListenFunc,
      this
   };

   AutoAppIPtr(SimManager);
   pSimManager->Listen(&desc);

   AutoAppIPtr(DamageModel);
   pDamageModel->Listen(kDamageMsgDamage | kDamageMsgTerminate, DamageListener, this);

#ifdef NEW_NETWORK_ENABLED
   NetInit();
#endif

   return S_OK;
}

///////////////////////////////////////
// weapon callbacks
void cAIManager::AIInitWeaponCallbacks()
{
   RegisterWeaponEventCallback(kAllWeaponEvents, AIWeaponListener, this);
}

void cAIManager::AITermWeaponCallbacks()
{
   DeregisterWeaponEventCallback(kAllWeaponEvents, AIWeaponListener);
}

///////////////////////////////////////

STDMETHODIMP cAIManager::End()
{
   AutoAppIPtr(SimManager);

   pSimManager->Unlisten(&IID_IAIManager);
   PhysDestroyListener(m_physListenerHandle);
   m_pBehaviorSets->ReleaseAll();

#ifdef NEW_NETWORK_ENABLED
   NetTerm();
#endif
   return S_OK;
}

///////////////////////////////////////

int cAIManager::AIScheduleSort(cAIManager::sAIScheduleInfo * const * ppLeft, cAIManager::sAIScheduleInfo * const * ppRight)
{
   if ((*ppLeft)->IsRunRequired())
   {
      if ((*ppRight)->IsRunRequired())
         return 0;
      return -1;
   }
   if ((*ppRight)->IsRunRequired())
   {
      return 1;
   }

   return ((*ppLeft)->lastRun - (*ppRight)->lastRun);
}

///////////////////////////////////////

BOOL cAIManager::PreFrameValidate()
{
#ifndef SHIP
   static cAITimer warningTimer(kAIT_10Sec);
   if (!g_fAIPathFindInited)
   {
      if (warningTimer.Expired())
      {
         mprintf("WARNING: No pathfinding database. AI will not run\n");
         warningTimer.Reset();
      }
      return FALSE;
   }
#else
   if (!g_fAIPathFindInited)
      return FALSE;
#endif

#ifndef SHIP
   //
   // Validate object system invariant
   //
   AutoAppIPtr(ObjectSystem);
   cAIArray AIs(m_AIs);
   ObjID    id;
   BOOL     fDelete;
   int      i;

   for (i = 0; i < AIs.Size(); i++)
   {
      fDelete = FALSE;
      id = AIs[i]->GetID();
      if (!g_pAIProperty->IsRelevant(id))
      {
         mprintf("ERROR: A concrete AI exists for object %d, but the object doesn't\n"
                  "have the AI property\n", id);
         fDelete = TRUE;
      }
      else if (!pObjectSystem->Exists(id))
      {
         mprintf("ERROR: A concrete AI exists for object %d, but the object doesn't exist\n", id);
         fDelete = TRUE;
      }
      if (fDelete)
         DestroyAI(id);
   }

   sPropertyObjIter iter;
   IAI *            pAI;

   g_pAIProperty->IterStart(&iter);
   while (g_pAIProperty->IterNext(&iter, &id))
   {
      pAI = GetAI(id);
      if (OBJ_IS_CONCRETE(id) && !pAI)
      {
         sAIProp * pProp;
         g_pAIProperty->Get(id, &pProp);
         if (stricmp(pProp->szBehaviorSet, "Null") != 0)
         {
            mprintf("ERROR: Object %d has the AI property, but no concrete AI\n", id);
            CreateAI(id, pProp->szBehaviorSet);
         }
      }
      SafeRelease(pAI);
   }
   g_pAIProperty->IterStop(&iter);

#endif
   return TRUE;
}

///////////////////////////////////////
//
// Run one frame
//

#define kDefaultScheduleBudget 5
sAIScheduleSettings g_AIScheduleSettings = { FALSE, kDefaultScheduleBudget };

DECLARE_TIMER(AI_AIMAN_ONF_Visibility, Average);
DECLARE_TIMER(AI_AIMAN_ONF_Schedule, Average);
DECLARE_TIMER(AI_AIMAN_ONF_RunAIs, Average);
DECLARE_TIMER(AI_AIMAN_ONF_Post, Average);

///////////////////

//cAITimer g_AIVisUpdateTimer(kAIT_10Hz);

void cAIManager::OnNormalFrame()
{
   int i;

   if (!SimStateCheckFlags(kSimAI))
      return;

#ifndef SHIP
   if (Networking() && config_is_defined("net_no_ais"))
      return;
#endif

   AUTO_TIMER(AI_AIMAN_ONF);

   if (!PreFrameValidate())
      return;

   //
   // Update player lighting.
   //

   m_FrameStartTime = tm_get_millisec();

   //if (g_AIVisUpdateTimer.Expired())
   {
   
      TIMER_Start(AI_AIMAN_ONF_Visibility);

      AutoAppIPtr(AINetServices);
      tAIPlayerList players;

      pAINetServices->GetPlayers(&players);

      ObjID * pPlayerID = &players[0];

      while (*pPlayerID)
      {
         AIUpdateVisibility(*pPlayerID);
         pPlayerID++;
      }

      TIMER_MarkStop(AI_AIMAN_ONF_Visibility);
      //g_AIVisUpdateTimer.Reset();
   }
 
   for (i = 0; i < m_AIs.Size(); i++)
      m_AIs[i]->UpdateRenderedStatus();


   // given a percentage of frame threshhold, calculate 10 frame
   // average, and order cycling, possibly not running AIs that
   // put us over the limit:

   // run all AIs that are dying
   // run all AIs that have never been run
   // run all AIs that have been rendered in the last 10 frames
   // sort the rest by distance from the render camera, run them until
   // we've reached our budget limit (toml 03-23-98)

   m_flags |= kDeferDestroy;

   if (g_AIScheduleSettings.fActive)
   {
      ScheduledNormalFrame();
   }
   else
   {
      TIMER_Start(AI_AIMAN_ONF_RunAIs);

      for (i = 0; i < m_AIs.Size(); i++)
         m_AIs[i]->OnNormalFrame();

      TIMER_MarkStop(AI_AIMAN_ONF_RunAIs);
   }

   TIMER_Start(AI_AIMAN_ONF_Post);

   m_flags &= ~kDeferDestroy;

   for (i = 0; i < m_Destroys.Size(); i++)
      DestroyAI(m_Destroys[i]->GetID());
   m_Destroys.SetSize(0);

   for (i = 0; i < m_SelfSlays.Size(); i++)
   {
      AutoAppIPtr(DamageModel); // local as this is only passed through when a frog explodes -- i.e., rarely
      pDamageModel->SlayObject(m_SelfSlays[i], m_SelfSlays[i]);
   }
   m_SelfSlays.SetSize(0);

   TransferDeferred();

   g_pAIPatrolPropObs->SetClean();

   m_pConversationManager->Frame();

   m_FrameStartTime = -1;

   TIMER_MarkStop(AI_AIMAN_ONF_Post);
}

///////////////////////////////////////

void cAIManager::ScheduledNormalFrame()
{
   int i;

   TIMER_Start(AI_AIMAN_ONF_Schedule);

   AITrace1(Schedule, "Scheduling %d AIs", m_Schedule.Size());

   cDynArray_<sAIScheduleInfo *, 64> SortedSchedule;

   // Identify all the AIs that are being rendered
   for (i = 0; i < m_Schedule.Size(); i++)
   {
      AssertMsg(m_Schedule[i].pAI == m_AIs[i], "Inconsistent AI manager schedule info");
      if (m_Schedule[i].pAI->WasRendered())
         m_Schedule[i].lastRender = AIGetTime();
      SortedSchedule.Append(&m_Schedule[i]);
   }

   // Sort the AIs, rendered up front, then by time since having been run
   SortedSchedule.Sort(AIScheduleSort);

   // First, run all of the rendered AIs
   AITrace(Schedule, "Required runs:");
   AITraceRaw(Schedule, "   ");
   for (i = 0; i < m_Schedule.Size(); i++)
   {
      if (SortedSchedule[i]->IsRunRequired() || !SortedSchedule[i]->lastRun)
      {
         AITraceRaw3(Schedule, "[%d (%d, %d)],", GetAIIndex(SortedSchedule[i]->pAI->GetID()), SortedSchedule[i]->pAI->GetID(), AIGetTime() - SortedSchedule[i]->lastRun);

         SortedSchedule[i]->lastRun = AIGetTime();

         TIMER_Stop(AI_AIMAN_ONF_Schedule);
         TIMER_Start(AI_AIMAN_ONF_RunAIs);

         SortedSchedule[i]->pAI->OnNormalFrame();

         TIMER_Stop(AI_AIMAN_ONF_RunAIs);
         TIMER_Start(AI_AIMAN_ONF_Schedule);
      }
      else
         break;
   }
   AITraceRaw(Schedule, "\n");

   // Now, run the rest until time is up, always running at least one
   if (i < m_Schedule.Size())
   {
      AITrace(Schedule, "Optional runs:");
      AITraceRaw(Schedule, "   ");
      do
      {
         AITraceRaw3(Schedule, "[%d (%d, %d)],", GetAIIndex(SortedSchedule[i]->pAI->GetID()), SortedSchedule[i]->pAI->GetID(), AIGetTime() - SortedSchedule[i]->lastRun);

         SortedSchedule[i]->lastRun = AIGetTime();

         TIMER_Stop(AI_AIMAN_ONF_Schedule);
         TIMER_Start(AI_AIMAN_ONF_RunAIs);

         SortedSchedule[i++]->pAI->OnNormalFrame();

         TIMER_Stop(AI_AIMAN_ONF_RunAIs);
         TIMER_Start(AI_AIMAN_ONF_Schedule);

      } while (i < m_Schedule.Size() && tm_get_millisec() - m_FrameStartTime <= g_AIScheduleSettings.budget);

      AITraceRaw(Schedule, "\n");

// @TBD (toml 02-26-00): should un-ship these, and have a callback that can be used to stop non-locomotive AIs during off-frames?
#ifndef SHIP
      if (g_AIScheduleTrace && i < m_Schedule.Size())
      {
         AITrace(Schedule, "Not run:");
         AITraceRaw(Schedule, "   ");
         do
         {
            AITraceRaw3(Schedule, "[%d (%d, %d)],", GetAIIndex(SortedSchedule[i]->pAI->GetID()), SortedSchedule[i]->pAI->GetID(), AIGetTime() - SortedSchedule[i]->lastRun);
            i++;
         } while (i < m_Schedule.Size());
         AITraceRaw(Schedule, "\n");
      }
#endif
   }

   TIMER_MarkStop(AI_AIMAN_ONF_Schedule);
   TIMER_Mark(AI_AIMAN_ONF_RunAIs);

   AITrace1(Schedule, "Total time: %d", tm_get_millisec() - m_FrameStartTime);
}

///////////////////////////////////////

void cAIManager::DatabaseReset()
{
   for (int i = 0; i < m_AIs.Size(); i++)
   {
      m_AIs[i]->End();
      if (m_AIs[i]->Release() != 0)
      {
         mprintf("ERROR: Attempted to destroy an instance of cAI (%d) that may still be in use\n", m_AIs[i]->GetID());
         while (m_AIs[i]->Release())
            ;
      }
   }
   m_AIs.SetSize(0);
   m_Schedule.SetSize(0);
   m_NetHostedAIs.SetSize(0);
}

///////////////////////////////////////

void cAIManager::DatabasePostLoad()
{
}

///////////////////////////////////////
//
// Route sim messages to AIs
//

void cAIManager::DispatchListenFunc(const sDispatchMsg* pMsg, const sDispatchListenerDesc* pDesc)
{
   cAIArray & AIs = ((cAIManager *)pDesc->data)->m_AIs;
   int i;

   switch (pMsg->kind)
   {
      case kSimInit:
         for (i = 0; i < AIs.Size(); i++)
            ((IInternalAI *)(AIs[i]))->NotifySimStart();
         // Fall through
      case kSimResume:
         AIUpdateDoors();
         AISightCastCollectRefs();
         AIPhysCastCollectObjects();
#ifndef SHIP
         g_AIEfficiencyDebug = config_is_defined("ai_test_eff");
#endif
         break;

      case kSimTerm:
         for (i = 0; i < AIs.Size(); i++)
            ((IInternalAI *)(AIs[i]))->NotifySimEnd();

      case kSimSuspend:
         break;
   }
}

///////////////////////////////////////
//
// Route damage messages to AIs
//

eDamageResult LGAPI cAIManager::DamageListener(const sDamageMsg* pMsg, tDamageCallbackData data)
{
   cAIArray & AIs = ((cAIManager *)data)->m_AIs;

   for (int i = 0; i < AIs.Size(); i++)
   {
      if (AIs[i]->GetID() == pMsg->victim)
      {
         return ((IInternalAI *)(AIs[i]))->NotifyDamage(pMsg);
      }
   }
   return kDamageNoOpinion;
}

///////////////////////////////////////
//
// Route weapon messages to AIs
//

// this is complicated somewhat by the fact that we dont only want to send messages to victim
//  and culprit.  In practice, the culprit might be your opponent, and he/she/it might be, say,
//  starting an attack.  And if that were happening, you'd like to know.
// so, in cases with a culprit but no victim, we look for attack links to the culprit, and send
//  messages to anyone who considers the culprit a current opponent

void cAIManager::AIWeaponListener(eWeaponEvent ev, ObjID victim, ObjID culprit, void *data)
{
   cAIArray & AIs = ((cAIManager *)data)->m_AIs;
   int        i;

   // Notify culprit
   for (i = 0; i < AIs.Size(); i++)
   {
      if (AIs[i]->GetID() == culprit)
      {
         ((IInternalAI *)(AIs[i]))->NotifyWeapon(ev, victim, culprit);
         break;
      }
   }

   // @TBD (toml 07-15-98): this clearly needs a better heuristic

   // Notify victim(s)
   if (ev & kStartEndEvents)
   {
      cDynArray<BOOL> notified(AIs.Size());
      sLink           link;

      memset(notified, 0, AIs.Size() * sizeof(BOOL));

      // First notify anyone in active engagement with culprit
      ILinkQuery * pQuery = g_pAIAttackLinks->Query(LINKOBJ_WILDCARD,culprit);
      for (; !pQuery->Done(); pQuery->Next())
      {
         pQuery->Link(&link);
         i = ((cAIManager *)data)->GetAIIndex(link.source);
         if (i != kAINoIndex)
         {
            ((IInternalAI *)(AIs[i]))->NotifyWeapon(ev, link.source, culprit);
            notified[i] = TRUE;
         }
      }
      SafeRelease(pQuery);

      // Then notify AIs that witness the attack
      pQuery = g_pAIAwarenessLinks->Query(LINKOBJ_WILDCARD, culprit);
      for (; !pQuery->Done(); pQuery->Next())
      {
         pQuery->Link(&link);
         i = ((cAIManager *)data)->GetAIIndex(link.source);
         if (i != kAINoIndex && !notified[i])
         {
            const sAIAwareness * pAwareness = AIs[i]->GetAwareness(culprit);
            if (pAwareness->flags & kAIAF_HaveLOS)
            {
               ((IInternalAI *)(AIs[i]))->NotifyWeapon(ev, link.source, culprit);
               // notified[i] = TRUE; not needed, but would if have further logic below (toml 08-11-98)
            }
         }
      }
      SafeRelease(pQuery);
   }
}

///////////////////////////////////////

void LGAPI cAIManager::PropListener(sPropertyListenMsg* pMsg, PropListenerData data)
{
   if (!g_pAIManager)
      return;

   static int fInDispatch;

   Assert_(!fInDispatch);

   cAIArray & AIs = g_pAIManager->m_AIs;

   fInDispatch = TRUE;

   for (int i = 0; i < AIs.Size(); i++)
      AIs[i]->NotifyProperty((IProperty *)data, pMsg);

   fInDispatch = FALSE;
}

///////////////////////////////////////

void LGAPI cAIManager::PhysicsListener(ObjID objID, const sPhysListenMsg* pMsg, PhysListenerData data)
{
   if (!g_pAIManager)
      return;

   static int fInDispatch;

   Assert_(!fInDispatch);

   cAIArray & AIs = g_pAIManager->m_AIs;

   fInDispatch = TRUE;

   for (int i = 0; i < AIs.Size(); i++)
      if (AIs[i]->GetID() == objID)
         AIs[i]->NotifyCollision(pMsg);

   fInDispatch = FALSE;
}

///////////////////////////////////////////////////////////////////////////////

