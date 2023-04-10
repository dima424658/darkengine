/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/objpnp.c,v 1.35 2000/02/19 13:11:09 toml Exp $
// HotRegion PnP gadget

#include <gadblist.h>
#include <lgsprntf.h>

#include <command.h>

#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <editobj.h>
#include <brinfo.h>
#include <gedit.h>


#include <linkedit.h>
#include <linkedst.h>

#include <iobjed.h>
#include <objedbas.h>

#include <traitman.h>
#include <otreeed.h>

#include <brushGFH.h>
#include <PnPtools.h>
#include <objsys.h>

#include <objedit.h>

#include <mnumprop.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception - is WILDLY OUT OF DATE
 *      __________
 *     |0 pretty  |
 *     |  picture |         (4 lines worth)
 *     |  of the  |
 *     |__object__|         (dclick on type should bring up pal)
 * 1{???}    4{floor me}   
 * 2{prop}   5{class}
 * 3{links}  6{update}      (bring up link manager, something else)
 */

/* correct artists conception - note 9 buttons, not 7
 *    0 { nonexistant
 *         picture }
 *    1   { name }
 *    2  xxxxx
 *
 * 3{???}    6{floor me}   
 * 4{prop}   7{class}
 * 5{links}  8{update}      (bring up link manager, something else)
 */


#define NUM_BUTTONS (0x9)
static Rect objPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (objPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 0
#define BTN_BELOW 3
#define VERT_DIV  4
#define HORIZ_DIV (0.5)

void objPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   buildYLineRect(curRect++,whole,0,0,1,2,2);
   buildYLineRect(curRect++,whole,3,0,1,2,2);
   buildYLineRect(curRect++,whole,4,0,1,2,2);
   // fixup the big box (these are brutal hacks, sorry)
   OUR_RECTS[0].lr.y=OUR_RECTS[1].ul.y-3;
   
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,0,2,2);
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,1,2,2);
}

#define EDIT_XXXX    1
#define EDIT_PROP    2
#define EDIT_LINK    3
#define EDIT_FLOOR   4
#define EDIT_CLASS   5
#define EDIT_UPDATE  6

// statics for the PnP itself
static int type=0;
static editBrush *objPnP_br;
static char objPnP_buf[64];

static _PnP_GadgData objPnP;

void objPnP_setvars(editBrush *br, bool update)
{
   if (update) br=objPnP_br;
   else        objPnP_br=br;

   if (br==NULL) return;  // no data, go home

   if (update)
   {
      brObj_Type(br)=type;
      ObjSetModelNumber(brObj_ID(br), &type);
      editobjUpdateBrushfromObj(brObj_ID(br));  // Brush size might need to change.
   }
   else
      type=brObj_Type(br);
}

static void objPnP_PopUp(int data)
{
   ObjID id;
   
   if (objPnP_br==NULL) return;
   
   id = brObj_ID(objPnP_br);

   if (data==EDIT_UPDATE)
   {
      editobjUpdateObjfromBrush(objPnP_br);
      gedit_redraw_3d();
   }
   else if (data == EDIT_PROP)
   {
      sObjEditorDesc eddesc = { "" };
      IObjEditors* eds = AppGetObj(IObjEditors); 

      IObjEditor* ed = IObjEditors_Create(eds,&eddesc,id); 
      IObjEditor_Go(ed,kObjEdModal);
      SafeRelease(ed);
      SafeRelease(eds);
   }
   else if (data == EDIT_LINK)
   {
      extern BOOL show_all_links; 
      sLinkEditorDesc desc = { "", kLinkEditAllButtons};
      if (show_all_links) desc.flags |= kLinkEditShowAll; 
      EditLinks(&desc,id, LINKOBJ_WILDCARD, RELID_WILDCARD); 
   }
   else if (data==EDIT_CLASS)
   {
      ITraitManager* traitman = AppGetObj(ITraitManager);
      ObjID arch = ITraitManager_GetArchetype(traitman,id);
      SafeRelease(traitman);
      EditObjHierarchy(arch);
   }
   else if (data==EDIT_FLOOR)
   {
      mxs_vector down;
      down.x = down.y = 0; down.z = -20.0F;
      gedit_floor_brush(objPnP_br,&down);
   }
   else if (data==EDIT_XXXX)
      Warning(("Watch it... thats XXXX"));
}

void Create_objPnP(LGadRoot* root, Rect* bounds, editBrush *br)
{
   GFHSetCoordMask(0);
   objPnP_buildRects(bounds);
   PnP_GadgStart(&objPnP,root);
   objPnP_setvars(br,FALSE);
   PnP_PictureBox(getRect(0),NULL,0);

   strncpy(objPnP_buf,ObjEditName(brObj_ID(br)),sizeof(objPnP_buf));
   objPnP_buf[sizeof(objPnP_buf)-1] = '\0';

   PnP_TextBox(getRect(1),objPnP_buf);
   //   PnP_ButtonOneShot(getRect(3),"XXXX",objPnP_PopUp,EDIT_XXXX);
   PnP_ButtonOneShot(getRect(4),"Properties",objPnP_PopUp,EDIT_PROP);
   PnP_ButtonOneShot(getRect(5),"Links",objPnP_PopUp,EDIT_LINK);
   PnP_ButtonOneShot(getRect(6),"Floor Me",objPnP_PopUp,EDIT_FLOOR);
   PnP_ButtonOneShot(getRect(7),"Class",objPnP_PopUp,EDIT_CLASS);
   PnP_ButtonOneShot(getRect(8),"Update",objPnP_PopUp,EDIT_UPDATE);
}

void Destroy_objPnP(void)
{
   PnP_GadgFree(&objPnP);
   objPnP_br=NULL;
}

void Update_objPnP(GFHUpdateOp op, editBrush *br)
{
   objPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)
      PnP_FullRedraw(&objPnP);
}
