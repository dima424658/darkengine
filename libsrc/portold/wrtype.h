/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/wrtype.h,v 1.7 1997/10/06 15:51:47 TOML Exp $

// World Representation Types

#ifndef __WRTYPE_H
#define __WRTYPE_H

#include <matrix.h>

#ifdef __cplusplus
extern "C"
{
#endif

  // types

typedef struct Location
{
   mxs_vector vec;

     // private data
   short cell; // internal data (cell this point is in)
   short hint; // a nearby cell
} Location;

typedef mxs_angvec Facing;

typedef struct Position
{
   Location loc;
   Facing   fac;
} Position;

typedef mxs_vector Vertex;
typedef mxs_vector Vector;

  //
  // constructors take pointers to both sides
  //

//
// recommended constructors;  "src" is a Location ptr
//

#define MakeHintedLocationFromVector(dest, vec, src)  \
        (MakeRawLocationFromVector(dest, vec),        \
         SetHintFromLocation(dest, src))

#define MakeHintedLocation(dest, nx,ny,nz, src)       \
        (MakeRawLocation(dest, nx,ny,nz),             \
         SetHintFromLocation(dest, src))

#define MakeHintedPositionFromVectors(dest, vec, ang, src) \
        (MakeRawPositionFromVectors(dest, vec, ang),       \
         SetHintFromLocation(dest, src))

//
// you can change a location in place, but if you do, you
// have to call this function to update the internal info
//

     // implementation note: just use the old location info
     // as a hint for the new one; don't have the old x,y,z
     // but that's ok

#define UpdateChangedLocation(loc) SetHintFromLocation(loc, loc)
#define UpdateChangedFacing(fac)   (0)

#define UpdateChangedPosition(pos) \
          UpdateChangedLocation(&(pos)->loc)

#define CELL_INVALID  -1
#define POLY_INVALID  -1
#define PLANE_INVALID -1

//
// constructors to use if there's absolutely no useful hint
//

#define MakeLocationFromVector(a,b) \
        ((a)->vec = *(b), (a)->cell=CELL_INVALID, (a)->hint=CELL_INVALID)

#define MakeLocation(a,nx,ny,nz) \
        ((a)->vec.x = (nx), (a)->vec.y = (ny), (a)->vec.z = (nz), (a)->cell=CELL_INVALID, (a)->hint=CELL_INVALID)

#define MakeOrientationFromAngVec(a,b) \
        (*(a) = *(b))

#define MakePositionFromVectors(a,b,c) \
        (MakeLocationFromVector(&((a)->loc), b), \
         MakeOrientationFromAngVec(&((a)->fac), c))

//////////////////////////////////////////////////////////////////////////////////////

  // internal use

#define MakeRawLocationFromVector(a,b) \
        ((a)->vec = *(b))

#define MakeRawLocation(a,nx,ny,nz) \
        ((a)->vec.x = (nx), (a)->vec.y = (ny), (a)->vec.z = (nz))

#define MakeRawPositionFromVectors(a,b,c) \
        (MakeRawLocationFromVector(&((a)->loc), b), \
         MakeOrientationFromAngVec(&((a)->fac), c))


#define HintFromLocation(loc)           \
    ((loc)->cell == CELL_INVALID ? (loc)->hint : (loc)->cell)

  // note order is very important here, in case src==dest
#define SetHintFromLocation(dest, src)  \
    ((dest)->hint = HintFromLocation(src), (dest)->cell = CELL_INVALID)

#ifdef __cplusplus
};
#endif

#endif
