/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrsht.h,v 1.5 1999/04/22 17:56:09 JON Exp $
//
//
//

#ifndef __AICBRSHT_H
#define __AICBRSHT_H

#include <aicbrnew.h>
#include <aicbrmod.h>
#include <fix.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

typedef struct sAIRangedShootParams 
{
   eAIRangedModeApplicability m_rangeApplicability[kAIRC_NumRanges];
   BOOL m_confirmRange;
   BOOL m_confirmLOF;
   fix m_rotationSpeed;
   mxs_vector m_launchOffset; // this doesn't really belong here, but its convenient
} sAIRangedShootParams;

///////////////////////////////////////////////////////////////////////////////

class cAIRangedShoot: public cAIRangedMode
{
public:
   cAIRangedShoot(cAINewRangedSubcombat* pOwner);
   virtual void Reset(void);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int SuggestInterrupt(void);
   virtual BOOL CheckPreconditions(void);
   virtual BOOL CheckContinuation(void);
   virtual cAIAction* SuggestAction(void); 

private:
   ObjID             projectile;
   sAIProjectileRel *proj_link_data;
   mxs_vector        targeting_location;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedShoot::cAIRangedShoot(cAINewRangedSubcombat* pOwner): 
   proj_link_data(NULL),
   cAIRangedMode(pOwner) 
{}

inline eAIRangedModeID cAIRangedShoot::GetModeID(void) const {return kAIRC_ShootMode;}
inline const char* cAIRangedShoot::GetName(void) const {return "Shoot";}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRSHT_H */


