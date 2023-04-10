/*
 * $Source: x:/prj/tech/libsrc/skel/RCS/skel.c $
 * $Revision: 1.29 $
 * $Author: kate $
 * $Date: 1996/12/05 13:11:24 $
 *
 * Oi, skeleton libary datatype operators
 *
 */


#include <res.h>
#include <string.h>
#include <skel.h>
#include <_skel.h>
#include <mprintf.h>


skt_vcall_func skd_joint_call_tab[MAX_VCALLS];
skt_vcall_func skd_seg_call_tab[MAX_VCALLS];
uchar          skd_joint_call_joint[MAX_VCALLS];
mxs_vector     skd_seg_call_vecs[MAX_VCALLS];
mxs_vector     skd_vhot_vecs[MAX_VCALLS];
sk_poly_prep_func skel_poly_prep;


// Kevin!  You wanker!
uchar skd_dumb_clut[256];

// allocate and initialize a skeleton, uses
// Malloc, beware
sks_skel *sk_alloc_skel(ubyte segn,ubyte pointn,ubyte orn)
{
   sks_skel *sk ;

   sk = Malloc(sk_size_skel(segn,pointn,orn));
   sk_init_skel(sk,segn,pointn,orn);

   return sk;
}

void sk_free_skel(sks_skel *s)
{
   Free(s);
}

// returns target size of the skeleton, given the model
int   sk_size_skel(ubyte segn,ubyte pointn,ubyte orn)
{
   int sz;

   sz = sizeof(sks_skel);
   sz += segn * sizeof(sks_seginfo);
   sz += orn * sizeof(mxs_vector);
   sz += pointn * sizeof(mxs_vector);
   return sz;
}

// initialize a skeleton given the model
sks_skel *sk_init_skel(sks_skel *sk,ubyte segn,ubyte pointn,ubyte orn)
{
   sk->sg = (sks_seginfo *)(sk+1); // first byte after end of structure
   // allocate space for orientation vectors
   sk->pn = (mxs_vector *)(sk->sg + segn);
   sk->or = sk->pn + pointn;

   sk->segn = segn;
   sk->pointn = pointn;
   sk->orn = orn;

   return sk;
}

void sk_set_poly_prep(sk_poly_prep_func f)
{
   skel_poly_prep=f;
}

mds_model *skel_default_poly_prep(sks_seginfo *cseg,mxs_vector *pos)
{
   return cseg->model;
}

void sk_set_bmap_clut(sks_skel *sk, ubyte *clut)
{
   int i;

   for (i=0;i<sk->segn;++i) {
      sk->sg[i].clut = clut;
   }
}

void sk_set_skel_sort(sks_skel *sk,void (*qsort)(sks_skel *s,ubyte *rl,r3s_point *tmp))
{
   sk->qsort = qsort;
}

// Does a MKREF of handle and resid
void sk_res_lock(sks_skel *sk,Id resid)
{
   int i;

   switch (sk->type) {
      case BMAP:
         for (i=0;i<sk->segn;++i) {
            sk->sg[i].co = RefLock(MKREF(resid,sk->sg[i].handle));
         }
      break;
      case POLY:
         for (i=0;i<sk->segn;++i) {
            sk->sg[i].model = RefLock(MKREF(resid,sk->sg[i].handle));
         }
      break;
   }
}

// Assumes handle is refid
void sk_lock(sks_skel *sk)
{
   int i;

   switch (sk->type) {
      case BMAP:
         for (i=0;i<sk->segn;++i) {
            sk->sg[i].co = RefLock(sk->sg[i].handle);
         }
      break;
      case POLY:

         for (i=0;i<sk->segn;++i) {
            sk->sg[i].model = RefLock(sk->sg[i].handle);
         }
      break;
   }
}

void sk_unlock(sks_skel *sk)
{
   int i;

   if (sk->type < BMAP) return;

   for (i=0;i<sk->segn;++i) {
      RefUnlock(sk->sg[i].handle);
   }
}

void sk_res_unlock(sks_skel *sk,Id resid)
{
   int i;

   if (sk->type < BMAP) return;

   for (i=0;i<sk->segn;++i) {
      RefUnlock(MKREF(resid,sk->sg[i].handle));
   }
}


void sk_set_joint_call(int seg_id,skt_vcall_func v_func,int j)
{
   if (seg_id >= MAX_VCALLS) {
      Warning(("sk_set_vcall: tried vcall %d, max %d\n",seg_id,MAX_VCALLS));
      return;
   }

   skd_joint_call_tab[seg_id] = v_func;
   skd_joint_call_joint[seg_id] = j;
}

void sk_set_seg_call(int seg_id,skt_vcall_func v_func,fix xoff,fix yoff,fix zoff)
{
   if (seg_id >= MAX_VCALLS) {
      Warning(("sk_set_vcall: tried vcall %d, max %d\n",seg_id,MAX_VCALLS));
      return;
   }

   skd_seg_call_tab[seg_id] = v_func;  

   skd_seg_call_vecs[seg_id].x = xoff;
   skd_seg_call_vecs[seg_id].y = yoff;
   skd_seg_call_vecs[seg_id].z = zoff;
}

void sk_set_joint_vhot(int seg_id,int vhot_num,int j)
{
   if (seg_id >= MAX_VCALLS) {
      Warning(("sk_set_vhot: tried vcall %d, max %d\n",seg_id,MAX_VCALLS));
      return;
   }

   skd_joint_call_tab[seg_id] = (skt_vcall_func)(vhot_num+1);
   skd_joint_call_joint[seg_id] = j;
}

void sk_set_seg_vhot(int seg_id,int vhot_num,fix xoff,fix yoff,fix zoff)
{
   if (seg_id >= MAX_VCALLS) {
      Warning(("sk_set_vhot: tried vcall %d, max %d\n",seg_id,MAX_VCALLS));
      return;
   }

   skd_seg_call_tab[seg_id] = (skt_vcall_func)(vhot_num+1);

   skd_seg_call_vecs[seg_id].x = xoff;
   skd_seg_call_vecs[seg_id].y = yoff;
   skd_seg_call_vecs[seg_id].z = zoff;
}



void sk_init(void)
{
   int i;
   for (i=0;i<MAX_VCALLS;++i) {
      skd_joint_call_tab[i] = NULL;
      skd_seg_call_tab[i] = NULL;
   }
   for (i=0;i<256;++i) {
      skd_dumb_clut[i] = i;
   }
   skel_poly_prep=skel_default_poly_prep;
}

