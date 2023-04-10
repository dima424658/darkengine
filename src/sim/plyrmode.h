/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/plyrmode.h,v 1.12 2000/03/09 22:57:21 adurant Exp $
//
// Player mode header
//
#pragma once

#ifndef __PLYRMODE_H
#define __PLYRMODE_H

#include <phmoapi.h>
#include <comtools.h>
#include <simtime.h>

#ifdef __cplusplus
#include <dynarray.h>
#endif

////////////////////////////////////////

EXTERN void PlayerModeInit();
EXTERN void PlayerModeTerm();
EXTERN void PlayerModeReset();

EXTERN void PlayerModeUpdate();

#ifdef __cplusplus
class cPlayerMode;
EXTERN cPlayerMode *g_pPlayerMode;
#endif

////////////////////////////////////////

enum ePlayerMode
{
   kPM_Stand,
   kPM_Crouch,
   kPM_Swim,
   kPM_Climb,
   kPM_BodyCarry,
   kPM_Slide,
   kPM_Jump,
   kPM_Dead,
   kPM_NumModes,
};

typedef enum ePlayerMode ePlayerMode;

typedef struct
{
   ePlayerMotion atRest;
   ePlayerMotion stride[2];
} sModeMotions;

typedef struct
{
   float transSpeedScale;
   float rotSpeedScale;
} sModeSpeedScale;

#define SCALE_NAME_SIZE 32

typedef struct
{
   char            name[SCALE_NAME_SIZE];
   sModeSpeedScale scales;
} sScaleElem;

////////////////////////////////////////

EXTERN sModeMotions    g_ModeMotions[kPM_NumModes];
EXTERN sModeSpeedScale g_ModeBaseSpeeds[kPM_NumModes];

EXTERN ePlayerMode GetPlayerMode();
EXTERN void SetPlayerMode(ePlayerMode mode);

EXTERN void AddSpeedScale(const char *name, float trans_scale, float rot_scale);
EXTERN void RemoveSpeedScale(const char *name);

EXTERN float GetTransSpeedScale();
EXTERN float GetRotSpeedScale();

EXTERN void SetJumpState(BOOL state);
EXTERN BOOL IsJumping();

EXTERN void SetSpeedToggleState(BOOL state);
EXTERN BOOL IsCreeping();
EXTERN BOOL IsRunning();

EXTERN void SetSpeedToggleMeansFaster(BOOL faster);
EXTERN BOOL SpeedToggleMeansFaster();

EXTERN void SetLeaningState(BOOL state);
EXTERN BOOL IsLeaning();

EXTERN void SetForceSidestepState(BOOL state);
EXTERN BOOL GetForceSidestepState();

EXTERN void UpdatePlayerSpeed();

EXTERN void SetForwardState(float state);
EXTERN void SetSidestepState(float state);
EXTERN void SetTurningState(float state);

EXTERN void PlayerModeStartFadeIn(tSimTime time, uchar r, uchar g, uchar b);
EXTERN void PlayerModeStartFadeOut(tSimTime time, uchar r, uchar g, uchar b);
EXTERN BOOL PlayerModeGetFade(uchar* pR, uchar* pG, uchar* pB, float *pAlpha);

typedef void (*fOnDeathCallback)();

////////////////////////////////////////

#ifdef __cplusplus

F_DECLARE_INTERFACE(ITagFile);

class cPlayerMode
{
public:

   cPlayerMode();
   ~cPlayerMode();

   void Read(ITagFile *file);
   void Write(ITagFile *file);

   void SetMode(ePlayerMode mode);
   ePlayerMode GetMode() const;

   void Update();

   void ActivateStrideMotion(BOOL left);
   void ActivateLandingMotion();
   void ActivateRestMotion();

   void DoStrideSound(int play_vol, const mxs_vector &foot_loc);
   void DoLandingSound(int play_vol, const mxs_vector &foot_loc);

   void InstallOnDeathCallback(fOnDeathCallback callback);
   void DoDeath();

   void UpdatePlayerSpeed();

   void AddSpeedScale(const char *name, float trans_scale, float rot_scale);
   void RemoveSpeedScale(const char *name);

   float GetTransSpeedScale() const;
   float GetRotSpeedScale() const;

   void SetJumpState(BOOL state);
   BOOL IsJumping() const;

   void SetSpeedToggleState(BOOL state);
   BOOL IsCreeping() const; //speed toggle on, and means slow
   BOOL IsRunning() const; //speed toggle on, and means fast

   void SetSpeedToggleMeansFaster(BOOL faster);
   BOOL SpeedToggleMeansFaster() const;

   void SetLeaningState(BOOL state);
   BOOL IsLeaning() const;

   void SetForceSidestepState(BOOL state);
   BOOL GetForceSidestepState() const;

   void SetForwardState(float state);
   void SetSidestepState(float state);
   void SetTurningState(float state);

   void StartFadeIn(tSimTime time, uchar r, uchar g, uchar b);
   void StartFadeOut(tSimTime time, uchar r, uchar g, uchar b);
   BOOL GetFade(uchar* pR, uchar* pG, uchar* pB, float *pAlpha);

private:

   ObjID GetFlowArch(int player_submod);
   
   BOOL m_jumpState;
   BOOL m_speedtoggleState;
   BOOL m_speedtogglemeansFaster;
   BOOL m_leaningState;
   BOOL m_forceSidestepState;

   float m_forwardState;
   float m_sidestepState;
   float m_turningState;

   int  m_timeOfDeath;

   ePlayerMode           m_curMode;
   cDynArray<sScaleElem> m_speedScale;

   cDynArray<fOnDeathCallback> m_onDeathCallbackList;

   int  m_timeOfFadeStart;
   float m_fadeRate;
   uchar m_fadeR;
   uchar m_fadeG;
   uchar m_fadeB;
};

#endif

////////////////////////////////////////

#endif





