/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaicrb.h,v 1.2 1999/04/16 17:06:46 JON Exp $
//
//
//

#ifndef __SHKAICRB_H
#define __SHKAICRB_H

#include <aicbrbkp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIShockRangedBackup: public cAIRangedBackup
{
public:
   cAIShockRangedBackup(cAINewRangedSubcombat* pOwner);

   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);

private:
   mxs_vector m_backupVec;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIShockRangedBackup::cAIShockRangedBackup(cAINewRangedSubcombat* pOwner): cAIRangedBackup(pOwner) {}

inline const char* cAIShockRangedBackup::GetName(void) const {return "Shock Backup";}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAICRB_H */


