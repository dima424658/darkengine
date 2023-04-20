/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/plyrmode.cpp,v 1.24 2000/03/09 22:57:22 adurant Exp $
//
// Player mode system
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <port.h>
#include <str.h>
#include <config.h>
#include <tagfile.h>
#include <simtime.h>
#include <rand.h>

#include <flowarch.h>
#include <playrobj.h>
#include <plyrmov.h>
#include <objmedia.h>
#include <schema.h>
#include <esnd.h>
#include <propman.h>
#include <plyrspd.h>
#include <camera.h>
#include <headmove.h>
#include <doorphys.h>  // @HACK: for door sound blocking

#include <physapi.h>
#include <phmods.h>
#include <phmod.h>
#include <phcore.h>

#include <plyrmode.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

#define KEYBOARD_TURNING_SIMULATES_MOUSELOOK

////////////////////////////////////////////////////////////////////////////////

cPlayerMode *g_pPlayerMode = NULL;

sModeMotions g_ModeMotions[kPM_NumModes] = 
{
   { kMoNormal,   { kMoStrideLeft, kMoStrideRight } },
   { kMoCrouch,   { kMoCrawlLeft, kMoCrawlRight } },
   { kMoNormal,   { kMoNormal, kMoNormal } },
   { kMoNormal,   { kMoNormal, kMoNormal } },
   { kMoWithBody, { kMoWithBodyLeft, kMoWithBodyRight } },
   { kMoNormal,   { kMoNormal, kMoNormal } },
   { kMoNormal,   { kMoNormal, kMoNormal } },
   { kMoNormal,   { kMoNormal, kMoNormal } },
};

sModeSpeedScale g_ModeBaseSpeeds[kPM_NumModes] = 
{
   { 1.0, 1.0 },  // stand
   { 0.6, 0.6 },  // crouch
   { 0.7, 0.7 },  // swim
   { 0.5, 0.7 },  // climb
   { 1.0, 1.0 },  // body carry (scale values set from script)
   { 1.0, 1.0 },  // slide
   { 1.0, 1.0 },  // jump
   { 0.0, 0.0 },  // dead
};

////////////////////////////////////////////////////////////////////////////////

void PlayerModeInit()
{
   Assert_(g_pPlayerMode == NULL);

   g_pPlayerMode = new cPlayerMode;
   headmoveKeyboardReset();
   headmoveSetRelPosX(0);
   headmoveSetRelPosY(0);
}

void PlayerModeTerm()
{
   Assert_(g_pPlayerMode != NULL);

   delete g_pPlayerMode;
   g_pPlayerMode = NULL;
}

void PlayerModeReset()
{
   if (g_pPlayerMode)
      PlayerModeTerm();
   PlayerModeInit();
}

void PlayerModeUpdate()
{
   if (g_pPlayerMode)
      g_pPlayerMode->Update();
}

////////////////////////////////////////////////////////////////////////////////

ePlayerMode GetPlayerMode()
{
   return g_pPlayerMode->GetMode();
}

void SetPlayerMode(ePlayerMode mode)
{
   g_pPlayerMode->SetMode(mode);
}

////////////////////////////////////////////////////////////////////////////////

void AddSpeedScale(const char *name, float trans_scale, float rot_scale)
{
   g_pPlayerMode->AddSpeedScale(name, trans_scale, rot_scale);
}

void RemoveSpeedScale(const char *name)
{
   g_pPlayerMode->RemoveSpeedScale(name);
}

float GetTransSpeedScale()
{
   return g_pPlayerMode->GetTransSpeedScale();
}

float GetRotSpeedScale()
{
   return g_pPlayerMode->GetRotSpeedScale();
}

////////////////////////////////////////////////////////////////////////////////

void UpdatePlayerSpeed()
{
   g_pPlayerMode->UpdatePlayerSpeed();
}

////////////////////////////////////////////////////////////////////////////////

void SetJumpState(BOOL state)
{
   g_pPlayerMode->SetJumpState(state);
}

BOOL IsJumping()
{
   return g_pPlayerMode->IsJumping();
}

////////////////////////////////////////////////////////////////////////////////

//This is so wrong. AMSD

void SetSpeedToggleState(BOOL state)
{
   g_pPlayerMode->SetSpeedToggleState(state);
}

BOOL IsCreeping()
{
   return g_pPlayerMode->IsCreeping();
}

BOOL IsRunning()
{
   return g_pPlayerMode->IsRunning();
}

void SetSpeedToggleMeansFaster(BOOL faster)
{
   g_pPlayerMode->SetSpeedToggleMeansFaster(faster);
}

BOOL SpeedToggleMeansFaster()
{
   return g_pPlayerMode->SpeedToggleMeansFaster();
}

////////////////////////////////////////////////////////////////////////////////

void SetLeaningState(BOOL state)
{
   g_pPlayerMode->SetLeaningState(state);
}

BOOL IsLeaning() 
{
   return g_pPlayerMode->IsLeaning();
}
////////////////////////////////////////////////////////////////////////////////

void SetForceSidestepState(BOOL state)
{
   g_pPlayerMode->SetForceSidestepState(state);
}

BOOL GetForceSidestepState()
{
   return g_pPlayerMode->GetForceSidestepState();
}

////////////////////////////////////////////////////////////////////////////////

void SetForwardState(float state)
{
   g_pPlayerMode->SetForwardState(state);
}

void SetSidestepState(float state)
{
   g_pPlayerMode->SetSidestepState(state);
}

void SetTurningState(float state)
{       
   g_pPlayerMode->SetTurningState(state);
}

////////////////////////////////////////////////////////////////////////////////

void PlayerModeStartFadeIn(tSimTime time, uchar r, uchar g, uchar b)
{
   g_pPlayerMode->StartFadeIn(time, r, g, b);
}

void PlayerModeStartFadeOut(tSimTime time, uchar r, uchar g, uchar b)
{
   g_pPlayerMode->StartFadeOut(time, r, g, b);
}

BOOL PlayerModeGetFade(uchar* pR, uchar* pG, uchar* pB, float *pAlpha)
{
   return g_pPlayerMode->GetFade(pR, pG, pB, pAlpha);
}

////////////////////////////////////////////////////////////////////////////////

cPlayerMode::cPlayerMode()
{
   m_curMode = kPM_Stand;
   m_speedScale.SetSize(0);

   m_jumpState = FALSE;
   m_speedtoggleState = FALSE;
   m_speedtogglemeansFaster = FALSE;

   m_forceSidestepState = 0;

   m_forwardState = 0.0;
   m_sidestepState = 0.0;
   m_turningState = 0.0;

   m_timeOfDeath = 0;
   m_timeOfFadeStart = 0;
   m_fadeRate = 0;

   m_onDeathCallbackList.SetSize(0);
}

cPlayerMode::~cPlayerMode()
{
   m_speedScale.SetSize(0);
   m_onDeathCallbackList.SetSize(0);
}

////////////////////////////////////////

// @HACK: we really want to specified whether speed modifiers are transient
static const char *NullifyOnLoadList[] = 
{
   "PlayerAbility",
   "SwordEquip",
   "BowDraw",
};

void cPlayerMode::Read(ITagFile *file)
{
   file->Read((char *)&m_curMode, sizeof(ePlayerMode));

   SetMode(m_curMode);

   int size;
   file->Read((char *)&size, sizeof(int));
   
   m_speedScale.SetSize(size);
   for (int i=0; i<size; i++)
      file->Read((char *)&m_speedScale[i], sizeof(sScaleElem));

   // @HACK: nullify  any items on the zero speed list
   int i, j;
   int num = sizeof(NullifyOnLoadList) / sizeof(NullifyOnLoadList[0]);

   for (i=0; i<num; i++)
   {
      for (j=0; j<m_speedScale.Size(); j++)
      {
         if (!strcmp(NullifyOnLoadList[i], m_speedScale[j].name))
         {
            m_speedScale[j].scales.transSpeedScale = 1.0;
            m_speedScale[j].scales.rotSpeedScale = 1.0;
         }
      }
   }
}
 
void cPlayerMode::Write(ITagFile *file)
{
   file->Write((char *)&m_curMode, sizeof(ePlayerMode));

   int size;
   size = m_speedScale.Size();
   
   file->Write((char *)&size, sizeof(int));
   for (int i=0; i<size; i++)
      file->Write((char *)&m_speedScale[i], sizeof(sScaleElem));
}

////////////////////////////////////////

void cPlayerMode::SetMode(ePlayerMode mode)
{
   Assert_((mode >= 0) && (mode < kPM_NumModes));

   #ifndef SHIP
   if (config_is_defined("PlayerModeSpew"))
      mprintf("Setting mode to %d\n", mode);
   #endif

   PlayerMotionSetRest(g_ModeMotions[mode].atRest);
   ActivateRestMotion();

   BOOL mode_changed = (m_curMode != mode);

   m_curMode = mode;

   if (mode_changed)
      UpdatePlayerSpeed();

   if (m_curMode == kPM_Dead)
      DoDeath();
}

ePlayerMode cPlayerMode::GetMode() const
{
   return m_curMode;
}

////////////////////////////////////////

void cPlayerMode::Update()
{
   switch (m_curMode)
   {
      case kPM_Dead:
      {
         // @TODO: fade to black?
         // well, actually this is query-driven instead of update-driven
         // see GetFadeAmount
         break;
      }
   }
}

////////////////////////////////////////

BOOL cPlayerMode::GetFade(uchar* pR, uchar* pG, uchar* pB, float *pAlpha)
{
   if (m_fadeRate == 0) 
   {
      //mprintf("get fade FALSE, rate is %g\n",m_fadeRate);
      return FALSE;
   }
   if (m_fadeRate>0)
      *pAlpha = (GetSimTime() - m_timeOfFadeStart)*m_fadeRate;
   else
      *pAlpha = 1+(GetSimTime() - m_timeOfFadeStart)*m_fadeRate;
   if (*pAlpha > 0)
   {
      if (*pAlpha > 1) 
         *pAlpha = 1;
   }
   else
   {
      // alpha = 0 -> its all over
      //mprintf("get fade FALSE because alpha is %g  = (%d - %d) * %g\n",*pAlpha,GetSimTime(),m_timeOfFadeStart,m_fadeRate);
      m_fadeRate = 0;
      return FALSE;
   }
   *pR = m_fadeR;
   *pG = m_fadeG;
   *pB = m_fadeB;
   //mprintf("yay fade is true: %d, %d, %d\n",*pR,*pG,*pB);
   return TRUE;
}

void cPlayerMode::StartFadeIn(tSimTime time, uchar r, uchar g, uchar b)
{
   if (time) 
   {
      m_fadeRate = -1. / float(time);
      m_fadeR = r;
      m_fadeG = g;
      m_fadeB = b;
      // the -1 is so that we don't find up thinking we are done instantly
      m_timeOfFadeStart = GetSimTime() - 1;
   }
}

void cPlayerMode::StartFadeOut(tSimTime time, uchar r, uchar g, uchar b)
{
   if (time) 
   {
      m_fadeRate = 1. / float(time);
      m_fadeR = r;
      m_fadeG = g;
      m_fadeB = b;
      // the -1 is so that we don't find up thinking we are done instantly
      m_timeOfFadeStart = GetSimTime() - 1;
   }
}

////////////////////////////////////////

void cPlayerMode::ActivateStrideMotion(BOOL left)
{
   if (!IsLeaning())
      PlayerMotionActivate(g_ModeMotions[m_curMode].stride[left]);

   if (m_curMode == kPM_Climb)
   {
      cPhysModel *pClimbModel = g_PhysModels.Get(PhysGetClimbingObj(PlayerObject()));

      if (pClimbModel && pClimbModel->IsRope())
      {
         mxs_vector player_vel;

         PhysGetVelocity(PlayerObject(), &player_vel);

         PhysHitRope(pClimbModel->GetObjID(), mx_mag_vec(&player_vel));
      }
   }
}

void cPlayerMode::ActivateLandingMotion()
{
   if ((m_curMode == kPM_Stand) || (m_curMode == kPM_Jump))
      PlayerMotionActivate(kMoJumpLand);
}

void cPlayerMode::ActivateRestMotion()
{
   if (!IsLeaning())
      PlayerMotionActivate(PlayerMotionGetRest());
}

////////////////////////////////////////

void cPlayerMode::DoStrideSound(int play_vol, const mxs_vector &foot_loc)
{
   sSchemaCallParams schema_params;

   // We don't network player strides, because they chew too much
   // bandwidth and may not be in synch with the avatar anyway:
   schema_params.flags = SCH_ADD_VOLUME | SCH_NO_NETWORK;
   schema_params.volume = play_vol;

   switch (m_curMode)
   {
      case kPM_Stand:
      case kPM_Crouch:
      {
         eMediaState media_state = PhysGetObjMediaState(PlayerObject());
   
         if ((media_state == kMS_Air) && PhysObjOnGround(PlayerObject()))
         {
            cTagSet Event("Event Footstep");
            ESndPlayLoc(&Event, PlayerObject(), g_pPlayerMovement->GetGroundObj(), &foot_loc, &schema_params);
         }
         else
         if (media_state == kMS_Liquid_Standing)
         {
            ObjID flow_arch = GetFlowArch(PLAYER_FOOT);

            cTagSet Event("Event Footstep");
            Event.Add(cTag("MediaLevel", "Foot"));
            ESndPlayLoc(&Event, PlayerObject(), flow_arch, &foot_loc, &schema_params);
         }
         
         break;
      }

      case kPM_Swim:
      {
         eMediaState media_state = PhysGetObjMediaState(PlayerObject());
   
         if (media_state == kMS_Liquid_Wading)
         {
            ObjID flow_arch = GetFlowArch(PLAYER_BODY);

            mxs_vector body_loc;
            PhysGetSubModLocation(PlayerObject(), PLAYER_BODY, &body_loc);

            cTagSet Event("Event Footstep");
            Event.Add(cTag("MediaLevel", "Body"));
            ESndPlayLoc(&Event, PlayerObject(), flow_arch, &body_loc, &schema_params);
         }
         else
         if (media_state == kMS_Liquid_Submerged)
         {
            ObjID flow_arch = GetFlowArch(PLAYER_HEAD);
         
            mxs_vector head_loc;
            PhysGetSubModLocation(PlayerObject(), PLAYER_HEAD, &head_loc);

            cTagSet Event("Event Footstep");
            Event.Add(cTag("MediaLevel", "Head"));
            ESndPlayLoc(&Event, PlayerObject(), flow_arch, &head_loc, &schema_params);
         }

         break;
      }

      case kPM_Climb:
      {
         ObjID climbing_obj = PhysGetClimbingObj(PlayerObject());

         cTagSet Event("Event Climbstep");
         ESndPlayLoc(&Event, PlayerObject(), climbing_obj, &ObjPosGet(PlayerObject())->loc.vec, &schema_params);

         break;
      }

      case kPM_BodyCarry:
      {
         // @TODO: body-carry-specific footsteps?

         break;
      }

      case kPM_Slide:
      {
         // @TODO: sliding footsteps?

         break;
      }
   }
}

void cPlayerMode::DoLandingSound(int play_vol, const mxs_vector &foot_loc)
{
   sSchemaCallParams schema_params;

   schema_params.flags = SCH_ADD_VOLUME;
   schema_params.volume = play_vol;

   // What media are we landing on?
   eMediaState media_state = PhysGetObjMediaState(PlayerObject());

   if (media_state == kMS_Liquid_Standing)
   {
      // Landing on water
      ObjID flow_arch = GetFlowArch(PLAYER_FOOT);

      cTagSet Event("Event Footstep, Landing true");
      ESndPlayLoc(&Event, PlayerObject(), flow_arch, &foot_loc, &schema_params);
   }
   else
   if (media_state == kMS_Air)
   {
      // Landing on ground
      cTagSet Event("Event Footstep, Landing true");
      ESndPlayLoc(&Event, PlayerObject(), g_pPlayerMovement->GetGroundObj(), &foot_loc, &schema_params);
   }

   // @TBD: landing in other media?
}

////////////////////////////////////////

void cPlayerMode::InstallOnDeathCallback(fOnDeathCallback callback)
{
   // scan for duplicates
   for (int i=0; i<m_onDeathCallbackList.Size(); i++)
   {
      if (m_onDeathCallbackList[i] == callback)
         return;
   }

   m_onDeathCallbackList.Append(callback);
}

void cPlayerMode::DoDeath()
{
   int i;
   
   for (i=0; i<m_onDeathCallbackList.Size(); i++)
      m_onDeathCallbackList[i]();

   cPhysModel *pModel;

   if ((pModel = g_PhysModels.GetActive(PlayerObject())) == NULL)
      return;

   mxs_vector head_loc = pModel->GetLocationVec(PLAYER_HEAD);
   mxs_vector zero;

   // snap all physics models to head, and let fall
   mx_zero_vec(&zero);
   for (i=0; i<pModel->NumSubModels(); i++)
   {
      pModel->SetSubModOffset(i, zero);
      pModel->SetLocationVec(i, head_loc);
   }   

   pModel->SetLocationVec(head_loc);

   mxs_vector rotation;

   mx_mk_vec(&rotation, ((float)(Rand() % 1024) / 1024.0) * 3,
                        ((float)(Rand() % 1024) / 1024.0) * 3,
                        ((float)(Rand() % 1024) / 1024.0) * 0.2);

   pModel->GetDynamics(PLAYER_HEAD)->SetRotationalVelocity(rotation);

   headmoveSetRelPosX(0);
   headmoveSetRelPosY(0);

   m_timeOfDeath = GetSimTime();
}

////////////////////////////////////////

#define NET_ROTATE_SPEED      (ROTATE_SPEED / PlayerCamera()->zoom)
#define NET_SLOW_ROTATE_SPEED (SLOW_ROTATE_SPEED / PlayerCamera()->zoom)

void cPlayerMode::UpdatePlayerSpeed()
{
   cPhysModel    *pModel;

   if ((pModel = g_PhysModels.GetActive(PlayerObject())) == NULL)
      return;

   float trans_scale = GetTransSpeedScale();
   float rot_scale = GetRotSpeedScale();

   #ifndef SHIP
   if (config_is_defined("PlayerModeSpew"))
   {
      mprintf("updating speed (trans %g, rot %g)\n", trans_scale, rot_scale);
      mprintf("  forward = %g\n", m_forwardState);
      mprintf("  sidestep = %g\n", m_sidestepState);
      mprintf("  rotate = %g\n", m_turningState);
      mprintf("  speedtoggleon = %d\n", m_speedtoggleState);
      mprintf("  toggleisfaster = %d\n", m_speedtogglemeansFaster);
      mprintf("  slideon = %d\n", m_forceSidestepState);
   }
   #endif


   // forward
   if (m_forwardState == 0)
      PhysStopAxisControlVelocity(PlayerObject(), 0);
   else
   if (m_forwardState > 0)
   {
#if 0
      if ((m_forwardState == 1) || m_speedtoggleState)
         PhysAxisControlVelocity(PlayerObject(),
                                 0,
                                 (m_speedtogglemeansFaster ? FAST_MOVE_SPEED : SLOW_MOVE_SPEED) * trans_scale);
      else
         PhysAxisControlVelocity(PlayerObject(), 0, MOVE_SPEED * trans_scale);
#else
      if (m_speedtoggleState)
         PhysAxisControlVelocity(PlayerObject(),
                                 0,
                                 SLOW_MOVE_SPEED * trans_scale * m_forwardState * (m_speedtogglemeansFaster ? 2 : 0.5));
      else
         PhysAxisControlVelocity(PlayerObject(), 0, SLOW_MOVE_SPEED * trans_scale * m_forwardState);
#endif

      m_leaningState = FALSE;
   }
   else
   {
#if 0
      if ((m_forwardState == -1) || m_speedtoggleState)
         PhysAxisControlVelocity(PlayerObject(),
                                 0,
                                 (m_speedtogglemeansFaster ? FAST_BACK_SPEED : SLOW_BACK_SPEED) * trans_scale);
      else
         PhysAxisControlVelocity(PlayerObject(), 0, BACK_SPEED * trans_scale);
#else
      if (m_speedtoggleState)
         PhysAxisControlVelocity(PlayerObject(),
                                 0,
                                 SLOW_BACK_SPEED * trans_scale * -m_forwardState * (m_speedtogglemeansFaster ? 2 : 0.5));
      else
         PhysAxisControlVelocity(PlayerObject(), 0, SLOW_BACK_SPEED * trans_scale * -m_forwardState);
#endif

      m_leaningState = FALSE;
   }

   // sidestep
   if (m_sidestepState == 0 && (!m_forceSidestepState || m_turningState == 0))
      PhysStopAxisControlVelocity(PlayerObject(), 1);
   else
#if 0
   if (m_sidestepState > 0 || (m_forceSidestepState && m_turningState > 0))
   {
      if ((m_sidestepState == 1) || 
          (m_forceSidestepState && m_turningState == 1) ||
          (m_speedtoggleState && !m_speedtogglemeansFaster))
         PhysAxisControlVelocity(PlayerObject(), 1, -SLOW_SIDESTEP_SPEED * trans_scale);
      else
         PhysAxisControlVelocity(PlayerObject(), 1, -SIDESTEP_SPEED * trans_scale);
   }
   else
   {
      if ((m_sidestepState == -1) ||
         (m_forceSidestepState && m_turningState == -1) ||
         (m_speedtoggleState && !m_speedtogglemeansFaster) 
         PhysAxisControlVelocity(PlayerObject(), 1, SLOW_SIDESTEP_SPEED * trans_scale);
      else
         PhysAxisControlVelocity(PlayerObject(), 1, SIDESTEP_SPEED * trans_scale);
   }
#else
   {
      float move_amt;

      if (m_sidestepState != 0.0)
         move_amt = m_sidestepState;
      else
      if (m_forceSidestepState && (m_turningState != 0.0))
         move_amt = m_turningState;
    
      //only slow down sidestep.  Don't speed it up.
      if (m_speedtoggleState && !m_speedtogglemeansFaster)
         PhysAxisControlVelocity(PlayerObject(), 1, SLOW_SIDESTEP_SPEED * trans_scale * -move_amt / 2);
      else
         PhysAxisControlVelocity(PlayerObject(), 1, SLOW_SIDESTEP_SPEED * trans_scale * -move_amt);
   }
#endif

   // rotation
#ifdef KEYBOARD_TURNING_SIMULATES_MOUSELOOK
   if (m_turningState == 0 || m_forceSidestepState)
      headmoveSetKeyboardInput(0);
   else
      headmoveSetKeyboardInput(m_turningState);
#else
   if (m_turningState == 0 || m_forceSidestepState)
      PhysStopControlRotationalVelocity(PlayerObject());
   else
   if (m_turningState > 0)
   {
      if ((m_turningState == 1) || (m_speedtoggleState && !m_speedtogglemeansFaster))
         PhysAxisControlRotationalVelocity(PlayerObject(), 2, -NET_SLOW_ROTATE_SPEED * rot_scale);
      else
         PhysAxisControlRotationalVelocity(PlayerObject(), 2, -NET_ROTATE_SPEED * rot_scale);
      m_leaningState = FALSE;
   }
   else
   {
      if ((m_turningState == -1) || (m_speedtoggleState && !m_speedtogglemeansFaster))
         PhysAxisControlRotationalVelocity(PlayerObject(), 2, NET_SLOW_ROTATE_SPEED * rot_scale);
      else
         PhysAxisControlRotationalVelocity(PlayerObject(), 2, NET_ROTATE_SPEED * rot_scale);

      m_leaningState = FALSE;
   }
#endif
}

////////////////////////////////////////

void cPlayerMode::AddSpeedScale(const char *name, float trans_scale, float rot_scale)
{
   for (int i=0; i<m_speedScale.Size(); i++)
   {
      if (!strcmp(name, m_speedScale[i].name))
      {
         // we're replacing a value already specified
         m_speedScale[i].scales.transSpeedScale = trans_scale;
         m_speedScale[i].scales.rotSpeedScale = rot_scale;
         return;
      }
   }

   // new entry
   sScaleElem newElem;

   strncpy(newElem.name, name, SCALE_NAME_SIZE);
   newElem.scales.transSpeedScale = trans_scale;
   newElem.scales.rotSpeedScale = rot_scale;

   m_speedScale.Append(newElem);

   UpdatePlayerSpeed();
}

void cPlayerMode::RemoveSpeedScale(const char *name)
{
   for (int i=0; i<m_speedScale.Size(); i++)
   {
      if (!strcmp(name, m_speedScale[i].name))
      {
         m_speedScale.DeleteItem(i);

         UpdatePlayerSpeed();
         return;
      }
   }
}

float cPlayerMode::GetTransSpeedScale() const
{
   float value = g_ModeBaseSpeeds[m_curMode].transSpeedScale;

   for (int i=0; i<m_speedScale.Size(); i++)
      value *= m_speedScale[i].scales.transSpeedScale;

   return value;
}

float cPlayerMode::GetRotSpeedScale() const
{
   float value = g_ModeBaseSpeeds[m_curMode].rotSpeedScale;

   for (int i=0; i<m_speedScale.Size(); i++)
      value *= m_speedScale[i].scales.rotSpeedScale;

   return value;
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMode::SetJumpState(BOOL state)
{
   m_jumpState = state;
}

BOOL cPlayerMode::IsJumping() const
{
   return m_jumpState;
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMode::SetSpeedToggleState(BOOL state)
{
   m_speedtoggleState = state;
}

BOOL cPlayerMode::IsCreeping() const
{
   return (m_speedtoggleState && !m_speedtogglemeansFaster);
}


BOOL cPlayerMode::IsRunning() const
{
   return (m_speedtoggleState && m_speedtogglemeansFaster);
}

void cPlayerMode::SetSpeedToggleMeansFaster(BOOL faster)
{
   m_speedtogglemeansFaster = faster;
}

BOOL cPlayerMode::SpeedToggleMeansFaster() const
{
   return m_speedtogglemeansFaster;
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMode::SetLeaningState(int state)
{
   m_leaningState = state;

   // @HACK: for door sound leaning
   if (state == TRUE)
      CheckDoorLeanSoundBlocking();
}

BOOL cPlayerMode::IsLeaning() const
{
   return m_leaningState;
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMode::SetForceSidestepState(BOOL state)
{
   m_forceSidestepState = state;
}

BOOL cPlayerMode::GetForceSidestepState() const
{
   return m_forceSidestepState;
}

////////////////////////////////////////////////////////////////////////////////

void cPlayerMode::SetForwardState(float state)
{
   if (state > 2.0)
      m_forwardState = 2.0;
   else
   if (state < -2.0)
      m_forwardState = -2.0;
   else
      m_forwardState = state;
}

void cPlayerMode::SetSidestepState(float state)
{
   if (state > 2.0)
      m_sidestepState = 2.0;
   else
   if (state < -2.0)
      m_sidestepState = -2.0;
   else
      m_sidestepState = state;
}

void cPlayerMode::SetTurningState(float state)
{
   if (state > 2.0)
      m_turningState = 2.0;
   else
   if (state < -2.0)
      m_turningState = -2.0;
   else
      m_turningState = state;
}

////////////////////////////////////////////////////////////////////////////////

ObjID cPlayerMode::GetFlowArch(int player_submod)
{
   cPhysModel *pModel = g_PhysModels.Get(PlayerObject());
   Assert_(pModel);
   
   Location submod_loc;
   MakeHintedLocationFromVector(&submod_loc, 
                                &pModel->GetLocationVec(player_submod), \
                                &pModel->GetLocation(player_submod));
   int submod_cell = CellFromLoc(&submod_loc);

   AutoAppIPtr_(PropertyManager, pPropMan);
   IFlowGroupProperty *pFlowProp = (IFlowGroupProperty *)pPropMan->GetPropertyNamed(PROP_FLOW_GROUP_NAME);

   ObjID flow_arch = pFlowProp->GetObj(WR_CELL(submod_cell)->motion_index);

   SafeRelease(pFlowProp);

   return flow_arch;
}

////////////////////////////////////////////////////////////////////////////////


