/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//

#ifndef __DPCAIGBS_H
#define __DPCAIGBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCProxyBehaviorSet
//
// This class exists because Deep Cover uses at least one non-core action type which
// needs to be sent over the network (currently its the ShootGun action).  This
// class implements CreateCustomProxyAction to create proxy shoot gun actions.

class cAIDPCProxyBehaviorSet : public cAINetProxyBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, EnactProxyCustomAction)(tAIActionType type, IAI *pAI, void *netmsg);
};

#pragma pack()
#endif /* !__DPCAIGBS_H */






