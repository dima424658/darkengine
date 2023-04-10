/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/primfast.h,v 1.3 2000/01/29 13:12:59 adurant Exp $
// fast unsafe non checking primal brush use
// really just for editbr, which "knows what it is doing"
#pragma once

#ifndef __PRIMFAST_H
#define __PRIMFAST_H

#include <primals.h>

// setup for fast
EXTERN int *primalBrs_FU_faceedgebase;
EXTERN primalInfo *primalBrs_FastUnsafe;

EXTERN void primalBr_SetFastUnsafePrimal(int primal_id);

// get edge pt 0 or 1 (n) from edge (edge) of current unsafe prim
#define primFastUnsafe_EdgePt(edge,n)      primalBrs_FastUnsafe->edge_list[((edge)*2)+(n)]

// you first faceedgesetup the face... then you can reference FaceEdge(edge), -1 means we are out..
#define primFastUnsafe_FaceEdgeSetup(face) \
   primalBrs_FU_faceedgebase=&primalBrs_FastUnsafe->face_edge_list[(face)*primalBrs_FastUnsafe->face_stride]
#define primFastUnsafe_FaceEdge(edge)      primalBrs_FU_faceedgebase[edge]

#define primFastUnsafe_EdgeCnt()           primalBrs_FastUnsafe->edges
#define primFastUnsafe_PointCnt()          primalBrs_FastUnsafe->points
#define primFastUnsafe_FaceCnt()           primalBrs_FastUnsafe->faces  

#endif  // __PRIMFAST_H
