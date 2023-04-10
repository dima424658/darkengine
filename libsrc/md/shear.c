// $Header: x:/prj/tech/libsrc/md/RCS/shear.c 1.2 1996/12/13 12:42:38 kate Exp $

#include <string.h>
#include <math.h>

#include <lg.h>
#include <matrix.h>
#include <r3ds.h>
#include <md.h>

#define max(x,y) (((x)>(y))?(x):(y))

// This is implemented in scale.c
extern float radius_from_bbox(mxs_vector *bmin,mxs_vector *bmax);


void md_shear_vec(mxs_vector *v, int ax_src, int ax_targ, mxs_real s)
{
   v->el[ax_targ]=v->el[ax_targ]+s*v->el[ax_src];
}

// d is plane equation value from dot(n,v)-d=0, where n is the
// normal to the plane, and v is a point on the plane.  Can be
// interpreted geometrically as the distance along n one must
// travel from the origin to intersect the plane.
// new d values are computed using observation:
// dot(T(d_old*n_old),n_new)=d_new, where T is a linear transform.
// the normal transform can be thought of as shearing
// by -s with ax_src and ax_targ switched, so the inverse is shearing by s
float calcDShift(float d,mxs_vector *newNorm,int ax_src,int ax_targ,float s)
{
   mxs_vector oldNorm;
   
   mx_copy_vec(&oldNorm,newNorm);
   // we know this is old norm without renormalizing because 
   // new norm was never normalized
   md_shear_vec(&oldNorm,ax_targ,ax_src,s);
  
   // T(n_old)
   md_shear_vec(&oldNorm,ax_src,ax_targ,s);
   return d*mx_dot_vec(&oldNorm,newNorm);
}

// Shear a model according to the shearing factor s.  The shearing is 
// done in source space, in the ax_src-ax_targ plane.
// For example, if ax_targ=2 (z-axis) and ax_src=1 (y-axis), then
// the transformation is x->x, y->y, z->z+sy.  The result is that a rectangle in
// the y-z plane centered at the origin will become a parallelogram with 
// two vertical sides, and two sides with slope s.
// 
// If the model *dst is NULL, it allocates memory for it.
// light is whether or not to retransform the lighting vectors.  Depending,
// you may actually want it to be the same, plus it's the slowest thing to
// do
mds_model *md_shear_model(mds_model *dst,mds_model *src,int ax_src,int ax_targ,mxs_real s,bool light)
{
   mds_vhot *v;
   mxs_vector *pt;
   mxs_vector trans;
   mds_pgon *pgon;
   int i;

   mx_unit_vec(&trans,ax_targ);
   trans.el[ax_src]=s;

   if (dst == NULL) {
      dst = Malloc(src->mod_size);
   }

   // copy it over
   memcpy(dst,src,src->mod_size);

   // shear the bbox. 
   // pcen is unchanged, since shearing done relative to center
   md_shear_vec(&dst->bmin,ax_src,ax_targ,s);
   md_shear_vec(&dst->bmax,ax_src,ax_targ,s);

   // Recompute bounding sphere
   dst->radius = radius_from_bbox(&dst->bmin,&dst->bmax);

   // Recompute max pgon radius, by assuming worst case
   dst->max_pgon_radius *= sqrt(1+s*s);

   // Scale the vhot lists
   v = md_vhot_list(dst);
   for (i=dst->vhots;i>0;--i) {
      md_shear_vec(&v->v,ax_src,ax_targ,s);
      v++;
   }

   // Scale the points
   pt = md_point_list(dst);
   for (i=dst->verts;i>0;--i) {
      md_shear_vec(pt,ax_src,ax_targ,s);
      pt++;
   }

   // lights, norms and pgons and nodes
   // are done per subobj

   for (i=0;i<dst->subobjs;++i) {
      int j;
      mxs_vector *norm;
      mds_subobj *sb = &(md_subobj_list(dst)[i]);
      uchar *n;   // node

      // Subobject header:
      // trans.. gotta think about that...
      // I think we just shear the offset
      md_shear_vec(&(sb->trans.vec),ax_src,ax_targ,s);

      // lighting normals:
      if (light) {
         mds_light *lt;
         mxs_vector l;

         lt = md_light_list(dst)+sb->light_start;

         for (j=sb->light_num;j>0;--j) {
            // Unpack the normal
            l.x = X_NORM(lt->norm);
            l.y = Y_NORM(lt->norm);
            l.z = Z_NORM(lt->norm);
            md_shear_vec(&l,ax_targ,ax_src,-s);
            // renormalize
            mx_normeq_vec(&l);
            // repack
            md_norm2light(&(lt->norm),&l);
            lt++;
         }
      }

      // These must not be renormalized for d value computation to work.
      // d values need to be recomputed both in pgons and nodes
      // the normal transform can be thought of as shearing
      // by -s with ax_src and ax_targ switched and then
      // renormalizing.

      norm = md_norm_list(dst)+sb->norm_start;
      for (j=sb->norm_num;j>0;--j) {
         md_shear_vec(norm,ax_targ,ax_src,-s);
         norm++;
      }

      // Go through the nodes
      // XXX redo this
      n = (uchar *)dst + dst->node_off + sb->node_start;
      for (j=sb->node_num;j>0;--j) {
         switch (*n) {
            case MD_NODE_SUBOBJ:
            {
               mds_node_subobj *node = (mds_node_subobj *)n;
               n = (char *)(node+1);
               ++j;  // doesn't count
               break;
            }
            case MD_NODE_VCALL:
            {
               mds_node_vcall *node = (mds_node_vcall *)n;

               n = (char *)(node+1);
               break;
            }
            case MD_NODE_CALL:
            {
               mds_node_call *node = (mds_node_call *)n;

               n = (char *)(node+1);
               n += (node->pgons_before+node->pgons_after)*sizeof(short);
               break;
            }
            case MD_NODE_SPLIT:
            {
               mds_node_split *node = (mds_node_split *)n;

               node->d=calcDShift(node->d,md_norm_list(dst)+sb->norm_start+node->norm,ax_src,ax_targ,s);
               n = (char *)(node+1);
               n += (node->pgons_before+node->pgons_after)*sizeof(short);
               break;
            }
            case MD_NODE_RAW:
            {
               mds_node_raw *node = (mds_node_raw *)n;
               n = (char *)(node+1);
               n += node->num*sizeof(short);
               break;
            }
         }
      }
   }

   // Go through the polygon list
   // and touch up the d values
   // XXX redo this
   pgon = (mds_pgon *)(dst->pgon_off+(uchar*)dst);
   for (i=dst->pgons;i>0;--i) {
      uchar *next;
      pgon->d=calcDShift(pgon->d,md_norm_list(dst)+pgon->norm,ax_src,ax_targ,s);
      next = (uchar *)pgon;
      next += sizeof(mds_pgon);
      next += 2*pgon->num*sizeof(ushort);
      if ( ((pgon->type)&MD_PGON_PRIM_MASK)==MD_PGON_PRIM_TMAP) {
         next += pgon->num * sizeof(ushort);
      }
      pgon = (mds_pgon *)next;
   }

   return dst;

}
