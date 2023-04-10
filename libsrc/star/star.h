/*
 * $Source: x:/prj/tech/libsrc/star/RCS/star.h $
 * $Revision: 1.6 $
 * $Author: PATMAC $
 * $Date: 1998/07/01 18:40:48 $
 *
 * Star library header
 *
 * $Log: star.h $
 * Revision 1.6  1998/07/01  18:40:48  PATMAC
 * Add extern "C"
 * 
 * Revision 1.5  1998/02/18  15:57:56  KEVIN
 * added function to set pixel rendering function for next call to StarRender().
 * 
 * Revision 1.4  1997/11/04  16:22:23  KEVIN
 * 16 bit support.
 * 
 * Revision 1.3  1997/11/02  20:05:51  buzzard
 * anti-aliased stars cleanup
 * 
 * Revision 1.2  1997/01/31  23:25:39  JAEMZ
 * Updated star library for the nineties
 * 
 * Revision 1.1  1994/10/24  23:27:39  jaemz
 * Initial revision
 * 
*/

#ifndef __STAR_H
#define __STAR_H

#include <dev2d.h>
#include <r3ds.h>

#ifdef __cplusplus
extern "C" {
#endif
// This is for the anti-alised size of the stars.  Fear me.
extern   int std_size;

// sets global pointers in the star library
// to the number of stars, their positions, their colors
// Note that the old star library used shorts for vectors but
// that's going to be slower and memory has increased by at least
// a factor of 2, if not 4, since System Shock.
void StarSet(int n,mxs_vector *vlist,uchar *clist);

// allocates the necessary space for stars using Malloc
// returns neg 1 if problem
int StarMalloc(int n);

// frees star space using Free
void StarFree();

// stuffs random vectors and colors into the set areas
// randomly assigning a color range to them
// Col is the starting col in the range, range, is the number 
// of entries.
void StarRand(uchar col,uchar range);

// Renders the given polygon in the star color, which is currently
// hardcoded to color zero, and enters the polygon list into the master
// list used for clipping the stars before their full transforms.
// Pass in unclipped poly
void StarPoly(int n,r3s_phandle *vp);

// Just enters the list.  You are responsible for making sure the
// region is already color zero.  Note that this is how SS did the cool
// texture map gratings in the botany bay.  
// Pass in unclipped poly, we'll clip
void StarEmpty(int n,r3s_phandle *vp);

// Render to an empty sky, you'll have
// to blacken it for us to color 0
// No polygon list is used, just a half plane
void StarSky(void);

// renders star field in the polygon defined by the vertex list
// uses your 3d context, so make sure that's been set
// call this before doing a frame end.  You can put it in
// an object frame if you'd like to rotate them and such
// In fact, you need to put it in the space of the viewer, so you
// center it about the viewers head.  We don't do it internally
// since you may want to combine rotation anyway.
// Must be in a block
void StarRender(void);

// Specify a pixel rendering function to be used in the next call
// to StarRender().  Gets reset to NULL after every call to StarRender().
void StarSetPixFunc(gdupix_func *pix_func);

// As above but sets the view position to zero.  This eliminates weird
// jiggle caused by float accuracy problems
void StarRenderSimple();

// Base is the black color, range includes
// the white color
// wsize is canvas size to meet or exceed to 
// anti-alias
void StarSetAntiAlias(int dark_col,int bright_col,int wsize);

// Set flag whether to check background color before rendering star
void StarSetCheckBkgnd(BOOL check);


#ifdef __cplusplus
}
#endif
#endif // star.h





