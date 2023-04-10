/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/lightpnp.c,v 1.24 2000/02/19 13:10:55 toml Exp $
// lighting PnP gadget

#include <config.h>
#include <gadblist.h>
#include <lgsprntf.h>

#include <command.h>

#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <gedit.h>
#include <brinfo.h>

#include <animlit.h>

#include <brushGFH.h>
#include <PnPtools.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception
 *
 *        0 pic?
 *
 *    1   [type]         (currently 0 or 1, fill in br->tx_id
 *    2<- bright ->      (currently 0.0-16.0, store in br->sz.el[0]
 *    3<- focal  ->      (currently 0.0-16.0?, store in br->sz.el[1]
 *    4<-ambient ->      (ambient for this, if an omni)
 *
 *   < r > < g > < b >    (not supported any time soon, so lets not bother)
 */

#define NUM_BUTTONS (0x5)
static Rect lightPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (lightPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 0
#define BTN_BELOW 4
#define VERT_DIV  1
#define HORIZ_DIV 0

void lightPnP_buildRects(Rect *whole)
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
extern int global_ambient;
static bool _local_type=FALSE;
static editBrush *lightPnP_br=NULL;

static _PnP_GadgData lightPnP;

// these 2 i changed the brushes might be kinda wrong...

// update means the world should be set from the vars, else the opp
static void lightPnP_setvars(editBrush *br, bool update)
{
   lightPnP_br=br;
   if (br==NULL) return;
   if (update)
   {
      brLight_Type(br)=_local_type;
      i_changed_the_brush(br,FALSE,FALSE,FALSE);
   }
   else
   {
      _local_type=brLight_Type(br);
   }
}

#define LIGHT_BRIGHT 1

#pragma disable_message(202)
static void lightPnP_Slide(PnP_SliderOp op,Rect *where, float val, int data)
{
   if ((lightPnP_br==NULL) || (op != PnP_SliderUpdateOp))
      return;
#if 0
   if ((data==LIGHT_BRIGHT)&&(brLight_Handle(lightPnP_br) != -1))
      LightSetBrightness(brLight_Handle(lightPnP_br),(int)val);// - 1
#endif
   i_changed_the_brush(lightPnP_br,FALSE,FALSE,FALSE);
}

static void lightPnP_ChangeInt(Rect *where, bool val, int data)
{
   lightPnP_setvars(lightPnP_br,TRUE);
}
#pragma enable_message(202)

void Create_lightPnP(LGadRoot* root, Rect* bounds, editBrush *br)
{
   GFHSetCoordMask(GFH_NO_D|GFH_NO_W|GFH_NO_H);
   lightPnP_buildRects(bounds);   
   PnP_GadgStart(&lightPnP,root);   
   lightPnP_setvars(br,FALSE);
   PnP_PictureBox(getRect(0),NULL,0);       // making this a bool * is super icky
   PnP_ButtonToggle(getRect(1),"Type:Omni","Type:Spot",&_local_type,lightPnP_ChangeInt,0);
   PnP_SliderFloat(getRect(2),"Bright",1.0,1024.0,4.0,&brLight_Bright(br),
                    lightPnP_Slide,LIGHT_BRIGHT,PNP_SLIDER_VSLIDE);
   PnP_SliderFloat(getRect(3),"Hue",0,1.0,0.05,&brLight_Hue(br),lightPnP_Slide,0,PNP_SLIDER_VSLIDE);
   PnP_SliderFloat(getRect(4),"Saturation",0.0,1.0,0.05,&brLight_Saturation(br),lightPnP_Slide,0,PNP_SLIDER_VSLIDE);
}

void Destroy_lightPnP(void)
{
   PnP_GadgFree(&lightPnP);
   lightPnP_br=NULL;
}

void Update_lightPnP(GFHUpdateOp op, editBrush *br)
{
   lightPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)
      PnP_FullRedraw(&lightPnP);
}

