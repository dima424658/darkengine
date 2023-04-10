/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidefend.h,v 1.7 1998/10/06 15:03:05 TOML Exp $
//
//
//

#ifndef __AIDEFEND_H
#define __AIDEFEND_H

#include <aiapicmp.h>
#include <aibasabl.h>
#include <aiutils.h>

#pragma pack(4)
#pragma once

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitDefendAbility(IAIManager *);
BOOL AITermDefendAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDefend 
//

class cAIDefend : public cAIComponent
{
public:
   cAIDefend();

   // Standard component methods
   STDMETHOD_(const char *, GetName)(void);
   STDMETHOD_(void, Init)(void);
   
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

private:

   void Seek();

   cAITimer m_Timer;
   BOOL     m_fDefending;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDEFEND_H */
