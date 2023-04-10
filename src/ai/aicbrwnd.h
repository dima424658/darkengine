/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrwnd.h,v 1.2 1999/04/16 17:03:33 JON Exp $
//
//
//

#ifndef __AICBRWND_H
#define __AICBRWND_H

#include <aibasact.h>
#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedWound: public cAIRangedMode
{
public:
   cAIRangedWound(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int SuggestInterrupt(void);
   virtual int GetPriority(void);
   virtual cAIAction* SuggestAction(void); 
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedWound::cAIRangedWound(cAINewRangedSubcombat* pOwner): cAIRangedMode(pOwner) {}
 
inline eAIRangedModeID cAIRangedWound::GetModeID(void) const {return kAIRC_WoundMode;}
inline const char* cAIRangedWound::GetName(void) const {return "Wound";}

inline int cAIRangedWound::GetPriority(void) {return kAIRC_PriHigh;}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRWND_H */


