/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/roomutil.h,v 1.3 2000/01/31 10:00:41 adurant Exp $
//
// Room System Utilities header
//
#pragma once

#ifndef __ROOMUTIL_H
#define __ROOMUTIL_H

#include <matrixs.h>

#include <roombase.h>


////////////////////////
//
// Function prototypes
//
extern mxs_real PointPlaneDist(const tPlane &plane, const mxs_vector &point);
extern BOOL     RayPlaneIntersection(const tPlane &plane, const mxs_vector &from, const mxs_vector &dir, mxs_vector *intersection);
extern BOOL     LinePlaneIntersection(const tPlane &plane, const mxs_vector &from, const mxs_vector &to, mxs_vector *intersection);
extern BOOL     OBBsIntersect(const tOBB &b1, const tOBB &b2);
extern BOOL     PointInOBB(const tOBB &obb, const mxs_vector &pt);
extern void     FindOBBPortal(const tOBB &b1, const tOBB &b2, tPlane *portalPlane, 
                              tPlane portalEdges[], int *portalEdgeSize, mxs_vector *center);
extern void     GetOBBPlanes(const tOBB &obb, tPlane plane_list[]);

#endif





