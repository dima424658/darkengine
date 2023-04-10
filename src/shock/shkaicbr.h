/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkaicbr.h,v 1.3 2000/01/29 13:40:15 adurant Exp $
#pragma once

#ifndef __SHKAICBR_H
#define __SHKAICBR_H

#include <objtype.h>
#include <aicbrnew.h>
#include <aicbrtyp.h>
#include <aimultcb.h>

////////////////////////////////////////////////////////////////////////////////
//
// Physcast callback
//

class cPhysModel;

extern BOOL ShockRangedPhyscastCallback(ObjID objID, const cPhysModel* pModel, sAIRangedCombatPhyscastData* pData);

class cAIShockRangedSubcombat: public cAINewRangedSubcombat
{
public:
   cAIShockRangedSubcombat(void);

   STDMETHOD_(void, Init)();
};

//////////////////////////////////////

inline cAIShockRangedSubcombat::cAIShockRangedSubcombat(void):
   cAINewRangedSubcombat()
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockCombatRanged
//

class cAIShockCombatRanged : public cAISimpleMultiCombat
{
public:
   cAIShockCombatRanged();

private:
   cAIShockRangedSubcombat m_RangedSubcombat;
};

///////////////////////////////////////

inline cAIShockCombatRanged::cAIShockCombatRanged()
{
   m_RangedSubcombat.SetPhyscastCallback(tPhyscastObjTest(ShockRangedPhyscastCallback));
   SetSubcombat(&m_RangedSubcombat);
}

///////////////////////////////////////////////////////////////////////////////


#endif __SHKAICBR_H