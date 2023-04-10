/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrfle.h,v 1.2 1999/04/16 17:02:49 JON Exp $
//
//
//

#ifndef __AICBRFLE_H
#define __AICBRFLE_H

#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

typedef struct sAIRangedFleeParams
{
   eAIRangedModeApplicability m_rangeApplicability[2];
   float m_angleRange;
   int m_numPoints;
   float m_distance;
   float m_clearance;
} sAIRangedFleeParams;

///////////////////////////////////////////////////////////////////////////////

class cAIRangedFlee: public cAIRangedMode
{
public:
   cAIRangedFlee(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int SuggestInterrupt(void);
   virtual BOOL CheckPreconditions(void);
   virtual cAIAction* SuggestAction(void); 

private:
   cAIRangedLocation* m_pTargetAILoc;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedFlee::cAIRangedFlee(cAINewRangedSubcombat* pOwner): cAIRangedMode(pOwner), m_pTargetAILoc(NULL) {}

inline eAIRangedModeID cAIRangedFlee::GetModeID(void) const {return kAIRC_FleeMode;}
inline const char* cAIRangedFlee::GetName(void) const {return "Flee";}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRFLE_H */


