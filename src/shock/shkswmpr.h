/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkswmpr.h,v 1.1 1998/11/18 11:03:57 JON Exp $
//
// Swarm properties
//

#ifndef __SHKSWMPR_H
#define __SHKSWMPR_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAISwarmProperty);

///////////////////////////////////////////////////////////////////////////////

void ShockAIInitSwarmProp(void);
void ShockAITermSwarmProp(void);

///////////////////////////////////////

#define PROP_AI_SWARM "AI_Swarm"

EXTERN IAISwarmProperty * g_pAISwarmProperty;

struct sAISwarmParams
{
   float m_closeDist;   // distance to close to before backing off
   float m_backOffDist; // distance to back off to
};

EXTERN const sAISwarmParams g_AIDefaultSwarmParams;

#define ShockAIGetSwarmParams(obj) \
   AIGetProperty(g_pAISwarmProperty, (obj), (sAISwarmParams *)&g_AIDefaultSwarmParams)

EXTERN float ShockAIGetCloseDist(ObjID objID);
EXTERN float ShockAIGetBackOffDist(ObjID objID);

#undef  INTERFACE
#define INTERFACE IAISwarmProperty
DECLARE_PROPERTY_INTERFACE(IAISwarmProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAISwarmParams *);
};

#pragma pack()

#endif /* !__SHKSWMPR_H */



