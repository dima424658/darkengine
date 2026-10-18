/*
 * $Source: x:/prj/tech/libsrc/skel/RCS/_skel.h $
 * $Revision: 1.7 $
 * $Author: kate $
 * $Date: 1996/12/05 13:12:27 $
 *
 * Skeleton libary private includes
 *
 */

#ifndef ___NEWSKEL_H
#define ___NEWSKEL_H

#include <skel.h>

#define MAX_VCALLS 32
typedef union sks_call_slot {
   skt_vcall_func func;
   int vhot;
} sks_call_slot;
EXTERN sks_call_slot skd_joint_call_tab[];
EXTERN sks_call_slot skd_seg_call_tab[];
EXTERN uchar       skd_joint_call_joint[];
EXTERN mxs_vector skd_seg_call_vecs[];
EXTERN mxs_vector skd_vhot_vecs[];
EXTERN sk_poly_prep_func skel_poly_prep;

// Sort the render list for the skeleton
// given the list of transformed points
// this is the slow, but correct version
void sk_midpoint_sort(sks_skel *s,ubyte *rl,r3s_point *tmp);

// Do no sort, woo woo!
void sk_no_sort(sks_skel *s,ubyte *rl);


#endif
