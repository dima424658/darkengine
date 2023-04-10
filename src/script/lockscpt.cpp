/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/lockscpt.cpp,v 1.3 1998/01/19 22:21:01 dc Exp $

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <lockscpt.h>
#include <lockprop.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Locked Script Service

DECLARE_SCRIPT_SERVICE_IMPL(cLockedSrv, Locked)
{
public:
   STDMETHOD_(BOOL, IsLocked)(const object &sobj)
   {
      ObjID obj = ScriptObjID(sobj);
      return ObjComputeLockedState(obj);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cLockedSrv, Locked);










