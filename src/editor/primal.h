/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/primal.h,v 1.11 2000/01/29 13:12:57 adurant Exp $
// primal brush systems
#pragma once

// @TODO: these should look at current primal brushes
// not be brutally hardcoded to a cube

#ifndef __PRIMAL_H
#define __PRIMAL_H

#include <matrixs.h>
#include <matrixds.h>
#include <primals.h>
#include <editbrs.h>

///////////////////
// simple silly initializer
EXTERN void primalBr_Init(void);
EXTERN void primalBr_Term(void);

//////////// new hipper type/side info
#define primalID_Make(type,sides)          (((type)<<9)+(sides-3))
#define primalID_FullMake(type,face,sides) (primalID_Make(type,sides)|face)
#define primalID_GetType(id_val)           ((id_val)>>9)
#define primalID_GetSides(id_val)          (((id_val)&0xff)+3)
#define primalID_GetFaceAlign(id_val)      (((id_val)&PRIMAL_ALIGN_FACE)!=0)

#define primalID_GetSideCode(id_val)       ((id_val)&0xff)
#define primalID_InternalTypeCode(id_val)  (id_val>>8) /// cheat and know Type/Align are one big thing, eh?

#define PRIMAL_ALIGN_FACE     (0x100)

#define PRIMAL_CUBE_IDX       primalID_Make(PRIMAL_TYPE_SPECIAL,4)
#define PRIMAL_LIGHT_IDX      primalID_Make(PRIMAL_TYPE_SPECIAL,USED_PRIMAL_SIDES)     //12
#define PRIMAL_LINE_IDX       primalID_Make(PRIMAL_TYPE_SPECIAL,USED_PRIMAL_SIDES-1)   //11
#define PRIMAL_WEDGE_IDX      primalID_Make(PRIMAL_TYPE_SPECIAL,USED_PRIMAL_SIDES-2)   //10
#define PRIMAL_DODEC_IDX      primalID_Make(PRIMAL_TYPE_SPECIAL,USED_PRIMAL_SIDES-3)   //9


#define PRIMAL_TYPE_SPECIAL   (0)
#define PRIMAL_TYPE_CYLINDER  (1)
#define PRIMAL_TYPE_PYRAMID   (2)
#define PRIMAL_TYPE_CORNERPYR (3)
#define PRIMAL_TYPE_MAX       (4)

#define USED_PRIMAL_TYPES     (PRIMAL_TYPE_MAX)
#define USED_PRIMAL_SIDES     (12)          // for now, since we are keeping to editbr_ limit, eh

// the actual array of primals, should you need it
EXTERN primalInfo *primals_list[2*(USED_PRIMAL_TYPES)][USED_PRIMAL_SIDES-2];

#define getPrimInfo(prim_id) primals_list[primalID_InternalTypeCode(prim_id)][primalID_GetSideCode(prim_id)]

// various defines which might be useful
// really "unused", except to check in create stuff
#define MAX_PRIMAL_PTS        (64)
#define MAX_PRIMAL_EDGES     (128)
#define MAX_PRIMAL_FACES      (96)

//////////////////
// basic max counts and sizes
#define primalBr_PointCnt(i) (getPrimInfo(i)->points)
#define primalBr_EdgeCnt(i)  (getPrimInfo(i)->edges)
#define primalBr_FaceCnt(i)  (getPrimInfo(i)->faces)

/////////////////
// get at the shape data

// return the two brush points to use for this edge
EXTERN int *primalBr_EdgePts(int primal, int edge_id);

// return the _cnt_ points to use for this face
EXTERN int *primalBr_FacePts(int primal, int face_id, int *cnt);

// return the _cnt_ edges to use for this face
EXTERN int *primalBr_FaceEdges(int primal, int face_id, int *cnt);

////////////////
// get at point data, build world raw and full data

// get the raw initial untransed point data for this primal brush point
EXTERN mxs_vector *primalBr_GetRawPt(int primal, int pt_id);

// build all the points in brush br into array pts, returns the cnt used
EXTERN int primalRawFull(int primal, mxs_vector *scale, mxs_vector *pts);
EXTERN void primalRawPart(int primal, mxs_vector *scale, mxs_vector *pts, int *pt_list, int cnt);

// convert (via quantization) a brush pos to a usable world pos
EXTERN void primalQuantizePos(editBrush *br, mxs_vector *pos);
EXTERN void primalQuantizeAng(mxs_angvec *ang_in, mxds_vector *ang_out);

// tools for standalone transformation of primal (as brush) into world space
EXTERN void primalWorldPart(editBrush *br, mxs_vector *pts, int *pt_list, int cnt);
EXTERN void primalWorldFull(editBrush *br, mxs_vector *pts);

///////////////
// internal for use by other primal systems

// sanity checks a nascent primal brush for sizes of the face/pts/edges being cool
EXTERN BOOL _primalBr_SzCheck(primalInfo *us);

// get/free memory for a priminfo whose faces,pts,edges,and facestride are set
EXTERN BOOL _primalBr_GetMem(primalInfo *us);
EXTERN void _primalBr_FreeMem(primalInfo *us);

#ifdef DBG_ON
EXTERN void primalBr_mprint(int primal_id);
#endif
                            
#endif
