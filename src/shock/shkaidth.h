/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaidth.h,v 1.2 1999/11/19 14:52:34 adurant Exp $
//
//

This file has been moved to aiqdeath.h AMSD


#ifndef __SHKAIDTH_H
#define __SHKAIDTH_H

#include <aibasabl.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIQuickDeath
//

class cAIQuickDeath : public cAIAbility
{
public:   
   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   // Notifications
   STDMETHOD_(void, OnDeath)(const sDamageMsg * pMsg);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIDTH_H */
