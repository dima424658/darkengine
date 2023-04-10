/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/gadget/util2d.c,v 1.17 1997/08/15 10:30:52 JAEMZ Exp $

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <dispapi.h>

#undef BTYPE

#include <2d.h>
#include <lzw.h>
#include <string.h>
#include <res.h>
#include <2dres.h>
#include <fdesc.h>
#include <cursors.h>
#include <cfgdbg.h>

#include <gadget.h> // for installable fns
#include <util2d.h>

char use565 = -1; // do we have an actual 565 16-bit screen, or a poncy 555 one?

int UtilCountRefs(Id id)
{
   int retval;
   RefTable *prt;

   prt = ResReadRefTable(id);
   if (prt == NULL)
   {
      Warning(("LibResCountRefs: Could not get id %x!\n",id));
      return(-1);
   }

   retval = prt->numRefs;
   ResFreeRefTable(prt);
   return(retval);
}

/* Locks the resource system id referred to as bmap_id, and returns the grs_bitmap *
that is generated when the whole thing is unpacked out of its FrameDesc and appropriate
bits pointers are hacked.  Whoever calls this is responsible to unlocking the resource
themselves, ultimately. */

// Returns NULL for failure
grs_bitmap *UtilLockBitmap(Id bmap_id)
{
   FrameDesc *fdesc = (FrameDesc *)ResLock(bmap_id);
   AssertMsg1(fdesc, "UtilLockBitmap: Lock of id %x returned NULL!", bmap_id);

   if (fdesc == NULL)
   {
      Warning(("UtilLockBitmap: Lock of id %x returned NULL!\n",bmap_id));
      return(NULL);
   }

   fdesc->bm.bits = (uchar *)(fdesc + 1);
   return(&(fdesc->bm));

}

grs_bitmap *UtilLockBitmapRef(Ref bmap_id)
{
   FrameDesc *fdesc;
#ifdef DBG_ONAIFJAIDJFAJPDF
   int r = UtilCountRefs(REFID(bmap_id));

   if (r < REFINDEX(bmap_id))
   {
      bmap_id = MKREF(REFID(bmap_id), r);
      CriticalMsg1("UtilLockBitmapRef: bmap_id %x is invalid!\n",bmap_id);
   }
#endif

   fdesc = (FrameDesc *)RefLock(bmap_id);
   AssertMsg1(fdesc, "UtilLockBitmapRef: Lock of ref %x returned NULL!", bmap_id);

   if (fdesc == NULL)
   {
      Warning(("UtilLockBitmapRef: Lock of ref %x returned NULL!\n",bmap_id));
      return(NULL);
   }

   fdesc->bm.bits = (uchar *)(fdesc + 1);
   return(&(fdesc->bm));

}

#define DEFAULT_CYCLES   4 * 1024

grs_bitmap *UtilGetCompressedPartial(Ref bmap_id, uchar *uncomp_buf, int cycles, UtilCompressState *ucs,
                                     UtilCompressCallback cb, grs_bitmap *bmp)
{
   FrameDesc *fdesc;
   int amt; // how much total are we doing here
   bool done = FALSE;
   bool cont = TRUE;
   int use_cyc;
   fdesc = (FrameDesc *)RefLock(bmap_id);
   AssertMsg1(fdesc, "UtilGetCompressedPartial: Lock of ref %x returned NULL!", bmap_id);

   if (fdesc == NULL)
   {
      Warning(("UtilLockBitmapRef: Lock of ref %x returned NULL!\n",bmap_id));
      return(NULL);
   }

   amt = fdesc->bm.row * fdesc->bm.h;
   if (ucs->init_state == 0)
   {
      ucs->init_state = 1;
//      memset(ucs->lzw_buffer,0,UTIL2D_COMPRESS_BUFFER_SIZE);
//      memset(ucs->lzw_state,0,UTIL2D_COMPRESS_STATE_SIZE);
      LzwExpandBuff2BuffPartialStart((uchar *)(fdesc + 1), uncomp_buf, 0, amt, ucs->lzw_state, ucs->lzw_buffer);
   }

   ConfigSpew("compress",("."));
   while (cont)
   {
      if (cycles)
      {
         cont = FALSE;
         use_cyc = cycles;
      }
      else
      {
         use_cyc = DEFAULT_CYCLES;
//         LzwExpandBuff2Buff((uchar *)(fdesc + 1), uncomp_buf, 0, amt);
//         fdesc->bm.bits = uncomp_buf;
//         return(&(fdesc->bm));
      }

      done = (LzwExpandBuff2BuffPartial((uchar *)(fdesc + 1), uncomp_buf, 0, amt, ucs->lzw_state, use_cyc) != 0);
      if (done)
      {
         ConfigSpew("compress",("!\n"));
         fdesc->bm.bits = uncomp_buf;
         ucs->init_state = 0;
//         memset(ucs->lzw_buffer,0,LZW_BUFF_SIZE);
//         memset(ucs->lzw_state,0,LZW_PARTIAL_STATE_SIZE);
         memcpy(bmp,&(fdesc->bm),sizeof(grs_bitmap));
         RefUnlock(bmap_id);
         return(bmp);
      }
      // allow the app to get in a word edgewise
      if (cont && cb)
         cb();
   }
   RefUnlock(bmap_id);
   return(NULL);
}

int UtilDrawBitmapRef(Ref bmap_id, short x, short y)
{
   grs_bitmap *bmp;  // pointer to bitmap to draw
   Rect r;

   bmp = UtilLockBitmapRef(bmap_id);
   if (bmp == NULL)
   {
      Warning(("UtilDrawBitmapRef: bmp is NULL!\n"));
      return(-1);
   }

   // draw it, and clean up after ourselves, including dealing with mousie
   r.ul.x = x;
   r.ul.y = y;
   r.lr.x = x + bmp->w;
   r.lr.y = y + bmp->h;

   uiHideMouse(&r);
   gr_bitmap(bmp,x,y);
   uiShowMouse(&r);

   RefUnlock(bmap_id);

   return(0);
}

int UtilRefAnchor(Ref r, Point *anchor)
{
   FrameDesc *f;
   if (r == 0)
      return(-1);
   f = (FrameDesc *)RefLock(r);
   if (f == NULL)
   {
      Warning(("UtilRefAnchor: Could not lock bitmap ref %x!",r));
      return(-1);
   }
   if (anchor != NULL)
      *anchor = f->anchorArea.ul;
   else
   {
      anchor->x = f->bm.w / 2;
      anchor->y = f->bm.h / 2;
   }
   RefUnlock(r);
   return(0);
}


grs_canvas *UtilScreen(short action)
{
   IDisplayDevice * pDisplayDevice = AppGetObj(IDisplayDevice);
   switch(action)
   {
      case UTIL_PUSH:
         IDisplayDevice_Lock(pDisplayDevice);
         gr_push_canvas(grd_scr_canv);
         break;
      case UTIL_POP:
         gr_pop_canvas();
         IDisplayDevice_Unlock(pDisplayDevice);
         break;
   }
   SafeRelease(pDisplayDevice);
   return(grd_screen_canvas);
}

// unsets the cliprect so that anything can draw!
void UtilClearCliprect()
{
   gr_set_cliprect(0,0,grd_canvas->bm.w,grd_canvas->bm.h);
}

// 16 bit palette fun tools!

typedef struct {
   Ref ref;
   ushort *pal;
} PalSet;

#define MAX_POSSIBLE_PALSETS  32
PalSet palsets[MAX_POSSIBLE_PALSETS];
short max_palsets = 0;

// Given a Ref for an 8-bit palette, goes and does the right magic to get the palette
// right in 16 bit for that 8-bit palette, caching appropriate computations and all that
// good stuff.

// Someday this may want to load pre-computed palettes from disk, if it is too slow...

// Boy, this whole thing is really quite broken in that we have a palref for every camera view these days.
// Basically, this needs to get totally reworked.

ushort *UtilGetPalConv(Ref palref)
{
   ushort *use_pal = NULL;
   int i;

   // Have we figured out our actual screen color type?
   if (use565 == -1)
   {
      sGrModeInfoEx ModeInfo;
      IDisplayDevice * pDisplayDevice = AppGetObj(IDisplayDevice);

      /* Whee! check whether ddraw gives us 555 or 565 when we ask for a 16 bit mode!*/
      // pDisplayDevice->SetMode(GRM_640x480x16, CLEAR_LINEAR);
      IDisplayDevice_GetMode(pDisplayDevice,&ModeInfo);
      if ((ModeInfo.redMask == 0xf800)&&
          (ModeInfo.greenMask == 0x07e0)&&
          (ModeInfo.blueMask == 0x001f))    // we've got 565!
         use565 = TRUE;
      else if ((ModeInfo.redMask == 0x7c00)&&
               (ModeInfo.greenMask == 0x03e0)&&
               (ModeInfo.blueMask == 0x001f))    // we've got 555!
         use565 = FALSE;
      else
         CriticalMsg("UtilGetPalConv(): Failed to set valid 640x480x16 graphics mode.\n");
      SafeRelease(pDisplayDevice);
   }

   // Lets try and see if we've used this palette before
   for (i=0; i < max_palsets; i++)
   {
      // good, we have.
      if (palsets[i].ref == palref)
      {
         use_pal = palsets[i].pal;
         break;
      }
   }

   // No dice, let's create the right palette and store it away.
   if (use_pal == NULL)
   {
      if (max_palsets < MAX_POSSIBLE_PALSETS)
      {
         gr_set_pal_imgref(palref);
         if (use565)
            use_pal = gr_alloc_pal565_from_pal888(grd_pal);
         else
            use_pal = gr_alloc_pal555_from_pal888(grd_pal);
         palsets[max_palsets].ref = palref;
         palsets[max_palsets].pal = use_pal;
         max_palsets++;
      }
      else {
         Warning(("UtilGetPalRef: Out of PalSet room for ref %x!\n",palref));
         free(palsets[MAX_POSSIBLE_PALSETS-1].pal);
         gr_set_pal_imgref(palref);
         use_pal = gr_alloc_pal565_from_pal888(grd_pal);
         palsets[MAX_POSSIBLE_PALSETS-1].ref = palref;
         palsets[MAX_POSSIBLE_PALSETS-1].pal = use_pal;
      }
   }
   return(use_pal);
}

// Actually go and set the palette
void UtilSetPalConv(Ref palref)
{
   gr_set_pal16(UtilGetPalConv(palref),0);
}

void UtilSetPalConvList(Ref palref, int n)
{
   gr_set_pal16(UtilGetPalConv(palref),n);
}

void UtilClearPalConv(Ref palref)
{
   int i,j; // iter
   for (i=0; i < max_palsets; i++)
   {
      if (palsets[i].ref == palref)
      {
         Free(palsets[i].pal);
         for (j=i; j < max_palsets-1; j++)
         {
            palsets[j] = palsets[j+1];
         }
         max_palsets--;
      }
   }
   return;
}

void UtilSetPal(Ref palref)
{
   gr_set_pal_imgref(palref);
}

// like gr_set_cliprect, but makes sure that all the parameters are
// okey dokey fine before going ahead and sending 'em.
void UtilSetCliprect(short x1, short y1, short x2, short y2)
{
   if (x1 < 0)
      x1 = 0;
   if (x1 > grd_canvas->bm.w)
      x1 = grd_canvas->bm.w;

   if (y1 < 0)
      y1 = 0;
   if (y1 > grd_canvas->bm.h)
      y1 = grd_canvas->bm.h;

   if (x2 < 0)
      x2 = 0;
   if (x2 > grd_canvas->bm.w)
      x2 = grd_canvas->bm.w;

   if (y2 < 0)
      y2 = 0;
   if (y2 > grd_canvas->bm.h)
      y2 = grd_canvas->bm.h;

   gr_set_cliprect(x1,y1,x2,y2);
}

#define REDMASK      0x1F
#define REDSHIFT     11
#define GREENMASK    0x3F
#define GREENSHIFT   5
#define BLUEMASK     0x1F
#define BLUESHIFT    0

#define MAX_COLOR_VAL      100

long UtilMakeColor(short r, short g, short b)
{
   if (r > MAX_COLOR_VAL)
      Warning(("UtilMakeColor: r value of %d is too large!\n",r));
   if (g > MAX_COLOR_VAL)
      Warning(("UtilMakeColor: g value of %d is too large!\n",g));
   if (b > MAX_COLOR_VAL)
      Warning(("UtilMakeColor: b value of %d is too large!\n",b));
   r = (r * REDMASK) / MAX_COLOR_VAL;
   g = (g * GREENMASK) / MAX_COLOR_VAL;
   b = (b * BLUEMASK) / MAX_COLOR_VAL;
   return( ((r & REDMASK) << REDSHIFT) | ((g & GREENMASK) << GREENSHIFT) | ((b & BLUEMASK) << BLUESHIFT) );
}

int UtilSetColor(short r, short g, short b)
{
   gr_set_fcolor(UtilMakeColor(r,g,b));
   return(0);
}

// Draws a string in a given font and palette to a 16 bit canvas
int UtilStringWrap16(char *s, short x, short y, char pal, short color, Id font, short width)
{
   short w, h; // size of string when drawn
   uchar *bits; // temp memory
   grs_canvas canv;  // our 8-bit canvas
   grs_bitmap bmp; // our 8-bit bitmap linked to canv
   grs_font *f; // the font we are using

   // do some setup of font, the canvas, etc
   f = (grs_font *)ResLock(font);

   // need to wrap?
   if (width != 0)
      gr_font_string_wrap(f,s,width);

   gr_font_string_size(f,s,&w,&h);
   if (lgad_malloc)
      bits = (uchar *)lgad_malloc(w * h * sizeof(char));
   else
      bits = (uchar *)Malloc(w * h * sizeof(char));
   memset(bits,0,sizeof(char) * w * h);
   gr_init_bitmap(&bmp, bits, BMT_FLAT8, BMF_TRANS, w, h);
   gr_make_canvas(&bmp, &canv);

   // draw the text to our new 8-bit canvas
   gr_push_canvas(&canv);
   UtilSetPal(MKREF(lgad_pal_id,pal));
   gr_set_font(f);
   gr_set_fcolor(color);
   gr_string(s,0,0);
   gr_pop_canvas();
   gr_close_canvas(&canv);

   // do the 8 to 16 blit, wahoo!
   UtilSetPalConv(MKREF(lgad_pal_id,pal));
   gr_bitmap(&bmp,x,y);

   // cleanup!
   gr_close_bitmap(&bmp);
   if (width != 0)
      gr_font_string_unwrap(s);
   if (lgad_free)
      lgad_free((char *)bits);
   else
      Free(bits);
   ResUnlock(font);

   return(0);
}
