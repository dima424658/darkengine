/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $ Header: $

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   fxspan.c

   Some of our special effects run off of a generic span buffer
   system.  This one.

   Note that so far this is software only; we'll have to work out
   something equivalent for hardware later.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#include <stdlib.h>
#include <string.h>

#include <lg.h>
#include <2d.h>

#include <fxspans.h>
#include <fxspan.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define SPAN_BYTES(wd, cnv) \
   (((cnv)->bm.type == BMT_FLAT16)? ((wd) + (wd)) : (wd))


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   This clips a set of spans against the clipping rectangle of a given
   canvas.  We return FALSE if all the spans were off the canvas,
   TRUE if there's anything left.  x and y are the upper-left corner
   of the sFXSpanSet within the canvas.

   All the upper extrema here--rights and bottoms--are really one past
   the values their names make them sound like, in the usual C way.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
bool FXSpanClipToCanvas(sFXSpanSet *set, grs_canvas *canvas, fix x, fix y)
{
   int i;
   int left, right;
   int int_x = fix_int(x);
   int int_y = fix_int(y);
   sFXSpan *span = set->span_list;
   bool anything_left = FALSE;

   // The bounds we're clipping to are in the coordinate system of the
   // span set.  So from here on in, our calculations won't bother
   // with the canvas' coordinate system.
   int clip_left = max(0, canvas->gc.clip.i.left - int_x);
   int clip_right = canvas->gc.clip.i.right - int_x;
   int clip_top = max(0, canvas->gc.clip.i.top - int_y);
   int clip_bottom = min(canvas->gc.clip.i.bot - clip_top,
                         set->first_span_used + set->num_spans_used);

   // don't use spans above the top of the canvas
   if (set->first_span_used < clip_top) {
      if ((set->first_span_used + set->num_spans_used) < clip_top)
         return FALSE;
      set->num_spans_used = clip_top - set->first_span_used;
      set->first_span_used = clip_top;
   }

   // clear out anything below the bottom
   if ((set->first_span_used + set->num_spans_used) > clip_bottom) {
      if (set->first_span_used > clip_bottom)
         return FALSE;
      set->num_spans_used = clip_bottom - set->first_span_used;
   }

   // Each span within the canvas gets clipped horizontally.  Glee.
   for (i = set->first_span_used; i < clip_bottom; i++) {
      left = span[i].left;
      right = left + span[i].length - 1;

      // Does this span overlap the clipping area?
      if (left < clip_right || right > clip_left) {
         anything_left = TRUE;

         span[i].left = max(left, clip_left);
         span[i].length = min(right, clip_right) - span[i].left + 1;
AssertMsg((span[i].length >= 1), "!!!!!");
      } else
         span[i].left = SPAN_NOT_USED;
   }

   return anything_left;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   A set of spans is as wide as its bounding rectangle.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanSetBoundingWidth(sFXSpanSet *set)
{
   int span_width;
   int width = 0;
   sFXSpan *span = set->span_list + set->first_span_used;
   sFXSpan *span_after_last = span + set->num_spans_used;

   while (span != span_after_last) {
      span_width = span->left + span->length;
      if (span_width > width)
         width = span_width;

      ++span;
   }

   set->bounding_width = width;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   As in all the rendering routines, we expect the client generating
   the spans to clip them before passing them in.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanDrawSetFromSnarf(sFXSpanSet *set, sFXSpanSnarf *snarf, 
                            grs_canvas *canvas, 
                            fix x_in_canvas, fix y_in_canvas,
                            fix x_in_snarf, fix y_in_snarf)
{
   int source_row = snarf->row;
   char *source = snarf->bits
      + source_row * (fix_int(y_in_snarf) + set->first_span_used)
      + SPAN_BYTES(fix_int(x_in_snarf), canvas);
   int destination_row = canvas->bm.row;
   char *destination = canvas->bm.bits
      + destination_row * (fix_int(y_in_canvas) + set->first_span_used)
      + SPAN_BYTES(fix_int(x_in_canvas), canvas);
   sFXSpan *span = set->span_list + set->first_span_used;
   sFXSpan *span_after_last = span + set->num_spans_used;

   while (span != span_after_last) {
      if (span->left != SPAN_NOT_USED)
         memcpy(destination + span->left,
                source + SPAN_BYTES(span->left, canvas), 
                SPAN_BYTES(span->length, canvas));

      source += source_row;
      destination += destination_row;
      ++span;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   As in all the rendering routines, we expect the client generating
   the spans to clip them before passing them in.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanDrawSetFromCanvas(sFXSpanSet *set, grs_canvas *canvas, 
                             fix x_source, fix y_source,
                             fix x_destination, fix y_destination)
{
   int row = canvas->bm.row;
   char *source, *destination;
   sFXSpan *span_at_top = set->span_list + set->first_span_used;
   sFXSpan *span_after_last = span_at_top + set->num_spans_used;

   // Are we moving this thing up, or down?  This affects which
   // direction we copy the spans in.  Here's the down case...
   if (y_source < y_destination) {
      source = canvas->bm.bits
         + row * (fix_int(y_source)
                + set->first_span_used + set->num_spans_used)
         + SPAN_BYTES(fix_int(x_source), canvas);
      destination = canvas->bm.bits
         + row * (fix_int(y_destination)
                + set->first_span_used + set->num_spans_used)
         + SPAN_BYTES(fix_int(x_destination), canvas);

      do {
         --span_after_last;
         if (span_after_last->left != SPAN_NOT_USED)
            memmove(destination + SPAN_BYTES(span_after_last->left, canvas), 
                    source + SPAN_BYTES(span_after_last->left, canvas),
                    SPAN_BYTES(span_after_last->length, canvas));
         source -= row;
         destination -= row;
      } while (span_after_last != span_at_top);

      // ...and here's moving up.
   } else {
      source = canvas->bm.bits
         + row * (fix_int(y_source) + set->first_span_used)
         + SPAN_BYTES(fix_int(x_source), canvas);
      destination = canvas->bm.bits
         + row * (fix_int(y_destination) + set->first_span_used)
         + SPAN_BYTES(fix_int(x_destination), canvas);

      while (span_at_top != span_after_last) {
         if (span_at_top->left != SPAN_NOT_USED)
            memmove(destination + SPAN_BYTES(span_after_last->left, canvas), 
                    source + SPAN_BYTES(span_after_last->left, canvas),
                    SPAN_BYTES(span_after_last->length, canvas));
         source += row;
         destination += row;
         ++span_at_top;
      }
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   Drawing heat is much like copying spans from a snarfed buffer.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanDrawFlatColor(sFXSpanSet *set, sFXSpanSnarf *snarf, 
                         grs_canvas *canvas, 
                         fix x_in_canvas, fix y_in_canvas,
                         fix x_in_snarf, fix y_in_snarf,
                         uchar color)
{
   int destination_row = canvas->bm.row;
   char *destination = canvas->bm.bits
      + destination_row * (fix_int(y_in_canvas) + set->first_span_used)
      + fix_int(x_in_canvas);
   sFXSpan *span = set->span_list + set->first_span_used;
   sFXSpan *span_after_last = span + set->num_spans_used;

   while (span != span_after_last) {
      if (span->left != SPAN_NOT_USED)
         memset(destination + span->left, color, span->length);

      destination += destination_row;
      ++span;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   snarf (SNARF') v.: To be, or act in the manner of, a snarfer.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanSnarf(sFXSpanSnarf *snarf, sFXSpanSet *set, 
                 grs_canvas *canvas, fix x_in_canvas, fix y_in_canvas)
{
   int int_x = fix_int(x_in_canvas);
   int int_y = fix_int(y_in_canvas);
   int source_row = canvas->bm.row;
   char *source = canvas->bm.bits
      + source_row * (int_y + set->first_span_used)
      + SPAN_BYTES(int_x, canvas);
   int destination_row = snarf->row;
   char *destination = snarf->bits
      + destination_row * set->first_span_used;
   sFXSpan *span = set->span_list + set->first_span_used;
   sFXSpan *span_after_last = span + set->num_spans_used;

   // We expect that this span list fits in this canvas at these
   // coordinates.  So remember to clip!
   while (span != span_after_last) {
      if (span->left != SPAN_NOT_USED)
         memcpy(destination + span->left,
                source + SPAN_BYTES(span->left, canvas),
                SPAN_BYTES(span->length, canvas));

      source += source_row;
      destination += destination_row;
      ++span;
   }
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   We assume on entry that the bits field of the snarf is not valid,
   since these buffers are potentially pretty big and not used that
   often.  So remember to Unsnarf!

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanSnarfSetup(sFXSpanSnarf *snarf, int height, int width,
                      grs_canvas *canvas)
{
   int size;
   width = SPAN_BYTES(width, canvas);
   size = height * width;

   // We assume on entry that the bits field of the snarf is not
   // valid, since these buffers are potentially pretty big and not
   // used that often.  So remember to Unsnarf!
   snarf->size = size;
   snarf->bits = Malloc(size);
   snarf->height = height;
   snarf->width = width;
   snarf->row = width;
}


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   unsnarf (SNARF') v.: To be, or act in the manner of, an unsnarfer.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */
void FXSpanUnsnarf(sFXSpanSnarf *snarf)
{
   Free(snarf->bits);
   snarf->bits = 0;
   snarf->size = 0;
}
