/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshray.h,v 1.2 2000/01/29 13:39:11 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   meshray.h

   RPG engine wrapper for mesh-ray intersection

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _MESHRAY_H_
#define _MESHRAY_H_

#include <mm.h>
#include <objtype.h>

EXTERN BOOL MeshRayIntersection(mxs_vector *pStart, mxs_vector *pEnd,
                                BOOL bEarlyOut, ObjID Obj,
                                mms_ray_hit_detect_info *pDetectInfo);

#endif // ~_MESHRAY_H_

