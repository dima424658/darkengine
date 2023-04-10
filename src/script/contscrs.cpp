/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/contscrs.cpp,v 1.4 1998/12/14 22:02:15 XEMU Exp $

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <contscrs.h>
#include <appagg.h>

// must be last header
#include <dbmem.h>

//
// Contain script service proper
//

DECLARE_SCRIPT_SERVICE_IMPL(cContainSrv,Container)
{
   IContainSys* pContainSys; 

   STDMETHOD_(void,Init)()
   {
      pContainSys = AppGetObj(IContainSys); 
   }

   STDMETHOD_(void,End)()
   {
      SafeRelease(pContainSys);
   }


   STDMETHOD(Add)(object obj, object container, int type, int flags) 
   {
      return pContainSys->Add(container,obj,type,flags); 
   }

   STDMETHOD(Remove)(object obj, object container) 
   {
      return pContainSys->Remove(container,obj); 
   }

   STDMETHOD(MoveAllContents)(object src, object targ, int flags = CTF_COMBINE)
   {
      return pContainSys->MoveAllContents(targ,src,flags); 
   }

   STDMETHOD(StackAdd)(object src, int quantity) 
   {
      return pContainSys->StackAdd(src, quantity);
   }

   STDMETHOD_(eContainType,IsHeld)(object container, object containee) 
   {
      return pContainSys->IsHeld(container,containee);
   }
}; 

IMPLEMENT_SCRIPT_SERVICE_IMPL(cContainSrv, Container);

