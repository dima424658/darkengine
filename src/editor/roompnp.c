/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/roompnp.c,v 1.15 2000/02/19 13:11:21 toml Exp $
// skeletal room PnP stuff

#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <editobj.h>
#include <brinfo.h>
#include <gedit.h>
#include <comtools.h>

#include <gadblist.h>
#include <brushGFH.h>
#include <PnPtools.h>
#include <iobjsys.h>
#include <objsys.h>
#include <osysbase.h>
#include <objedit.h>
#include <linkedit.h>
#include <linkedst.h>
#include <linktype.h>

#include <otreeed.h>
#include <editinh.h>
#include <edinhst.h>

#include <modalui.h>

#include <iobjed.h>
#include <objedbas.h>
#include <edittool.h>

#include <ged_room.h>
#include <roomeax.h>
#include <roomprop.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception
 * 
 * 0 Our Name (id)
 * 1 RoomID
 * 2 Type
 * 3 xxxx
 * 4{create}  7{show}   
 * 5{edit}    8{show sel}
 * 6{links}   9{compile}
 */

#define NUM_BUTTONS (0xA)
static Rect roomPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (roomPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 4
#define BTN_BELOW 3
#define VERT_DIV  4
#define HORIZ_DIV (0.5)

void roomPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   for (i=0; i<BTN_ABOVE; i++)
      buildYLineRect(curRect++,whole,i,0,1,2,2);
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,0,2,2);   
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,1,2,2);
}

#define EDIT_CREATE  1
#define EDIT_EDIT    2
#define EDIT_LINKS   3
#define EDIT_COMPILE 4
#define EDIT_SHOWSEL 5
#define EDIT_SHOWALL 6

// statics for the PnP itself
static ObjID roomObjId=0;
static editBrush *roomPnP_br;
static char roomPnP_myName[64];
static char roomPnP_myNum[64];
static char roomPnP_myType[64];

static _PnP_GadgData roomPnP;

void Update_roomPnP(GFHUpdateOp op, editBrush *br);

void roomPnP_setvars(editBrush *br, bool update)
{
   IObjectSystem *pObjSys = AppGetObj(IObjectSystem);
   int room_id;
   int room_type;

   if (update) br=roomPnP_br;
   else        roomPnP_br=br;

   if (br==NULL) return;  // no data, go home

   if (update)
   {
      brRoom_ObjID(br)=roomObjId;
      // do whatever i have to do to make it so, invalidate, so on
      // perhaps this means an i_changed_the_brush, eh?
   }
   else
      roomObjId=brRoom_ObjID(br);

   // Set name
   strncpy(roomPnP_myName,ObjEditName(brRoom_ObjID(br)),sizeof(roomPnP_myName));
   roomPnP_myName[sizeof(roomPnP_myName)-1] = '\0';

   room_id = brRoom_InternalRoomID(br);
   room_type = GetRoomAcoustics(room_id);

   sprintf(roomPnP_myNum, "%d", room_id);

   if (room_type < 0)
      sprintf(roomPnP_myType, "-unset-");
   else
      strncpy(roomPnP_myType, g_RoomTypes[room_type], sizeof(roomPnP_myType));

   SafeRelease(pObjSys);
}

static int  last_draw_sel = -1;
static BOOL draw_all = FALSE;

static void roomPnP_PopUp(int data)
{
   ObjID root_room;
   ObjID default_room;
   ObjID room_id;

   if (roomPnP_br == NULL) 
      return;

   room_id = brRoom_ObjID(roomPnP_br);

   // Get the root and default room ids
   root_room    = ged_room_get_root();
   default_room = ged_room_get_default();

   // Verify that our default archtype is concrete
   // @TODO: should make sure that its a valid room too
   if (OBJ_IS_ABSTRACT(ged_room_get_default_arch()))
      ged_room_set_default_arch(default_room);

   switch (data)
   {
      case EDIT_CREATE:
      {
         EditObjHierarchyRoom(room_id);

         // Slam it to room brush
         gedit_set_default_brush_type(brType_ROOM);

         // If they selected the root archetype, change to the default room
         if (ged_room_get_default_arch() == root_room)
            ged_room_set_default_arch(default_room);
            
         // Set the current brush to the selected room
         brRoom_ObjID(roomPnP_br) = ged_room_get_default_arch();

         // And update the PnP
         Update_roomPnP(GFH_FORCE, roomPnP_br);
      }
      break;

      case EDIT_EDIT:
      {
         sObjEditorDesc eddesc = { "" };
         IObjEditors* eds = AppGetObj(IObjEditors); 

         IObjEditor* ed = IObjEditors_Create(eds,&eddesc,room_id); 
         IObjEditor_Go(ed,kObjEdModal);
         SafeRelease(ed);
         SafeRelease(eds);

         break;
      }

      case EDIT_LINKS:
      {
         sLinkEditorDesc desc = { "", kLinkEditAllButtons };
         EditLinks(&desc, room_id, LINKOBJ_WILDCARD, RELID_WILDCARD);

         break;
      }
         
      case EDIT_SHOWALL:
      {
         last_draw_sel = -1;
         draw_all = !draw_all;
         ged_room_draw_all_connections(draw_all);

         break;
      }

      case EDIT_SHOWSEL:
      {
         if (brushGetType(roomPnP_br) == brType_ROOM)
         {
            if (brRoom_InternalRoomID(roomPnP_br) != last_draw_sel)
            {
               last_draw_sel = brRoom_InternalRoomID(roomPnP_br);
               ged_room_draw_connections(last_draw_sel, TRUE);
            }
            else
            {
               ged_room_draw_connections(-1, TRUE);
               last_draw_sel = -1;
            }
         }

         break;
      }

      case EDIT_COMPILE:
         ged_compile_rooms();
         break;
   }

}

void Create_roomPnP(LGadRoot* root, Rect* bounds, editBrush *br)
{
   GFHSetCoordMask(0);
   roomPnP_buildRects(bounds);
   PnP_GadgStart(&roomPnP,root);
   roomPnP_setvars(br,FALSE);

   PnP_TextBox(getRect(0),roomPnP_myName);
   PnP_TextBox(getRect(1),roomPnP_myNum);
   PnP_TextBox(getRect(2),roomPnP_myType);

   PnP_ButtonOneShot(getRect(4),"Create",roomPnP_PopUp,EDIT_CREATE);
   PnP_ButtonOneShot(getRect(5),"Edit",roomPnP_PopUp,EDIT_EDIT);
   PnP_ButtonOneShot(getRect(6),"Links",roomPnP_PopUp,EDIT_LINKS);
   PnP_ButtonOneShot(getRect(7),"Show All",roomPnP_PopUp,EDIT_SHOWALL);
   PnP_ButtonOneShot(getRect(8),"Show Sel",roomPnP_PopUp,EDIT_SHOWSEL);
   PnP_ButtonOneShot(getRect(9),"Compile",roomPnP_PopUp,EDIT_COMPILE);
}

void Destroy_roomPnP(void)
{
   PnP_GadgFree(&roomPnP);
   roomPnP_br=NULL;
}

void Update_roomPnP(GFHUpdateOp op, editBrush *br)
{
   roomPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)
      PnP_FullRedraw(&roomPnP);
}
