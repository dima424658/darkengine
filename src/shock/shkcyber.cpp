/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcyber.cpp,v 1.7 2000/02/19 13:20:10 toml Exp $

#include <appagg.h>

#include <lresname.h>
#include <schema.h>
#include <scrptpr_.h>

#include <shkprop.h>
#include <shkutils.h>
#include <shkgame.h>
#include <shkcurm.h>
#include <shkhrm.h>
#include <shkobjst.h>
#include <shkgunpr.h>
#include <shkrsrch.h>

#include <shkcyber.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

//--------------------------------------------------------------------------------------
void ShockCyberCursor(void)
{
   IRes *hnd;

   ClearCursor();
   shock_cursor_mode = SCM_CYBER;
   hnd = LoadPCX("cybercur","iface\\");
   SetCursorByHandle(hnd); 
   SafeFreeHnd(&hnd);
}
//--------------------------------------------------------------------------------------
// what happens when the cybercursor is clicked on an object
void ShockCyberObject(ObjID obj)
{
   // this determines which kind of MFD to bring up, etc.
   if (ObjGetObjState(obj) == kObjStateUnresearched)
   {
      // deal with researching it
      ShockResearch(obj);
   }
   else if (ObjGetObjState(obj) == kObjStateBroken)
   {
      if (gPropRepairDiff->IsRelevant(obj))
      {
         ShockHRMDisplay(obj,1); // modify
      }
      else
         Warning(("ShockCyberObject: obj %d is broken, but has no repair difficulty!\n",obj));
   }
   else
   {
      if (IsPlayerGun(obj) && gPropModifyDiff->IsRelevant(obj))
      {
         ShockHRMDisplay(obj,2); // modify
      }
      else if (gPropHackDiff->IsRelevant(obj))
      {
         // need to filter on whether hackable or not... property?
         ShockHRMDisplay(obj,0); // hack
      }
   }
}
//--------------------------------------------------------------------------------------
