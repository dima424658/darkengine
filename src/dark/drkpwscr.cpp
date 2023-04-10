/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkpwscr.cpp,v 1.3 2000/02/19 12:27:24 toml Exp $
//

#include <lg.h>

// script headers
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>
#include <objscrpt.h>

// drk powerups
#include <drkpwups.h>
#include <drkpwscr.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

DECLARE_SCRIPT_SERVICE_IMPL(cDarkPowerupsSrv, DrkPowerups)
{
public:
   // cause a world flash to trigger
   STDMETHOD_(void,TriggerWorldFlash)(object flash_obj)
   {
      ObjID obj = ScriptObjID(flash_obj);
      DrkTriggerWorldFlash(obj);
   }

   // try to deploy an arch at src_obj, returns success
   // WILL go ahead and create the arch_obj if there is room
   STDMETHOD_(BOOL,ObjTryDeploy)(object src_object, object deploy_arch)
   {
      ObjID src_obj  = ScriptObjID(src_object);
      ObjID arch_obj = ScriptObjID(deploy_arch);
      return DrkObjTryDeploy(src_obj,arch_obj);
   }

   // hack for now to allow cleaning up nearby blood
   STDMETHOD_(void,CleanseBlood)(object water_src_object, float rad)
   {
      ObjID water_src_obj  = ScriptObjID(water_src_object);
      TryCleanBlood(water_src_obj,rad);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDarkPowerupsSrv, DrkPowerups);
