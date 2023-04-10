/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plycbllm.cpp,v 1.55 1999/05/15 10:32:31 JON Exp $

#include <plyrbase.h>
#include <plyablty.h>
#include <plycbllm.h>
#include <mclntapi.h>
#include <motschem.h>
#include <mschbase.h>
#include <playrobj.h>
#include <objtype.h>
#include <objsys.h>
#include <objpos.h>
#include <osysbase.h>
#include <propbase.h>
#include <mnamprop.h>
#include <iobjsys.h>
#include <cretprop.h>
#include <creatapi.h>
#include <ctagset.h>
#include <camera.h>
#include <command.h>
#include <matrixs.h>
#include <fix.h>
#include <rendprop.h>
#include <dmgmodel.h>

#include <dbmem.h> // must be last header

typedef class cPlayerCerebellum
{
public:
   cPlayerCerebellum();
   ~cPlayerCerebellum();

   void StartAction(tPlayerActionType actType = kPlayerActionTypeInvalid);
   void FinishAction();
   void AbortAction();

   void MakeBody();
   void DestroyBody();
   ObjID GetObjID() { return m_RightArmObj; }

   void Update(ulong dt);

   void SetupManeuver(int skillID);
   void SetupMode(int mode);
   void FinishMode();

   cPlayerAbility *GetCurrentAbility() { return m_pCurAbility; }

private:
   void SetInput(ePlayerInput input, tPlayerActionType actType=kPlayerActionTypeInvalid) { m_Input=input; m_ActionType=actType; }

   int GetConnectSkill(int skillNum, ePlayerInput input, tPlayerActionType actType=kPlayerActionTypeInvalid);
   int GetNextSkill();
   void UpdateNextSkill();

   ePlayerTransition GetTransitionType() { return m_pCurAbility?(m_pCurAbility->GetTransitionType(m_CurSkill)):(kTrans_Invalid); }

   IMotionCoordinator *m_pMCoord;
   ObjID m_RightArmObj;
   int m_CurSkill;
   int m_NextSkill;
   ePlayerInput m_Input;
   tPlayerActionType m_ActionType;

   cPlayerAbility *m_pCurAbility;
} cPlayerCerebellum;

// callbacks
static fPlayerFrameCallback g_fFrameCallback=NULL;
static fPlayerAbilityCallback g_fAbilityCallback=NULL;
static fPlayerModeChangeCallback g_fEnterModeCallback=NULL;
static fPlayerModeChangeCallback g_fLeaveModeCallback=NULL;
static fPlayerIdleCallback g_fIdleCallback=NULL;


static cPlayerCerebellum *g_pPlayerCerebellum = NULL;

static int g_NextPlayerMode=kPlayerModeInvalid;
static int g_CurrentPlayerMode=kPlayerModeInvalid;

////////////////////////////////////////////////////////////////
///
///   Property Listeners
///
////////////////////////////////////////////////////////////////

void LGAPI PlayerCreatureListener(sPropertyListenMsg* msg, PropListenerData data)
{
   if(!g_pPlayerCerebellum||g_pPlayerCerebellum->GetObjID()!=msg->obj)
      return;
   if(msg->type&kListenPropUnset)
   {
      g_pPlayerCerebellum->DestroyBody();
   }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/////
///// Player Cerebellum Implementation
/////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void cPlayerCerebellum::DestroyBody()
{
   if(m_pMCoord)
      m_pMCoord->SetMotor(NULL);
   m_CurSkill=kSkillInvalid;

   if(m_RightArmObj!=OBJ_NULL)
   {
      ObjID oldObj=m_RightArmObj;

      m_RightArmObj=OBJ_NULL;
      AutoAppIPtr(DamageModel);
      pDamageModel->SlayObject(oldObj, OBJ_NULL);
   }
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::MakeBody()
{
   IObjectSystem* objsys;
   ObjID obj;
   mxs_vector pos;
   mxs_angvec facing;
   IMotor *pMotor;
   const char* name = "PlyrArm";
   int ctype;

   DestroyBody(); // in case it already exists

   if(!m_pCurAbility)
      return;

   objsys = AppGetObj(IObjectSystem);

   obj=m_RightArmObj=objsys->BeginCreate(ROOT_ARCHETYPE,kObjectConcrete);

   // first position at origin, so virtual motion will work.. XXX this is lame KJ 12/97
   mx_zero_vec(&pos);
   facing.tx=facing.ty=facing.tz=0;
   ObjPosUpdate(obj,&pos,&facing);

   objsys->NameObject(obj,name);
   objsys->SetObjTransience(obj,TRUE); 
   objsys->EndCreate(obj);

   SafeRelease(objsys);

   Label *modname;

   if(m_pCurAbility->GetModelName(&modname))
      ObjSetModelName(GetObjID(),modname->text);

   m_pCurAbility->GetCreatureType(&ctype);
   ObjSetCreatureType(obj,ctype);

   ObjSetHasRefs(obj,FALSE); // don't want it to get sorted in scene

   if(NULL!=(pMotor=CreatureGetMotorInterface(obj)))
   {
      m_pMCoord->SetMotor(pMotor);
      pMotor->SetMotFrameOfReference(MFRT_ARM);  // was kMFRT_Global - MTR 8/31
   }
}

////////////////////////////////////////////////////////////////


cPlayerCerebellum::cPlayerCerebellum()
{  
   m_pMCoord=MotSysCreateMotionCoordinator(); 
   m_RightArmObj=OBJ_NULL; 
   m_CurSkill=kSkillInvalid;
   m_NextSkill=kSkillInvalid;
   m_Input=kPlyrInput_None;
   m_pCurAbility=NULL;
}

////////////////////////////////////////////////////////////////

cPlayerCerebellum::~cPlayerCerebellum()
{ 
   if(m_pMCoord)
   {
      delete m_pMCoord;
      m_pMCoord=NULL;
   }   
   if(m_RightArmObj!=OBJ_NULL) 
      DestroyBody(); 

   // for halting sound schemas etc..
   if(m_pCurAbility)
   {
      m_pCurAbility->End();
      m_pCurAbility=NULL;
   }
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::SetupManeuver(int skillID)
{
   sMcMoveParams param;
   IMotionPlan *pPlan;

   // clear input
   m_Input=kPlyrInput_None;

   // clean up old skill
   if(m_CurSkill!=kSkillInvalid && m_pCurAbility)
   {
      m_pCurAbility->CleanupSkill(m_CurSkill);
   }

   if(skillID==kSkillInvalid)
   {
      m_pMCoord->SetCurrentManeuver(NULL);
      m_CurSkill=kSkillInvalid;
      return;
   }

   param.tags.Clear();
   param.mask=0;
   if(m_pCurAbility)
   {
      m_pCurAbility->SetupSkill(skillID);
      m_pCurAbility->GetMotionTags(skillID,&param.tags);
   }
   if(NULL!=(pPlan=m_pMCoord->BuildPlan(NULL,&param)))
   {
      m_pMCoord->SetCurrentManeuver(pPlan->PopFirstManeuver());
      delete pPlan;
   }
   m_CurSkill=skillID;
   m_NextSkill=kSkillInvalid; // will need to be re-computed from current skill
   return;
}

////////////////////////////////////////////////////////////////

int cPlayerCerebellum::GetConnectSkill(int skillNum, ePlayerInput input, tPlayerActionType actType)
{
   if(skillNum==kSkillInvalid)
      return kSkillInvalid;

   if(m_pCurAbility)
   {
      return m_pCurAbility->GetNextSkillID(skillNum,input,actType);
   }

   return kSkillInvalid;
}

////////////////////////////////////////////////////////////////

int cPlayerCerebellum::GetNextSkill()
{
   if(m_CurSkill==kSkillInvalid)
      return kSkillInvalid;

   if(m_NextSkill!=kSkillInvalid)
      return m_NextSkill;

   return GetConnectSkill(m_CurSkill, m_Input, m_ActionType);
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::UpdateNextSkill()
{
   if(m_NextSkill)
      m_NextSkill=GetConnectSkill(m_NextSkill,m_Input,m_ActionType);
   else
      m_NextSkill=GetConnectSkill(m_CurSkill,m_Input,m_ActionType);
}


////////////////////////////////////////////////////////////////

void cPlayerCerebellum::FinishMode()
{
   SetInput(kPlyrInput_Abort);

   if(GetTransitionType()==kTrans_Immediate)
   {
      if(m_pMCoord)
         m_pMCoord->SetCurrentManeuver(NULL);
      UpdateNextSkill();
   }
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::StartAction(tPlayerActionType actionType)
{
   SetInput(kPlyrInput_Start, actionType);
   
   if(GetTransitionType()==kTrans_Immediate)
   {
      if(m_pMCoord)
         m_pMCoord->SetCurrentManeuver(NULL);
      UpdateNextSkill();
   }
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::FinishAction()
{
   SetInput(kPlyrInput_Finish);

   if(GetTransitionType()==kTrans_Immediate)
   {
      if(m_pMCoord)
         m_pMCoord->SetCurrentManeuver(NULL);
      UpdateNextSkill();
   }
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::AbortAction()
{
   SetInput(kPlyrInput_Abort);
   
   if(GetTransitionType()==kTrans_Immediate)
   {
      if(m_pMCoord)
         m_pMCoord->SetCurrentManeuver(NULL);
      UpdateNextSkill();
   }
}

////////////////////////////////////////////////////////////////

void cPlayerCerebellum::Update(ulong dt)
{
   int skillID;

   if(!m_pMCoord)
      return;

   // do the frame callback
   if(g_fFrameCallback)
   {
      // XXX this should look at return value of frame
      // callback and do something with it.
      (*g_fFrameCallback)(dt,(void *)g_CurrentPlayerMode);
   }
   if(m_pMCoord->GetStatus()==kMCoord_Idle)
   {
      if(g_NextPlayerMode!=kPlayerModeInvalid)
      {
         // keep going through abort maneuvers until none left, then you're ready
         // to switch.
         SetInput(kPlyrInput_Abort);
         if(kSkillInvalid==(skillID=GetNextSkill()))
         {
            // change modes
            SetupMode(g_NextPlayerMode);
            g_NextPlayerMode=kPlayerModeInvalid;
            return;
         }
      } else
      {
         skillID=GetNextSkill();
      }
      SetupManeuver(skillID);

      // Inform any callbacks if we're idling
      if(m_pCurAbility)
      {
         if(g_fIdleCallback&&m_pCurAbility->IsIdle(skillID))
         {
            g_fIdleCallback(g_CurrentPlayerMode);
         }
      }
   }
}


void cPlayerCerebellum::SetupMode(int mode)
{
   // leave current mode
   if(m_pCurAbility)
      m_pCurAbility->End();
   if(g_fLeaveModeCallback)
   {
      g_fLeaveModeCallback(g_CurrentPlayerMode);
   }
   SetupManeuver(kSkillInvalid);
   DestroyBody();
   g_CurrentPlayerMode=mode;


   // start new mode

   if(mode==kPlayerModeInvalid)
      return;

   // must get ability for mode before able to make body
   if(g_fAbilityCallback)
      g_fAbilityCallback(mode,&m_pCurAbility,NULL);

   MakeBody();

   // must make body before calling this callback, since some
   // things want to refer to it here (like sword handler).
   if(g_fEnterModeCallback)
   {
      // this sets the ability pointer for the player mode.
      // this pointer must remain valid while mode is current mode.
      g_fEnterModeCallback(mode);
   }

   if(m_pCurAbility)
   {
      m_pCurAbility->Begin();
      SetupManeuver(m_pCurAbility->GetStartSkillID());
   }
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/////
///// Global Functions  -- Control Inputs
/////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

void PlayerStartAction()
{
   g_pPlayerCerebellum->StartAction();
}

void PlayerStartActionOfType(tPlayerActionType type)
{
   g_pPlayerCerebellum->StartAction(type);
}

void PlayerFinishAction()
{
   g_pPlayerCerebellum->FinishAction();
}

void PlayerAbortAction()
{
   g_pPlayerCerebellum->AbortAction();
}

ObjID PlayerArm()
{
   if (g_pPlayerCerebellum)
      return g_pPlayerCerebellum->GetObjID();
   return OBJ_NULL; 
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/////
/////    Schema Utility Funcs
/////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

BOOL PlayerGetMotControllerID(int *pID)
{
   Assert_(g_pPlayerCerebellum);

   cPlayerAbility * pA=g_pPlayerCerebellum->GetCurrentAbility();

   if(!pA)
      return FALSE;

   return pA->GetCurControllerID(pID);
}

BOOL PlayerGetSkillData(sMPlayerSkillData *pSkillData)
{
   Assert_(g_pPlayerCerebellum);

   cPlayerAbility * pA=g_pPlayerCerebellum->GetCurrentAbility();
   sMPlayerSkillData *pD;

   if(!pA)
      return FALSE;

   if(NULL!=(pD=pA->GetCurSkillData()))
   {
      *pSkillData=*pD;
      return TRUE;
   }
   return FALSE;
}


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
/////
///// Global Functions  -- Cerebellum Setup
/////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

#ifndef SHIP
static void RegisterCerebellumCommands();
#endif


void PlayerCbllmInit()
{
#ifndef SHIP
   RegisterCerebellumCommands();
#endif
}

void PlayerCbllmSetFrameCallback(fPlayerFrameCallback callback)
{
   g_fFrameCallback=callback;
}

void PlayerCbllmSetEnterModeCallback(fPlayerModeChangeCallback callback)
{
   g_fEnterModeCallback=callback;
}

void PlayerCbllmSetAbilityCallback(fPlayerAbilityCallback callback)
{
   g_fAbilityCallback=callback;
}

void PlayerCbllmSetLeaveModeCallback(fPlayerModeChangeCallback callback)
{
   g_fLeaveModeCallback=callback;
}

void PlayerCbllmSetIdleCallback(fPlayerIdleCallback callback)
{
   g_fIdleCallback=callback;
}

void PlayerCbllmCreate()
{
   g_pPlayerCerebellum = new cPlayerCerebellum;

   AssertMsg(g_pPlayerCerebellum,"Could not create player cerebellum\n");
}

void PlayerCbllmDestroy()
{
   PlayerSwitchMode(kPlayerModeInvalid); // so handler knows we're finished
   if(g_pPlayerCerebellum)
   {
      delete g_pPlayerCerebellum;
      g_pPlayerCerebellum=NULL;
   }
}

void PlayerCbllmUpdate(ulong dt)
{
   if(g_pPlayerCerebellum)
   {
      g_pPlayerCerebellum->Update(dt);
   }
}

// @TODO: take power out of plycbllm
BOOL PlayerCbllmGetPower(float *pPower)
{
   *pPower=1.0;
   return TRUE;
}

void PlayerSwitchMode(int mode)
{
   // do nothing if same as current mode, except perhaps switch bow model..
   if(mode==g_CurrentPlayerMode&&g_NextPlayerMode==kPlayerModeInvalid)
   {
      return;
   }
   if(mode==kPlayerModeInvalid) // must switch NOW
   {
      // change modes
      g_NextPlayerMode=mode;
      if(g_pPlayerCerebellum)
         g_pPlayerCerebellum->SetupMode(mode);
   } else
   {
      if(g_NextPlayerMode==kPlayerModeInvalid) // so not already finishing
      {
         g_pPlayerCerebellum->FinishMode();
      }
      g_NextPlayerMode=mode;
   }
}

int PlayerGetMode()
{
   return g_CurrentPlayerMode;
}

int PlayerGetNextMode()
{
   return g_NextPlayerMode;
}

////////////////////////////////////////////////////
//
// PLAYER ARM ALIGNMENT UTILITIES
//
// for debugging/tweaking purposes
// @TODO: reimplement these
#ifndef SHIP

EXTERN BOOL g_PlayerArmAttached;

Command PlayerCerebellumCommands[] =
{
   { "player_detach_arm", TOGGLE_BOOL,&g_PlayerArmAttached, "toggle whether arm attached to player camera"},
   { "player_spew_arm_offset", FUNC_VOID, PlayerCbllmSpewArmOffset, "spew arm offset from camera to monochrome" },
};

void RegisterCerebellumCommands()
{
   COMMANDS(PlayerCerebellumCommands,HK_GAME_MODE);
}

static BOOL CalcArmOffset(mxs_vector *pArmRelPos, mxs_angvec *pArmRelAng)
{
   Position *pArmPos;
   mxs_vector camPos;
   mxs_matrix mat,armRelOrient,armWorldOrient;
   mxs_angvec camAng;
   ObjID armObj;

   if(!g_pPlayerCerebellum)
      return FALSE;
   if(OBJ_NULL==(armObj=g_pPlayerCerebellum->GetObjID() ) )
      return FALSE;
   if(NULL==(pArmPos=ObjPosGet(armObj)))
      return FALSE;
   CameraGetLocation(PlayerCamera(),&camPos,&camAng);
   mx_sub_vec(pArmRelPos,&pArmPos->loc.vec,&camPos);
   mx_ang2mat(&mat,&camAng);
   mx_transeq_mat(&mat); // inverse in conjugate transpose for unitary matrices
   mx_mat_muleq_vec(&mat,pArmRelPos); // offset of arm in camera space

   mx_ang2mat(&armWorldOrient,&pArmPos->fac);
   mx_mul_mat(&armRelOrient,&mat,&armWorldOrient); // arm orient in camera space
   mx_mat2ang(pArmRelAng,&armRelOrient);

   return TRUE;
}


void PlayerCbllmSpewArmOffset()
{
   mxs_vector armRelPos;
   mxs_angvec armRelAng;

   if(!CalcArmOffset(&armRelPos,&armRelAng))
      return;
   mprintf("\n\n");
   mprintf("arm position: %g, %g, %g\n",armRelPos.x,armRelPos.y,armRelPos.z);   
   mprintf("arm orientation: %d, %d, %d\n",fixang_to_degrees(armRelAng.tx),\
   fixang_to_degrees(armRelAng.ty), fixang_to_degrees(armRelAng.tz) );
}

#endif // ifndef SHIP


