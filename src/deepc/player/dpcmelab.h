/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////
#pragma once

#ifndef __DPCMELAB_H
#define __DPCMELAB_H

#ifndef __PLYABLTY_H
#include <plyablty.h>
#endif // !__PLYABLTY_H

class cPlayerMeleeAbility : public cPlayerAbility
{
public:
   cPlayerMeleeAbility(const sPlayerAbilityDesc *pDesc=NULL);
   virtual ~cPlayerMeleeAbility() {}

   virtual int GetNextSkillID(int skillID, ePlayerInput input, tPlayerActionType actType);

   virtual void SetupSkill(int skillID);

   virtual BOOL GetSoundSchemaName(int skillID,Label *pName);

   ///////

   void SetWeaponType(int wtype) { m_WeaponType=wtype; }
   int  GetWeaponType() const { return m_WeaponType; }

private:
   int m_WeaponType;
};

extern cPlayerMeleeAbility g_meleeAbility;

#endif // __DPCMELAB_H