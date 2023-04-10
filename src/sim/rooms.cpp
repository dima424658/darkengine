/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/rooms.cpp,v 1.45 2000/03/27 12:30:46 adurant Exp $
//
// cRooms functions
//

#include <lg.h>
#include <config.h>
#include <comtools.h>
#include <appagg.h>
#include <lazyagg.h>
#include <contain.h>

#include <objpos.h>
#include <objedit.h>
#include <string.h>
#include <iobjsys.h>
#include <osysbase.h>
#include <objquery.h>
#include <hashpp.h>
#include <hshpptem.h>

#include <timer.h>

#include <physapi.h>
#include <rooms.h>
#include <roomutil.h>

#include <mprintf.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <propman.h>
#include <autoprop.h>

#include <rooscrpt.h>
// for typing info for the message
#include <playrobj.h>
#include <aiapi.h>
#include <rendprop.h>

// Must be last header
#include <dbmem.h>

LazyAggMember(IContainSys) pContainSys;

BOOL g_RoomsOK = FALSE;

////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER
template cPortalCallbackTable;
template cRoomTable;
#endif

////////////////////////////////////////////////////////////////////////////////

BOOL TrueFunc(ObjID objID)
{
   return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

// The one, and only, automap callback!
// making it static was needed to get around an order-of-init mess - patmc

tWatchCallback cRooms::m_AutomapCallback = NULL;

#ifndef SHIP
static int gMapPage = -1;
static int gMapLocation = -1;
#endif

////////////////////////////////////////////////////////////////////////////////

cRooms::cRooms(void)
{
   int handle;
   
   m_RoomList.SetSize(0);
   m_RoomTable.Clear();

   m_PortalCallbackTable.Clear();
   m_EnterCallbackTable.Clear();
   m_ExitCallbackTable.Clear();

   m_TransitionCallbackList.SetSize(0);
   m_WatchCallbackList.SetSize(0);

   AddWatch(TrueFunc, &handle);

   AssertMsg1(handle == kAllObjWatchHandle, "Bad handle for global object watch: %d", handle);
}

////////////////////////////////////////

cRooms::~cRooms(void)
{
   int i;

   for (i=0; i<m_RoomList.Size(); i++)
      delete m_RoomList[i];

   m_RoomList.SetSize(0);
   m_RoomTable.Clear();

   m_RoomPortalList.SetSize(0);

   m_PortalCallbackTable.Clear();
   m_EnterCallbackTable.Clear();
   m_ExitCallbackTable.Clear();

   m_TransitionCallbackList.SetSize(0);
   m_WatchCallbackList.SetSize(0);

   #ifndef SHIP
   gMapPage = -1;
   gMapLocation = -1;
   #endif
}

////////////////////////////////////////////////////////////////////////////////

void cRooms::Read(RoomReadWrite movefunc)
{
   cRoom *pRoom;
   int    num, i;

   movefunc(&num, sizeof(int), 1);

   for (i=0; i<num; i++)
   {
      pRoom = new cRoom(movefunc);      
      AddRoom(pRoom);
   }
}

////////////////////////////////////////

void cRooms::Write(RoomReadWrite movefunc)
{
   int num, i;

   num = GetNumRooms();

   movefunc(&num, sizeof(int), 1);
   
   for (i=0; i<num; i++)
      GetRoom(i)->Write(movefunc);
}

////////////////////////////////////////////////////////////////////////////////

cRoom *cRooms::GetRoomNamed(char *roomName) const
{
   for (int i=0; i<m_RoomList.Size(); i++)
   {
      if (!strcmp(m_RoomList[i]->GetName(), roomName))
         return m_RoomList[i];
   }

   return NULL;
}

////////////////////////////////////////////////////////////////////////////////

void cRooms::ClearRooms()
{
   int i;

   for (i=0; i<m_RoomList.Size(); i++)
      delete m_RoomList[i];
   m_RoomList.SetSize(0);

   m_RoomTable.Clear();

   m_RoomPortalList.SetSize(0);

   #ifndef SHIP
   gMapPage = -1;
   gMapLocation = -1;
   #endif
}

////////////////////////////////////////////////////////////////////////////////

void cRooms::AddWatch(tWatchCallback callback, int *handle)
{
   int i;

   // Search for unused handle
   for (i=0; i<m_WatchCallbackList.Size(); i++)
   {
      if (m_WatchCallbackList[i] == NULL)
         break;
   }

   if (i == m_WatchCallbackList.Size())
   {
      // No unused slots
      *handle = m_WatchCallbackList.Size();
      m_WatchCallbackList.Append(callback);
   }
   else
   {
      // Found an unused slot
      *handle = i;
      m_WatchCallbackList[i] = callback;
   }

   // Update all rooms
   for (i=0; i<m_RoomList.Size(); i++)
      m_RoomList[i]->AddWatch(*handle);
}

////////////////////////////////////////

void cRooms::RemoveWatch(int handle)
{
   AssertMsg2(handle < m_WatchCallbackList.Size(), "Attempt to remove watch %d of %d", handle, m_WatchCallbackList.Size());
   AssertMsg1(m_WatchCallbackList[handle] != NULL, "Attempt to remove nonexistant watch %d", handle);

   // We don't actually remove the elment because we need to preserve handles
   m_WatchCallbackList[handle] = NULL;

   // Update all rooms
   for (int i=0; i<m_RoomList.Size(); i++)
      m_RoomList[i]->RemoveWatch(handle);
}

/////////////////////////////////////
// message connections

// Lets send us some messages, eh?
void SendRoomMessages(ObjID exit_room, ObjID enter_room, ObjID moving_obj)
{
   sRoomMsg::eObjType move_obj_type;

   if (PlayerObjectExists() && (moving_obj == PlayerObject()))
      move_obj_type = sRoomMsg::kPlayer;
   else 
   if (ObjIsAI(moving_obj))
      move_obj_type = sRoomMsg::kCreature;
   else
   {  // @TODO: remote player check here...
      move_obj_type=sRoomMsg::kObject;
   }

   AutoAppIPtr_(ScriptMan,pScriptMan);

   if (exit_room!=OBJ_NULL)
   {
      sRoomMsg exitmsg(exit_room,enter_room,moving_obj,move_obj_type,sRoomMsg::kExit);   
      pScriptMan->SendMessage(&exitmsg);
   }

   if (enter_room!=OBJ_NULL)
   {
      sRoomMsg entermsg(exit_room,enter_room,moving_obj,move_obj_type,sRoomMsg::kEnter);   
      pScriptMan->SendMessage(&entermsg);
   }

   sRoomMsg objmsg(exit_room,enter_room,moving_obj,move_obj_type);
   pScriptMan->SendMessage(&objmsg);   
}

////////////////////////////////////////////////////////////////////////////////

void cRooms::UpdateRoomData(ObjID objID, const mxs_vector &new_loc)
{
   cRoom  *pRoom;
   cRoom  *pFarRoom;
   cRoom  *pFarFarRoom = NULL;
   int     i, j;

   if (g_pRooms == NULL)
      return;

   pRoom = g_pRooms->GetObjRoom(objID);

   // Doesn't know what room it's in
   if (pRoom == NULL)
   {
      #ifndef SHIP
      if (config_is_defined("RoomSpew") && ObjHasRefs(objID))
         mprintf("%s didn't know what room it was in.\n", ObjWarnName(objID));
      #endif

      FindObjRoom(objID, new_loc);
      //send an enter message, at least
      if ((g_pRooms->GetObjRoom(objID) != NULL))      
         SendRoomMessages(OBJ_NULL,g_pRooms->GetObjRoom(objID)->GetObjID(),objID);

      #ifndef SHIP
      if (config_is_defined("OutOfRoomSpew"))
      {
         if ((g_pRooms->GetObjRoom(objID) == NULL) && g_pRooms->GetNumRooms() > 0)
            mprintf("%s is not within the room database.\n", ObjWarnName(objID));
      }
      #endif

      return;
   }

   BOOL found_room = FALSE;

   // Check against each portal
   for (i=0; i<pRoom->GetNumPortals() && !found_room; i++)
   {
      if (PointPlaneDist(pRoom->GetPortalPlane(i), new_loc) > ON_PLANE_EPSILON)
      {
         pFarRoom = pRoom->GetPortalFarRoom(i);

         // Verify that we're in the "far" room
         if (pFarRoom->PointInside(new_loc))
            found_room = TRUE;
      }
   }

   // If we didn't find an adjacent room, check if we've actually left
   if (!found_room)
   {
      if (pRoom->PointInside(new_loc))
         return;
   }

   // If we've actually left, check the *next* adjacent rooms
   if (!found_room)
   {
      for (i=0; i<pRoom->GetNumPortals() && !found_room; i++)
      {
         pFarRoom = pRoom->GetPortalFarRoom(i);

         if (pFarRoom->PointInside(new_loc))
         {
            pFarFarRoom = NULL;
            found_room = TRUE;
         }

         for (j=0; j<pFarRoom->GetNumPortals() && !found_room; j++)
         {
            pFarFarRoom = pFarRoom->GetPortalFarRoom(j);

            if (pFarFarRoom != pRoom)
            {
               if (pFarFarRoom->PointInside(new_loc))
                  found_room = TRUE;
            }
         }
      }
   }

   // If we found our room, update
   if (found_room)
   {
      static cDynArray<ObjID> containObjList;
      sContainIter *iter;

      // Build list of contained objects
      containObjList.SetSize(0);

      iter = pContainSys->IterStart(objID);
      while (!iter->finished)
      {
         containObjList.Append(iter->containee);
         pContainSys->IterNext(iter);
      }
      pContainSys->IterEnd(iter);

      UpdateObjRoom(objID, pRoom, pFarRoom);
      for (i=0; i<containObjList.Size(); i++)
         UpdateObjRoom(containObjList[i], pRoom, pFarRoom);

      if (pFarFarRoom)
      {
         UpdateObjRoom(objID, pFarRoom, pFarFarRoom);
         for (i=0; i<containObjList.Size(); i++)
            UpdateObjRoom(containObjList[i], pFarRoom, pFarFarRoom);
      }
   }
   else
   {
      //send an exit message, at least
      if ((g_pRooms->GetObjRoom(objID) != NULL))            
         SendRoomMessages(g_pRooms->GetObjRoom(objID)->GetObjID(),OBJ_NULL,objID);
      g_pRooms->SetObjRoom(objID, NULL);
   }
}

///////////////////////////////////////

void cRooms::UpdateObjRoom(ObjID objID, cRoom *pRoomFrom, cRoom *pRoomTo)
{
   tPortalCallback  callback;
   int          obj_list_size;
   const ObjID *obj_list;
   int i, j;

   ObjID exit_room_obj = pRoomFrom->GetObjID();
   ObjID enter_room_obj = pRoomTo->GetObjID();

   #ifndef SHIP
   if (config_is_defined("RoomSpew"))
   {
      mprintf("%s going from %s (%d) to %s (%d)\n", ObjWarnName(objID), ObjWarnName(exit_room_obj), 
              pRoomFrom->GetRoomID(), ObjWarnName(enter_room_obj), pRoomTo->GetRoomID());
   }
   #endif

   #ifndef SHIP
   if (config_is_defined("AutomapSpew") && (objID == PlayerObject()))
   {
      AutoAppIPtr_(PropertyManager, pPropMan);
      IAutomapProperty *pAutomapProp = (IAutomapProperty *)pPropMan->GetPropertyNamed(PROP_AUTOMAP_NAME);

      if (pAutomapProp != NULL)
      {
         sAutoMapProperty *pAutomap;
         if (pAutomapProp->Get(pRoomTo->GetObjID(), &pAutomap))
         {
            if ((pAutomap->page != gMapPage) || (pAutomap->location != gMapLocation))
            {
               mprintf("AutoMap going from (%d)[%d, %d] to (%d)[%d, %d]\n", pRoomFrom->GetRoomID(), gMapPage, gMapLocation, 
                                                                            pRoomTo->GetRoomID(), pAutomap->page, pAutomap->location);

               gMapPage = pAutomap->page;
               gMapLocation = pAutomap->location;
            }
         }
         else
         {
            mprintf("Room %d is not mapped!\n", pRoomTo->GetRoomID());
            gMapPage = -1;
            gMapLocation = -1;
         }
      }
   }
   #endif

   // Update watches
   for (i=0; i<m_WatchCallbackList.Size(); i++)
   {
      if ((m_WatchCallbackList[i] != NULL) && (m_WatchCallbackList[i](objID)))
      {
         // Find index of objID in watch list (ick! slow! should be better!)
         pRoomFrom->GetObjList(&obj_list, &obj_list_size, i);
         for (j=0; j<obj_list_size; j++)
         {
            if (obj_list[j] == objID)
               break;
         }   
         if (j != obj_list_size)
         {
            pRoomFrom->RemoveWatchElement(i, j);
            pRoomTo->AddWatchElement(i, objID);
         }
      }
   }

   // Call any applicable callbacks
   if ( m_AutomapCallback && PlayerObjectExists() && (objID == PlayerObject()) )
   {
      m_AutomapCallback( enter_room_obj );
   }

   if (exit_room_obj != enter_room_obj)
   {
      for (i=0; i<m_TransitionCallbackList.Size(); i++)
         m_TransitionCallbackList[i](objID, pRoomTo, pRoomFrom);

      if (m_PortalCallbackTable.Lookup((((long)exit_room_obj) << 16) | enter_room_obj, (void **)(&callback)))
         callback(objID, pRoomFrom, pRoomTo);
      
      if (m_EnterCallbackTable.Lookup(enter_room_obj, (void **)(&callback)))
         callback(objID, pRoomFrom, pRoomTo);

      if (m_ExitCallbackTable.Lookup(exit_room_obj, (void **)(&callback)))
         callback(objID, pRoomFrom, pRoomTo);
      
      SendRoomMessages(exit_room_obj, enter_room_obj, objID);
   }

   // Move object to new room
   m_RoomTable.Set(objID, pRoomTo);
}

///////////////////////////////////////

#define MAX_ROOMS_IN  8

cRoom *cRooms::RoomFromPoint(const mxs_vector &loc)
{
   static cRoom *RoomsInsideList[MAX_ROOMS_IN];
   int NumRoomsInside = 0;
   int i;

   memset(RoomsInsideList, 0, sizeof(cRoom *) * MAX_ROOMS_IN);

   for (i=0; i<m_RoomList.Size(); i++)
   {
      if (m_RoomList[i]->PointInside(loc))
         RoomsInsideList[NumRoomsInside++] = m_RoomList[i];
      AssertMsg(NumRoomsInside < MAX_ROOMS_IN, "Inside too many rooms at once, increase MAX_ROOMS_IN");
      if (NumRoomsInside >= MAX_ROOMS_IN)
         return NULL;
   }

   if (NumRoomsInside == 0)
      return NULL;

   if (NumRoomsInside == 1)
      return RoomsInsideList[0];

   int    room1_index;
   int    room2_index;
   cRoom *room1 = NULL;
   cRoom *room2 = NULL;
   while (NumRoomsInside > 1)
   {
      // Find two rooms
      for (i=0; i<MAX_ROOMS_IN; i++)
      {
         if (RoomsInsideList[i])
         {
            if (!room1)
            {
               room1 = RoomsInsideList[i];
               room1_index = i;
            }
            else
            if (!room2)
            {
               room2 = RoomsInsideList[i];
               room2_index = i;
               break;
            }
         }
      }

      AssertMsg1(room1 && room2, "RoomFromPoint: room count of %d, but couldn't find rooms", NumRoomsInside);

      // Find the portal between them (ugh)
      if (room1->GetNumPortals() < room2->GetNumPortals())
      {
         for (i=0; i<room1->GetNumPortals(); i++)
         {
            if (room1->GetPortal(i)->GetFarRoom() == room2)
               break;
         }

         if (i == room1->GetNumPortals())
         {
            if (config_is_defined("room_portal_warn"))
            {
               Warning(("RoomFromPoint: couldn't find portal between rooms %d and %d\n", room1->GetRoomID(), room2->GetRoomID()));
               Warning(("  (%g %g %g) & (%g %g %g)\n", room1->GetCenterPt().x, room1->GetCenterPt().y, room1->GetCenterPt().z,
                                                       room2->GetCenterPt().x, room2->GetCenterPt().y, room2->GetCenterPt().z));
            }
            return NULL;
         }
      
         if (PointPlaneDist(room1->GetPortal(i)->GetPlane(), loc) > ON_PLANE_EPSILON)
            RoomsInsideList[room1_index] = NULL;
         else
            RoomsInsideList[room2_index] = NULL;
      }
      else
      {
         for (i=0; i<room2->GetNumPortals(); i++)
         {
            if (room2->GetPortal(i)->GetFarRoom() == room1)
               break;
         }

         if (i == room2->GetNumPortals())
         {
            if (config_is_defined("roomportal_warn"))
            {
               Warning(("RoomFromPoint: couldn't find portal between rooms %d and %d\n", room1->GetRoomID(), room2->GetRoomID()));
               Warning(("  (%g %g %g) & (%g %g %g)\n", room1->GetCenterPt().x, room1->GetCenterPt().y, room1->GetCenterPt().z,
                                                       room2->GetCenterPt().x, room2->GetCenterPt().y, room2->GetCenterPt().z));
            }
            return NULL;
         }
      
         if (PointPlaneDist(room2->GetPortal(i)->GetPlane(), loc) > ON_PLANE_EPSILON)
            RoomsInsideList[room2_index] = NULL;
         else
            RoomsInsideList[room1_index] = NULL;
      }

      NumRoomsInside--;
   }

   // Find the remaining room
   for (i=0; i<MAX_ROOMS_IN; i++)
   {
      if (RoomsInsideList[i])
         return RoomsInsideList[i];
   }

   return NULL;
}

///////////////////////////////////////

BOOL cRooms::FindObjRoom(ObjID objID, const mxs_vector &loc)
{
   int           obj_list_size;
   const ObjID  *obj_list;
   cRoom *pOldRoom = GetObjRoom(objID);
   cRoom *pRoom = RoomFromPoint(loc);
   int i, j;

   if (pRoom != NULL)
   {
      if (pRoom != pOldRoom)
      {
         // Update watches
         for (i=0; i<m_WatchCallbackList.Size(); i++)
         {
            if ((m_WatchCallbackList[i] != NULL) && (m_WatchCallbackList[i](objID)))
            { 
               if (pOldRoom != NULL)
               {
                  // Find index of objID in watch list (ick! slow! should be better!)
                  pOldRoom->GetObjList(&obj_list, &obj_list_size, i);
                  for (j=0; j<obj_list_size; j++)
                  {
                     if (obj_list[j] == objID)
                        break; 
                  }
                  if (j != obj_list_size)
                  {
                     if (pOldRoom != NULL)
                        pOldRoom->RemoveWatchElement(i, j);
                  }
               }
            }
         }

         SetObjRoom(objID, pRoom);

         if (PhysObjHasPhysics(objID))
         {
            if ((pOldRoom != NULL) || (fabs(PhysGetGravity(objID) - GetRoomGravity(pRoom->GetObjID())) > 0.001))
            {
               PhysicsListenerLock = TRUE;
               PhysSetGravity(objID, PhysGetGravity(objID) * GetRoomGravity(pRoom->GetObjID()));
               PhysicsListenerLock = FALSE;
            }
         }
      }

      return TRUE;
   }
   else
   {
      SetObjRoom(objID, NULL);
      return FALSE;
   }
}

///////////////////////////////////////

void cRooms::SetObjRoom(ObjID objID, cRoom *room)
{
   int i, j, k;

   if (!OBJ_IS_CONCRETE(objID))
      return;

   // First check if we're removing from room
   if (room == NULL)
   {
      const ObjID *obj_list;
      int          obj_list_size;
      cRoom       *old_room;

      // Get the room the object's in 
      if ((old_room = GetObjRoom(objID)) != NULL)
      {
         // For each active watch
         for (j=0; j<m_WatchCallbackList.Size(); j++)
         {
            old_room->GetObjList(&obj_list, &obj_list_size, j);

            // Check if it's in the list
            for (k=0; k<obj_list_size; k++)
            {
               if (obj_list[k] == objID)
                  break;
            }

            // If it is, remove it
            if (k != obj_list_size)
               old_room->RemoveWatchElement(j, k);
         }
      }

      m_RoomTable.Delete(objID);
   }
   else
   {
      // Add to table
      m_RoomTable.Set(objID, room);

      // Update Watches
      for (i=0; i<m_WatchCallbackList.Size(); i++)
      {
         if ((m_WatchCallbackList[i] != NULL) && (m_WatchCallbackList[i](objID)))
            room->AddWatchElement(i, objID);
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

void cRooms::AddAutomapCallback( tWatchCallback callback )
{
   // NOTE: there can only be one automap callback
   assert( m_AutomapCallback == NULL );
   m_AutomapCallback = callback;
}

////////////////////////////////////////////////////////////////////////////////

void cRooms::RemoveAutomapCallback( tWatchCallback callback )
{
   m_AutomapCallback = NULL;
}

