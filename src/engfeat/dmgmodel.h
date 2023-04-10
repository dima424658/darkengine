/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgmodel.h,v 1.10 2000/02/05 19:37:54 adurant Exp $
#pragma once

#ifndef __DMGMODEL_H
#define __DMGMODEL_H

#include <comtools.h>
#include <objtype.h>

////////////////////////////////////////////////////////////
// ABSTRACT DAMAGE MODEL INTERFACE
//
// The goal of this API is to allow systems (e.g. physics)
// to interact with the damage model without intrinsically understanding 
// its game mechanics.  It provides hooks for game systems to listen to 
// damage events.  
//
// The damage model assumes that objects have 3 states: Living, Dying, and Dead.
// It provides an explicit API for changing the states of objects, and for game systems
// to listen in on state changes.
//
F_DECLARE_INTERFACE(IDamageModel);

//
// Impact-related types
//
typedef struct sImpact sImpact;

//
// Damage-related types
//
typedef int tDamageKind;
typedef ulong eDamageResult;
typedef struct sDamage sDamage;


//
// Callback types
//
typedef void* tDamageCallbackData;
typedef eDamageResult (LGAPI *tDamageFilterFunc)(ObjID victim, ObjID culprit, sDamage* damage, tDamageCallbackData data);

typedef struct sDamageMsg sDamageMsg;
typedef ulong tDamageMsgMask;
typedef eDamageResult (LGAPI *tDamageListenerFunc)(const sDamageMsg* msg, tDamageCallbackData data);

//
// THE COM INTERFACE
//

#undef INTERFACE
#define INTERFACE IDamageModel
DECLARE_INTERFACE_(IDamageModel, IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   // Handle an object impacting something, 
   // returns the fate of the hittee.  
   STDMETHOD_(eDamageResult,HandleImpact)(THIS_ ObjID hittee, ObjID hitter, sImpact* impact, struct sChainedEvent* cause DEFAULT_TO(NULL)) PURE;
   
   // Damage an object
   STDMETHOD_(eDamageResult,DamageObject)(THIS_ ObjID victim, ObjID culprit, 
                                          sDamage* damage, 
                                          struct sChainedEvent* cause DEFAULT_TO(NULL), BOOL allowzero DEFAULT_TO(FALSE)) PURE; 

   // Kill an object, causing it to begin its death throes, if any. 
   // the object will be terminated when its death throes end.
   STDMETHOD_(eDamageResult,SlayObject)(THIS_ ObjID victim, ObjID culprit, 
                                        struct sChainedEvent* cause DEFAULT_TO(NULL)) PURE;

   // Turn an object into a corpse.  Used to signal to the damage model that
   // the object's death throes are done.  If you want a living object to die, 
   // SlayObject is recommended instead.
   STDMETHOD_(eDamageResult,TerminateObject)(THIS_ ObjID victim, struct sChainedEvent* cause DEFAULT_TO(NULL)) PURE;

   // Return an object to life, just like it was a brand new object
   STDMETHOD_(eDamageResult,ResurrectObject)(THIS_ ObjID victim, ObjID culprit, struct sChainedEvent* cause DEFAULT_TO(NULL)) PURE; 

   // Filter all damage that occurs.  Your callback may mutate the sDamage in place.
   // Filters are applied in the order they are installed.
   STDMETHOD(Filter)(THIS_ tDamageFilterFunc func, tDamageCallbackData data) PURE;

   // Receive notification when damage events occur
   STDMETHOD(Listen)(THIS_ tDamageMsgMask interests, tDamageListenerFunc func, tDamageCallbackData data) PURE;

};
#undef INTERFACE

#define IDamageModel_HandleImpact(p, a, b, c)      COMCall4(p, HandleImpact, a, b, c, NULL)
#define IDamageModel_DamageObject(p, a, b, c)      COMCall4(p, DamageObject, a, b, c, NULL)
#define IDamageModel_SlayObject(p, a, b)           COMCall3(p, SlayObject, a, b, NULL)
#define IDamageModel_TerminateObject(p, a)         COMCall2(p, TerminateObject, a, NULL)
#define IDamageModel_Filter(p, a, b)               COMCall2(p, Filter, a, b)
#define IDamageModel_Listen(p, a, b, c)            COMCall3(p, Listen, a, b, c)

// Create the empty damage model, that only passes events on to callbacks
EXTERN void InitEmptyDamageModel(void); 

// Create Act/React Effects for damage model
EXTERN void InitDamageReactions(void);

#endif // __DMGMODEL_H

