/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCHOME_H
#define __DPCHOME_H

#include <fix.h>

#include <objtype.h>
#include <propface.h>

// Homing projectile property

struct sHoming
{
   int   m_targetType;
   float m_distFilter;
   int   m_headingFilter;
   int   m_maxTurn;
   int   m_updateFreq;
};

#define PROP_HOMING "Homing"

F_DECLARE_INTERFACE(IHomingProperty);

#undef INTERFACE
#define INTERFACE IHomingProperty

DECLARE_PROPERTY_INTERFACE(IHomingProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sHoming*);  // Type-specific accessors, by reference
};

EXTERN IHomingProperty *g_pHomingProperty;

// Scan for targets
EXTERN ObjID TargetScan(ObjID projID);
// Home on target
EXTERN void Home(ObjID projID, ObjID targetID);

EXTERN void ProjectileTargetInit(void);
EXTERN void ProjectileTargetTerm(void);

#endif __DPCHOME_H
