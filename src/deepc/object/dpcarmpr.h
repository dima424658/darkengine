/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCARMPR_H
#define __DPCARMPR_H

#ifndef __PROPDEF_H
#include <propdef.h>
#endif // !__PROPDEF_H

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

#endif // __DPCARMPR_H