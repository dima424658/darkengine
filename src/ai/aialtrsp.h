/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aialtrsp.h,v 1.1 1998/08/11 11:03:16 TOML Exp $
//
//
//

#ifndef __AIALTRSP_H
#define __AIALTRSP_H

#include <aiapicmp.h>
#include <aitrig.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitAlertResponseAbility(IAIManager *);
BOOL AITermAlertResponseAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIAlertResponse
//

class cAIAlertResponse : public cAITriggeredPScripted
{
public:
   cAIAlertResponse();
   ~cAIAlertResponse();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIALTRSP_H */
