/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//
//

#ifndef __AISUSRSP_H
#define __AISUSRSP_H

#include <aiapicmp.h>
#include <aitrig.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitSuspiciousResponseAbility(IAIManager *);
BOOL AITermSuspiciousResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISuspiciousResponse
//

class cAISuspiciousResponse : public cAITriggeredPScripted
{
public:
   cAISuspiciousResponse();
   ~cAISuspiciousResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnFoundSuspicious)(ObjID suspobj);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AISUSRSP_H */
