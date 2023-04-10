/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/primals.h,v 1.2 2000/01/29 13:12:58 adurant Exp $
// structures for primal brush system/setup
#pragma once

#ifndef __PRIMALS_H
#define __PRIMALS_H

#include <matrixs.h>

typedef struct {
   int points, edges, faces;
   int face_stride;      // number of edge or face entries per face, counting bonus -1 at end
   mxs_vector *pt_array; 
   int *edge_list;
   int *face_pts_list;
   int *face_edge_list;
} primalInfo;

#endif  // __PRIMALS_H
