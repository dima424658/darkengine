/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/pupscrpt.cpp,v 1.4 1998/10/05 17:27:26 mahk Exp $

#include <comtools.h>
#include <appagg.h>
#include <cfgdbg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <pupscrpt.h>

// @Note (toml 05-25-98): E3 hack follows...
#include <aiscrabl.h>

#include <cbllmapi.h>

// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
//
// BODY SCRIPTING SERVICE IMPLEMENTATION
//

DECLARE_SCRIPT_SERVICE_IMPL(cPuppetSrv, Puppet)
{
public:
   STDMETHOD_(boolean,PlayMotion)(const object obj, const char *name)
   {

      ObjID objID=ScriptObjID(obj);
      ICerebellum *pCbllm;

      cAIBasicScript * pScriptAbility = cAIBasicScript::AccessBasicScript(objID);
      if (pScriptAbility)
         return pScriptAbility->PlayMotion(name);

      ConfigSpew("PuppetTrace",("me playing %s on %d\n",name,objID));

      if(NULL!=(pCbllm=CbllmGetFromObj(objID)))
      {
         pCbllm->PlayMotion(name);
      }
      return TRUE;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cPuppetSrv, Puppet);


