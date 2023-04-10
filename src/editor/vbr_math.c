/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vbr_math.c,v 1.9 2000/02/19 13:13:43 toml Exp $
// math ops for virtual brush manipulation fun (tm)

#include <lg.h>
#include <matrix.h>
#include <matrixd.h>
#include <mxcvrt.h>
#include <mprintf.h>
#include <math.h>

#include <editbr.h>
#include <editbr_.h>
#include <brinfo.h>
#include <brrend.h>
#include <brquery.h>
#include <primal.h>

#include <vbrush.h>
#include <vbr_math.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// control parameters for brush motion
bool vBrush_relative_motion=FALSE;
bool vBrush_axial_scale=FALSE;

/////////////
// utility/setup stuff

#ifdef VEC_MPRINT
// print a matrix for debugging for now
static void br_prn_mat(mxs_matrix *m)
{
   char buf[128];
   int i;
   for (i=0; i<3; i++)
   {
      sprintf(buf," %4.6f %4.6f %4.6f\n",m->el[i],m->el[i+3],m->el[i+6]);
      mprintf(buf);
   }
}

static void br_prn_vec(mxs_vector *v)
{
   char buf[128];   
   sprintf(buf," %4.6f %4.6f %4.6f",v->el[0],v->el[1],v->el[2]);
   mprintf(buf);
}

// print a matrix for debugging for now
static void br_prn_mat_d(mxds_matrix *m)
{
   char buf[128];
   int i;
   for (i=0; i<3; i++)
   {
      sprintf(buf," %4.12lf %4.12lf %4.12lf\n",m->el[i],m->el[i+3],m->el[i+6]);
      mprintf(buf);
   }
}

static void br_prn_vec_d(mxds_vector *v)
{
   char buf[128];   
   sprintf(buf," %4.12lf %4.12lf %4.12lf",v->el[0],v->el[1],v->el[2]);
   mprintf(buf);
}
#endif

// would be cool if FacePts could get pulled out of here
// finds an origin for (us),
//   someday may filter ang_in if necessary (ie. an edge) (ang can be NULL)
//   returns TRUE if a complex edge thing, else FALSE if only focused on brush as whole
BOOL find_brush_origin(editBrush *us, mxs_vector *origin, mxs_angvec *ang_in)
{
   mxs_vector *pvec, sum_vec;
   int i, *pt_list, pt_val, pt_cnt=0;
   
   if ((brSelect_Flag(us)&brSelect_COMPLEX)||(us->cur_face!=-1))
   {
      pvec=brushBuildSolo(us);
      if (brSelect_Flag(us)&brSelect_COMPLEX)
         if (brSelect_Flag(us)&brSelect_EDGE)
         {
            pt_list=primalBr_EdgePts(us->primal_id,brSelect_Edge(us));
            pt_cnt=2;
         }
         else
         {
            pt_val=brSelect_Point(us);
            pt_list=&pt_val;
            pt_cnt=1;
         }
      else // since we know cur_face!=-1, we can just use the face
         pt_list=primalBr_FacePts(us->primal_id,us->cur_face,&pt_cnt);
      if (pt_cnt&&pt_list)
      {  // now go through and find the median point of all the points in the list
         mx_zero_vec(&sum_vec);
         for (i=0; i<pt_cnt; i++)
            mx_addeq_vec(&sum_vec,&pvec[pt_list[i]]);
         mx_scale_vec(origin,&sum_vec,(float)(1.0/pt_cnt));
         return TRUE;  // we have done it, long live us, so on
      }
   }
   mx_copy_vec(origin,&us->pos);
   return FALSE;
}

// we have no ways of doing stuff with angvecs, so here are some things
BOOL sub_angvecs(mxs_angvec *targ, mxs_angvec *v1, mxs_angvec *v2)
{
   targ->el[0]=v1->el[0]-v2->el[0];
   targ->el[1]=v1->el[1]-v2->el[1];
   targ->el[2]=v1->el[2]-v2->el[2];
   return targ->el[0]|targ->el[1]|targ->el[2];
}

#ifdef NEEDED
static void invert_angvec(mxs_angvec *vec, mxs_angvec *invvec)
{
   invvec->el[0]=-vec->el[0];
   invvec->el[1]=-vec->el[1];
   invvec->el[2]=-vec->el[2];
}
#endif

#define ANG_EPSILON (0.00001)
static BOOL sub_double_ang_vectors(mxds_vector *targ, mxds_vector *a1, mxds_vector *a2)
{
   mxd_sub_vec(targ,a1,a2);
   return (fabs(targ->el[0])>ANG_EPSILON)||(fabs(targ->el[1])>ANG_EPSILON)||(fabs(targ->el[2])>ANG_EPSILON);
}

/////// TRANSLATION
mxs_vector *br_trans_vec;
void br_translate_setup(mxs_vector *raw_vec)
{
   mxs_vector tmp;
   if (vBrush_relative_motion)
   {
      mxs_matrix rot;
      mx_ang2mat(&rot,&vBrush_GetSel()->ang);
      mx_mat_mul_vec(&tmp,&rot,raw_vec);
      mx_copy_vec(raw_vec,&tmp);
   }
   br_trans_vec=raw_vec;
}

void br_translate(editBrush *us)
{
   mx_addeq_vec(&us->pos,br_trans_vec);
}

/////// SCALING
#define MIN_SCALE (0.1)
#define MIN_DENOM (0.01)
BOOL compute_scale_fac(mxs_vector *scale, mxs_vector *old_v, mxs_vector *new_v)
{
   if ((old_v->el[0]<MIN_DENOM)||(old_v->el[1]<MIN_DENOM)||(old_v->el[2]<MIN_DENOM))
      return FALSE;
   scale->el[0]=new_v->el[0]/old_v->el[0];
   scale->el[1]=new_v->el[1]/old_v->el[1];
   scale->el[2]=new_v->el[2]/old_v->el[2];
   if ((scale->el[0]<MIN_SCALE)||(scale->el[1]<MIN_SCALE)||(scale->el[2]<MIN_SCALE))
      return FALSE;
   return TRUE;
}

static mxs_vector   scale_origin;    // where in worldspace do we start from?
static mxds_vector  source_ang;      // this is just used for comparing if we match?

static mxs_vector  *scale_local;
static mxds_matrix  scale_xform_mat;
void br_scale_setup(mxs_vector *raw_vec)
{
   editBrush *master=vBrush_GetSel();
   mxds_matrix rot_in;
   mxds_vector scale_d;

   mx_vec_d_from_s(&scale_d,raw_vec);
   primalQuantizeAng(&master->ang,&source_ang);
   mxd_rad2mat(&scale_xform_mat,&source_ang);  // this is L-1, from brush to world, really
   mxd_trans_mat(&rot_in,&scale_xform_mat);    // so this is L, from world offset to brush offset
   mxd_mat_eltmuleq_vec(&rot_in,&scale_d);     // this is ISL, scaled world offset in brush space
   mxd_muleq_mat(&scale_xform_mat,&rot_in);    // and now back to world space, L(-1)ISL
   scale_local=raw_vec;                        // in brush space scale facs, for sz if we can
   find_brush_origin(master,&scale_origin,&master->ang); // hey, we need the origin, eh?
}

// actually go do the scale
void br_scale(editBrush *us)
{
   mxs_vector  offset_s,xform_offset_s;
   mxds_vector offset,xform_offset;
   BOOL        do_scale=vBrush_axial_scale;

   // we take the offset vector, multiply it by L(-1)ISL, add new offset back to origin
   mx_sub_vec(&offset_s,&us->pos,&scale_origin); // get offset of brush from source of scale_d
   mx_vec_d_from_s(&offset,&offset_s);
   mxd_mat_mul_vec(&xform_offset,&scale_xform_mat,&offset); // compute new offset
   mx_vec_s_from_d(&xform_offset_s,&xform_offset);
   mx_add_vec(&us->pos,&scale_origin,&xform_offset_s); // move the brush to there

   if (!do_scale)     
   {  // not scaling for sure, so check if we are same angle as source brush
      mxds_vector br_ang, diff_ang;
      primalQuantizeAng(&us->ang,&br_ang);    // are we at the same angle?
      do_scale=!sub_double_ang_vectors(&diff_ang,&source_ang,&br_ang);
   }
   if (do_scale)       //   we take the size of brush times brush space scale vec (per-element)   
      mx_elmuleq_vec(&us->sz,scale_local);
}

/////// ROTATION

static mxds_matrix rot_base;     // out is from brush to axial, back is back
static mxs_vector  rot_origin;

BOOL ang_quantize_and_sub(mxds_vector *diff, mxs_angvec *v1, mxs_angvec *v2)
{
   mxds_vector a1, a2;
   primalQuantizeAng(v1,&a1);
   primalQuantizeAng(v2,&a2);
   return sub_double_ang_vectors(diff,&a1,&a2);
}

void br_rotate_setup(mxs_angvec *rot)
{
   editBrush *master=vBrush_GetSel();
   mxds_matrix lmat, rmat;
   
   primalQuantizeAng(&master->ang,&source_ang);
   find_brush_origin(master,&rot_origin,&master->ang);
   if (vBrush_relative_motion)
   {
      mxd_rad2mat(&lmat,&source_ang);   // L is axial to master brush
      mxd_ang2mat(&rmat,rot);           // R is the rotation of the object
      mxd_muleq_mat(&rmat,&lmat);       // so this is RL
      mxd_trans_mat(&rot_base,&lmat);   // this is L(-1), since L is orthonormal
      mxd_muleq_mat(&rot_base,&rmat);   // and here is L(-1)RL
   }
   else
      mxd_ang2mat(&rot_base,rot);       // if non relative, just build axial rot matrix
}

void br_rotate_rad_setup(mxds_vector *real_rot)
{
   editBrush *master=vBrush_GetSel();
   mxds_matrix lmat, rmat;

   primalQuantizeAng(&master->ang,&source_ang);
   find_brush_origin(master,&rot_origin,&master->ang);
   if (vBrush_relative_motion)
   {
      mxd_rad2mat(&lmat,&source_ang);   // L is axial to master brush
      mxd_rad2mat(&rmat,real_rot);      // R is the rotation of the object
      mxd_muleq_mat(&rmat,&lmat);       // so this is RL
      mxd_trans_mat(&rot_base,&lmat);   // this is L(-1), since L is orthonormal
      mxd_muleq_mat(&rot_base,&rmat);   // and here is L(-1)RL
   }
   else
      mxd_rad2mat(&rot_base,real_rot);  // if non relative, just build axial rot matrix
}

void br_rotate(editBrush *us)
{
   mxs_vector  offset_s, xform_offset_s;
   mxds_vector offset, xform_offset;   
   mxds_vector br_ang, br_inv_ang;
   mxds_matrix tmp, tmp2;

   primalQuantizeAng(&us->ang,&br_ang);
   mxd_neg_vec(&br_inv_ang,&br_ang);
   mxd_rad2mat(&tmp2,&br_ang);         // U: this goes from brush to axial space
   mxd_mul_mat(&tmp,&rot_base,&tmp2);   // now we are L(-1)RLU
   mxd_mat2ang(&us->ang,&tmp);
   mxd_mat2rad(&br_ang,&tmp);   

   // we take the offset vector, multiply it by L(-1)RL, add new offset back to origin
   mx_sub_vec(&offset_s,&us->pos,&rot_origin);         // offset of brush from origin
   mx_vec_d_from_s(&offset,&offset_s);   
   mxd_mat_mul_vec(&xform_offset,&rot_base,&offset);  // compute new offset
   mx_vec_s_from_d(&xform_offset_s,&xform_offset);   
   mx_add_vec(&us->pos,&rot_origin,&xform_offset_s);   // move the brush to there
}

////////////////////
// horrible "back and store" of vector fields

typedef struct {
   short      br_id;  // 2
   mxs_angvec ang;    // +6=8
   mxs_vector pos;    // +12=20
   mxs_vector sz;     // +12=32
} brushVecs;

static brushVecs *curVecData    = NULL;
static int        curVecCnt     = 0;
static int        curVecAlloced = 0;

#define ALLOC_CHUNK 64

static BOOL store_vec(editBrush *us)
{
   if (curVecCnt>=curVecAlloced)
   {  // get more memory
      curVecAlloced+=ALLOC_CHUNK;
      if (curVecAlloced==ALLOC_CHUNK)  // first one
         curVecData=(brushVecs *)Malloc(curVecAlloced*sizeof(brushVecs));
      else
         curVecData=(brushVecs *)Realloc(curVecData,curVecAlloced*sizeof(brushVecs));
   }
   curVecData[curVecCnt].br_id = us->br_id;
   curVecData[curVecCnt].ang   = us->ang;
   curVecData[curVecCnt].pos   = us->pos;
   curVecData[curVecCnt].sz    = us->sz;
   curVecCnt++;
   return TRUE;
}

BOOL store_BrushVecs(void)
{
   curVecCnt=0;
   if (brFilter(vBrush_inCurGroup,store_vec)!=curVecCnt)
      Warning(("Huh?!? odd vec store craziness\n"));
   return curVecCnt>0;
}

void clear_BrushVecs(void)
{
   if (curVecAlloced)
      Free(curVecData);
   curVecAlloced=0;
   curVecData=NULL;
}

void restore_BrushVecs(int which)
{
   int i;
   
   for (i=0; i<curVecCnt; i++)
   {
      brushVecs *vec_data=&curVecData[i];
      editBrush *us=brFind(vec_data->br_id);
      if (us==NULL)
         Warning(("Missing brush in restore_BrushVecs\n"));
      else
      {
         if (which&brVec_ANG) us->ang=vec_data->ang;
         if (which&brVec_POS) us->pos=vec_data->pos;
         if (which&brVec_SZ)  us->sz =vec_data->sz;
      }
   }
}
