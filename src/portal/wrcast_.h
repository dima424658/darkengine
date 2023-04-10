/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/wrcast_.h,v 1.3 2000/01/29 13:37:40 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   wrcast_.h

   We're exporting some bits and pieces of the raycaster so the
   spherecaster can use them.

   THIS IS NO LONGER USED.  The bits we're exporting these days
   are more encapsulated, and are in port.h.  This is still in
   the backing tree in case we need to pull some really old code.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#ifndef _WRCAST__H_
#define _WRCAST__H_

#include <wrdb.h>

extern PortalPolygonCore *_portal_raycast_cur_poly;
extern mxs_vector *_portal_raycast_cur_vectors;
extern uchar *_portal_raycast_point_indirection;
extern mxs_vector _portal_raycast_perp_edge;

extern bool _PortalConvexHullXYPosZ(void);
extern bool _PortalConvexHullXYNegZ(void);
extern bool _PortalConvexHullYZPosX(void);
extern bool _PortalConvexHullYZNegX(void);
extern bool _PortalConvexHullZXPosY(void);
extern bool _PortalConvexHullZXNegY(void);

typedef struct {
   PortalPolygonCore *poly;
   uint vertex_offset;
} _PortalRaycastExit;

#endif
