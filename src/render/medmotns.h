/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/medmotns.h,v 1.3 2000/01/29 13:39:07 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   medmotns.h

   structure file for medium motion

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _MEDMOTNS_H_
#define _MEDMOTNS_H_

#include <medmotnt.h>

// The _change fields are for making the water move: dposition/second.
struct sMedMoCellMotion {
   mxs_vector center_change;
   mxs_ang angle_change;
};

#endif // ~_MEDMOTNS_H_
