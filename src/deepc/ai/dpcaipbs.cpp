/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//cAIDPCProxyBehaviorSet implementation

#include <dpcaipbs.h>

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <aiacttyp.h>
#include <aigunact.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIDPCProxyBehaviorSet
//

STDMETHODIMP_(const char *) cAIDPCProxyBehaviorSet::GetName()
{
   return "CustomNetProxy";
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIDPCProxyBehaviorSet::EnactProxyCustomAction(tAIActionType type, IAI *pAI, 
                                                                     void *netmsg)
{
   if (type != kAIAT_FireGun)
      Warning(("Unknown action type received in net message\n"));
   else
   {
      cAIGunAction::EnactProxyShootGun(pAI, netmsg);
   }
}

