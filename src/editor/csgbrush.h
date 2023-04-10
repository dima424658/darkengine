/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/csgbrush.h,v 1.15 2000/01/29 13:11:22 adurant Exp $
#pragma once

#ifndef __CSGBRUSH_H
#define __CSGBRUSH_H

//  Interface to brushes for CSG stuff

//  brushes have a unique numeric id, and
//  the csg engine wants to access certain
//  information from it.  An array of structs
//  is the most obvious implementation; we
//  use an array of pointers which point to
//  structs which are managed elsewhere

//  Rather than reading the structs directly,
//  since all that might change, we do all the
//  accesses here; the macros can be turned into
//  structure indices, function calls, whatever

#include <editbr_.h>
#include <ged_csg.h>
#include <brtypes.h>

#define MAX_CSG_BRUSHES   7068
EXTERN struct _editBrush *csg_brush[MAX_CSG_BRUSHES];
EXTERN int csg_num_brushes;

///////////////////////////
// actual interface elements to csg data structures

  // step 1: we need to be able to index by brush id

#define CSG_BRUSH(x)                     csg_brush[x]

  // step 2: we need to get at some information about the whole brush

#define CB_TIMESTAMP(x)                  (CSG_BRUSH(x)->timestamp)
#define CB_MEDIUM(x)                     (CSG_BRUSH(x)->media)

//#define CB_MEDIUM_HOTREGION               4  // evaporate
#define CB_MEDIUM_HOTREGION               (-brType_HOTREGION)

  // step 3: we need to get information about faces and their textures

#define CB_FACE_TEXTURE(x,y)             gedcsgGetFaceTexture(x,y)

#define CB_MAX_ALIGN                     (256)

#define get_face(x,y)                    (gedcsgGetFaceTexInfo(x,y))
#define CB_FACE_TEX_SCALE(x,y)           (gedcsgGetFaceScale(x,y))
#define CB_FACE_TEX_ALIGN_U(x,y)         ((float)(get_face(x,y)->tx_x*4)/((float)CB_MAX_ALIGN))
#define CB_FACE_TEX_ALIGN_V(x,y)         ((float)(get_face(x,y)->tx_y*4)/((float)CB_MAX_ALIGN))
#define CB_FACE_TEX_ROT(x,y)             (get_face(x,y)->tx_rot)

#define CB_FACE_BRUSH_ALIGN_TEX(x,y,u,v) gedcsgGetFaceTextureAligned(x,y,u,v)

#define CB_FACE_IS_SELF_LUMINOUS(x,y)    gedcsgGetFaceTextureSelfLit(x,y)

#endif  // __CSGBRUSH_H
