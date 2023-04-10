/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   portwatr.h

   The partly-transparent surface of water slides around to reflect
   the current.  This code assumes that the water's surface is not
   perfectly vertical.

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _PORTWATR_H_
#define _PORTWATR_H_

#include <matrix.h>

#include <wrdb.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Here's where we actually mess with the texture anchor and (u, v).
extern void portal_position_portal_texture(mxs_vector *dest_u, 
                                           mxs_vector *dest_v, 
                                           mxs_vector *dest_anchor,
                                           mxs_vector *anchor,
                                           PortalPolygonRenderInfo *render,
                                           PortalPlane *plane,
                                           PortalCellMotion *motion);

#ifdef __cplusplus
};
#endif

#endif // ~_PORTWATR_H_