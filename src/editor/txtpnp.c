/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/txtpnp.c,v 1.48 2000/02/19 13:11:37 toml Exp $
// Brush Texture PnP gadget

#include <string.h>

#include <gadblist.h>
#include <lgsprntf.h>
#include <guistyle.h>
#include <config.h>
#include <winui.h>

#include <resapi.h>
#include <storeapi.h>

#include <command.h>
#include <status.h>

#include <editor.h>
#include <editbr.h>
#include <editbr_.h>
#include <gedit.h>
#include <ged_csg.h>
#include <brinfo.h>

#include <brushGFH.h>
#include <PnPtools.h>
#include <swappnp.h>
#include <txtrpal.h>
#include <texmem.h>
#include <family.h>
#include <txtpnp.h>
#include <primal.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/* artists conception
 *            ___________
 * 1<align>   |0 pict of  |  (grid flips to the brush grid pnp)
 * 2<grid>    |   brush   |  (show causes main 3d view to try to look at this wall)
 * 3<show>    |           |  (reset resets all fields to defaults)
 * 4<reset>   |___________|
 *        5<-media->
 *        6<-face->          (face is 0-n int)
 *       7<- texture ->      (texture is 0-n (string list))
 */

#define NUM_BUTTONS (0x8)
static Rect txtPnP_rects[NUM_BUTTONS];
#define OUR_RECTS (txtPnP_rects)
#define getRect(n) (&OUR_RECTS[n])

#define BTN_ABOVE 4
#define BTN_BELOW 3
#define VERT_DIV  4
#define HORIZ_DIV (0.45)

void txtPnP_buildRects(Rect *whole)
{
   Rect *curRect=OUR_RECTS;
   int i;

   buildYLineRect(curRect++,whole,0,HORIZ_DIV,1,2,1);
   for (i=0; i<BTN_ABOVE; i++)
      buildYLineRect(curRect++,whole,i,HORIZ_DIV,0,2,1);
   for (i=0; i<BTN_BELOW; i++)
      buildYLineRect(curRect++,whole,VERT_DIV+i,0,1,2,2);
   // fixup the big box (these are brutal hacks, sorry)
   OUR_RECTS[0].lr.y=OUR_RECTS[4].lr.y;
}

#define BUTTON_ALIGN   1
#define BUTTON_GRID    2
#define BUTTON_SHOW    3
#define BUTTON_RESET   4

#define FROBS_PICTURE  5

#define FROBS_FACE     6
#define FROBS_MEDIA    7
#define FROBS_TEXTURE  8

#define NUM_OWNED (FROBS_TEXTURE+1)

// statics for the PnP itself
static int pnpOwners[NUM_OWNED];
static editBrush *txtPnP_br;
static int face, media, texture;
static BOOL im_detached=FALSE;

static _PnP_GadgData txtPnP;

// to get texture id stuff
extern short *_geditGetTxPtr(editBrush *br);

// media data
extern const char *mediaop_names[];
extern int num_media_ops;

// face data
static const char *cube_faces[] =  // for a natural cube
  {"default", "north", "west", "south", "east", "top", "bottom"};
static const char *side_names[] =  // for various sides of a multibrush
  {"side 1","side 2","side 3","side 4","side 5","side 6","side 7","side 8","side 9","side 10",
   "side 11","side 12","side 13","side 14","side 15","side 16","side 17","side 18","side 19", "side 20"};
const char *faces[] =       // the actual use me in the PnP thing
  {"", "", "", "", "", "", "", "", "", "", "", "", ""};

int cur_max_face=0, cur_primal=-1;

static void txtPnP_rebuild_face_info(int primal_id)
{
   void *str_gadg=PnP_GetStringSubGadget(pnpOwners[FROBS_FACE]);
   int i=0, j;
   if (primal_id==PRIMAL_CUBE_IDX)
      for (i=0; i<sizeof(cube_faces)/sizeof(char *); i++)
         faces[i]=cube_faces[i];
   else
   {
      BOOL is_pyr=(primalID_GetType(primal_id)==PRIMAL_TYPE_PYRAMID)||(primalID_GetType(primal_id)==PRIMAL_TYPE_CORNERPYR);
      int eff_face_cnt=primalBr_FaceCnt(primal_id)-(is_pyr?1:2);
      faces[i++]=cube_faces[0];
      for (j=0; j<eff_face_cnt; j++)
         faces[i++]=side_names[j];
      if (!is_pyr)
         faces[i++]=cube_faces[5];
      faces[i++]=cube_faces[6];
   }
   cur_primal=primal_id;
   cur_max_face=primalBr_FaceCnt(primal_id);
   if (face>cur_max_face) face=-1; // ?? or max?
   if (str_gadg)
      PnP_StringSliderSetStrings(str_gadg,faces,cur_max_face); // or +1?
}

// this is sort of a mess
// basically, this is here to sync the globals and the brush
// update is TRUE if we know the globals just changed
// and false if we know the brush just changed, so we want to reload the globals
// the messy part is the interdependance, esp. the update=FALSE in the face change
//  since if the globals changed the face, we want to update it (ie. reload the brush
//  from the face varaible) but then disable update so that we reload the appropriate
//  new texture from the brush into the global for the new face
static void txtPnP_setvars(editBrush *br, bool update)
{
   bool change=FALSE;

   if (im_detached) return;
   if (br==NULL) { txtPnP_br=NULL; return; }

   if (br->primal_id!=cur_primal)
   {
      txtPnP_rebuild_face_info(br->primal_id);
      PnP_Redraw(pnpOwners[FROBS_FACE],NULL);
   }

   PnP_SetCurrent(&txtPnP);
   if (face!=br->cur_face+1)    // since face is 0-n+1 whereas in brush is -1,0-n
   {
      if (update)
      {
         br->cur_face=face-1;
         gedit_redraw_selection();
         update=FALSE;          // since face drives the other, set update to false
      }                         // so that texture reloads from the newly set face
      else
         face=br->cur_face+1;
      PnP_Redraw(pnpOwners[FROBS_FACE],NULL);
      change=TRUE;
   }
   if (media!=br->media)
   {
      if (update)
         br->media=media;
      else
         media=br->media;
      PnP_Redraw(pnpOwners[FROBS_MEDIA],NULL);
      change=TRUE;
   }
   if (texture!=(int)(*_geditGetTxPtr(br)))
   {
      if (update)
         gedit_texture_brush(br,texture);
      else
         texture=(int)(*_geditGetTxPtr(br));
      PnP_Redraw(pnpOwners[FROBS_TEXTURE],NULL);
      PnP_Redraw(pnpOwners[FROBS_PICTURE],NULL); // make sure we do the picture too
      change=TRUE;
   }
   txtPnP_br=br;
   if (change&&update)
      gedit_redraw_3d();
}

// just do these one of command sort of things
static void txtPnP_OneShots(int lid)
{
   switch (lid)
   {
      case BUTTON_GRID:  PnP_ExecCommandInt("start_pnp", PNP_GRID); break;
      case BUTTON_ALIGN: PnP_ExecCommandInt("start_pnp", PNP_ALIGN); break;
      case BUTTON_SHOW:  CommandExecute("cam_to_brush"); break;
      case BUTTON_RESET:
         PnP_ExecCommandInt("reset_brush_tx",texture);
         txtPnP_setvars(txtPnP_br,FALSE);
         break;
   }
}

// the idea here is that this just makes sure the globals are right
// then calls setvars, which deals with making the brush consistent with the globals
// or, if say a face changes, the globals consistent with the brush
static void txtPnP_IntFrob(PnP_SliderOp op,Rect *where, int val, int data)
{
   if (op != PnP_SliderUpdateOp)
      return;
   switch (data)
   {
      case FROBS_FACE:  // deal with 
         if (face>cur_max_face)
            face=0;
         break;
      case FROBS_MEDIA:     break;
   }
   txtPnP_setvars(txtPnP_br,TRUE);
}

void texture_pal_cmd(void);

static void txtPnP_TxtFrob(PnP_SliderOp op,Rect *where, int val, int data)
{
   if (op == PnP_SliderUpdateOp)
   {
      if ((val==-1)&&(face==0))
         texture=0;          // cant set to default if you are whole brush
      if (val>=texmemGetMax())
      {
         int mid=texmemGetMax()+((BACKHACK_IDX-texmemGetMax())/2);
         if ((texture<mid)||(texture>=BACKHACK_IDX)) texture=BACKHACK_IDX;
         else texture=texmemGetMax()-1;
      }
      txtPnP_setvars(txtPnP_br,TRUE);
   }
   else
      texture_pal_cmd();
}

static void stringToCanvasBottom(char *str, BOOL shadow, BOOL hilite)
{
   short w, h;
   int x,y;
   
   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_string_size(str,&w,&h);
   x=(grd_canvas->bm.w-w)/2;
   y=grd_canvas->bm.h-h-2;
   if (shadow)
   {
      gr_set_fcolor(guiStyleGetColor(NULL,StyleColorBG));
      gr_string(str,x+2,y+2);
   }
   if (hilite)
      gr_set_fcolor(guiStyleGetColor(NULL,StyleColorHilite));
   else
      gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG));
   gr_string(str,x,y);
   guiStyleCleanupFont(NULL,StyleFontNormal);
}

static BOOL FillInName(char *buf, BOOL full, int handle)
{
   IRes *pRes = texmemGetHandle(handle);
   if (pRes)
   {
      if (full)
      {
         char *p;
         IRes_GetCanonPathName(pRes, &p);
         strcpy(buf, p);
         Free(p);
      }
      else
         strcpy(buf,IRes_GetName(pRes));
      strlwr(buf);
      return TRUE;
   }
   return FALSE;
}

// if which is -1, uses the current texture of the current brush
BOOL drawTexturetoCanvas(int mt, editBrush *br, BOOL active)
{
   char buf[128];
   
   if ((mt==-1)&&(br!=NULL))
      mt=(int)(*_geditGetTxPtr(br));
   
   if (texmemValidIdx(mt))
      gr_scale_bitmap(texmemGetTexture(mt),0,0,grd_canvas->bm.w,grd_canvas->bm.h);
   else
   {
      gr_clear(0);
      return FALSE;
   }

   if (FillInName(buf,FALSE,mt))
      stringToCanvasBottom(buf,TRUE,active);
   else
      stringToCanvasBottom("Jorge",TRUE,active);

   return TRUE;
}

static void draw_string_hack(int color, char *str)
{
   gr_clear(color);
   stringToCanvasBottom(str,TRUE,TRUE);
}

#pragma off(unreferenced)
// this really should probably just load from the brush
// since we really hope it is consistent and right when we get here
static void txtPnP_Picture(int data)
{
   int mt=texture;

   if (im_detached||(txtPnP_br==NULL))
      draw_string_hack(0,"Detached");
   else if (brSelect_Flag(txtPnP_br)&brSelect_VIRTUAL)
      draw_string_hack(0,"MULTI\nBRUSH");
   else if (mt==BACKHACK_IDX)
      draw_string_hack(0,"SKY HACK");      
   else
   {
      if (mt==-1)
         if (txtPnP_br->cur_face!=-1)
            mt=txtPnP_br->tx_id;
      drawTexturetoCanvas(mt,NULL,FALSE);
   }
}
#pragma on(unreferenced)

BOOL txtPnP_allow_attach=FALSE;

void Create_txtPnP(LGadRoot *root, Rect *bounds, editBrush *br)
{
   txtPnP_allow_attach=config_is_defined("allow_attach");
//   im_detached=FALSE;
   GFHSetCoordMask(GFH_ALL_COORDS);
   txtPnP_buildRects(bounds);
   PnP_GadgStart(&txtPnP,root);
   txtPnP_setvars(br,FALSE);
   pnpOwners[FROBS_PICTURE]=PnP_PictureBox(getRect(0),txtPnP_Picture,0);
   if (!brushGFH_AlignNTextureUp())
   {  // not a lot of point in these if both PnP's are up, eh?
      PnP_ButtonOneShot(getRect(1),"Align",txtPnP_OneShots,BUTTON_ALIGN);
      PnP_ButtonOneShot(getRect(2),"Grid",txtPnP_OneShots,BUTTON_GRID);
   }
   PnP_ButtonOneShot(getRect(3),"Show",txtPnP_OneShots,BUTTON_SHOW);
   PnP_ButtonOneShot(getRect(4),"Reset",txtPnP_OneShots,BUTTON_RESET);
   pnpOwners[FROBS_MEDIA]=
      PnP_SliderString(getRect(5),"Op",num_media_ops,mediaop_names,&media,txtPnP_IntFrob,FROBS_MEDIA,PNP_SLIDER_MENU);
   pnpOwners[FROBS_FACE]=
      PnP_SliderString(getRect(6),"Face",br->num_faces+1,faces,&face,txtPnP_IntFrob,FROBS_FACE,PNP_SLIDER_MENU|PNP_SLIDER_WRAP);
   if (!config_is_defined("txt_click"))
   {
      pnpOwners[FROBS_TEXTURE]=
         PnP_SliderInt(getRect(7),"Texture",-1,BACKHACK_IDX,1,&texture,txtPnP_TxtFrob,
                       FROBS_TEXTURE,PNP_SLIDER_VSLIDE); // |PNP_SLIDER_CLICKS);
   }
   else
   {
      pnpOwners[FROBS_TEXTURE]=
         PnP_SliderInt(getRect(7),"Texture",-1,BACKHACK_IDX,1,&texture,txtPnP_TxtFrob,
                       FROBS_TEXTURE,PNP_SLIDER_VSLIDE|PNP_SLIDER_CLICKS);
   }
}

void Destroy_txtPnP(void)
{
   PnP_GadgFree(&txtPnP);
   txtPnP_br=NULL;
}

void Update_txtPnP(GFHUpdateOp op, editBrush *br)
{
   if (!im_detached)
      txtPnP_setvars(br,FALSE);
   if (op==GFH_FORCE)
      PnP_FullRedraw(&txtPnP);
   TexturePaletteUpdate(GFH_MINIMAL);
}

BOOL txtPnP_attached(void)
{
   return (txtPnP_br!=NULL) && (!im_detached);
}

void txtPnP_detach(void)
{
   if (!im_detached)
   {
      im_detached=TRUE;
      PnP_SetCurrent(&txtPnP);
      PnP_Redraw(pnpOwners[FROBS_PICTURE],NULL); // make sure we do the picture too
   }
}

void txtPnP_attach(void)
{
   im_detached=FALSE;
   if (TexturePaletteVisible())  // hey, we be on screen
      txtPnP_setvars(txtPnP_br,FALSE);
   GFHRedrawPnPnSuch();
}

static bool txtpal_update(int txt)
{
   if (TpalIsExtraButton(txt))
      if (TpalGetExtraCode(txt)==TPAL_XTRA_SKY)
         txt=BACKHACK_IDX;
      else
      {
         switch (TpalGetExtraCode(txt))
         {
         case TPAL_XTRA_DETACH:
            if (txtPnP_allow_attach)
            {
               if (im_detached)
                  txtPnP_attach();
               else
                  txtPnP_detach();
            }
            else // if (assign_button) - if !allow_attach treat detach/attach as paint into current
               if (txtPnP_br)
               {
                  im_detached=FALSE;              // do we need these?
                  txtPnP_setvars(txtPnP_br,TRUE);
                  im_detached=TRUE;
               }
            break;
         case TPAL_XTRA_REMOVE:
            if (texture!=BACKHACK_IDX)
            {  // ask if they really want to remove it
               char buf[200];
               strcpy(buf,"Do you really want to Remove\n");
               if (FillInName(buf+strlen(buf),TRUE,texture))
                  if (winui_GetYorN(buf))
                  {
                     family_free_single(texture);
                     texture=0;
                  }
            }
            break;
         default:
            Warning(("Txtpal_Update in default case\n"));
            break;
         }
         TexturePaletteSelect(texture);
         return FALSE;
      }
   texture = txt;
   {
      char buf[200];
      if (FillInName(buf,TRUE,texture))
         Status(buf);
   }
   if (txtPnP_allow_attach)
      if ((txtPnP_br)&&(!im_detached))
         txtPnP_setvars(txtPnP_br,TRUE);
   return FALSE;
}

void texture_pal_cmd(void)
{
   ToggleTexturePalette(txtpal_update);
   if (TexturePaletteVisible())  // hey, we be on screen
   {
      if (!txtPnP_allow_attach)
         txtPnP_detach();
      TexturePaletteSelect(texture);
   }
}

// returns -2 (NO_TXT_PAL) if not, else current texture selection
int txtPnP_pal_query(void)
{
   if (TexturePaletteVisible())  // hey, we be on screen
      return texture;
   else
      return TXTPNP_NO_PAL_UP;
}
