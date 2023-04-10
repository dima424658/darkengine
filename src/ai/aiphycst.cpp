/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiphycst.cpp,v 1.3 2000/03/06 17:19:22 toml Exp $
//

#include <aitype.h>
#include <aiphycst.h>
#include <aiavoid.h>
#include <aipthobb.h>

///////////////////////////////////////////////////////////////////////////////

#define kNumFilterArrays 3
static cDynArray<ObjID> _g_Filters[kNumFilterArrays];

///////////////////////////////////////

void AIPhysRaycastSetObjFilter(eAIPRObjFilter filter)
{
   if (filter == kAIPR_Any)
   {
      PhysRaycastClearObjlist();
   }
   else
   {
      AssertMsg(filter >= 0 && filter < kNumFilterArrays, "Invalid object filter in AIPhysRaycast");
      PhysRaycastSetObjlist(_g_Filters[filter], _g_Filters[filter].Size());
   }
}

///////////////////////////////////////

void AIPhysRaycastClearObjFilter()
{
   PhysRaycastClearObjlist();
}

///////////////////////////////////////

void AIPhysCastCollectObjects()
{
   cDynArray<sAIObjectAvoidMark> blockings;
   cAIObjCells                   pathables;
   
   AIGetAllObjMarkDB(&blockings);
   AIGetAllPathableOBBCells(&pathables);

   _g_Filters[kAIPR_Pathables].SetSize(0);
   _g_Filters[kAIPR_Pathblocking].SetSize(0);
   _g_Filters[kAIPR_PathblockingOrPathable].SetSize(0);
   
   int i, j;
   
   for (i = 0; i < blockings.Size(); i++)
   {
      _g_Filters[kAIPR_Pathblocking].Append(blockings[i].object);
      _g_Filters[kAIPR_PathblockingOrPathable].Append(blockings[i].object);
   }
   
   for (i = 0; i < pathables.Size(); i++)
   {
      _g_Filters[kAIPR_Pathables].Append(pathables[i].objID);
      for (j = 0; j < _g_Filters[kAIPR_PathblockingOrPathable].Size(); j++)
      {
         if (_g_Filters[kAIPR_PathblockingOrPathable][j] == pathables[i].objID)
            break;
      }
      
      if (j == _g_Filters[kAIPR_PathblockingOrPathable].Size())
         _g_Filters[kAIPR_PathblockingOrPathable].Append(pathables[i].objID);
   }
}

///////////////////////////////////////

int AIPhysRaycast(Location &start, Location &end, Location *hit, ObjID *hit_obj, mxs_real radius, int filter)
{
   return PhysRaycast(start, end, hit, hit_obj, radius, filter);
}

///////////////////////////////////////////////////////////////////////////////
