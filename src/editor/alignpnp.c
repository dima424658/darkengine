/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/alignpnp.c,v 1.23 2000/02/19 12:27:36 toml Exp $
// Brush Texture PnP gadget

#include <gadblist.h>
#include <lgsprntf.h>

#include <command.h>

#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <gedit.h>
#include <csgbrush.h>
#include <csg.h>

#include <brushGFH.h>
#include <PnPtools.h>
#include <swappnp.h>
#include <texmem.h>

#include <mprintf.h>
#include <rand.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception
 *            ___________
 * 1<txt>     | grid rep |  (grid flips to the brush grid pnp)
 * 2<grid>    |so you can|  (show causes main 3d view to try to look at this wall)
 * 3<reset>   | vslider  |  (reset resets all fields to defaults)
 * 4<brush>   |__________|  ( aligns the texture to the brush
 *  5<-uoff->   6<-voff->   (0-256 ints
 *     7 <- scale ->        (scale 4 default, who knows - wacky scale thing)
 *     8  <- rot ->         (rot is angular, not implemented)
 */

#define NUM_BUTTONS (0x9)
static Rect alignPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (alignPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 4
#define BTN_BELOW 3
#define VERT_DIV  4
#define HORIZ_DIV (0.50)

void alignPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   buildYLineRect(curRect++,whole,0,HORIZ_DIV,1,2,1);
   for (i=0; i<BTN_ABOVE; i++)
      buildYLineRect(curRect++,whole,i,HORIZ_DIV,0,2,1);
   buildYLineRect(curRect++,whole,VERT_DIV,HORIZ_DIV,0,2,2);
   buildYLineRect(curRect++,whole,VERT_DIV,HORIZ_DIV,1,2,2);
   for (i=1; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,0,1,2,2);
   // fixup the big box (these are brutal hacks, sorry)
   OUR_RECTS[0].lr.y=OUR_RECTS[4].lr.y;
}

#define BUTTON_TXT     1
#define BUTTON_GRID    2
#define BUTTON_RESET   3
#define BUTTON_ALIGN   4

#define FROBS_XOFFS    5
#define FROBS_YOFFS    6
#define FROBS_SCALE    7
#define FROBS_ROT      8

#define FROBS_PICTURE  9

#define NUM_OWNED (FROBS_PICTURE+1)
static int pnpOwners[NUM_OWNED];

// particular state for the gadget
static editBrush *alignPnP_br;
static int        xoff, yoff, scale, texture;
static fixang     rot;
static TexInfo   *alignTexInf=(TexInfo *)0xffffffff; // @HACK: try and make sure first use has to refresh this
static bool       _local_align=FALSE;

// for now, this is our PnP Set
static _PnP_GadgData alignPnP;

// hide or show all the buttons of face info, for on/off when you switch to null face
static void frobs_hide(bool which)
{
   int i;
   for (i=FROBS_XOFFS; i<=FROBS_ROT; i++)
      PnP_Hide(pnpOwners[i],which);
}

// TODO: bug is that on first run through, if you are set to cur_face -1
//  then both alignTexInf and this_face are NULL, so we dont execute switch
//  so, then we dont hide the frobs cause we dont think anything changed

// this is sort of a mess, as it syncs the globals and the brush
// update is TRUE if we know the globals just changed, false to reload globals
// here we have to make sure cur_face didnt secretly change behind our back
static void alignPnP_setvars(editBrush *br, bool update)
{
   BOOL change=FALSE, face_change=FALSE;
   TexInfo *this_face;
   int mt;

   if (!update)       alignPnP_br=br;   // not an update: set us from data
   else if (br==NULL) br=alignPnP_br;   // else allow reload from brush

   if (br==NULL) return;

   if (br->cur_face==-1) this_face=NULL;   // no face data to work with;
   else                  this_face=&br->txs[br->cur_face];
   if (update&&(this_face!=alignTexInf))
      Warning(("Hey, face switched behind the scene??\n"));

   PnP_SetCurrent(&alignPnP);
   if (alignTexInf!=this_face)
   {
      if (alignTexInf==NULL)   // unhide the boxes
      {
         frobs_hide(FALSE);
      }
      alignTexInf=this_face;
      face_change=TRUE;
      PnP_Redraw(pnpOwners[FROBS_PICTURE],NULL);
   }
   if (alignTexInf!=NULL)
   {
      BOOL rotalign;
      _varforPnP_synch(alignTexInf->tx_x,xoff,update,pnpOwners[FROBS_XOFFS],change);
      _varforPnP_synch(alignTexInf->tx_y,yoff,update,pnpOwners[FROBS_YOFFS],change);
      _varforPnP_synch(alignTexInf->tx_scale,scale,update,pnpOwners[FROBS_SCALE],change);
      _varforPnP_synch(alignTexInf->tx_rot,rot,update,pnpOwners[FROBS_ROT],change);

      rotalign=(rot==TEXINFO_HACK_ALIGN);
      if ((_local_align)!=rotalign)
         PnP_Redraw(pnpOwners[BUTTON_ALIGN],NULL);
      PnP_Hide(pnpOwners[FROBS_ROT],rotalign);

      mt=alignTexInf->tx_id;
      if (mt==-1) mt=alignPnP_br->tx_id;
      if (mt!=texture) 
         PnP_Redraw(pnpOwners[FROBS_PICTURE],NULL);
   }
   else if (face_change)
      frobs_hide(TRUE);
   if (change&&update)
      if (gedit_reassign_texture(br,FALSE))
         gedit_redraw_3d();
}

// @HACK: this is ultra-gross, but im not sure how to get this to bootstrap right
//   in particular, we detect fine, but too early, ie. i cant hide the frobs, cause 
//   they dont exist yet.  sounds like i really need a 3rd input state... ie. i
//   need Update Gadget From World (false), Update World From Gadget (true), and Initialize
static void sneaky_hideshow_refresh(void)
{
   alignTexInf=(TexInfo *)0xffffffff; // @HACK: try and make sure first use has to refresh this
   rot=0;
   alignPnP_setvars(alignPnP_br,FALSE);
   PnP_FullRedraw(&alignPnP);
}

// the idea here is that this just makes sure the globals are right
// then calls setvars, which deals with making the brush consistent with the globals
static void alignPnP_IntFrob(PnP_SliderOp op, Rect *where, int val, int data)
{
   if (op == PnP_SliderUpdateOp)
      alignPnP_setvars(NULL,TRUE);
}

static void alignPnP_FixangFrob(PnP_SliderOp op, Rect *where, fixang val, int data)
{
   if (op == PnP_SliderUpdateOp)
      alignPnP_setvars(NULL,TRUE);
}

// @TODO: figure out why we cant get the ROTATE gadget to update, eh?
// just do these one of command sort of things
static void alignPnP_OneShots(int lid)
{
   switch (lid)
   {
      case BUTTON_TXT:
         PnP_ExecCommandInt("start_pnp", PNP_TEXTURE);
         break;
      case BUTTON_GRID:
         PnP_ExecCommandInt("start_pnp", PNP_GRID);
         break;
      case BUTTON_RESET:
         if (alignTexInf!=NULL)
         {  
            BOOL change=FALSE;
            int tmp=alignTexInf->tx_id;
            *alignTexInf=defTexInf;
            alignTexInf->tx_id=tmp;
            // ok, this is pretty gross - we cant just setvars since no update will mean no reassign textures
            PnP_SetCurrent(&alignPnP);
            _varforPnP_synch_FALSE(alignTexInf->tx_x,xoff,pnpOwners[FROBS_XOFFS],change);
            _varforPnP_synch_FALSE(alignTexInf->tx_y,yoff,pnpOwners[FROBS_YOFFS],change);
            _varforPnP_synch_FALSE(alignTexInf->tx_scale,scale,pnpOwners[FROBS_SCALE],change);
            _varforPnP_synch_FALSE(alignTexInf->tx_rot,rot,pnpOwners[FROBS_ROT],change);
            if (change&&gedit_reassign_texture(alignPnP_br,FALSE))
               gedit_redraw_3d();
         }
         break;
   }
}

static void alignPnP_Toggle(Rect *where, bool val, int data)
{
   if (rot==TEXINFO_HACK_ALIGN) rot=0; else rot=TEXINFO_HACK_ALIGN;
   alignPnP_setvars(NULL,TRUE);
}

// this really should probably just load from the brush
// since we really hope it is consistent and right when we get here
// but hey, oh well

// @TODO: this should really call the txtpnp stuff, which we should generalize
static void alignPnP_Picture(int data)
{
   if (alignPnP_br)
      if (alignPnP_br->cur_face==-1)
         gr_clear(0);                   // cant really do this
      else
      {
         int mt=alignPnP_br->txs[alignPnP_br->cur_face].tx_id;
         if (mt==-1)
            mt=alignPnP_br->tx_id;
         texture=mt;
         if (texmemValidIdx(mt))
            gr_scale_bitmap(texmemGetTexture(mt),0,0,grd_canvas->bm.w,grd_canvas->bm.h);
         else
            gr_clear(0);
      }
}
#pragma on(unreferenced)

#define MIN_SCALE 8
#define MAX_SCALE 24

void Create_alignPnP(LGadRoot *root, Rect *bounds, editBrush *br)
{
   GFHSetCoordMask(GFH_ALL_COORDS);
   alignPnP_buildRects(bounds);
   PnP_GadgStart(&alignPnP,root);
   alignPnP_setvars(br,FALSE);
   pnpOwners[FROBS_PICTURE]=PnP_PictureBox(getRect(0),alignPnP_Picture,0);
   if (!brushGFH_AlignNTextureUp())
   {  // not a lot of point in these if both PnP's are up, eh?
      PnP_ButtonOneShot(getRect(1),"Txt",alignPnP_OneShots,BUTTON_TXT);
      PnP_ButtonOneShot(getRect(2),"Grid",alignPnP_OneShots,BUTTON_GRID);
   }
   PnP_ButtonOneShot(getRect(3),"Reset",alignPnP_OneShots,BUTTON_RESET);
   // @TODO: make this a maintained toggle??
   pnpOwners[BUTTON_ALIGN]=
      PnP_ButtonToggle(getRect(4),"AlignNorm","AlignBr",&_local_align,alignPnP_Toggle,BUTTON_ALIGN);

   pnpOwners[FROBS_XOFFS]=
      PnP_SliderInt(getRect(5),"U",0,CB_MAX_ALIGN,1,&xoff,alignPnP_IntFrob,FROBS_XOFFS,PNP_SLIDER_VSLIDE);
   pnpOwners[FROBS_YOFFS]=
      PnP_SliderInt(getRect(6),"V",0,CB_MAX_ALIGN,1,&yoff,alignPnP_IntFrob,FROBS_YOFFS,PNP_SLIDER_VSLIDE);
   pnpOwners[FROBS_SCALE]=
      PnP_SliderInt(getRect(7),"Scale",0,MAX_SCALE,1,&scale,alignPnP_IntFrob,FROBS_SCALE,0);
   pnpOwners[FROBS_ROT]=
      PnP_SliderFixang(getRect(8),"Rot",0,0xFFFF,1,&rot,alignPnP_FixangFrob,FROBS_ROT,PNP_SLIDER_VSLIDE|PNP_SLIDER_WRAP);
   sneaky_hideshow_refresh();    // probably the problem on return back from game to edit mode, eh?
}

void Destroy_alignPnP(void)
{
   PnP_GadgFree(&alignPnP);
   alignPnP_br=NULL;
}

void Update_alignPnP(GFHUpdateOp op, editBrush *br)
{
   alignPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)
      sneaky_hideshow_refresh();
}
