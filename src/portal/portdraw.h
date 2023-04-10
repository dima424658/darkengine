/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   portdraw.h

   export for portdraw.c

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _PORTDRAW_H_
#define _PORTDRAW_H_


extern int check_surface_visible(PortalCell *cell, 
                                 PortalPolygonCore *poly, int voff);

extern void portal_mip_setup(float zoom);

extern ushort *cur_anim_light_index_list;

#endif

