/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/dmgbase.h,v 1.6 2000/01/29 13:19:25 adurant Exp $
#pragma once

#ifndef __DMGBASE_H
#define __DMGBASE_H
#include <dmgmodel.h>
#include <chainevt.h>

////////////////////////////////////////////////////////////
// DAMAGE MODEL BASE TYPES
//

//
// Impact structure
// some day this might contain all kinds of data
// (impulse, energy transfer, energy absorption, etc)
// 

struct sImpact 
{
   float magnitude;
}; 


//
// Damage structure
//
// source, type and magnitude of the damage. 
// Interpretation of these fields is up to the specific damage model implementation 
//

typedef int tDamage;

struct sDamage
{
   tDamage amount; // how much?
   tDamageKind kind; // what kind?
};

//
// Damage result
// Determines the fate of an object.  In increasing order of severity
//

enum eDamageResult_ 
{
   kDamageNoOpinion,  // no opinion
   kDamageStatusQuo,  // retain current state
   kImpactBounce,     // valid for impact only.  Bounce.
   kImpactStick,      // valid for impact only.  Stick.
   kDamageSlay,       // slay the damaged object
   kDamageTerminate,  // terminate the damaged object
   kDamageResurrect,  // ressurect the object 
   kDamageDestroy,    // ObjSys::Destroy the damaged object outright.
   kNumDamageResults
};

//
// Damage messages
//

enum eDamageMsgKind_
{
   kDamageMsgImpact     = 1 << 0,
   kDamageMsgDamage     = 1 << 1,
   kDamageMsgSlay       = 1 << 2, 
   kDamageMsgTerminate  = 1 << 3,
   kDamageMsgResurrect  = 1 << 4, 
}; 

typedef ulong eDamageMsgKind;

typedef struct sImpactMsgData sImpactMsgData;

union uDamageMsgData 
{
   void* raw;
   sImpact* impact;
   sDamage* damage; 
   tDamageKind slay;
   void* terminate;
};

struct sDamageMsgData
{
   eDamageMsgKind kind;
   ObjID victim;
   ObjID culprit;
   union uDamageMsgData data;
};

//
// sDamageMsg is C++-only 
//

#ifdef __cplusplus
struct sDamageMsg : public sChainedEvent, public sDamageMsgData
{
   sDamageMsg(eChainedEventKind k, const sDamageMsgData* dat = NULL, sChainedEvent* ev = NULL)
      : sChainedEvent(k,sizeof(sDamageMsg),ev), 
        sDamageMsgData((dat) ? *dat : sDamageMsgData())
   {
   }
        
}; 
#endif 

#endif // __DMGBASE_H




