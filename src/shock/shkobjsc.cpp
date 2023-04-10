/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkobjsc.cpp,v 1.1 1998/08/30 18:42:28 JON Exp $

#include <shkobjsc.h>

#include <appagg.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <traitman.h>
#include <traitbas.h>
#include <propman.h>
#include <objquery.h>
#include <scrptpr_.h>

// must be last header
#include <dbmem.h>

DECLARE_SCRIPT_SERVICE_IMPL(cShockObjSrv, ShockObj)
{
public:
   STDMETHOD_(ObjID, FindScriptDonor)(ObjID objID, string name)
   {
      sScriptProp *pScriptProp;
      ObjID donorID = OBJ_NULL;

      AutoAppIPtr(TraitManager);
      IObjectQuery *pQuery = pTraitManager->Query(objID, kTraitQueryAllDonors);
      AutoAppIPtr(PropertyManager);
      // no #define for script property name
      IScriptProperty *pScriptProperty = (IScriptProperty*)(pPropertyManager->GetPropertyNamed("Scripts"));
      while (!pQuery->Done() && (donorID == OBJ_NULL))
      {
         if (pQuery->Object() != objID)
            if (pScriptProperty->Get(pQuery->Object(), &pScriptProp))
            {
               for (int i=0; i<SCRIPTS_PER_PROP; i++)
                  if (name == string(pScriptProp->script[i]))
                     donorID = pQuery->Object();
            }
         pQuery->Next();
      }
      SafeRelease(pQuery);
      return donorID;
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cShockObjSrv, ShockObj);
