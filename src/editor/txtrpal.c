/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/txtrpal.c,v 1.16 2000/02/19 13:13:39 toml Exp $

#include <string.h>
#include <stdlib.h>

#include <mprintf.h>
#include <dev2d.h>
#include <gadbox.h>
#include <gadblist.h>

#include <vumanui.h>
#include <editbr.h>
#include <editbr_.h>
#include <gedit.h>
#include <cmdbutts.h>
#include <txtrpal.h>
#include <txtpnp.h>
#include <texmem.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static struct _TextPal
{
   LGadRoot* root;  // grabbed from view manager
   LGadButtonList list;
   DrawElement* elems;
   int *xtra;       // to hold callback data, for now
   Rect* rects;
   TexturePalCall update;
   int numbutts;
} TextPal;

static BOOL phoenix_rising=FALSE;

#define TP_EXTRA_BUTTONS 4 
#define TP_DONE_BUILD_INDEX(palptr)   ((palptr)->numbutts-5+TPAL_XTRA_DONE)
#define TP_DETACH_BUILD_INDEX(palptr) ((palptr)->numbutts-5+TPAL_XTRA_DETACH)
#define TP_REMOVE_BUILD_INDEX(palptr) ((palptr)->numbutts-5+TPAL_XTRA_REMOVE)
#define TP_SKY_BUILD_INDEX(palptr)    ((palptr)->numbutts-5+TPAL_XTRA_SKY)

static void textpal_draw_cb(DrawElement* elem, DrawElemState state)
{
   extern BOOL drawTexturetoCanvas(int mt, editBrush *br, BOOL active);
   int n=(int)elem->draw_data2;

   if ((n >= 0)&&(n<TextPal.numbutts-TP_EXTRA_BUTTONS))
      drawTexturetoCanvas(n,NULL,state==dsDEPRESSED);
   else
      gr_clear(0);
}

////////////////////////////////////////

#pragma off(unreferenced)
static void destroy_me(void* arg)
{
   DestroyTexturePalette();
}

static bool textpal_button_cb(ushort action, int button, void* data, LGadBox* vb)
{
   bool destroy = (action & LGADBUTT_LDOUBLE) != 0;
   int cb_val=TextPal.xtra[button];

   if (TpalGetExtraCode(cb_val)==TPAL_XTRA_DONE)
      destroy = TRUE;
   else
      if (action & (LGADBUTT_LCLICK|LGADBUTT_LDOUBLE))
         if (TextPal.update != NULL)
            destroy = TextPal.update(cb_val) || destroy;
   if (destroy)
      uiDefer(destroy_me,NULL);
   return TRUE;
}
#pragma on(unreferenced)

////////////////////////////////////////

#define DIMS_RATIO_X 3
#define DIMS_RATIO_Y 1

void CreateTexturePalette(TexturePalCall update)
{
   struct _TextPal* pal = &TextPal;
   int n = texmemGetMax()+TP_EXTRA_BUTTONS;
   int i; 

   pal->update = update;

   pal->root = vmGrabViews(vmGrabTwoWide);
   if (pal->root == NULL)
   {
      Warning(("Failed to grab texture palette views\n"));
      return;
   }
   
   pal->rects = Malloc(n*sizeof(*pal->rects));
   pal->elems = Malloc(n*sizeof(*pal->elems));
   pal->xtra  = Malloc(n*sizeof(*pal->xtra));
   pal->numbutts = n;

   for (i = 0; i < n; i++)
   {
      DrawElement* elem = &pal->elems[i];
      ElementClear(elem);
      if (i < n-TP_EXTRA_BUTTONS)
      {
         elem->draw_type  = DRAWTYPE_CALLBACK;
         elem->draw_data  = textpal_draw_cb;
         elem->draw_data2 = (void*)i;
         pal->xtra[i]=i;
      }
      else
      {
         elem->draw_type  = DRAWTYPE_TEXT;
         pal->xtra[i]=(-(1+i-(n-TP_EXTRA_BUTTONS)));
         
         if (i==TP_DONE_BUILD_INDEX(pal))
            elem->draw_data = "Done";
         else if (i==TP_SKY_BUILD_INDEX(pal))
            elem->draw_data = "Sky";
         else if (i==TP_REMOVE_BUILD_INDEX(pal))
            elem->draw_data = "Remove\nfrom\nLevel";
         else if (i==TP_DETACH_BUILD_INDEX(pal))
         {
            if (txtPnP_allow_attach)
               elem->draw_data = "detach\nattach";
            else
               elem->draw_data = "put on\nbrush";
         }
         else
            elem->draw_data = "Bug!\nReport\nMe";
         elem->draw_flags = BORDER(DRAWFLAG_BORDER_OUTLINE);
      }
   }
   
   // compute rectangles
   {
      Rect* box = LGadBoxRect(pal->root);
      Rect bounds = { {0,0},};
      Point ratio; 
      Point dims; 
      int xr = 0 ,yr = 0;
      bounds.lr = MakePoint(RectWidth(box),RectHeight(box));

      // compute ratio. -- dc: used to go <n, now <n-EXTRA
      for (i = 0; i < n - TP_EXTRA_BUTTONS; i++)
      {
         grs_bitmap *bm=texmemGetTexture(i);
         xr += bm->w;
         yr += bm->h;
      }
      for (i = 0; i < TP_EXTRA_BUTTONS; i++)
       { xr += 32; yr += 32; }   // hack to try and get rid of tiny tiny palette??
      ratio.x = (short)max(RectWidth(box)*n/xr,1);
      ratio.y = (short)max(RectHeight(box)*n/yr,1);
      dims = ratio;
      
      // scale dimensions until we have enough
      while(dims.x * dims.y < n)
      {
         dims.x++;
         dims.y = dims.x*ratio.y/ratio.x;
      }
      LayoutRectangles(&bounds,pal->rects,n,dims,MakePoint(0,0));
   }

   // build the button list finally
   {
      LGadButtonListDesc desc;
      memset(&desc,0,sizeof(desc));
      desc.num_buttons = n;
      desc.button_rects = pal->rects;
      desc.button_elems = pal->elems;
      desc.cb = textpal_button_cb;
      desc.flags=BUTTONLIST_RADIO_FLAG;

      LGadCreateButtonListDesc(&pal->list,pal->root,&desc);
   }
   pal->numbutts = n;
   LGadDrawBox(VB(pal->root),NULL);
}

////////////////////////////////////////

void DestroyTexturePalette(void)
{
   if (TextPal.root == NULL)
      return;
   LGadHideBox(VB(TextPal.root),TRUE);
   LGadDestroyButtonList(&TextPal.list);
   Free(TextPal.elems);
   Free(TextPal.rects);
   Free(TextPal.xtra);

   vmReleaseViews(TextPal.root);
   TextPal.root = NULL;
   if (!phoenix_rising)
      txtPnP_attach();     // if we got detached, reattach to brush
}

////////////////////////////////////////

void ToggleTexturePalette(TexturePalCall update)
{
   if (TextPal.root == NULL)
      CreateTexturePalette(update);
   else
      DestroyTexturePalette();
}

////////////////////////////////////////

bool TexturePaletteVisible(void)
{
   return TextPal.root != NULL;
}

////////////////////////////////////////

void TexturePaletteSelect(int texture)
{
   LGadRadioButtonSelect(&TextPal.list,texture);
   LGadDrawBox(VB(TextPal.root),NULL);
}

////////////////////////////////////////

static void rise_like_phoenix(void* update)
{
   phoenix_rising=TRUE;
   DestroyTexturePalette();
   CreateTexturePalette((TexturePalCall)update);
   phoenix_rising=FALSE;
}

void TexturePaletteUpdate(GFHUpdateOp op)
{
   if (TextPal.root != NULL)
   {
      if (op == GFH_FORCE || TextPal.numbutts != texmemGetMax()+TP_EXTRA_BUTTONS)
      {
         TexturePalCall update = TextPal.update;
         uiDefer(rise_like_phoenix,update);
      }
   }
}

/////////////////////////////////////////
