/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// Ranged-combat-related properties and relations
//
#pragma once

#ifndef __DPCAIRCP_H
#define __DPCAIRCP_H

#include <property.h>
#include <relation.h>

////////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IDPCRangedProperty);

#define DPCAI_RANGED_PROP_NAME "DPCAIRange"

struct sDPCRanged
{
   int   m_minimumDist;
   int   m_maximumDist;
   float m_firingDelay;   
   int   m_fireWhileMoving;
};

class cDPCRanged: public sDPCRanged
{
public:
   cDPCRanged();
};

#undef  INTERFACE
#define INTERFACE IDPCRangedProperty
DECLARE_PROPERTY_INTERFACE(IDPCRangedProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cDPCRanged *);
};

////////////////////////////////////////////////////////////////////////////////

EXTERN IDPCRangedProperty *g_pDPCRangedProp;

////////////////////////////////////////////////////////////////////////////////

EXTERN void DPCAIInitRangedProp();
EXTERN void DPCAITermRangedProp();

EXTERN sDPCRanged* DPCAIGetRangedProp(ObjID objID);

////////////////////////////////////////////////////////////////////////////////

#endif






