/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaitpr.h,v 1.1 1998/07/14 11:18:37 JON Exp $
//
// Turret properties
//

#ifndef __SHKAITPR_H
#define __SHKAITPR_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAITurretProperty);

///////////////////////////////////////////////////////////////////////////////

void AIInitTurretProp(void);
void AITermTurretProp(void);

///////////////////////////////////////

#define PROP_AI_TURRET "AI_Turret"

EXTERN IAITurretProperty * g_pAITurretProperty;

struct sAITurretParams
{
   // raise/lower params
   int m_jointRaise;
   float m_inactivePos;
   float m_activePos;
   float m_raiseSpeed;
   // rotate params
   int m_jointRotate;
   float m_facingEpsilon;
   float m_fireEpsilon;
};

EXTERN const sAITurretParams g_AIDefaultTurretParams;

#define AIGetTurretParams(obj) \
   AIGetProperty(g_pAITurretProperty, (obj), (sAITurretParams *)&g_AIDefaultTurretParams);

#undef  INTERFACE
#define INTERFACE IAITurretProperty
DECLARE_PROPERTY_INTERFACE(IAITurretProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAITurretParams *);
};

#pragma pack()

#endif /* !__SHKAITPR_H */



