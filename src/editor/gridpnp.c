/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/gridpnp.c,v 1.15 2000/02/19 13:10:51 toml Exp $
// Brush Grid PnP gadget

#include <gadblist.h>
#include <lgsprntf.h>

#include <command.h>

#include <editor.h>
#include <editbr.h>

#include <brushGFH.h>
#include <PnPtools.h>
#include <swappnp.h>

#include <mprintf.h>
#include <rand.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception
 *           ___________
 * 1<txtr>   |0 pic of |  (txtr flips to the brush txtr pnp)
 * 2<tog>    | grid??? |  (tog toggles grid on/off, about to change)
 * 3<reset>  |_________|  (reset resets all fields to defaults)
 * 4 world     us snap
 * 5<-size-> 8<-xoffs->   (size,pos are float 0->4.0, rot angle 0-360?)
 * 6<- pos-> 9<-yoffs->     (applied to created brushes, global)
 * 7<- rot-> A<-scale->   (xoffs,yoffs,scale are per brush for snapping)
 *                               (which is currently unsupported)
 */

#define NUM_BUTTONS (0xB)
static Rect gridPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (gridPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 3
#define BTN_BELOW 3
#define VERT_DIV  4
#define HORIZ_DIV (0.50)

void gridPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   buildYLineRect(curRect++,whole,0,HORIZ_DIV,1,2,2);
   for (i=0; i<BTN_ABOVE; i++)
      buildYLineRect(curRect++,whole,i,HORIZ_DIV,0,2,2);
   buildYLineRect(curRect++,whole,i,0,1,2,2);   // bonus Line!!
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,0,2,2);
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,HORIZ_DIV,1,2,2);
   // fixup the big box (these are brutal hacks, sorry)
   OUR_RECTS[0].lr.y=OUR_RECTS[3].lr.y;
}

// this would draw a picture or something, probably takes more args
// for now, probably just draw a box of Rand() color to know when it redraws
void gridPnP_Picture(int data)
{

}

#define BUTTON_TEXTURE 1
#define BUTTON_ALIGN   2
#define BUTTON_TOGGLE  3
#define BUTTON_RESET   4

#define FROBS_W_SCALE  5

//static int g_scale=16;        // is 4.0.  so 2^16 is 4, so formula
static editBrush *gridPnP_br=NULL;

static _PnP_GadgData gridPnP;

void gridPnP_setvars(editBrush *br)
{
   gridPnP_br=br;
}

// this will be called for all one shots with the final parameter
// or we could have a separate callback for each oneshot, if that is easier
void gridPnP_OneShots(int lid)
{
   switch (lid)
   {
      case BUTTON_TEXTURE:  PnP_ExecCommandInt("start_pnp", PNP_TEXTURE); break;
      case BUTTON_ALIGN:    PnP_ExecCommandInt("start_pnp", PNP_ALIGN); break;
      case BUTTON_TOGGLE:   CommandExecute("grid_toggle"); break;
      case BUTTON_RESET:    break;
   }
}

#ifdef WORKED
void gridPnP_Update(PnP_SliderOp op, Rect *where, int val, int data)
{
   extern void grid_pow2_scale(int pow2);
   if (op != PnP_SliderUpdateOp)
      return;
   switch (data)
   {
      case FROBS_W_SCALE: grid_pow2_scale(val); break;
   }
}

void gridPnP_Update_1(PnP_SliderOp op, Rect *where, float val, int data)
{
   mprintf("at %d\n",data);
}

void gridPnP_Update_2(PnP_SliderOp op, Rect *where, fixang val, int data)
{
   mprintf("at %d (%x)\n",data,val);
}

void gridPnP_Update_3(PnP_SliderOp op, Rect *where, short val, int data)
{
   mprintf("at %d (%d)\n",data,val);
}

static float gridVar;
static fixang gridVar2;
static short gridVar3;
#endif

// create a grid PnP
void Create_gridPnP(LGadRoot *root, Rect *bounds, editBrush *br)
{
   GFHSetCoordMask(GFH_ALL_COORDS);
   gridPnP_buildRects(bounds);
   PnP_GadgStart(&gridPnP,root);
   PnP_PictureBox(getRect(0),gridPnP_Picture,0);
   PnP_ButtonOneShot(getRect(1),"Texture",gridPnP_OneShots,BUTTON_TEXTURE);
   PnP_ButtonOneShot(getRect(2),"Align",gridPnP_OneShots,BUTTON_ALIGN);   
#ifdef WORKED
   PnP_ButtonOneShot(getRect(2),"Toggle",gridPnP_OneShots,BUTTON_TOGGLE);
   PnP_ButtonOneShot(getRect(3),"Reset",gridPnP_OneShots,BUTTON_RESET);
   PnP_TextBox(getRect(4),"   World  -  Us Snap  ");
   PnP_SliderInt(getRect(5),"Size",0,24,1,&g_scale,gridPnP_Update,FROBS_W_SCALE,0);
   PnP_SliderFloat(getRect(6),"Pos",0.0,4.0,0.01,&gridVar,gridPnP_Update_1,0,FALSE);
   PnP_SliderFixang(getRect(7),"Rot",0,0,&gridVar2,gridPnP_Update_2,0,FALSE);
   PnP_SliderShort(getRect(8),"XOff",0,0,&gridVar3,gridPnP_Update_3,0,FALSE);
   PnP_SliderShort(getRect(9),"YOff",0,0,&gridVar3,gridPnP_Update_3,0,FALSE);
   PnP_SliderShort(getRect(10),"Scale",0,0,&gridVar3,gridPnP_Update_3,0,FALSE);
#endif
}

void Destroy_gridPnP(void)
{
   PnP_GadgFree(&gridPnP);
   gridPnP_br=NULL;
}

void Update_gridPnP(GFHUpdateOp op, editBrush *br)
{

}

