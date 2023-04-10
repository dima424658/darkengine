/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPLAYR_H
#define __DPCPLAYR_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

#ifndef __OSYSTYPE_H
#include <osystype.h>
#endif // !__OSYSTYPE_H

#ifndef __LABEL_H
#include <label.h>
#endif  // !__LABEL_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

typedef enum eStats;
typedef enum eTechSkills;
typedef enum eWeaponSkills;
typedef enum ePlayerEquip;
typedef enum eTrait;

typedef struct sStatsDesc
{
   int m_stats[5];
} sStatsDesc;

typedef struct sTraitsDesc
{
   int m_traits[4];
} sTraitsDesc;

typedef struct sWeaponSkills
{
   int m_wpn[4];
} sWeaponSkills;

typedef struct sTechSkills
{
   int m_tech[5];
} sTechSkills;

typedef struct sLogData
{
   int m_data[4];
} sLogData;

static void LGAPI ListenFunc(sPropertyListenMsg* msg, PropListenerData data);

F_DECLARE_INTERFACE(IDPCPlayer); 

#undef INTERFACE
#define INTERFACE IDPCPlayer

DECLARE_INTERFACE_(IDPCPlayer,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // returns the current cost to bump up a stat to the next level
   STDMETHOD_(int,StatCost)(int which) PURE;
   STDMETHOD_(int,WeaponSkillCost)(int which) PURE;
   STDMETHOD_(int,TechSkillCost)(int which) PURE;
   
   // is a >= b ?
   STDMETHOD_(BOOL,CheckStats)(sStatsDesc *a, sStatsDesc *b, BOOL spew) PURE;
   STDMETHOD_(BOOL,CheckWeaponSkills)(sWeaponSkills *a, sWeaponSkills *b, BOOL spew) PURE;

   STDMETHOD_(BOOL,Equip)(ObjID player, ePlayerEquip slot, ObjID o, BOOL spew) PURE;
   STDMETHOD(SetEquip)(ObjID player, ePlayerEquip slot, ObjID o) PURE;
   STDMETHOD_(ObjID,GetEquip)(ObjID player, ePlayerEquip slot) PURE;
   STDMETHOD(ClearEquip)(void) PURE;
   STDMETHOD(AlternateWeapons)(ObjID player) PURE;
   STDMETHOD_(void,SetWeaponModel)(ObjID o, BOOL sfx = TRUE) PURE;
   STDMETHOD_(void,NowNextWeapon)(BOOL WaitAFrame) PURE;
   
   STDMETHOD_(BOOL,CheckRequirements)(ObjID o, BOOL spew) PURE;

   STDMETHOD_(ObjID,GetCurrentGun)(void) PURE;

   STDMETHOD_(void,Frame)(void) PURE;

   STDMETHOD(UseInternalModel)(void) PURE;
   STDMETHOD(UseExternalModel)(void) PURE;
   STDMETHOD_(char *,GetExternalModel)(void) PURE;

   STDMETHOD(RecalcData)(ObjID obj) PURE; 

   // look up data, 
   STDMETHOD_(int,GetStat)(eStats which) PURE;
   STDMETHOD_(int,GetTechSkill)(eTechSkills which) PURE;
   STDMETHOD_(int,GetWeaponSkill)(eWeaponSkills which) PURE;
   STDMETHOD_(BOOL,GetWeaponSkills)(ObjID obj, sWeaponSkills *wpn) PURE;
   STDMETHOD_(BOOL,HasTrait)(ObjID obj, eTrait which) PURE;
   STDMETHOD_(BOOL,HasTraitMulti)(eTrait which) PURE;

   STDMETHOD(AddStats)(sStatsDesc *p1, sStatsDesc *p2) PURE;
   STDMETHOD_(BOOL,GetStats)(ObjID obj, sStatsDesc *stats) PURE;

   STDMETHOD_(int,GetPool)(ObjID obj) PURE;
   STDMETHOD(AddPool)(ObjID obj, int quantity) PURE;
   STDMETHOD(SetPool)(ObjID obj, int value) PURE;

   // combat related
   STDMETHOD_(float, GetStimMultiplier)(ObjID gunID) PURE;
};

#undef INTERFACE

void DPCPlayerCreate();
EXTERN void DPCSwapGuns(void);

#define MAX_STAT_VAL 8
#define MAX_SKILL_VAL 6

#endif
