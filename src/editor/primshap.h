/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/primshap.h,v 1.6 2000/01/29 13:13:00 adurant Exp $
// primshap(e)... code and data for generating the primal shapes
#pragma once

#ifndef __PRIMSHAP_H
#define __PRIMSHAP_H

EXTERN BOOL PrimShape_CreateCube(primalInfo *new_cube);
EXTERN BOOL PrimShape_CreateLight(primalInfo *new_light);

EXTERN BOOL PrimShape_CreateLine(primalInfo *new_line);
EXTERN BOOL PrimShape_CreateTri(primalInfo *new_tri);

EXTERN BOOL PrimShape_CreateNGonPyr(primalInfo *new_pyr, int n, BOOL face_align, BOOL corner);
EXTERN BOOL PrimShape_CreateNGonCyl(primalInfo *new_cyl, int n, BOOL face_align);

EXTERN BOOL PrimShape_CreateWedge(primalInfo *new_wedge);
EXTERN BOOL PrimShape_CreateDodecahedron(primalInfo *new_dodec);

#endif  // __PRIMSHAP_H
