/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaipbs.h,v 1.1 1998/11/18 16:27:07 MROWLEY Exp $
//
//
//

#ifndef __SHKAIGBS_H
#define __SHKAIGBS_H

#include <aibasbhv.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockProxyBehaviorSet
//
// This class exists because Shock uses at least one non-core action type which
// needs to be sent over the network (currently its the ShootGun action).  This
// class implements CreateCustomProxyAction to create proxy shoot gun actions.

class cAIShockProxyBehaviorSet : public cAINetProxyBehaviorSet
{
public:
   //
   // Find out the behavior set name
   //
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, EnactProxyCustomAction)(tAIActionType type, IAI *pAI, void *netmsg);
};

#pragma pack()
#endif /* !__SHKAIGBS_H */






