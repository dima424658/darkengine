/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrstr.h,v 1.2 1999/04/16 17:03:22 JON Exp $
//
//
//

#ifndef __AICBRSTR_H
#define __AICBRSTR_H

#include <aibasact.h>
#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedStepRight: public cAIRangedMode
{
public:
   cAIRangedStepRight(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int GetPriority(void);
   virtual BOOL CheckPreconditions(void);
   virtual cAIAction* SuggestAction(void); 
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedStepRight::cAIRangedStepRight(cAINewRangedSubcombat* pOwner): cAIRangedMode(pOwner) {SetFlags(kAIRC_MoveMode);}
 
inline eAIRangedModeID cAIRangedStepRight::GetModeID(void) const {return kAIRC_RightMode;}
inline const char* cAIRangedStepRight::GetName(void) const {return "Right";}

inline int cAIRangedStepRight::GetPriority(void) {return kAIRC_PriNormal;}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRSTR_H */


