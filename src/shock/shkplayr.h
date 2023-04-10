/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkplayr.h,v 1.37 2000/01/31 09:58:30 adurant Exp $
#pragma once

#ifndef __SHKPLAYR_H
#define __SHKPLAYR_H

#include <objtype.h>
#include <osystype.h>
#include <label.h>
#include <propface.h>

typedef enum eStats;
typedef enum eTechSkills;
typedef enum eWeaponSkills;
typedef enum ePsiPowers;
typedef enum ePlayerEquip;
typedef enum eTrait;
typedef enum eImplant;

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

F_DECLARE_INTERFACE(IShockPlayer); 

#undef INTERFACE
#define INTERFACE IShockPlayer

DECLARE_INTERFACE_(IShockPlayer,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // returns the current cost to bump up a stat to the next level
   STDMETHOD_(int,StatCost)(int which) PURE;
   STDMETHOD_(int,WeaponSkillCost)(int which) PURE;
   STDMETHOD_(int,TechSkillCost)(int which) PURE;
   STDMETHOD_(int,PsiPowerCost)(int which) PURE;

   STDMETHOD_(BOOL, PsiLevel)(ObjID obj, int which) PURE;
   STDMETHOD_(BOOL, PsiPower)(ObjID obj, ePsiPowers which) PURE;
   STDMETHOD_(int, FindPsiLevel)(ePsiPowers which) PURE;
   STDMETHOD(AddPsiPower)(ObjID obj, ePsiPowers which) PURE;
   STDMETHOD_(BOOL, IsPsiTier)(ePsiPowers which) PURE;

   // is a >= b ?
   STDMETHOD_(BOOL,CheckStats)(sStatsDesc *a, sStatsDesc *b, BOOL spew) PURE;
   STDMETHOD_(BOOL,CheckWeaponSkills)(sWeaponSkills *a, sWeaponSkills *b, BOOL spew) PURE;

   STDMETHOD_(BOOL,Equip)(ObjID player, ePlayerEquip slot, ObjID o, BOOL spew) PURE;
   STDMETHOD(SetEquip)(ObjID player, ePlayerEquip slot, ObjID o) PURE;
   STDMETHOD_(ObjID,GetEquip)(ObjID player, ePlayerEquip slot) PURE;
   STDMETHOD(ClearEquip)(void) PURE;
   STDMETHOD(AlternateWeapons)(ObjID player) PURE;
   STDMETHOD_(void,SetWeaponModel)(ObjID o, BOOL sfx = TRUE) PURE;
   
   STDMETHOD_(BOOL,CheckRequirements)(ObjID o, BOOL spew) PURE;

   STDMETHOD_(ObjID,GetCurrentGun)(void) PURE;

   STDMETHOD(UseInternalModel)(void) PURE;
   STDMETHOD(UseExternalModel)(void) PURE;
   STDMETHOD_(char *,GetExternalModel)(void) PURE;

   STDMETHOD(RecalcData)(ObjID obj) PURE; 

   // look up data, 
   STDMETHOD_(int,GetStat)(eStats which) PURE;
   STDMETHOD_(int,GetTechSkill)(eTechSkills which) PURE;
   STDMETHOD_(int,GetWeaponSkill)(eWeaponSkills which) PURE;
   STDMETHOD_(BOOL,GetWeaponSkills)(ObjID obj, sWeaponSkills *wpn) PURE;
   STDMETHOD_(int,GetPsiSkill)(ePsiPowers which) PURE;
   STDMETHOD_(BOOL,HasTrait)(ObjID obj, eTrait which) PURE;
   STDMETHOD_(BOOL,HasTraitMulti)(eTrait which) PURE;
   STDMETHOD_(BOOL,HasImplant)(ObjID obj, eImplant which) PURE;

   STDMETHOD(AddStats)(sStatsDesc *p1, sStatsDesc *p2) PURE;
   STDMETHOD_(BOOL,GetStats)(ObjID obj, sStatsDesc *stats) PURE;

   STDMETHOD_(int,GetPool)(ObjID obj) PURE;
   STDMETHOD(AddPool)(ObjID obj, int quantity) PURE;
   STDMETHOD(SetPool)(ObjID obj, int value) PURE;

   // combat related
   STDMETHOD_(float, GetStimMultiplier)(ObjID gunID) PURE;
};

#undef INTERFACE

void ShockPlayerCreate();
EXTERN void ShockSwapGuns(void);

#define MAX_STAT_VAL 8
#define MAX_SKILL_VAL 6

#endif
