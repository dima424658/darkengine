/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpsipr.h,v 1.7 2000/01/31 09:58:49 adurant Exp $
// Shock psi related properties
#pragma once

#ifndef __SHKPSIPR_H
#define __SHKPSIPR_H

#include <linktype.h>
#include <propface.h>
#include <simtime.h>

#include <shkpltyp.h>
#include <shkpsity.h>
#include <shkpsibs.h>

#include <esndprop.h>

/////////////////////////////////////
// Psi Power property
// This is a property meant to be attached to an psi power archetype 
// 

F_DECLARE_INTERFACE(IPsiPowerProperty);

#undef INTERFACE
#define INTERFACE IPsiPowerProperty

DECLARE_PROPERTY_INTERFACE(IPsiPowerProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sPsiPower*);  // Type-specific accessors, by reference
};

#define PROP_PSI_POWER_DESC "PsiPower"

EXTERN IPsiPowerProperty *g_pPsiPowerProperty;

EXTERN void PsiPowerPropertyInit();

/////////////////////////////////////
// Psi Shield property
// This is a property meant to be attached to an psi shield archetype 
// 

F_DECLARE_INTERFACE(IPsiShieldProperty);

#undef INTERFACE
#define INTERFACE IPsiShieldProperty

DECLARE_PROPERTY_INTERFACE(IPsiShieldProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sPsiShield*);  // Type-specific accessors, by reference
};

#define PROP_PSI_SHIELD_DESC "PsiShield"

EXTERN IPsiShieldProperty *g_pPsiShieldProperty;

EXTERN void PsiShieldPropertyInit();

EXTERN sPsiShield defaultPsiShield;

/////////////////////////////////////
// Psi State property
// This is a property meant to be attached to the player
// 

F_DECLARE_INTERFACE(IPsiStateProperty);

#undef INTERFACE
#define INTERFACE IPsiStateProperty

class cPsiState: public sPsiState
{
public:
   cPsiState();
};

DECLARE_PROPERTY_INTERFACE(IPsiStateProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(cPsiState*);  // Type-specific accessors, by reference
};

#define PROP_PSI_STATE_DESC "PsiState"

EXTERN IPsiStateProperty *g_pPsiStateProperty;

EXTERN void PsiStatePropertyInit();
EXTERN sPsiState* PsiStateGet(ObjID objID);
EXTERN void PsiStateSetCurrentPower(ObjID objID, ePsiPowers power);
EXTERN void PsiStateSetPowerState(ObjID objID, ePsiPowerState state);
EXTERN void PsiStateSetActivateTime(ObjID objID, tSimTime time);
EXTERN void PsiStateSetPoints(ObjID objID, float points);
EXTERN void PsiStateSetMaxPoints(ObjID objID, float maxPoints);

///////////////////////////////////////////////////////
// Teleport link

EXTERN IRelation *g_pPsiTeleportLinks;

///////////////////////////////////////////////////////

// overload "twin property"
EXTERN BOOL PsiOverloaded(ObjID obj, ePsiPowers which);
EXTERN void PsiSetOverload(ObjID obj, ePsiPowers which, BOOL val);

EXTERN IIntProperty *g_PsiRadarProperty;
EXTERN IFloatProperty *g_PsiAlchemyProperty;

///////////////////////////////////////////////////////

#define PROP_ESND_PSI_NAME "Psi Tags"

EXTERN BOOL ObjGetESndPsi(ObjID obj, sESndTagList **ppList);
EXTERN BOOL ObjSetESndPsi(ObjID obj, sESndTagList *pList);

///////////////////////////////////////////////////////

EXTERN void PsiPropertiesInit(void);
EXTERN void PsiPropertiesTerm(void);

#endif