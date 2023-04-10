/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiwr.cpp,v 1.5 1999/03/10 12:59:03 ccarollo Exp $
//
//
//

extern "C"
{
#include <wr.h>
#include <wrfunc.h>
#include <portal.h>
}

#include <physcast.h>
#include <mvrutils.h>

#include <objlight.h>
#include <objpos.h>
#include <aiwr.h>

// Must be last header
#include <dbmem.h>

float AIGetObjectLighting(ObjID id)
{
   return compute_object_lighting(id);
}

BOOL AIRaycast(const Location *start_loc, Location *end_loc,
               Location *hit_loc, unsigned flags)
{
   // @TODO: these should probably be hinted
   if ((start_loc->hint == CELL_INVALID) && (start_loc->cell == CELL_INVALID))
   {
      if (!(flags & kAIR_NoHintWarn))
         Warning(("Hinting AI raycast\n"));
      ComputeCellForLocation((Location *)start_loc);
      if (start_loc->cell == CELL_INVALID)
      {
         *hit_loc = *start_loc;
         return FALSE;
      }
   }
   
   if (flags & kAIR_VsObjects)
   {
      BOOL  retval;
      ObjID dummyObj;

      PhysRaycastSetObjlist(standableObjectList, standableObjectList.Size());
      retval = PhysRaycast(*(Location *)start_loc, *end_loc, hit_loc, &dummyObj, 0.0, kCollideAll);
      PhysRaycastClearObjlist();
      
      return (retval == kCollideNone);
   }      
   else
      return PortalRaycast((Location *)start_loc, end_loc, hit_loc, (flags & kAIR_ZeroEpsilon));
}

BOOL AIIsInWorld(ObjID id)
{
   return (ComputeCellForLocation(&(ObjPosGet(id)->loc))!=CELL_INVALID);
}
