/*
 * $Source: x:/prj/tech/libsrc/md/RCS/scale.c $
 * $Revision: 1.6 $
 * $Author: dc $
 * $Date: 1997/07/03 04:26:20 $
 *
 * Model Library scaling routine
 *
 */

#include <string.h>
#include <math.h>

#include <lg.h>
#include <matrix.h>
#include <r3ds.h>
#include <md.h>

#define max(x,y) (((x)>(y))?(x):(y))

#ifndef SHIP
   #define TEST_VERSION(fname,model)   \
   do { \
      if (model->ver!=MD_CUR_VER) { \
         char safename[10]; \
         strncpy(safename,model->name,8); \
         safename[8]='\0'; \
         CriticalMsg3("%s: model %s is old version %d",fname,safename,model->ver); \
         return model; \
      } \
   } while(0)
#else
   #define TEST_VERSION(fname,model) 
#endif


float radius_from_bbox(mxs_vector *bmin,mxs_vector *bmax)
{
   float rad2,mn2,mx2;

   // greatest x
   mn2 = bmin->x*bmin->x;
   mx2 = bmax->x*bmax->x;
   rad2 = max(mn2,mx2);

   // greatest y
   mn2 = bmin->y*bmin->y;
   mx2 = bmax->y*bmax->y;
   rad2 += max(mn2,mx2);

   // greatest z
   mn2 = bmin->z*bmin->z;
   mx2 = bmax->z*bmax->z;
   rad2 += max(mn2,mx2);

   // radius is dist from center to max point
   return sqrt(rad2);
}


// Scale a model according to the scaling vector s.  The scaling is 
// done in source space.  If the model *dst is NULL, it allocates memory
// for it
// light is whether or not to retransform the lighting vectors.  Depending,
// you may actually want it to be the same, plus it's the slowest thing to
// do
mds_model *md_scale_model(mds_model *dst,mds_model *src,mxs_vector *s,bool light)
{
   mds_vhot *v;
   mxs_vector *pt;
   mxs_vector sn; // normal scale factor
   float sd;   // plane scale factor
   mds_pgon *pgon;
   int i;

   TEST_VERSION("md_scale_model",src);

   // This is what normals get scaled by
   sn.x = s->y*s->z;
   sn.y = s->z*s->x;
   sn.z = s->x*s->y;

   sd = s->x*s->y*s->z;

   if (dst == NULL) {
      dst = Malloc(src->mod_size);
   }

   // copy it over
   memcpy(dst,src,src->mod_size);

   // Scale the bbox and pcen
   mx_elmuleq_vec(&dst->bmin,s);
   mx_elmuleq_vec(&dst->bmax,s);
   mx_elmuleq_vec(&dst->pcen,s);

   // Recompute bounding sphere
   dst->radius = radius_from_bbox(&dst->bmin,&dst->bmax);

   // Recompute max pgon radius, by putting bounding
   // box around it and shrinking it
   dst->max_pgon_radius *= mx_mag_vec(s);

   // Scale the vhot lists
   v = md_vhot_list(dst);
   for (i=dst->vhots;i>0;--i) {
      mx_elmuleq_vec(&v->v,s);
      v++;
   }

   // Scale the points
   pt = md_point_list(dst);
   for (i=dst->verts;i>0;--i) {
      mx_elmuleq_vec(pt,s);
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
      // I think we just scale the offset
      mx_elmuleq_vec(&(sb->trans.vec),s);

      // These don't have to be normalized, but the d values
      // need to be recomputed both in pgons and nodes
      norm = md_norm_list(dst)+sb->norm_start;
      for (j=sb->norm_num;j>0;--j) {
         mx_elmuleq_vec(norm,&sn);
         norm++;
      }

      // lighting normals:
      if (light) {
         mds_light *lt;
         mxs_vector l;

         lt = md_light_list(dst)+sb->light_start;

         for (j=sb->light_num;j>0;--j) {
            // Unpack the normal
            l.x = X_NORM(lt->norm)*sn.x;
            l.y = Y_NORM(lt->norm)*sn.y;
            l.z = Z_NORM(lt->norm)*sn.z;
            // renormalize
            mx_normeq_vec(&l);
            // repack
            md_norm2light(&(lt->norm),&l);
            lt++;
         }
      }

      // Go through the nodes

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
               node->d *= sd;
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
   pgon = (mds_pgon *)(dst->pgon_off+(uchar*)dst);
   for (i=dst->pgons;i>0;--i) {
      uchar *next;
      pgon->d *= sd;
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
