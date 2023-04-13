/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Header: r:/prj/lib/src/h/rcs/2dres.h 1.9 1995/02/06 12:21:04 buzzard Exp $
 *
 * Macros to use 2d calls with resources
 *
 * $Log: 2dres.h $
 * Revision 1.9  1995/02/06  12:21:04  buzzard
 * Moved data structures out to fdesc.h
 * 
 * Revision 1.8  1995/01/25  18:11:51  jak
 * And finally more casts on all the calls to ResGet.
 * 
 * Revision 1.7  1995/01/25  17:48:04  jak
 * Added casts to make C++ happy
 * 
 * Revision 1.6  1994/07/06  18:38:48  jaemz
 * Added fields to cylbm frame
 * 
 * Revision 1.5  1994/06/15  11:51:49  jaemz
 * Added cylindrical bitmap object data types
 * 
 * Revision 1.4  1994/01/27  13:21:19  eric
 * Added gr_cpy_pal_image to copy image pallette from 
 * REFerence to a pallette in memory.
 * 
 * Revision 1.3  1993/09/28  01:12:21  kaboom
 * Converted #include "xxx" to #include <xxx> for watcom.
 * 
 * Revision 1.2  1993/08/24  20:04:52  rex
 * Turned framedesc's updateArea into union of updateArea, anchorArea, anchorPt
 * 
 * Revision 1.1  1993/04/27  12:06:52  rex
 * Initial revision
 */

#ifndef _2DRES_H
#define _2DRES_H

  // include 2d and special 2d-oriented
  //   resource types (e.g. FrameDesc)
#include <fdesc.h>
#include <res.h>

// These are the ref-based 2d macros.  They have the goofy do/while(0)
// so that you may use them in an if statement without C whining.

// Draw a bitmap, given its ref (unclipped)

#define gr_ubitmap_ref(ref,x,y)  do {     \
   FrameDesc *pfd = (FrameDesc*)RefGet(ref); \
   pfd->bm.bits = (uchar *)(pfd + 1);     \
   gr_ubitmap(&pfd->bm, x, y);            \
   } while (0)

// Draw a bitmap, given its ref (clipped)

#define gr_bitmap_ref(ref,x,y) do {       \
   FrameDesc *pfd = (FrameDesc*)RefGet(ref);          \
   pfd->bm.bits = (uchar *)(pfd + 1);     \
   gr_bitmap(&pfd->bm, x, y);             \
   } while(0)

// Draw a scaled bitmap, given its ref (unclipped)

#define gr_scale_ubitmap_ref(ref,x,y,w,h) do {  \
   FrameDesc *pfd = (FrameDesc*)RefGet(ref);                \
   pfd->bm.bits = (uchar *)(pfd + 1);           \
   gr_scale_ubitmap(&pfd->bm, x, y, w, h);      \
   } while(0)

// Draw a scaled bitmap, given its ref (clipped)

#define gr_scale_bitmap_ref(ref,x,y,w,h) do {   \
   FrameDesc *pfd = (FrameDesc*)RefGet(ref);                \
   pfd->bm.bits = (uchar *)(pfd + 1);           \
   gr_scale_bitmap(&pfd->bm, x, y, w, h);       \
   } while(0)

// Set an image's associated (partial) palette, if any

#define gr_set_pal_imgref(ref) do {             \
   FrameDesc *pfd = (FrameDesc*)RefGet(ref);                \
   if (pfd->pallOff) {                          \
      short *p = (short *)((uchar *) ResGet(REFID(ref)) + pfd->pallOff);   \
      gr_set_pal(*p, *(p+1), (uchar *)(p+2));   \
      }                                         \
   } while(0)

// Copy an image's associated (partial) palette to memory 
//  (palp is a pointer to start of destination pallette)

#define gr_cpy_pal_imgref(ref, palp) do {       \
   FrameDesc *pfd = (FrameDesc*)RefGet(ref);                \
   if (pfd->pallOff) {                          \
      short *p = (short *)((uchar *) ResGet(REFID(ref)) + pfd->pallOff);   \
      memcpy((uchar *)(palp + (*p * 3)), (uchar *)(p+2), *(p+1) * 3 );         \
      }                                         \
   } while(0)


#endif