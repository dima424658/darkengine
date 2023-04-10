/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomeax.cpp,v 1.4 2000/03/02 20:06:17 adurant Exp $
//
// Room EAX
//

#include <lg.h>

#include <dynarray.h>
#include <tagfile.h>
#include <command.h>
#include <mprintf.h>

#include <brinfo.h>
#include <brquery.h>
#include <editbr_.h>
#include <vbrush.h>
#include <gedit.h>
#include <brushgfh.h>

#include <roomeax.h>
#include <roomsys.h>
#include <rooms.h>

#include <traitman.h> //for TraitManager in room 
                      //archetype eax stuff --AMSD 3/2/00

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

#ifdef EDITOR

void RemapRoomType(char *input)
{
   int from, to;

   sscanf(input, "%d,%d", &from, &to);
   
   mprintf("Remapping room type %d to %d\n");

   g_pRoomEAX->Remap(from, to);

   GFHRedrawPnPnSuch();
}

void SetCurRoomType(int roomType)
{
   editBrush *us = vBrush_editBrush_Get();
   
   if (brushGetType(us) != brType_ROOM)
      return;

   int room_id = brRoom_InternalRoomID(us);

   g_pRoomEAX->SetAcoustics(room_id, roomType);

   GFHRedrawPnPnSuch();
}

BOOL is_room(editBrush *br)
{
   return (brushGetType(br) == brType_ROOM);
}

void AdvanceCurRoom()
{
   brCycle(1, is_room);
   gedit_cam_to_brush();
}

////////////////////////////////////////////////////////////////////////////////

void SetEaxArchetype(const char *pString)
{ 
   if ((pString == NULL) || (strlen(pString) == 0))
   {
      mprintf("Usage: eax_set_archetype <room archetype>,<room type>\n");
      return;
   }

   ObjID room_archetype;
   int   room_type;

   sscanf(pString, "%d,%d", &room_archetype, &room_type);

   AutoAppIPtr_(TraitManager, pTraitMan);

   for (int i=0; i<g_pRooms->GetNumRooms(); i++)
   {
      cRoom *pRoom = g_pRooms->GetRoom(i);
      if (pRoom)
      {
         ObjID room_obj = pRoom->GetObjID();
      
         while (room_obj != OBJ_NULL)
         {
            if (room_obj == room_archetype)
            {
               g_pRoomEAX->SetAcoustics(pRoom->GetRoomID(), room_type);
               break;
            }

            room_obj = pTraitMan->GetArchetype(room_obj);
         }
      }
   }

   GFHRedrawPnPnSuch();
}
   
////////////////////////////////////////////////////////////////////////////////

static Command g_RoomEAXCommands[] = 
{
   { "remap_room_type",  FUNC_STRING, RemapRoomType,   "Remap EAX room type",                           HK_EDITOR },
   { "set_room_type",    FUNC_INT,    SetCurRoomType,  "Set EAX room type of selected room brush",      HK_EDITOR },
   { "next_room",        FUNC_VOID,   AdvanceCurRoom,  "Select and go to next room brush",              HK_EDITOR },
   {"eax_set_archetype", FUNC_STRING, SetEaxArchetype, "Set all rooms of an archetype to an EAX value", HK_EDITOR},
};

#endif

////////////////////////////////////////

cRoomEAX *g_pRoomEAX = NULL;

void InitRoomEAX()
{
   Assert_(g_pRoomEAX == NULL);

   g_pRoomEAX = new cRoomEAX;

#ifdef EDITOR
   COMMANDS(g_RoomEAXCommands, HK_EDITOR);
#endif
}

void TermRoomEAX()
{
   Assert_(g_pRoomEAX != NULL);

   delete g_pRoomEAX;
   g_pRoomEAX = NULL;
}

void ResetRoomEAX()
{
   if (g_pRoomEAX)
      g_pRoomEAX->Reset();
   else
      InitRoomEAX();
}

int GetRoomAcoustics(int roomID)
{
   return g_pRoomEAX->GetAcoustics(roomID);
}
////////////////////////////////////////

cRoomEAX::cRoomEAX()
{
   Reset();
}

cRoomEAX::~cRoomEAX()
{
   Reset();
}

////////////////////////////////////////

void cRoomEAX::Read(ITagFile *file)
{
   int size;

   file->Read((char *)&size, sizeof(int));
   m_acousticsIndex.SetSize(size);

   for (int i=0; i<size; i++)
      file->Read((char *)&m_acousticsIndex[i], sizeof(int));
}

void cRoomEAX::Write(ITagFile *file) const
{
   int size = m_acousticsIndex.Size();

   file->Write((char *)&size, sizeof(size));

   for (int i=0; i<size; i++)
      file->Write((char *)&m_acousticsIndex[i], sizeof(int));
}

////////////////////////////////////////

void cRoomEAX::Reset()
{
   m_acousticsIndex.SetSize(0);
}

void cRoomEAX::Build()
{
}

////////////////////////////////////////

void cRoomEAX::Remap(int from, int to)
{
   for (int i=0; i<m_acousticsIndex.Size(); i++)
   {
      if (m_acousticsIndex[i] == from)
         m_acousticsIndex[i] = to;
   }
}

////////////////////////////////////////








