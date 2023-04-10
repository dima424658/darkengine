/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/engfeat.cpp,v 1.75 2000/01/24 12:08:42 adurant Exp $

//#define PROFILE_ON

#include <timings.h>
#include <comtools.h>
#include <appagg.h>

#include <loopapi.h>
#include <loopmsg.h>
#include <dbasemsg.h>
#include <dispbase.h>
#include <dispapi.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <objtype.h>
#include <engfeat.h>
#include <engfprop.h>
#include <simtime.h>

#include <mprintf.h>

#include <phcollev.h>
#include <ctrast.h>
#include <objslit.h>
#include <scrptapi.h>
#include <scrptcmd.h>
#include <scrptst.h>
#include <prjctile.h>
#include <tweqctrl.h>
#include <hdskprop.h>
#include <meshatt.h>
#include <hpprop.h>
#include <trapprop.h>
#include <gfxprop.h>
#include <slitprop.h>
#include <frobprop.h>
#include <frobctrl.h>
#include <sprkprop.h>
#include <doorprop.h>
#include <doorphys.h>
#include <doorrep.h>
#include <scrptprp.h>
#include <lockprop.h>
#include <contain.h>
#include <contprop.h>
#include <weapon.h>
#include <weaphalo.h>
#include <gamemode.h>
#include <metagame.h>
#include <playtest.h>
#include <invlimb.h>
#include <inv_rend.h>
#include <animtext.h>
#include <editsave.h>
#include <autovbr.h>
#include <pick.h>
#include <quesreac.h>
#include <iobjsys.h>
#include <objmedia.h>
#include <playrobj.h>
#include <sndnet.h>
#include <ctagnet.h>
#include <litnet.h>
#include <netsim.h>
#include <netsynch.h>
#include <textarch.h>
#include <preldprp.h>
#include <netman.h>
#include <susprop.h>

#include <panltool.h>
#include <simstate.h>
#include <simflags.h>

#include <simloop.h>
#include <ailoop.h>
#include <objloop.h>
#include <rendloop.h>
#include <scrnloop.h>
#include <netloop.h>
#include <simman.h>
#include <simdef.h>
#include <dspchdef.h>

#include <flowarch.h>

// For collision bashing
#include <filevar.h>
#include <stimtype.h>
#include <stimbase.h>
#include <stimsens.h>
#include <stimul8r.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <sdestool.h>
#include <simtime.h>
#include <physapi.h>
#include <dmgmodel.h>
#include <dmgbase.h>
#include <propag8r.h>
#include <mediag8r.h>

// For hand-to-hand
#include <weaphit.h>

// include this last to get guid for me instantiated
#include <dbmem.h>
#include <initguid.h>
#include <engfloop.h>

////////////////////////////////////////////////////////////
// BASH GAMESYS VARS
//

//
// TYPE
//
struct sBashVars
{
   float bash_threshold;
   float bash_coeff;
};


//
// SDESC
//

static sFieldDesc bash_fields[] =
{
   { "bash velocity threshold",  kFieldTypeFloat, FieldLocation(sBashVars,bash_threshold) },
   { "bash velocity coeff",  kFieldTypeFloat, FieldLocation(sBashVars,bash_coeff) },
};


static sStructDesc bash_sdesc = StructDescBuild(sBashVars,kStructFlagNone,bash_fields);

//
// VAR DESCRIPTOR
//


sFileVarDesc gBashVarDesc =
{
   kGameSysVar,         // Where do I get saved?
   "BASH",          // Tag file tag
   "Bash Vars",     // friendly name
   FILEVAR_TYPE(sBashVars),  // Type (for editing)
   { 1, 0},             // version
   { 1, 0},             // last valid version
};

//
// Defaults
//

static sBashVars def_vars =
{
   500.0,
   .01,
};

//
// Actually use the defaults when resetting
//

class cBashVars : public cFileVar<sBashVars,&gBashVarDesc>
{
   void Reset()
   {
      sBashVars& vars = *this;
      vars = def_vars;
   }
};

//
// The variable itself
//
static cBashVars gBashVars;

////////////////////////////////////////////////////////////
// GAME IMPACT LISTENER
//
// Here we generate a "bash" stim when an object gets bashed
//

static IStimSensors* gpSensors = NULL;
static IStimulator* gpStimulator = NULL;

static sPropertyDesc BashFactorPropDesc =
{
   BASH_FACTOR_PROP_NAME,
   0,
   NULL,
   0,
   0,
   { "Game", "Bash Factor" }
};

static sPropertyDesc BashParamsPropDesc =
{
   BASH_PARAMS_PROP_NAME,
   0,
   NULL,
   0,
   0,
   { "Game", "Bash Params" }
};

class cBashParamsOps : public cClassDataOps<sBashParamsProp>
{
public:
   STDMETHOD_(int,Version)() { return 1000; };
};

typedef cHashPropertyStore<cBashParamsOps> cBashParamsStore;
typedef cSpecificProperty<IBashParamsProperty, &IID_IBashParamsProperty, sBashParamsProp *, cBashParamsStore> cBashParamsPropertyBase;

class cBashParamsProperty : public cBashParamsPropertyBase
{
public:
   cBashParamsProperty(const sPropertyDesc *desc) : cBashParamsPropertyBase(desc) {};

   STANDARD_DESCRIBE_TYPE(sBashParamsProp);
};

static sFieldDesc BashParamsFields[] =
{
   { "Threshold",    kFieldTypeFloat,  FieldLocation(sBashParamsProp, threshold),  kFieldFlagNone },
   { "Coefficient",  kFieldTypeFloat,  FieldLocation(sBashParamsProp, coeff),      kFieldFlagNone },
};

static sStructDesc BashParamsDesc = StructDescBuild(sBashParamsProp, kStructFlagNone, BashParamsFields);

IFloatProperty      *g_pBashFactorProp = NULL;
IBashParamsProperty *g_pBashParamsProp = NULL;

static eDamageResult LGAPI on_impact(const sDamageMsg* msg, tDamageCallbackData )
{
   eDamageResult result = kDamageNoOpinion;
   ObjID from = msg->culprit;
   ObjID to   = msg->victim;

   // Check the weapon hit case.
   if (ResolveWeaponAttack(msg->culprit,msg->victim,msg->data.impact->magnitude,msg))
      return result;

   if ((to == PlayerObject()) && ((PhysGetObjMediaState(to) == kMS_Liquid_Submerged) ||
                                  (PhysGetObjMediaState(to) == kMS_Liquid_Wading)))
      return result;

   if ((from == PlayerObject()) && ((PhysGetObjMediaState(from) == kMS_Liquid_Submerged) ||
                                    (PhysGetObjMediaState(from) == kMS_Liquid_Wading)))
      return result;

   AutoAppIPtr_(ObjectSystem, gpObjSys);
   StimID stim = gpObjSys->GetObjectNamed("BashStim");
   StimSensorID sensid = gpSensors->LookupSensor(to,stim);

   // we have a sensor, so stimulate it
   if (sensid != SENSORID_NULL)
   {
      sStimEventData evdat = { stim, msg->data.impact->magnitude, 0, sensid, 0, GetSimTime() };
      sStimEvent event(&evdat,(sDamageMsg*)msg);

      float factor;
      sBashParamsProp *pBashParams;

      // Convert from velocity to damage
      event.intensity -= gBashVars.bash_threshold;
      event.intensity *= gBashVars.bash_coeff;

      // Factor in bashiness of culprit
      if (g_pBashFactorProp->Get(from, &factor))
         event.intensity *= factor;

      #ifndef SHIP
      if (config_is_defined("BashFullSpew"))
         mprintf("obj %d gets raw bash intensity of %g\n", to, event.intensity);
      #endif

      // Factor in receptiveness of victim
      if (g_pBashParamsProp->Get(to, &pBashParams))
         event.intensity = (event.intensity - pBashParams->threshold) * pBashParams->coeff;

      event.flags |= kStimEventNoDefer; // Don't defer collide events

      if (event.intensity > 0.0)
      {
         // Don't allow the player to bonk his head on terrain
         if ((to == PlayerObject()) && IsTextureObj(from))
         {
            sPhysClsnEvent *ev = (sPhysClsnEvent *)msg->Find(kEventKindCollision);
            if (ev)
            {
               cPhysClsn *pClsn = ev->collision;
               if (pClsn->GetSubModId() == PLAYER_HEAD)
                  return result;
            }
         }

         #ifndef SHIP
         if (config_is_defined("BashSpew"))
            mprintf("obj %d gets bashed for %g\n", to, event.intensity);
         #endif

         gpStimulator->StimulateSensor(sensid,&event);
      }
   }

   return result;
}

void BashPropsInit()
{
   AutoAppIPtr_(DamageModel,pDamage);

   Assert_(g_pBashFactorProp == NULL);
   g_pBashFactorProp = CreateFloatProperty(&BashFactorPropDesc, kPropertyImplVerySparse);

   Assert_(g_pBashParamsProp == NULL);
   g_pBashParamsProp = new cBashParamsProperty(&BashParamsPropDesc);

   StructDescRegister(&BashParamsDesc);
   StructDescRegister(&bash_sdesc);

   pDamage->Listen(kDamageMsgImpact,on_impact,NULL);

   gpSensors = AppGetObj(IStimSensors);
   gpStimulator = AppGetObj(IStimulator);
}

void BashPropsTerm()
{
   Assert_(g_pBashFactorProp != NULL);
   SafeRelease(g_pBashFactorProp);
   g_pBashFactorProp = NULL;

   Assert_(g_pBashParamsProp != NULL);
   SafeRelease(g_pBashParamsProp);
   g_pBashParamsProp = NULL;

   SafeRelease(gpSensors);
   SafeRelease(gpStimulator);
}

////////////////
// actual init/term

static IScriptMan * pScriptMan;
static IContainSys* pContainSys;

void init_sim_msg();

void EngineFeaturesInit(void)
{
   CreateScriptProp();
   HitPointsPropInit();
   TrapFlagsPropInit();
   ZBiasPropertyInit(); //zb
   BumpPropertyInit(); //zb
   SelfLitPropInit();
   ShadowPropInit();
   HeatDiskPropInit();
   SparkPropInit();
   MeshAttachPropInit();
   InitProjectiles();
   FrobInit();
   FrobPropsInit();
   RotDoorPropertyInit();
   TransDoorPropertyInit();
   SetupAllTweqs();
   InitWeapon();
   InitWeaponHaloSystem();
   EngineLockInit();
   InitPanelTools();
   ContrastInit();
   ScriptCommandsInit();
   PlaytestInit();
   init_sim_msg();
   invLimbInit();
   invRendInit();
   AnimTextureInit();
   PickInit();
   InitContainInheritProp();
   InitSuspiciousProp();
#ifdef EDITOR
   VbrushSaveLoadInit();
   AutoVBrushInit();
#endif
   QuestReactionsInit();
   FlowGroupPropInit();
   BashPropsInit();
#ifdef NEW_NETWORK_ENABLED
   TagsNetInit();
   SoundNetInit();
   AnimLightNetInit();
   NetSynchInit();
   NetSimInit();
#endif
   PreloadPropInit();

   InitDoorReports();

   // Create act/react propagators
   cAutoIPtr<IPropagator> cgator ( CreateContactPropagator());
   cAutoIPtr<IPropagator> rgator ( CreateRadiusPropagator());

   MediaPropagatorInit();

   pScriptMan = AppGetObj(IScriptMan);
   pScriptMan->GameInit();
   pContainSys = AppGetObj(IContainSys);
}

void EngineFeaturesTerm(void)
{
   ContrastTerm();
   EngineLockTerm();
   TermWeaponHaloSystem();
   TermWeapon();
   TerminateTweqs();
   TransDoorPropertyTerm();
   RotDoorPropertyTerm();
   TermPanelTools();
   FrobPropsTerm();
   FrobTerm();
   invLimbTerm();
   invRendTerm();
   PickTerm();
   QuestReactionsTerm();
   BashPropsTerm();
   FlowGroupPropTerm();
   MediaPropagatorTerm();
   TermContainInheritProp();
   TermSuspiciousProp();
#ifdef NEW_NETWORK_ENABLED
   TagsNetTerm();
   SoundNetTerm();
   AnimLightNetTerm();
   NetSynchTerm();
   NetSimTerm();
#endif
   PreloadPropTerm();

   TermDoorReports();

   pScriptMan->GameEnd();
   SafeRelease(pScriptMan);
   SafeRelease(pContainSys);
}

////////////////////
// Sim message handler
//

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimInit:
      case kSimResume:
         break;

      case kSimSuspend:
         break;
   }
}

static sDispatchListenerDesc sim_listen =
{
   &LOOPID_EngineFeatures, // my guid
   kSimInit|kSimResume|kSimSuspend,   // interests
   sim_msg,
};

static void init_sim_msg()
{
   AutoAppIPtr_(SimManager,pSimMan);
   pSimMan->Listen(&sim_listen);
}


////////////////
// per frame

DECLARE_TIMER(EF_ScriptMsgPump, Average);
DECLARE_TIMER(EF_RunAllTweqs, Average);
DECLARE_TIMER(EF_SelfLit, Average);
DECLARE_TIMER(EF_Shadow, Average);

void EngineFeaturesUpdateSimFrame(void)
{
   int ms=GetSimFrameTime();

   TIMER_Start(EF_ScriptMsgPump);
   pScriptMan->PumpMessages();
   TIMER_MarkStop(EF_ScriptMsgPump);

   TIMER_Start(EF_RunAllTweqs);
   RunAllTweqs(ms);
   TIMER_MarkStop(EF_RunAllTweqs);

   TIMER_Start(EF_SelfLit);
   SelfLitUpdateAll();
   TIMER_MarkStop(EF_SelfLit);

   TIMER_Start(EF_Shadow);
   ShadowUpdateAll();
   TIMER_MarkStop(EF_Shadow);
}

void EngineFeaturesUpdateRendFrame(void)
{
}


#ifdef MAYBE_LATER
// assumes the current canvas is the 3d draw canvas
void EngineFeaturesPostRender(void)
{
   mprintf("woo woo %x by %x\n",grd_canvas->bm.w,grd_canvas->bm.h);
}
#endif

////////////////
// loop client horror, so we get db messages, so on

static ulong our_factory_id;

static void db_message(DispatchData* msg)
{
   msgDatabaseData info;
   info.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         break;
      case kDatabaseLoad:
         // @TODO: we need a per-object script persistence api so we can do sub-partitioning.
         if (msg->subtype & kDBMission)
            ScriptStateLoad(info.load);
         break;
      case kDatabasePostLoad:
         PostLoadLockBuild();
         UnblockVisibilityForAllOpen();
         if (msg->subtype & kDBMission)
            pScriptMan->PostLoad();
         if (PlayerObjectExists())
         {
#ifdef NEW_NETWORK_ENABLED
            // The horror, the horror. Because we're not networking at
            // this point, we need to unequip all of the avatars as well
            // as the player, so that they stay in synch. Icky...
            AutoAppIPtr(NetManager);
            ObjID player;
            FOR_ALL_PLAYERS(pNetManager, &player)
            {
               UnSetWeapon(player);
            }
#else
            UnSetWeapon(PlayerObject());
#endif
         }
         if (!(msg->subtype & kFiletypeAll)) // the real post-post load
            PreloadObjects();
         break;
      case kDatabaseSave:
         if (msg->subtype & kDBMission)  // @TODO: see above partitioning comment
            ScriptStateSave(info.save);
         break;
   }

   pContainSys->DatabaseMessage(msg->subtype,(IUnknown*)info.load);

}

static eLoopMessageResult LGAPI EngFeatLoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{  // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   LoopMsg info;

   info.raw = hdata;
   switch (msg)
   {
      case kMsgAppInit:
         EngineFeaturesInit();
         break;
      case kMsgAppTerm:
         {
            EngineFeaturesTerm();
            AutoAppIPtr(LoopManager);
            pLoopManager->RemoveClientFactory(our_factory_id);
         }
         break;
      case kMsgResumeMode:
      case kMsgEnterMode:
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;
      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimGameSpec))
            EngineFeaturesUpdateSimFrame();
         PlaytestFrame();
         break;
   }
   return result;
}

static eLoopMessageResult LGAPI EngFeatRendFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{  // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   LoopMsg info;

   info.raw = hdata;
   switch (msg)
   {
      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimRender))
            EngineFeaturesUpdateRendFrame();
         break;

      case kMsgEndFrame:
         if (SimStateCheckFlags(kSimRender))
         {
            // @TBD (toml 02-04-98): we should probably store this pointer somewhere
            AutoAppIPtr(DisplayDevice);

            pDisplayDevice->Lock();
            ContrastUpdate();
            pDisplayDevice->Unlock();
         }

         break;

   }
   return result;
}

extern sLoopClientDesc EngFeatLoopDesc;
extern sLoopClientDesc EngFeatRendDesc;

static ILoopClient* LGAPI CreateEngFeatClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return CreateSimpleLoopClient(EngFeatLoopFunc,NULL,&EngFeatLoopDesc);
}

static ILoopClient* LGAPI CreateEngFeatRendClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return CreateSimpleLoopClient(EngFeatRendFunc,NULL,&EngFeatRendDesc);
}

sLoopClientDesc EngFeatLoopDesc =
{
   &LOOPID_EngineFeatures,
   "Engine Features",
   kPriorityNormal,
   kMsgDatabase|kMsgsAppOuter|kMsgEnd|kMsgsFrameMid|kMsgsMode,

   kLCF_Callback,
   CreateEngFeatClient,

   NO_LC_DATA,

   {
      {kConstrainBefore,   &LOOPID_SimFinish,   kMsgsFrameMid},
      {kConstrainBefore,   &LOOPID_AI,          kMsgsAppOuter},
      {kConstrainAfter,    &LOOPID_ObjSys,      kMsgsAppOuter},

      {kConstrainAfter,    &LOOPID_ObjSys,      kMsgDatabase},
      {kNullConstraint},
   }
};

// well, db wants to be before on reset/save, after on load
// rest, im not sure
//   {kConstrainBefore,&LOOPID_ObjSys,kMsgDatabase},

sLoopClientDesc EngFeatRendDesc =
{
   &LOOPID_EngineFeaturesRend,
   "Engine Render Features",
   kPriorityNormal,
   kMsgsFrameMid|kMsgsFrameEdge,

   kLCF_Callback,
   CreateEngFeatRendClient,

   NO_LC_DATA,

   {
      {kConstrainAfter,&LOOPID_SimFinish,kMsgsFrameMid},
      {kConstrainAfter,&LOOPID_Render,kMsgsFrameMid},
      {kConstrainAfter,&LOOPID_ScrnMan,kMsgsFrameEdge },
      {kNullConstraint},
   }
};


static const sLoopClientDesc *our_array[]={&EngFeatLoopDesc,&EngFeatRendDesc,NULL};

static tLoopClientID* EngineFeaturesClientIDs[] =
{
   &LOOPID_EngineFeatures,
   &LOOPID_EngineFeaturesRend,
};

sLoopModeDesc EngineFeaturesClients =
{
   { &LOOPID_EngineFeaturesClients, "Dark Base mode clients"},
   EngineFeaturesClientIDs,
   sizeof(EngineFeaturesClientIDs)/sizeof(EngineFeaturesClientIDs[0])
};

void EngineFeaturesCreate(void)
{
   ILoopClientFactory *factory=CreateLoopFactory(our_array);
   AutoAppIPtr(LoopManager);
   pLoopManager->AddClientFactory(factory,&our_factory_id);
   pLoopManager->AddMode(&EngineFeaturesClients);
   SafeRelease(factory)
}
