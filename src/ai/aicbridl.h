/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbridl.h,v 1.4 1999/12/09 20:10:25 BODISAFA Exp $
//
//
//

#ifndef __AICBRIDL_H
#define __AICBRIDL_H

#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

////////////////////////////////////////////////////////////////////////////////
//
// Notes: Idle must guarantee to always be applicable and pre-conditions TRUE
//

class cAIRangedIdle: public cAIRangedMode
{
public:
   cAIRangedIdle(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int GetPriority(void);
   virtual cAIAction* SuggestAction(void);
   
   virtual void OnGainingControl(void);
private:
};

/////////////////////////////////////////////////////////

inline cAIRangedIdle::cAIRangedIdle(cAINewRangedSubcombat* pOwner): cAIRangedMode(pOwner) {}

inline eAIRangedModeID cAIRangedIdle::GetModeID(void) const {return kAIRC_IdleMode;}
inline const char* cAIRangedIdle::GetName(void) const {return "Idle";}

inline int cAIRangedIdle::SuggestApplicability(void) {return 0;}
inline int cAIRangedIdle::GetPriority(void) {return kAIRC_PriMinimum;}

/////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBTRNG_H */


