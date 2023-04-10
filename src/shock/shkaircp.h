/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaircp.h,v 1.2 2000/01/29 13:40:19 adurant Exp $
//
// Ranged-combat-related properties and relations
//
#pragma once

#ifndef __SHKAIRCP_H
#define __SHKAIRCP_H

#include <property.h>
#include <relation.h>

////////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IShockRangedProperty);

#define SHKAI_RANGED_PROP_NAME "ShkAIRange"

struct sShockRanged
{
   int   m_minimumDist;
   int   m_maximumDist;
   float m_firingDelay;   
   int   m_fireWhileMoving;
};

class cShockRanged: public sShockRanged
{
public:
   cShockRanged();
};

#undef  INTERFACE
#define INTERFACE IShockRangedProperty
DECLARE_PROPERTY_INTERFACE(IShockRangedProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cShockRanged *);
};

////////////////////////////////////////////////////////////////////////////////

EXTERN IShockRangedProperty *g_pShockRangedProp;

////////////////////////////////////////////////////////////////////////////////

EXTERN void ShockAIInitRangedProp();
EXTERN void ShockAITermRangedProp();

EXTERN sShockRanged* ShockAIGetRangedProp(ObjID objID);

////////////////////////////////////////////////////////////////////////////////

#endif






