/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_room.cpp,v 1.24 1999/12/01 09:25:43 MAT Exp $
// room database gediting

#include <lg.h>
#include <iobjsys.h>
#include <objsys.h>
#include <objtype.h>
#include <objpos.h>
#include <objquery.h>

#include <editobj.h>   // gCreateEditBrush
#include <traitman.h>
#include <traitprp.h>
#include <traitbas.h>
#include <rendprop.h>

#include <brlist.h>
#include <brinfo.h>
#include <brrend.h>
#include <editbr_.h>
#include <editobj.h>

#include <gedit.h>
#include <ged_room.h>
#include <ged_line.h>

#include <roomsys.h>
#include <roomutil.h>
#include <roomeax.h>
#include <roomprop.h>
#include <rooms.h>
#include <room.h>
#include <rportal.h>
#include <doorphys.h>
#include <mprintf.h>
#include <roomtowr.h>
#include <psnd.h>

#include <config.h>
#include <command.h>
#include <hilight.h>
#include <wr.h>
#include <vbrush.h>

// Must be last header
#include <dbmem.h>

static ged_default_room = -1;

void ged_compile_rooms(void)
{
   ConvertRoomBrushes();
   RoomToWRProcess();
}

void ged_hilight_room(int roomID)
{
   editBrush *cur_edit_brush;
   int iter;

   hilight_begin();

   cur_edit_brush = blistIterStart(&iter);
   while (cur_edit_brush != NULL)
   {
      if (brushGetType(cur_edit_brush) == brType_ROOM)
      {
         if (brRoom_InternalRoomID(cur_edit_brush)  == roomID)
         {
            hilightAddByBrushId(cur_edit_brush->br_id);
            break;
         }
      }

      cur_edit_brush = blistIterNext(iter);
   }

   hilight_end(kHilightDone);
}

ObjID ged_get_room_named(char *name)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);

   return pObjSys->GetObjectNamed(name);
}

ObjID ged_create_room_obj(char *name, ObjID archetype, BOOL concrete)
{
   AutoAppIPtr_(TraitManager, pTM);
   ObjID new_room;

   if (concrete)
   {
      gCreateEditBrush = FALSE;
      new_room = pTM->CreateConcreteArchetype(name, archetype);
      gCreateEditBrush = TRUE;   
   }
   else
   {
      new_room = pTM->CreateArchetype(name, archetype);
   }

   return new_room;
}

void ged_make_room_obj(ObjID objID)
{
   if (OBJ_IS_CONCRETE(objID))
      ObjSetRenderType(objID, kRenderNotAtAll);
}

ObjID ged_room_get_root(void)
{
   ObjID rootRoom;

   // Do we have a root room?
   rootRoom = ged_get_room_named(ROOT_ROOM_NAME);
   Assert_(rootRoom != OBJ_NULL); 

   return rootRoom;
}

ObjID ged_room_get_default(void)
{
   // Do we have a default room?
   ObjID defaultRoom = ged_get_room_named(DEFAULT_ROOM_NAME);
   Assert_(defaultRoom != OBJ_NULL);

   return defaultRoom;
}

void ged_room_postload(void)
{
   ObjID rootRoom = ged_get_room_named(ROOT_ROOM_NAME); 
   if (rootRoom == OBJ_NULL)
      rootRoom = ged_create_room_obj(ROOT_ROOM_NAME, ROOT_ARCHETYPE, FALSE);

   AutoAppIPtr_(TraitManager, pTM);
   pTM->SetArchetype(rootRoom, OBJ_NULL);
      
   // make sure rooms don't have brushes
   ObjSetCanHaveBrush(rootRoom,FALSE); 
   ObjSetRenderType(rootRoom, kRenderNotAtAll);

   ObjID defaultRoom = ged_get_room_named(DEFAULT_ROOM_NAME);
   if (defaultRoom == OBJ_NULL)
      defaultRoom = ged_create_room_obj(DEFAULT_ROOM_NAME, rootRoom, TRUE);

   ged_default_room = defaultRoom;

   // Oh boy is this dorky.  
   // go through all rooms and make sure none have brushes
   cAutoIPtr<IObjectQuery> query (pTM->Query(rootRoom,kTraitQueryAllDescendents));
   for (; !query->Done(); query->Next())
   {
      ObjID obj = query->Object(); 
      if (OBJ_IS_CONCRETE(obj))
         editObjDeleteObjBrush(obj); 
   }

}

BOOL ged_is_room(ObjID objID)
{
   AutoAppIPtr_(TraitManager, pTM);
   ObjID root_room = ged_room_get_root();
   BOOL value;

   value = pTM->ObjHasDonor(objID, root_room);

   return value;
}

void ged_room_set_default_arch(ObjID objID)
{
   Assert_(ged_is_room(objID));
   
   ged_default_room = objID;
}

ObjID ged_room_get_default_arch(void)
{
   return ged_default_room;
}

ObjID ged_room_get_parent(ObjID objID)
{
   AutoAppIPtr_(TraitManager, pTM);

   return pTM->GetArchetype(objID);
}

int ged_room_get_depth(ObjID objID)
{
   AutoAppIPtr_(TraitManager, pTM);
   ObjID parent;
   ObjID root_room = ged_room_get_root();
   int   depth = 1;

   if (pTM->ObjHasDonor(objID, root_room))
   {
      parent = pTM->GetArchetype(objID);
      while (parent != root_room)
      {
         depth++;
         parent = pTM->GetArchetype(parent);
      }
   }
   else
      return -1;

   return depth;
} 

void ged_room_draw_connections(short roomID, BOOL clear)
{
   if (clear)
      gedDeleteChannelLines(1 << LINE_CH_ROOM);

   if ((!g_RoomsOK) || (roomID >= g_pRooms->GetNumRooms()))
   {
      ged_compile_rooms();
      return;
   }

   if (roomID != -1)
   {
      ged_line_load_channel = LINE_CH_ROOM;
      ged_line_view_channels |= (1 << LINE_CH_ROOM);

      cRoom *pRoom = g_pRooms->GetRoom(roomID);

      if (pRoom != NULL)
      {
         cRoomPortal *pRPortal;

         for (int i=0; i<pRoom->GetNumPortals(); i++)
         {
            pRPortal = pRoom->GetPortal(i);
    
            gedLineAddPal(&pRoom->GetCenterPt(), &pRPortal->GetCenter(), 0, 127);
            gedLineAddPal(&pRPortal->GetCenter(), &pRPortal->GetFarRoom()->GetCenterPt(), 0, 127);
         }
      }
   }

   if (clear)
      gedit_full_redraw();
}

void ged_room_draw_all_connections(BOOL state)
{
   gedDeleteChannelLines(1 << LINE_CH_ROOM);

   if (!g_RoomsOK)
      ged_compile_rooms();

   brushConvertStart();
   if (state == TRUE)
   {
      editBrush *editbr;
      int        iter_handle;

      editbr = blistIterStart(&iter_handle);
      while (editbr != NULL)
      {
         if ((brushGetType(editbr) == brType_ROOM) && brushConvertTest(editbr))
            ged_room_draw_connections(brRoom_InternalRoomID(editbr), FALSE);
         editbr = blistIterNext(iter_handle);
      }
   }
   brushConvertEnd();
   gedit_full_redraw();
}

void FixRooms()
{
   editBrush *cur_edit_brush;
   int iter;

   cur_edit_brush = blistIterStart(&iter);
   while (cur_edit_brush != NULL)
   {
      if (brushGetType(cur_edit_brush) == brType_ROOM)
      {
         if (!ged_is_room(brRoom_ObjID(cur_edit_brush)))
            brRoom_ObjID(cur_edit_brush) = ged_room_get_default();
      }

      cur_edit_brush = blistIterNext(iter);
   }

   ConvertRoomBrushes();
}

////////////////////////////////////////

void CheckRooms()
{
   cRoom     *pRoom;
   cRoom     *pCurRoom;
   editBrush *cur_edit_brush;
   int count = 0;
   int iter;

   if (!g_RoomsOK)
      ConvertRoomBrushes();

   hilight_begin();

   cur_edit_brush = blistIterStart(&iter);
   while (cur_edit_brush != NULL)
   {
      if (brushGetType(cur_edit_brush) == brType_ROOM)
      {
         if (brRoom_InternalRoomID(cur_edit_brush) < g_pRooms->GetNumRooms())
            pRoom = g_pRooms->GetRoom(brRoom_InternalRoomID(cur_edit_brush));
         else
            pRoom = NULL;

         if (pRoom)
         {
            for (int i=0; i<g_pRooms->GetNumRooms(); i++)
            {
               pCurRoom = g_pRooms->GetRoom(i);

               if (pCurRoom == pRoom)
                  continue;

               if (pCurRoom->PointInside(pRoom->GetCenterPt()))
               {
                  count++;

                  hilightAddByBrushId(cur_edit_brush->br_id);
               }
            }
         }
      }
     
      cur_edit_brush = blistIterNext(iter);
   }

   if (count != 0)
   {
      hilight_end(kHilightDone);
      mprintf("Room brushes with centers inside other room brushes have been hilighted.\n");
      return;
   }

   cur_edit_brush = blistIterStart(&iter);
   while (cur_edit_brush != NULL)
   {
      if (brushGetType(cur_edit_brush) == brType_ROOM)
      {
         if (brRoom_InternalRoomID(cur_edit_brush) < g_pRooms->GetNumRooms())
            pRoom = g_pRooms->GetRoom(brRoom_InternalRoomID(cur_edit_brush));
         else
            pRoom = NULL;

         if (pRoom)
         {
            for (int i=0; i<pRoom->GetNumPortals(); i++)
            {
               Location loc;
               MakeLocationFromVector(&loc, &pRoom->GetPortal(i)->GetCenter());

               if (CellFromLoc(&loc) == CELL_INVALID)
               {
                  count++;
  
                  hilightAddByBrushId(cur_edit_brush->br_id);
               }
            }
         }
      }
     
      cur_edit_brush = blistIterNext(iter);
   }

   if (count != 0)
   {
      hilight_end(kHilightDone);
      mprintf("Room brushes with portal centers outside the world have been hilighted.\n");
      return;
   }
   else
      hilight_end(kHilightCancel);
}


////////////////////////////////////////////////////////////////////////////////
//
// Convert the level's roombrushes to rooms and portals
//

static cDynArray<editBrush *> hb_list;
static cDynArray<editBrush *> obj_list;

BOOL ConvertRoomBrushes(void)
{
   editBrush   *cur_edit_brush;
   tOBB         obb1, obb2;
   cRoom       *pRoom;
   cRoom       *pRoomNear, *pRoomFar;
   cRoomPortal *pRoomPortal1, *pRoomPortal2;
   tPlane       portal_plane;
   tPlane       portal_edge_list[8];
   tPlane       plane_list[6];
   mxs_vector   obb_sum_scale;
   mxs_real     obbs_dist2;
   int          portal_edge_size;
   mxs_vector   portal_center;
   int          iter;
   int          num_portals;
   int          i, j;

   AutoAppIPtr(ObjectSystem);

   // Clear out any existing rooms
   ClearRooms();

   // Build list of room brushes & objects
   brushConvertStart();
   cur_edit_brush = blistIterStart(&iter);
   while (cur_edit_brush != NULL)
   {
      if (brushConvertTest(cur_edit_brush))
      {
         if (brushGetType(cur_edit_brush) == brType_ROOM)
            hb_list.Append(cur_edit_brush);

         if (brushGetType(cur_edit_brush) == brType_OBJECT)
            obj_list.Append(cur_edit_brush);
      }

      cur_edit_brush = blistIterNext(iter);
   }
   brushConvertEnd();

   // Bail if no rooms
   if (hb_list.Size() == 0)
   {
      obj_list.SetSize(0);
      return TRUE;
   }

   mprintf("Building rooms...\n");

   num_portals = 0;
   for (i=0; i<hb_list.Size(); i++)
   {
      // Convert first edit brush into OBB
      mx_copy_vec(&obb1.pos.loc.vec, &hb_list[i]->pos);
      mx_copy_vec(&obb1.scale, &hb_list[i]->sz);
      obb1.pos.fac = hb_list[i]->ang;

      // Create the room
      GetOBBPlanes(obb1, plane_list);      

      pRoom = new cRoom(pObjectSystem->GetName(brRoom_ObjID(hb_list[i])), 
                        brRoom_ObjID(hb_list[i]), plane_list, hb_list[i]->pos);
      g_pRooms->AddRoom(pRoom);

      brRoom_InternalRoomID(hb_list[i]) = pRoom->GetRoomID();

      // Check for portals 
      for (j=0; j<i; j++)
      {
         // Convert other edit brush into OBBs
         mx_copy_vec(&obb2.pos.loc.vec, &hb_list[j]->pos);
         mx_copy_vec(&obb2.scale, &hb_list[j]->sz);
         obb2.pos.fac = hb_list[j]->ang;

         // Check bounding sphere intersection
         obbs_dist2 = mx_dist2_vec(&obb1.pos.loc.vec, &obb2.pos.loc.vec);
         mx_add_vec(&obb_sum_scale, &obb1.scale, &obb2.scale);
         if (obbs_dist2 > (mx_mag2_vec(&obb_sum_scale)))
            continue;

         // Check for intersection
         if (OBBsIntersect(obb1, obb2))
         {
            cRoom *i_Room = g_pRooms->GetRoom(brRoom_InternalRoomID(hb_list[i]));
            cRoom *j_Room = g_pRooms->GetRoom(brRoom_InternalRoomID(hb_list[j]));

            // Build the portal between them
            FindOBBPortal(obb1, obb2, &portal_plane, portal_edge_list, &portal_edge_size, &portal_center);

            // Ignore degenerate intersections (edge-edge, etc)
            if (portal_edge_size == 0)
               continue;

            // Determine near and far rooms
            if (PointPlaneDist(portal_plane, i_Room->GetCenterPt()) < ON_PLANE_EPSILON)
            {
               pRoomNear = i_Room;
               pRoomFar  = j_Room;
            }
            else
            {
               pRoomNear = j_Room;
               pRoomFar  = i_Room;
            }

            // Add portal to first room
            pRoomPortal1 = new cRoomPortal(portal_plane, portal_edge_list, portal_edge_size, 
                                           portal_center, pRoomNear, pRoomFar);
            pRoomNear->AddPortal(pRoomPortal1);
            g_pRooms->AddPortal(pRoomPortal1);

            // Flip the normal
            mx_scaleeq_vec(&portal_plane.normal, -1.0);
            portal_plane.d *= -1.0;

            // Add portal to second room
            pRoomPortal2 = new cRoomPortal(portal_plane, portal_edge_list, portal_edge_size, 
                                           portal_center, pRoomFar, pRoomNear);
            pRoomFar->AddPortal(pRoomPortal2);
            g_pRooms->AddPortal(pRoomPortal2);

            pRoomPortal1->SetFarPortal(pRoomPortal2);
            pRoomPortal2->SetFarPortal(pRoomPortal1);

            num_portals += 2;
         }
      }
   }

   // Add any objects in the room
   for (i=0; i<obj_list.Size(); i++)
      g_pRooms->FindObjRoom(brObj_ID(obj_list[i]), ObjPosGet(brObj_ID(obj_list[i]))->loc.vec);

   mprintf(" %d rooms, %d portals, %g average portals per room\n", hb_list.Size(), 
           num_portals, ((float)num_portals) / ((float)hb_list.Size()));

   hb_list.SetSize(0);
   obj_list.SetSize(0);

   // Build EAX sound properties
   g_pRoomEAX->Build();

   g_pPropSnd->ClearBlockingFactors();
   GenerateAllDoorRooms();

   g_RoomsOK = TRUE;

   if (config_is_defined("check_rooms"))
      CheckRooms();

   return FALSE;
}

static Command g_RoomCommands[] = 
{
   {"rooms_build",     FUNC_VOID, ConvertRoomBrushes, "Convert rooms to internal rep",    HK_ALL},
   {"fix_rooms",       FUNC_VOID, FixRooms,           "Fix dangling room pointers",       HK_ALL},
   {"hilight_room_id", FUNC_INT,  ged_hilight_room,   "Hilight the specified room brush", HK_ALL},
};

void ged_room_init()
{
   COMMANDS(g_RoomCommands,HK_ALL);
}
