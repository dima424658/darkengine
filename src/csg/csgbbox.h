/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/csg/csgbbox.h,v 1.2 2000/01/29 12:58:26 adurant Exp $
#pragma once

extern void compute_poly_bbox(PortalPolygon *poly, BspVertex *minv, BspVertex *maxv);
extern void compute_ph_bbox(PortalPolyhedron *poly, BspVertex *minv, BspVertex *maxv);
