/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


// $Header: r:/t2repos/thief2/libsrc/gadget/drawelem.c,v 1.16 1998/07/20 22:45:43 mahk Exp $

#include <drawelem.h>
#include <2d.h>
#include <res.h>
#include <lgsprntf.h>
#include <rect.h>
#include <string.h>
#include <util2d.h>
#include <gadget.h>
#include <stdlib.h>
#include <cfgdbg.h>

Id lgad_btype_ids[MAX_BTYPES];
Id lgad_default_font = 0;
Id lgad_internal_id = 0;
char *(*lgad_string_get)(int num, char *buf, int bufsize) = NULL;

uchar elem_clut[256];

static guiStyle* draw_style = NULL;

void ElementSetStyle(guiStyle* style)
{
   draw_style = style;
}

#if 0
static ulong elem_fcolor(DrawElement* d)
{
   if (d->fcolor != 0)
      return d->fcolor;
   return 
      guiStyleGetColor(draw_style,StyleColorFG);
}
#endif

static ulong elem_bcolor(DrawElement* d)
{
   if (d->bcolor != 0)
      return d->bcolor;
   return 
      guiStyleGetColor(draw_style,StyleColorBG);
}


#define MAGIC_ELEM_CLUT_COLOR 253

/* All of these individual element drawing types all take a "size" parameter as their last argument.  If it
is non-NULL, then that means to not actually draw, but instead fill in the Point with the width of thing
as if it were going to draw.  This is primarily to prevent a lot of redundant code with the sizing functions. */

#pragma off(unreferenced)
// Boy, this is an exciting function!
void ElementDrawNone(DrawElement *d, DrawElemState state, short x, short y, short w, short h, Point *size)
{
   return;
}

// callbacks have a "size" of nothing since we don't know what is going to be in them.
void ElementDrawCallback(DrawElement *d, DrawElemState state, short x, short y, short w, short h, Point *size)
{

   DrawElemCallback dcb = (DrawElemCallback)d->draw_data;
   if (size)
   {
      size->x = 0;
      size->y = 0;
   }
   else if (dcb != NULL)
   {
      grs_canvas canv;
      
      gr_init_sub_canvas(grd_canvas,&canv,x,y,w,h);
      gr_cset_cliprect(&canv,max(grd_clip.left - x,0),
                             max(grd_clip.top - y,0),
                             min(grd_clip.right - x,w),
                             min(grd_clip.bot - y,h));
      gr_push_canvas(&canv);
      dcb(d,state);
      gr_pop_canvas();
      gr_close_sub_canvas(&canv);
   }
   return;
}

static int nTabStops = 0;
static int *pTabStops = NULL;
//
// set the tabs that will be used for all
// text draw elements with DRAWFLAG_FORMAT_TABBED
//
void
ElementSetGlobalTabs( int  nTabs,
                      int  *pTabs )
{
   nTabStops = nTabs;
   pTabStops = pTabs;
}


//
// return the current tab settings
//
void
ElementGetGlobalTabs( int  *pnTabs,
                      int  **ppTabs )
{
   *pnTabs = nTabStops;
   *ppTabs = pTabStops;
}


//
// draw text with tab stops (and carriage returns)
//
static void
draw_tabbed_string( char   *s,
                    int    x,
                    int    y )
{                    
   char tmp[ DRAWELEM_STRLEN];
   char c, *pNext;
   int leftX, yStep, nChars, curTab;

   if ( (nTabStops == 0) || (pTabStops == NULL) ) {
      // there are no tab stops set - just do an untabbed string draw
      gr_string( s, x, y );
      return;
   }

   leftX = x;
   yStep = gr_get_font()->h;

   pNext = s;

   nChars = 0;
   curTab = 0;
   while ( *pNext != 0 ) {
      c = *pNext++;
      switch( c ) {

         default:
            tmp[nChars++] = c;
            break;

         case '\t':
            if ( curTab < nTabStops ) {
               // dump the string up to this point
               tmp[nChars] = 0;
               gr_string(tmp, x, y );
               x = leftX + pTabStops[curTab];
               curTab++;
               nChars = 0;
            }
            break;

         case '\n':
            tmp[nChars] = 0;
            gr_string(tmp, x, y );
            y += yStep;
            x = leftX;
            curTab = 0;
            nChars = 0;
            break;
      }
   }

   // if there was any leftover string, display it
   if ( nChars != 0 ) {
      tmp[nChars] = 0;
      gr_string( tmp, x, y );
   }
}


StyleColorKind textcols[] = 
{
   StyleColorText,
   StyleColorHilite,
   StyleColorDim,
   StyleColorBright,
};


void ElementDrawText(DrawElement *d, DrawElemState state, short x, short y, short w, short h, Point *size)
{
   char s[DRAWELEM_STRLEN],s2[DRAWELEM_STRLEN];  // s carries the final string, some use s2 for formatting or
                                                 // other temporary string storage
   Id font_id = guiStyleAvail(draw_style) ? guiStyleGetFont(draw_style,StyleFontNormal) : lgad_default_font;  // what font to use, can be overriden
   short sw,sh; // size in width and height, used for centering and size-setting
   int *v; // variable contents
   int stylecol = textcols[state];
   int format;
   int color = (d->fcolor != 0) 
      ? d->fcolor 
      : guiStyleGetColor(draw_style,stylecol);

   switch (d->draw_type)
   {
      case DRAWTYPE_VAR:
         strcpy(s2,"%d");
         v = (int *)d->draw_data2;
         if (v == NULL)
            lg_sprintf(s,s2,0);
         else
            lg_sprintf(s,s2,*v);
         break;
      case DRAWTYPE_VARSTRING:
         {
            char **carr = (char **)(d->draw_data);
            v = (int *)d->draw_data2;
            if (v == NULL)
               strcpy(s,carr[0]);
            else
               strcpy(s,carr[*v]);
            break;
         }
      case DRAWTYPE_VARTEXTREF:
         {
            Ref *carr = (Ref *)(d->draw_data);
            int val;
            v = (int *)d->draw_data2;
            if (v == NULL)
               val = 0;
            else
               val = *v;
            if (lgad_string_get)
               lgad_string_get(carr[val],s,DRAWELEM_STRLEN);
            else
               strcpy(s,(char *)RefGet(carr[val]));
            break;
         }
      case DRAWTYPE_FORMAT:
         if (d->draw_type == DRAWTYPE_FORMAT)
            strcpy(s2,(char *)(d->draw_data));
      case DRAWTYPE_FORMATREF:  // NOTE FALLTHROUGH, to share formatting code!
         if (d->draw_type == DRAWTYPE_FORMATREF)
         {
            if (lgad_string_get == NULL)
               Warning(("ElementDrawText: lgad_string_get is null!\n"));
            else
               lgad_string_get((Ref)d->draw_data, s2, DRAWELEM_STRLEN);
         }
         v = (int *)d->draw_data2;
         if (v == NULL)
            lg_sprintf(s,s2,0);
         else
            lg_sprintf(s,s2,*v);
         break;
      case DRAWTYPE_TEXT:
         strcpy(s,(char *)(d->draw_data));
         break;
      case DRAWTYPE_TEXTREF:
         if (lgad_string_get)
            lgad_string_get((Ref)d->draw_data, s, DRAWELEM_STRLEN);
         else
            strcpy(s,(char *)RefGet((Ref)d->draw_data));
         break;
   }

   // For those that can swap out fonts, give them the chance
   switch (d->draw_type)
   {
      case DRAWTYPE_TEXT:
      case DRAWTYPE_TEXTREF:
         if (d->draw_data2 != NULL)
            font_id = (Id)(d->draw_data2);
         break;
   }

   // Actually do it
   gr_set_fcolor(color);
   gr_set_font((grs_font *)ResLock(font_id));

   format = (d->draw_flags & DRAWFLAG_FORMAT_BITS) >> DRAWFLAG_FORMAT_SHIFT;

   // Let the wrappers wrap, and the breakers break
   if (format & DRAWFLAG_FORMAT_WRAP)
   {
      // NOTE: this thing doesn't know about tab stops!
      gr_font_string_wrap(gr_get_font(),s,w);
      format &= ~DRAWFLAG_FORMAT_WRAP; // take the format wrap out
   }

   gr_string_size(s,&sw,&sh);
   // Note that if we are re-sizing, we don't do any real work but do some computation
   // Thus no code outside of the non-sizing case can ever reference x,y,w,h since they
   // are undefined for the sizing case.
   if (size)
   {
      size->x = sw; size->y = sh;
      switch (format)
      {
         case DRAWFLAG_FORMAT_LEFT:
         case DRAWFLAG_FORMAT_RIGHT:
            size->x += MIN_MARGIN;
            break;
         case DRAWFLAG_FORMAT_CENTER:
            size->x += 1; // to offset for rounding in the centering process
            break;
      }
   }
   else
   {
      switch (format)
      {
         case DRAWFLAG_FORMAT_CENTER:
            gr_string(s, x + (w - sw)/2, y + (h - sh)/2);
            break;
         case DRAWFLAG_FORMAT_LEFT:
            gr_string(s, x + MIN_MARGIN, y + (h - sh)/2);
            break;
         case DRAWFLAG_FORMAT_RIGHT:
            gr_string(s, x + w - sw - MIN_MARGIN, y + (h - sh)/2);
            break;
         case DRAWFLAG_FORMAT_TABBED:
            draw_tabbed_string( s, x + MIN_MARGIN, y + (h - sh)/2 );
            break;
      }
   }
   ResUnlock(font_id);
}


void ElementDrawBitmap(DrawElement *d, DrawElemState state, short x, short y, short w, short h, Point *size)
{
   grs_bitmap *draw_me; // what we should actually draw
   Ref draw_ref = 0; // What Ref we are drawing?
   int offs = 0;

   switch(d->draw_type)
   {
      case DRAWTYPE_BITMAP:
         if (state == dsDEPRESSED && d->draw_data2)
            draw_me = (grs_bitmap*)d->draw_data2; 
         else
            draw_me = (grs_bitmap *)d->draw_data;
         break;
      case DRAWTYPE_BITMAPOFFSET:
      {
         int limit = (int)d->draw_data2;
         if (limit == 0 || state < limit)
         {
            draw_me = *(((grs_bitmap**)d->draw_data) + state);
         }
         break;
      }
      case DRAWTYPE_RES:
         if (state == dsDEPRESSED && d->draw_data2 != 0)
            draw_ref = (Ref)d->draw_data2; 
         else
            draw_ref = (Ref)d->draw_data;
         goto common_ref;

      case DRAWTYPE_RESOFFSET:
         draw_ref = (Ref)d->draw_data;
         { 
            int limit = (int)d->draw_data2;
            if (limit == 0 || state < limit)
            {
               Ref ref = draw_ref + state; 
               RefTable* tbl = ResLock(REFID(ref));
               if (RefIndexValid(tbl,REFINDEX(ref)))
                  draw_ref = ref;
               ResUnlock(REFID(ref));
            }
         }
         goto common_ref; 
      case DRAWTYPE_VARRES:
         {
            int *v; // the variable offset
            Ref *rlist; // array of refs to offset into
            rlist = (Ref *)d->draw_data;
            v = (int *)d->draw_data2;
            AssertMsg(rlist, "ElementDrawBitmap: rlist is NULL for DRAWTYPE_VARRES!");

            if (rlist == NULL)
            {
               Warning(("ElementDrawBitmap: rlist is NULL for DRAWTYPE_VARRES!\n"));
               return;
            }

            if (v == NULL)
               draw_ref = rlist[0];
            else
               draw_ref = rlist[*v];
         }
   common_ref:
         draw_me = UtilLockBitmapRef(draw_ref);
         break;
   }


   AssertMsg(draw_me, "ElementDrawBitmap: draw_me is NULL!");
   if (draw_me == NULL)
   {
      Warning(("ElementDrawBitmap: draw_me is NULL!\n"));
      return;
   }


   if (size)
   {
      size->x = draw_me->w; size->y = draw_me->h;
   }
   else
      gr_bitmap(draw_me,x,y);
   if (draw_ref)
      RefUnlock(draw_ref);
}

#pragma on(unreferenced)

// Big lookup table for how we deal with each kind of element, with duplicate entries for
// common functionality.
typedef void (*ElemDrawFunc)(DrawElement *d, DrawElemState state, short x, short y, short w, short h, Point *size);

ElemDrawFunc elem_draw_funcs[] =
{
   ElementDrawNone, 
   ElementDrawText,     
   ElementDrawBitmap, 
   ElementDrawBitmap, 
   ElementDrawText,
   ElementDrawText, 
   ElementDrawBitmap, 
   ElementDrawText, 
   ElementDrawText, 
   ElementDrawText,
   /* 10 */
   ElementDrawCallback, 
   ElementDrawBitmap,
   ElementDrawText,
   ElementDrawBitmap
};

#define TREK_UL   0
#define TREK_U    1
#define TREK_UR   2
#define TREK_L    3
#define TREK_R    4
#define TREK_BL   5
#define TREK_B    6
#define TREK_BR   7

#define NUM_TREK_BM  (TREK_BR+1)

/* Actually draws a given DrawElement, assuming that the right canvas is already set, to fill the coordinates
passed it.  This is the function that actually has all the gruesome knowledge of how to carry out the core
work of a DrawElement.  In the event of a DRAWTYPE_CALLBACK it will use the data parameter to pass on. */
/* Note that the "data" field is explicitly for draw-time data (as opposed to the draw_data and draw_data2 fields
actually in the DrawElement structure, which are entirely for Setup-time info). */

void ElementDraw(DrawElement *d, DrawElemState state, short x, short y, short w, short h)
{
   int i; // iterator
   short m; // max size of intrusion on a side
   DrawElement *elem = d;
   ConfigSpew("draw_element", ("Element Draw %d %d to %d %d\n",x,y,x+w,y+h));

   while(!(elem->statemask & (1 << state)))
   {
      elem = elem->next;
      if (elem == NULL)
         break;
   }
   // if we didn't find an elem representing the current state,
   // look for a zero mask, connoting default
   if (elem == NULL)
   {
      elem = d;
      while (elem->statemask != 0)
      {
         elem = elem->next;
         if (elem == NULL)
            return;
      }
   }
   d = elem;

   elem_clut[MAGIC_ELEM_CLUT_COLOR] = elem_bcolor(d);

   // Draw the "outer" elements that are independant of the internal data, like "bevel" or "trek"
   switch((d->draw_flags & DRAWFLAG_BORDER_BITS) >> DRAWFLAG_BORDER_SHIFT)
   {
      case DRAWFLAG_BORDER_OUTLINE:
         gr_set_fcolor(guiStyleGetColor(draw_style,StyleColorBorder));
         gr_box(x,y,x+w,y+h);
         x++;y++;w = w - 2;h = h - 2; // to compensate for smaller area
         break;
      case DRAWFLAG_BORDER_BEVEL:
         gr_set_fcolor(guiStyleGetColor(draw_style,StyleColorBevelLight));
         gr_hline(x,y,x+w-1);
         gr_hline(x,y+h-1,x+w-1);
         gr_set_fcolor(guiStyleGetColor(draw_style,StyleColorBevelDark));
         gr_vline(x+w-1,y,y+h-1);
         gr_vline(x,y,y+h-1);
         x++;y++;w-=2;h-=2; // to compensate for smaller area
         break;
      case DRAWFLAG_BORDER_BITMAP:
         {
            grs_bitmap *trek_bms[NUM_TREK_BM];
            int which_btype = (d->draw_flags & DRAWFLAG_BTYPE_BITS) >> DRAWFLAG_BTYPE_SHIFT;
            AssertMsg1(which_btype <= MAX_BTYPES, "ElementDraw: btype of %d is invalid!\n", which_btype);
            if (which_btype > MAX_BTYPES)
            {
               Warning(("ElementDraw: btype of %d is invalid!\n"));
               which_btype = 0;
            }


            // fill in all the bitmaps
            for (i=0; i < NUM_TREK_BM; i++)
               trek_bms[i] = UtilLockBitmapRef(MKREF(lgad_btype_ids[which_btype],i));

            // upper row
            gr_clut_bitmap(trek_bms[TREK_UL],x,y,elem_clut);
            gr_clut_bitmap(trek_bms[TREK_UR],x + w - trek_bms[TREK_UR]->w, y,elem_clut);
            gr_clut_scale_bitmap(trek_bms[TREK_U], x + trek_bms[TREK_UL]->w, y,
               w - trek_bms[TREK_UL]->w - trek_bms[TREK_UR]->w, trek_bms[TREK_U]->h,elem_clut);

            // sides
            gr_clut_scale_bitmap(trek_bms[TREK_L], x, y + trek_bms[TREK_UL]->h,
               trek_bms[TREK_L]->w, h - trek_bms[TREK_UL]->h - trek_bms[TREK_BL]->h,elem_clut);
            gr_clut_scale_bitmap(trek_bms[TREK_R], x + w - trek_bms[TREK_R]->w, y + trek_bms[TREK_UR]->h,
               trek_bms[TREK_R]->w, h - trek_bms[TREK_UR]->h - trek_bms[TREK_BR]->h,elem_clut);

            // bottom row
            gr_clut_bitmap(trek_bms[TREK_BL], x, y + h - trek_bms[TREK_BL]->h,elem_clut);
            gr_clut_bitmap(trek_bms[TREK_BR], x + w - trek_bms[TREK_BR]->w, y + h - trek_bms[TREK_BL]->h,elem_clut);
            gr_clut_scale_bitmap(trek_bms[TREK_B], x + trek_bms[TREK_BL]->w, y + h - trek_bms[TREK_B]->h,
               w - trek_bms[TREK_BL]->w - trek_bms[TREK_BR]->w, trek_bms[TREK_B]->h,elem_clut);

            m = trek_bms[TREK_L]->w;
            x = x + m; w = w - m;
            w = w - trek_bms[TREK_R]->w;

            m = max(max(trek_bms[TREK_UL]->h,trek_bms[TREK_U]->h),trek_bms[TREK_UR]->h);
            y = y + m; h = h - m;
            h = h - max(max(trek_bms[TREK_BL]->h,trek_bms[TREK_B]->h),trek_bms[TREK_BR]->h);

            // unlock all those refs
            for (i=0; i < NUM_TREK_BM; i++)
            {
               RefUnlock(MKREF(lgad_btype_ids[which_btype],i));
               trek_bms[i] = NULL;
            }
         }
         break;
   }

//   if (d->bcolor)
//   {
//      gr_set_fcolor(d->bcolor);
//      gr_rect(x,y,x+w,y+h);
//   }

   // internal draw controls
   switch((d->draw_flags & DRAWFLAG_INTERNAL_BITS) >> DRAWFLAG_INTERNAL_SHIFT)
   {
      case DRAWFLAG_INT_TRANSP:
         break;
      case DRAWFLAG_INT_SOLID:
         // lay solid background
         if (elem_bcolor(d))
         {
            gr_set_fcolor(elem_bcolor(d));
            gr_rect(x,y,x+w,y+h);
         }
         break;

      case DRAWFLAG_INT_TREK:
         {
            grs_bitmap *trek_int_bms[3];
            // lock the art
            for (i=0; i <= 2; i++)
               trek_int_bms[i] = UtilLockBitmapRef(MKREF(lgad_internal_id,i));

            // lay solid background
            gr_set_fcolor(1); // um, why is this a hard-coded "1" ? 
            gr_rect(x,y,x+w,y+h);

            // draw the three parts
            gr_bitmap(trek_int_bms[0],x,y);
            gr_scale_bitmap(trek_int_bms[1],x + trek_int_bms[0]->w, y,
               w - trek_int_bms[0]->w, trek_int_bms[1]->h);
            gr_scale_bitmap(trek_int_bms[2],x, y + trek_int_bms[0]->h,
               trek_int_bms[2]->w, h - trek_int_bms[0]->h);

            // if there is an "inner" draw element put it over the title
            if (d->inner)
               ElementDraw(d->inner,NULL,x + trek_int_bms[0]->w, y, w - trek_int_bms[0]->w, trek_int_bms[1]->h);

            // alter drawing area
            x = x + trek_int_bms[2]->w;
            w = w - trek_int_bms[2]->w;
            m = max(trek_int_bms[0]->h,trek_int_bms[1]->h);
            y = y + m;
            h = h - m;

            // unlock all our art
            for (i=0; i <= 2; i++)
               RefUnlock(MKREF(lgad_internal_id,i));
         }
         break;
   }


   // Now draw the inner contents
   if (elem_draw_funcs[d->draw_type])
      elem_draw_funcs[d->draw_type](d,state,x,y,w,h,NULL);
}

void ElementSize(DrawElement *d, short *px, short *py)
{
   Point p; // filled in by the sizing function
   short iw,ih; // internal wid and ht

   if (elem_draw_funcs[d->draw_type])
   {
      // Arguments we pass on are "undefined" since in theory they are unused in this case.
      // zero seems most likely to cause it to die.
      elem_draw_funcs[d->draw_type](d,NULL,0,0,0,0,&p);
      *px = p.x;
      *py = p.y;
   }
   else
   {
      Warning(("ElementSize: Don't know how to draw element type %d!\n",d->draw_type));
      *px = 0; *py = 0;
   }
   // factor in wacky borders and internal elements
   ElementExtrasSize(d,&iw,&ih);
   *px += iw;
   *py += ih;
}

// determines the total x and y extent of overall "extra" draw elements, such as internal elements and border

// This could (and maybe should?) be composed of the two calls to the Offset and the OffsetAlternate to figure
// out total pixel displacement needed.  However, that is a lot of redundant work.  On the other hand, the speed
// difference really isn't very significant, so it is probably worth it for the memory savings.
void ElementExtrasSize(DrawElement *d, short *pw, short *ph)
{
   int i; // iterator
   *pw = 0;
   *ph = 0;
   switch((d->draw_flags & DRAWFLAG_BORDER_BITS) >> DRAWFLAG_BORDER_SHIFT)
   {
      case DRAWFLAG_BORDER_BEVEL:
      case DRAWFLAG_BORDER_OUTLINE:
         *pw += 2;
         *ph += 2;
         break;
      case DRAWFLAG_BORDER_BITMAP:  // note this could easily be extended to different types of border art
                                  // by altering the base resource that is used
         {
            grs_bitmap *trek_bms[NUM_TREK_BM];
            int which_btype = (d->draw_flags & DRAWFLAG_BTYPE_BITS) >> DRAWFLAG_BTYPE_SHIFT;

            for (i=0; i < NUM_TREK_BM; i++)
               trek_bms[i] = UtilLockBitmapRef(MKREF(lgad_btype_ids[which_btype],i));
            *pw = *pw + trek_bms[TREK_L]->w;
            *pw = *pw + trek_bms[TREK_R]->w;
            *ph = *ph + max(max(trek_bms[TREK_UL]->h,trek_bms[TREK_U]->h),trek_bms[TREK_UR]->h);
            *ph = *ph + max(max(trek_bms[TREK_BL]->h,trek_bms[TREK_B]->h),trek_bms[TREK_BR]->h);
            // unlock all those refs
            for (i=0; i < NUM_TREK_BM; i++)
            {
               RefUnlock(MKREF(lgad_btype_ids[which_btype],i));
               trek_bms[i] = NULL;
            }
         }
         break;
   }
   switch((d->draw_flags & DRAWFLAG_INTERNAL_BITS) >> DRAWFLAG_INTERNAL_SHIFT)
   {
      case DRAWFLAG_INT_TREK:
         {
            grs_bitmap *trek_bms[3];

            // lock the art
            for (i=0; i <= 2; i++)
               trek_bms[i] = UtilLockBitmapRef(MKREF(lgad_internal_id,i));

            *pw = *pw + trek_bms[2]->w;
            *ph = *ph + max(trek_bms[0]->h,trek_bms[1]->h);

            // unlock all our art
            for (i=0; i <= 2; i++)
               RefUnlock(MKREF(lgad_internal_id,i));
         }
         break;
   }
}

// *** Needs to become real when internal drawing elements exist!!!!!
#pragma off(unreferenced)

// Figure out how far into the draw element the extra (internals, borders) bits go
// for the upper and left
void ElementOffset(DrawElement *d, short *pw, short *ph)
{
   int i; // iterator
   *pw = 0;
   *ph = 0;
   switch((d->draw_flags & DRAWFLAG_BORDER_BITS) >> DRAWFLAG_BORDER_SHIFT)
   {
      case DRAWFLAG_BORDER_BEVEL:
      case DRAWFLAG_BORDER_OUTLINE:
         *pw += 1;
         *ph += 1;
         break;
      case DRAWFLAG_BORDER_BITMAP:
         {
            grs_bitmap *trek_bms[NUM_TREK_BM];
            int which_btype = (d->draw_flags & DRAWFLAG_BTYPE_BITS) >> DRAWFLAG_BTYPE_SHIFT;

            for (i=0; i < NUM_TREK_BM; i++)
               trek_bms[i] = UtilLockBitmapRef(MKREF(lgad_btype_ids[which_btype],i));
            *pw = *pw + trek_bms[TREK_L]->w;
            *ph = *ph + max(max(trek_bms[TREK_UL]->h,trek_bms[TREK_U]->h),trek_bms[TREK_UR]->h);
            // unlock all those refs
            for (i=0; i < NUM_TREK_BM; i++)
            {
               RefUnlock(MKREF(lgad_btype_ids[which_btype],i));
               trek_bms[i] = NULL;
            }
         }
         break;
   }
   switch((d->draw_flags & DRAWFLAG_INTERNAL_BITS) >> DRAWFLAG_INTERNAL_SHIFT)
   {
      case DRAWFLAG_INT_TREK:
         {
            grs_bitmap *trek_bms[3];
            // lock the art
            for (i=0; i <= 2; i++)
               trek_bms[i] = UtilLockBitmapRef(MKREF(lgad_internal_id,i));

            *pw = *pw + trek_bms[2]->w;
            *ph = *ph + max(trek_bms[0]->h,trek_bms[1]->h);

            // unlock all our art
            for (i=0; i <= 2; i++)
               RefUnlock(MKREF(lgad_internal_id,i));
         }
         break;
   }
}

// Figure out how far into the draw element the extra (internals, borders) bits go
// for the bottom and right
void ElementOffsetAlternate(DrawElement *d, short *pw, short *ph)
{
   int i; // iterator
   *pw = 0;
   *ph = 0;
   switch((d->draw_flags & DRAWFLAG_BORDER_BITS) >> DRAWFLAG_BORDER_SHIFT)
   {
      case DRAWFLAG_BORDER_BEVEL:
      case DRAWFLAG_BORDER_OUTLINE:
         *pw += 1;
         *ph += 1;
         break;
      case DRAWFLAG_BORDER_BITMAP:
         {
            grs_bitmap *trek_bms[NUM_TREK_BM];
            int which_btype = (d->draw_flags & DRAWFLAG_BTYPE_BITS) >> DRAWFLAG_BTYPE_SHIFT;

            for (i=0; i < NUM_TREK_BM; i++)
               trek_bms[i] = UtilLockBitmapRef(MKREF(lgad_btype_ids[which_btype],i));
            *pw = *pw + trek_bms[TREK_R]->w;
            *ph = *ph + max(max(trek_bms[TREK_BL]->h,trek_bms[TREK_B]->h),trek_bms[TREK_BR]->h);

            // unlock all those refs
            for (i=0; i < NUM_TREK_BM; i++)
            {
               RefUnlock(MKREF(lgad_btype_ids[which_btype],i));
               trek_bms[i] = NULL;
            }
         }
         break;
   }

   // thus far, no internal elements affect the alternative offset.
}

#pragma on(unreferenced)

void ElementClear(DrawElement *d)
{
   memset(d,0,sizeof(*d));
   //   d->fcolor = 33;
   //   d->bcolor = 1;
   d->draw_type = DRAWTYPE_NONE;
}

void DrawElementInit(void)
{
   int i; // iterator
   for (i=0; i < 256; i++)
      elem_clut[i] = i;
}
