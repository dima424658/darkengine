/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#ifndef __DPCAIDTH_H
#define __DPCAIDTH_H

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

#endif /* !__DPCAIDTH_H */
