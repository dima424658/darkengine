/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//

#ifndef __DPCAICRB_H
#define __DPCAICRB_H

#include <aicbrbkp.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cAIDPCRangedBackup: public cAIRangedBackup
{
public:
   cAIDPCRangedBackup(cAINewRangedSubcombat* pOwner);

   virtual const char* GetName(void) const;

   virtual int SuggestApplicability(void);

private:
   mxs_vector m_backupVec;
};
 
///////////////////////////////////////////////////////////////////////////////

inline cAIDPCRangedBackup::cAIDPCRangedBackup(cAINewRangedSubcombat* pOwner): cAIRangedBackup(pOwner) {}

inline const char* cAIDPCRangedBackup::GetName(void) const {return "DPC Backup";}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif // !__DPCAICRB_H


