/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgunmd.cpp,v 1.11 2000/02/19 13:25:18 toml Exp $

#include <stdlib.h>

#include <appagg.h>
#include <matrix.h>
#include <matrixs.h>
#include <fix.h>
#include <mprintf.h>

#include <iobjsys.h>
#include <mnamprop.h>
#include <osysbase.h>
#include <objpos.h>
#include <playrobj.h>
#include <rand.h>
#include <rendprop.h>
extern "C"
{
#include <camera.h>
}

#include <shkgunpr.h>
#include <shkplayr.h>
#include <shkctrl.h>

#include <shkgunmd.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


#define min(a,b)  (((a) < (b)) ? (a) : (b))
#define max(a,b)  (((a) > (b)) ? (a) : (b))


enum ePlayerGunState {kPlayerGunStationary, kPlayerGunWobble, kPlayerGunRaise, kPlayerGunLower, kPlayerGunRaised,
   kPlayerGunOff,};
enum ePlayerGunSwingState {kPlayerGunSwingStart, kPlayerGunSwingLeft, kPlayerGunSwingRight,};

const mxs_ang swingAmplitude = 0x0800;
const tSimTime swingPeriod = 800;
const tSimTime swingHalfPeriod = swingPeriod/2;
const mxs_ang swingReturn = 0x0050;
const float bobRate = 0.002;
const float bobAmplitude = 0.05;
const mxs_ang kPlayerGunRaisedPitch = 0;
const int kPlayerGunRaiseTime = 5000;
const mxs_ang kPlayerGunPitchRate = 0x0100;
const float kPlayerGunRaisedBob = 0.1;
const mxs_ang kPlayerGunPitch = -0x0800;
const mxs_ang kPlayerGunOffPitch = -0x1500;

class cPlayerGunModel
{
public:
   void On(ObjID gunID);
   void PosUpdate(void);
   void Off(void);
   void Kill(void);
   void Update(void);
   void Init(void);
   void Shutdown(void);
   BOOL Fire(void);
   BOOL IsOn(void);

private:
   ObjID m_objID; // ID of the object we are using to represent the gun in front of the player
   ObjID m_gunID; // ID of the actual gun
   ePlayerGunState m_state;
   ePlayerGunSwingState m_swingState;
   tSimTime m_swingStartTime;
   tSimTime m_bobStartTime;
   tSimTime m_lastFireTime;
   mxs_ang m_swing;
   mxs_ang m_pitch;
   mxs_ang m_pitchTarget;
   float m_bob;
   float m_bobTarget;

   void SetState(ePlayerGunState state);
   void SetControls(void);
   void SetSwingState(ePlayerGunSwingState state);
};

inline BOOL cPlayerGunModel::IsOn(void) {return m_state != kPlayerGunOff;}

cPlayerGunModel g_playerGunModel;
IObjectSystem *pObjSys;

void cPlayerGunModel::On(ObjID gunID)
{
   Label *pHandModelName;

   Kill();
   if ((m_gunID = gunID) == OBJ_NULL)
      return;

   // create the gun model object
   m_objID = pObjSys->BeginCreate(ROOT_ARCHETYPE, kObjectConcrete);
   pObjSys->NameObject(m_objID, "Player Gun");
   if (PlayerGunDescGetHandModel(m_gunID, &pHandModelName))
      ObjSetModelName(m_objID, (char*)pHandModelName);
   pObjSys->SetObjTransience(m_objID, TRUE);
   pObjSys->EndCreate(m_objID);

   m_bobTarget = 0;
   m_bob = 0;
   m_swing = 0;
   m_pitch = kPlayerGunOffPitch;
   m_pitchTarget = kPlayerGunPitch;
   m_state = kPlayerGunStationary;
   PosUpdate();
}

void cPlayerGunModel::SetSwingState(ePlayerGunSwingState state)
{
   m_swingState = state;
   m_swingStartTime = GetSimTime();
}

void cPlayerGunModel::SetState(ePlayerGunState state)
{
   m_state = state;
   switch (state)
   {
   case kPlayerGunStationary:
      if (g_playerMoveMode != kPlayerMoveNone)
      {
         SetSwingState(kPlayerGunSwingStart);
         m_bobStartTime = GetSimTime(); 
         SetState(kPlayerGunWobble);
      }
      break;
   case kPlayerGunWobble:
      if (g_playerMoveMode == kPlayerMoveNone)
         SetState(kPlayerGunStationary);
      break;
   case kPlayerGunRaise:
      if (m_pitch == kPlayerGunRaisedPitch)
//      if (m_bob == kPlayerGunRaisedBob)
      {
         SetState(kPlayerGunRaised);
         // fire the damn gun
         EnactFire(PlayerObject(), m_gunID);
         m_lastFireTime = GetSimTime();
      }
      break;
   case kPlayerGunRaised:
      if (GunStateIsFiring(m_gunID))
      {
         EnactFire(PlayerObject(), m_gunID);
         m_lastFireTime = GetSimTime();
      }
      else if (GetSimTime()>m_lastFireTime+kPlayerGunRaiseTime)
         SetState(kPlayerGunLower);
      break;
   case kPlayerGunLower:
      if (m_pitch == kPlayerGunPitch)
         SetState(kPlayerGunStationary);
      break;
   case kPlayerGunOff:
      if (m_pitch == kPlayerGunOffPitch)
         Kill();
      break;
   }
}

mxs_ang IntegrateAng(mxs_ang val, mxs_ang targ, mxs_ang rate)
{
   if (val != targ)
   {
      mxs_ang diff = targ-val;
      if (diff<MX_ANG_PI)
      {
         if (diff<rate)
            return targ;
         else
            return val+rate;
      }
      else 
      {
         if (diff>(fix_2pi-rate))
            return targ;
         else
            return val-rate;
      }
   }
   return targ;
}

void cPlayerGunModel::SetControls(void)
{
   switch (m_state)
   {
   case kPlayerGunWobble:
      {
         int delta = GetSimTime()-m_swingStartTime;
         switch (m_swingState)
         {
         case kPlayerGunSwingStart:
            if (delta>swingHalfPeriod)
               SetSwingState(kPlayerGunSwingRight);
            delta = GetSimTime()-m_swingStartTime;
            break;
         case kPlayerGunSwingLeft:
            if (delta>swingPeriod)
               SetSwingState(kPlayerGunSwingRight);
            delta = GetSimTime()-m_swingStartTime;
            break;
         case kPlayerGunSwingRight:
            if (delta>swingPeriod)
               SetSwingState(kPlayerGunSwingLeft);
            delta = GetSimTime()-m_swingStartTime;
            break;
         }
         switch (m_swingState)
         {
         case kPlayerGunSwingStart:
            m_swing = fix_mul(swingAmplitude, fix_from_float(float(delta)/(float(swingPeriod))));
            break;
         case kPlayerGunSwingLeft:
            m_swing = fix_mul(swingAmplitude, fix_from_float((float(delta)-float(swingHalfPeriod))/float(swingPeriod)));
            break;
         case kPlayerGunSwingRight:
            m_swing = fix_mul(swingAmplitude, fix_from_float((float(swingHalfPeriod)-float(delta))/float(swingPeriod)));
            break;
         }
         if (m_bob==m_bobTarget)
            m_bobTarget = (float(UnrecordedRand())/float(RAND_MAX))*bobAmplitude;
      }
      break;
   case kPlayerGunStationary:
      m_swing = IntegrateAng(m_swing, 0, swingReturn);
      // state is nothing or shooting
      m_bobTarget = 0;
      break;
   case kPlayerGunRaise:
      m_pitchTarget = kPlayerGunRaisedPitch;
      break;
   case kPlayerGunRaised:
      break;
   case kPlayerGunLower:
      m_pitchTarget = kPlayerGunPitch;
      break;
   case kPlayerGunOff:
      m_pitchTarget = kPlayerGunOffPitch;
      break;
   }
}

void cPlayerGunModel::PosUpdate(void)
{
   mxs_vector camPos;
   mxs_angvec camFacing;
   mxs_vector *pModelOffset;
   mxs_matrix playerRotMat;
   mxs_matrix adjustRotMat;
   mxs_matrix rotMat;
   mxs_angvec rotOffset;
   mxs_vector offset;
   mxs_ang heading;
   mxs_matrix *pFacingMat;
   mxs_matrix *pOffsetMat;
   mxs_matrix facingMat;
   mxs_matrix offsetMat;
   mxs_matrix facingMat2;
   mxs_matrix offsetMat2;

   if (m_objID == OBJ_NULL)
      return;
   CameraGetLocation(PlayerCamera(), &camPos, &camFacing);
   if (!PlayerGunDescGetModelOffset(m_gunID, &pModelOffset))
      return;

   SetState(m_state);
   // SetState can kill the model
   if (m_objID == OBJ_NULL)
      return;

   SetControls();

   // pitch
   m_pitch = IntegrateAng(m_pitch, m_pitchTarget, kPlayerGunPitchRate);

   // bob
   if (m_bob<m_bobTarget)
      m_bob = min(m_bob+bobRate, m_bobTarget);
   else if (m_bob>m_bobTarget)
      m_bob = max(m_bob-bobRate, m_bobTarget);

   // player facing
   mx_ang2mat(&playerRotMat, &camFacing);
   // gun heading adjustment
   heading = PlayerGunDescGetModelHeading(m_gunID);
   rotOffset.tx = 0;
   rotOffset.ty = 0;
   rotOffset.tz = heading;
   mx_ang2mat(&adjustRotMat, &rotOffset);
   // combine two rots
   mx_mul_mat(&rotMat, &playerRotMat, &adjustRotMat);
   // apply swing
   pFacingMat = &rotMat;
   pOffsetMat = &playerRotMat;
   if (m_swing != 0)
   {
      mxs_matrix swingMat;

      mx_mk_rot_z_mat(&swingMat, m_swing);
      // calc facing
      mx_mul_mat(&facingMat, &swingMat, pFacingMat);
      pFacingMat = &facingMat;

      // calc offset
      mx_mul_mat(&offsetMat, &swingMat, pOffsetMat);
      pOffsetMat = &offsetMat;
   }
   if (m_pitch != 0)
   {
      mx_rot_x_mat(&facingMat2, pFacingMat, -m_pitch);
      // calc facing
      pFacingMat = &facingMat2;

      // calc offset
      mx_rot_y_mat(&offsetMat2, pOffsetMat, -m_pitch);
      pOffsetMat = &offsetMat2;
   }
   // calc facing
   mx_mat2ang(&camFacing, pFacingMat);
   // calc offset
   mx_mat_mul_vec(&offset, pOffsetMat, pModelOffset);
   mx_addeq_vec(&camPos, &offset);
   camPos.z += m_bob;
   ObjPosUpdate(m_objID, &camPos, &camFacing);
}

void cPlayerGunModel::Kill(void)
{
   // destroy old model
   if (m_objID != OBJ_NULL)
   {
      pObjSys->Destroy(m_objID);
      m_objID = OBJ_NULL;
      m_state = kPlayerGunOff;
   }
}

void cPlayerGunModel::Off(void)
{
   SetState(kPlayerGunOff);
}

void cPlayerGunModel::Update(void)
{
   if (m_objID != OBJ_NULL)
   {
      PosUpdate();
   }
}

void cPlayerGunModel::Init(void)
{
   m_objID = NULL;
   pObjSys = AppGetObj(IObjectSystem);
   m_state = kPlayerGunOff;
}

void cPlayerGunModel::Shutdown(void)
{
   SafeRelease(pObjSys);
   pObjSys = NULL;
}

BOOL cPlayerGunModel::Fire(void)
{
   if (m_state == kPlayerGunOff)
      return FALSE;
   if (m_state == kPlayerGunRaised)
      return EnactFire(PlayerObject(), m_gunID);
   // start raising the gun
   m_state = kPlayerGunRaise;
   return FALSE;
}

// for c users
void PlayerGunModelOn(ObjID objID) {g_playerGunModel.On(objID);}
void PlayerGunModelPosUpdate(void) {g_playerGunModel.PosUpdate();}
void PlayerGunModelOff(void) {g_playerGunModel.Off();}
void PlayerGunModelKill(void) {g_playerGunModel.Kill();}
void PlayerGunModelUpdate(void) {g_playerGunModel.Update();}
void PlayerGunModelInit(void) {g_playerGunModel.Init();}
void PlayerGunModelShutdown(void) {g_playerGunModel.Shutdown();}
BOOL PlayerGunModelFire(void) {return g_playerGunModel.Fire();}
BOOL PlayerGunModelIsOn(void) {return g_playerGunModel.IsOn();}

