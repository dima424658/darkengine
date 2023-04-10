/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/keyscpt.cpp,v 1.5 1998/06/25 22:51:46 XEMU Exp $

#include <lg.h>

// COM fun
#include <appagg.h>
#include <aggmemb.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <keyscpt.h>
#include <keysys.h>
#include <keyprop.h>
#include <lockprop.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Locked Script Service

DECLARE_SCRIPT_SERVICE_IMPL(cKeySrv, Key)
{
public:
   STDMETHOD_(BOOL, TryToUseKey)(const object &key_object, const object &locked_object, eKeyUse how)
   {
      BOOL retval;
      ObjID key_obj = ScriptObjID(key_object);
      ObjID lock_obj = ScriptObjID(locked_object);
      AutoAppIPtr(KeySys);

      retval = pKeySys->TryToUseKey(key_obj, lock_obj, how);
      return(retval);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cKeySrv, Key);










