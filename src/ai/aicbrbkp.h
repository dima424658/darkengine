/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicbrbkp.h,v 1.3 1999/04/16 17:01:52 JON Exp $
//
//
//

#ifndef __AICBRBKP_H
#define __AICBRBKP_H

#include <aibasact.h>
#include <aicbrnew.h>
#include <aicbrmod.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIRangedBackup: public cAIRangedMode
{
public:
   cAIRangedBackup(cAINewRangedSubcombat* pOwner);

   virtual eAIRangedModeID GetModeID(void) const;
   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);
   virtual int GetPriority(void);
   virtual BOOL CheckPreconditions(void);
   virtual cAIAction* SuggestAction(void); 

private:
   cAIRangedLocation m_destAILoc;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIRangedBackup::cAIRangedBackup(cAINewRangedSubcombat* pOwner): 
   cAIRangedMode(pOwner),
   m_destAILoc(pOwner)
{
      SetFlags(kAIRC_MoveMode);
}
 
inline eAIRangedModeID cAIRangedBackup::GetModeID(void) const {return kAIRC_BackupMode;}
inline const char* cAIRangedBackup::GetName(void) const {return "Backup";}

inline int cAIRangedBackup::GetPriority(void) {return kAIRC_PriNormal;}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AICBRBKP_H */


