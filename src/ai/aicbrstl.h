/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrstl.h,v 1.2 1999/04/16 17:03:18 JON Exp $
//
//
//

#ifndef __AICBRSTL_H
#define __AICBRSTL_H

#include <aibasact.h>
#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedStepLeft: public cAIRangedMode
{
public:
   cAIRangedStepLeft(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int GetPriority(void);
   virtual BOOL CheckPreconditions(void);
   virtual cAIAction* SuggestAction(void); 
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedStepLeft::cAIRangedStepLeft(cAINewRangedSubcombat* pOwner): cAIRangedMode(pOwner) {SetFlags(kAIRC_MoveMode);}
 
inline eAIRangedModeID cAIRangedStepLeft::GetModeID(void) const {return kAIRC_LeftMode;}
inline const char* cAIRangedStepLeft::GetName(void) const {return "Left";}

inline int cAIRangedStepLeft::GetPriority(void) {return kAIRC_PriNormal;}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRSTL_H */


