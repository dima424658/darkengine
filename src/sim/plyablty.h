/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyablty.h,v 1.9 2000/01/31 10:00:17 adurant Exp $
#pragma once

#ifndef __PLYABLTY_H
#define __PLYABLTY_H

#include <label.h>
#include <plyrbase.h>
#include <matrixs.h>
#include <mschbase.h> // for sMPlayerSkillData

#define kSkillInvalid -1


// this is defined in plyablt_.h
typedef struct sPlayerSkillFullDesc sPlayerSkillFullDesc;

typedef struct sPlayerAbilityDesc
{
   Label modelName;
   int   creatureType;
   int   startSkillID;
   int   nSkills;
   sPlayerSkillFullDesc *pSkills;
} sPlayerAbilityDesc;

#ifdef __cplusplus

#include <ctagset.h>

// A player ability is a high-level thing the player can do, like
// wield a sword, lock-pick, or fire a bow.  Skills are the atomic
// chunks which comprise an ability, like "aim bow", "flex", "fire",
// "return to rest".
// This terminology is kind of goofy and at odds with what the AI
// thinks of as skills etc, but hey, it's there, and I started using the
// words first.      KJ 4/98

class cPlayerAbility
{
public:
   cPlayerAbility(const sPlayerAbilityDesc *pDesc=NULL);
   virtual ~cPlayerAbility();

   // stuff to let player ability do what it wants
   virtual void Begin();
   virtual void End();
   virtual void SetupSkill(int skillID) { DefaultSetupSkill(skillID); }
   virtual void CleanupSkill(int skillID) { DefaultCleanupSkill(skillID); }
   virtual BOOL GetSoundSchemaName(int skillID,Label *pName);

   // information cerebellum needs to be able to glean from ability
   virtual BOOL GetModelName(Label **pName); 
   virtual BOOL SetModelName(Label *pName); 
   virtual BOOL GetCreatureType(int *pType);
   virtual int GetStartSkillID();
   virtual int GetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType) { return DefaultGetNextSkillID(skillID,input,actType); }
   virtual BOOL IsIdle(int skillID);
   virtual ePlayerTransition GetTransitionType(int skillID);
   virtual BOOL GetMotionTags(int skillID, cTagSet *pTags);

   // to pass along to coordinator
   virtual BOOL GetCurControllerID(int *pID);
   // to pass along to motor controller
   virtual sMPlayerSkillData *GetCurSkillData();
   virtual int GetPowerUpTime() { return m_TimeToMax; }

   // for schema callback
   virtual int GetSoundSchemaHandle() { return m_sndSchHandle; }
   virtual void SetSoundSchemaHandle(int h) { m_sndSchHandle=h; }
protected:
   void DefaultSetupSkill(int skillID);
   void DefaultCleanupSkill(int skillID);
   void DefaultInit(const sPlayerAbilityDesc *pDesc);
   int  DefaultGetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType);

   int m_CurSkillID;
   sPlayerAbilityDesc m_Desc;
   int m_sndSchHandle;
   float m_TimeToMax;
   BOOL m_UseGlobalOffset;
   mxs_vector m_PosOffset;
   mxs_angvec m_RotOffset;
   sMPlayerSkillData m_SkillData;
   cDynClassArray<cTagSet> m_TagSetList;
};

#endif // cplusplus


#endif
