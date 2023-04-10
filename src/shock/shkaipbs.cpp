/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaipbs.cpp,v 1.2 1999/11/19 14:56:33 adurant Exp $
//
//
//

#include <shkaipbs.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <aiacttyp.h>
#include <aigunact.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIShockProxyBehaviorSet
//

STDMETHODIMP_(const char *) cAIShockProxyBehaviorSet::GetName()
{
   return "CustomNetProxy";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIShockProxyBehaviorSet::EnactProxyCustomAction(tAIActionType type, IAI *pAI, 
                                                                     void *netmsg)
{
   if (type != kAIAT_FireGun)
      Warning(("Unknown action type received in net message\n"));
   else
   {
      cAIGunAction::EnactProxyShootGun(pAI, netmsg);
   }
}

