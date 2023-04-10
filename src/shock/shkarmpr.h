/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkarmpr.h,v 1.4 2000/01/29 13:40:29 adurant Exp $
#pragma once

#ifndef __SHKARMPR_H
#define __SHKARMPR_H

#include <propdef.h>

// Armor stats

// The actual structure
typedef struct sArmor
{
   float m_toxic;
   float m_radiation;
   float m_combat;
} sArmor;

// Property stuff

F_DECLARE_INTERFACE(IArmorProperty);

#undef INTERFACE
#define INTERFACE IArmorProperty

DECLARE_PROPERTY_INTERFACE(IArmorProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sArmor*);   // Type-specific accessors, by reference
};

#define PROP_Armor "Armor"

EXTERN IArmorProperty *g_ArmorProperty;

EXTERN void ArmorPropertyInit();
EXTERN void ArmorPropertyTerm();
EXTERN BOOL ArmorGet(ObjID objID, sArmor **ppArmor);

#define ObjHasArmor(obj) IProperty_IsRelevant(g_ArmorProperty,obj)

#define ARMOR_SET(objID, ppParams) \
   PROPERTY_SET(g_ArmorProperty, objID, (sArmor**)ppParams)
#define ARMOR_DELETE(objID) \
   IProperty_Delete(g_ArmorProperty, objID)

#endif // __SHKARMPR_H