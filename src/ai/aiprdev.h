/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprdev.h,v 1.7 1999/11/19 20:50:32 adurant Exp $
//
// Device properties
//

#ifndef __AIDEVPR_H
#define __AIDEVPR_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIDeviceProperty);
F_DECLARE_INTERFACE(IAITurretProperty);
F_DECLARE_INTERFACE(IAICameraProperty);

///////////////////////////////////////////////////////////////////////////////

void AIInitDeviceProp(void);
void AITermDeviceProp(void);
void AIInitTurretProp(void);
void AITermTurretProp(void);
void AIInitCameraProp(void);
void AITermCameraProp(void);

///////////////////////////////////////

#define PROP_AI_Device "AI_Device"

EXTERN IAIDeviceProperty * g_pAIDeviceProperty;

struct sAIDeviceParams
{
   // raise/lower params
   int m_jointActivate;
   float m_inactivePos;
   float m_activePos;
   float m_activateSpeed;
   // rotate params
   int m_jointRotate;
   float m_facingEpsilon;
   //more part of raise/lower, put here for alignment
   BOOL m_activateRotate;
};

EXTERN const sAIDeviceParams g_AIDefaultDeviceParams;

#define AIGetDeviceParams(obj) \
   AIGetProperty(g_pAIDeviceProperty, (obj), (sAIDeviceParams *)&g_AIDefaultDeviceParams)

#undef  INTERFACE
#define INTERFACE IAIDeviceProperty
DECLARE_PROPERTY_INTERFACE(IAIDeviceProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAIDeviceParams *);
};

///////////////////////////////////////

#define PROP_AI_Turret "AI_Turret"

EXTERN IAITurretProperty * g_pAITurretProperty;

struct sAITurretParams
{
   float m_fireEpsilon;
   tSimTime m_firePause;
   float m_pitchEpsilon;
   float m_maxRange;
};

EXTERN const sAITurretParams g_AIDefaultTurretParams;

#define AIGetTurretParams(obj) \
   AIGetProperty(g_pAITurretProperty, (obj), (sAITurretParams *)&g_AIDefaultTurretParams)

#undef  INTERFACE
#define INTERFACE IAITurretProperty
DECLARE_PROPERTY_INTERFACE(IAITurretProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAITurretParams *);
};

///////////////////////////////////////

#define PROP_AI_Camera "AI_Camera"

EXTERN IAICameraProperty * g_pAICameraProperty;

struct sAICameraParams
{
   float m_scanAngle1;
   float m_scanAngle2;
   float m_scanSpeed;
};

EXTERN const sAICameraParams g_AIDefaultCameraParams;

#define AIGetCameraParams(obj) \
   AIGetProperty(g_pAICameraProperty, (obj), (sAICameraParams *)&g_AIDefaultCameraParams)

#undef  INTERFACE
#define INTERFACE IAICameraProperty
DECLARE_PROPERTY_INTERFACE(IAICameraProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAICameraParams *);
};

#pragma pack()

#endif /* !__AIDEVPR_H */



