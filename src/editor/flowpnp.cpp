/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/flowpnp.cpp,v 1.9 2000/02/19 13:10:39 toml Exp $
// flow PnP gadget

#include <config.h>
#include <gadblist.h>
#include <lgsprntf.h>

#include <command.h>

#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <gedit.h>
#include <edmedmo.h>
#include <gedmedmo.h>

#include <brushGFH.h>
#include <PnPtools.h>

#include <appagg.h>
#include <flowarch.h>
#include <propman.h>
#include <iobjed.h>
#include <objedbas.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception
 *
 *        0 pic?
 *
 *    1  [ group ]  
 *    2  [ edit group ]
 *    3  [ update in world ]
 */

#define NUM_BUTTONS 5
static Rect flowPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (flowPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 0
#define BTN_BELOW 4
#define VERT_DIV  3
#define HORIZ_DIV 0

void flowPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   buildYLineRect(curRect++,whole,0,HORIZ_DIV,1,2,2);
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,0,1,2,2);
   // fixup the big box (these are brutal hacks, sorry)
   OUR_RECTS[0].lr.y=OUR_RECTS[1].ul.y-2;
}

// actual setup for the PnP
static editBrush *flowPnP_br=NULL;
static _PnP_GadgData flowPnP;
static int _local_index;
static IFlowGroupProperty* flowprop = NULL; 

#define DO_GROUP 0
#define DO_EDIT 1
#define DO_UPDATE_WORLD 2
#define DO_EDIT_OBJ 3

// update means the world should be set from the vars, else the opp
static void flowPnP_setvars(editBrush *br, bool update)
{
   flowPnP_br=br;
   if (br==NULL) return;
   if (update)
   {
      brFlow_Index(br)=_local_index;
      i_changed_the_brush(br,FALSE,FALSE,FALSE);
   }
   else
   {
      _local_index=brFlow_Index(br);
   }
}

// Here's what mouse clicks do.
static void flowPnP_DoGroupClick(int button)
{  
   switch (button)
   {
      case DO_EDIT:
      {
         EdMedMoSDESC(brFlow_Index(flowPnP_br));
         break;
      }

      case DO_UPDATE_WORLD:
      {
         GEdMedMoMarkWaterAllBrushes();
         break;
      }

      case DO_EDIT_OBJ:
      {
         ObjID obj = flowprop->GetObj(brFlow_Index(flowPnP_br)); 

         sObjEditorDesc objeddesc = { "" }; 
         AutoAppIPtr_(ObjEditors,pEditors);
         IObjEditor* ed = pEditors->Create(&objeddesc,obj); 

         ed->Go(kObjEdModal);

         SafeRelease(ed); 
         break; 
      }
   }
}

static void flowPnP_IntFrob(PnP_SliderOp op, Rect *where, int val, int data)
{
   if (op==PnP_SliderUpdateOp)
      flowPnP_setvars(flowPnP_br,TRUE);
}

// Hey, this is a lot smaller than the 256 possible flow groups.  
// If we discover that this is enough, maybe we should shrink the db. 
#define MAX_FLOW_INDEX 64

EXTERN void Create_flowPnP(LGadRoot* root, Rect* bounds, editBrush *br)
{
   GFHSetCoordMask(GFH_ALL_COORDS);
   flowPnP_buildRects(bounds);
   PnP_GadgStart(&flowPnP,root);   
   flowPnP_setvars(br,FALSE);
   PnP_PictureBox(getRect(0), NULL, 0); // making this a bool * is super icky
   PnP_SliderInt(getRect(1),"Group", 1, MAX_FLOW_INDEX, 1,
                 &_local_index, flowPnP_IntFrob, DO_GROUP, PNP_SLIDER_VSLIDE);
   PnP_ButtonOneShot(getRect(2), "Edit Group", flowPnP_DoGroupClick, DO_EDIT);
   PnP_ButtonOneShot(getRect(3), "Update In World", flowPnP_DoGroupClick, 
                     DO_UPDATE_WORLD);
   PnP_ButtonOneShot(getRect(4), "Properties", flowPnP_DoGroupClick, 
                     DO_EDIT_OBJ);

   AutoAppIPtr_(PropertyManager,pPropMan); 
   cAutoIPtr<IProperty> prop ( pPropMan->GetPropertyNamed(PROP_FLOW_GROUP_NAME)); 
   Verify(SUCCEEDED(prop->QueryInterface(IID_IFlowGroupProperty,(void**)&flowprop))); 
   

}

EXTERN void Destroy_flowPnP(void)
{
   PnP_GadgFree(&flowPnP);
   flowPnP_br=NULL;
   SafeRelease(flowprop); 
}

EXTERN void Update_flowPnP(GFHUpdateOp op, editBrush *br)
{
   flowPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)
      PnP_FullRedraw(&flowPnP);
}

