/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCAICBR_H
#define __DPCAICBR_H

#include <objtype.h>
#include <aicbrnew.h>
#include <aicbrtyp.h>
#include <aimultcb.h>

////////////////////////////////////////////////////////////////////////////////
//
// Physcast callback
//

class cPhysModel;

extern BOOL DPCRangedPhyscastCallback(ObjID objID, const cPhysModel* pModel, sAIRangedCombatPhyscastData* pData);

class cAIDPCRangedSubcombat: public cAINewRangedSubcombat
{
public:
   cAIDPCRangedSubcombat(void);

   STDMETHOD_(void, Init)();
};

//////////////////////////////////////

inline cAIDPCRangedSubcombat::cAIDPCRangedSubcombat(void):
   cAINewRangedSubcombat()
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCCombatRanged
//

class cAIDPCCombatRanged : public cAISimpleMultiCombat
{
public:
   cAIDPCCombatRanged();

private:
   cAIDPCRangedSubcombat m_RangedSubcombat;
};

///////////////////////////////////////

inline cAIDPCCombatRanged::cAIDPCCombatRanged()
{
   m_RangedSubcombat.SetPhyscastCallback(tPhyscastObjTest(DPCRangedPhyscastCallback));
   SetSubcombat(&m_RangedSubcombat);
}

///////////////////////////////////////////////////////////////////////////////


#endif __DPCAICBR_H