// $Header: x:/prj/tech/libsrc/mm/RCS/xformseg.c 1.7 1998/06/30 18:59:27 buzzard Exp $
// this module contains the various segment transforming routines

#include <lg.h>
#include <xformseg.h>
#include <r3d.h>
#include <mm.h>
#include <mm_.h>
#include <g2.h>


static set_uvs(int num, int pt_start, mms_uvn *uvns)
{
   r3s_point *p_ptr;
   mms_uvn *uvn_ptr=uvns;
   int i;

   num+=pt_start;
   for(i = pt_start; i < num; i++, uvn_ptr++)
   {
      p_ptr=mmd_buff_point(i);
      p_ptr->grp.u=uvn_ptr->u;
      p_ptr->grp.v=uvn_ptr->v;      
   }
}

void handle_light_cback(int num,r3s_point *pts,mms_uvn *uvns,mxs_vector *loc)
{
   int j;
   float *output;
   if (mmd_rgb_lighting) {
      output = Malloc(3 * num * sizeof(*output));
      mmd_light_cback(num, output, uvns, loc);
      for (j=0; j < num; ++j) {
         g2s_point *g2p = (g2s_point *) &pts->grp;
         g2p->i = output[j*3+0];
         g2p->h = output[j*3+1];
         g2p->d = output[j*3+2];
         pts = (r3s_point *) ((char *) pts + r3d_glob.cur_stride);
      }
   } else {
      output = Malloc(num * sizeof(*output));
      mmd_light_cback(num, output, uvns, loc);
      for (j=0; j < num; ++j) {
         pts->grp.i = output[j];
         pts = (r3s_point *) ((char *) pts + r3d_glob.cur_stride);
      }
   }
   Free(output);
}

#define BIG_FLOAT 10000.0

// XXX normals are not computed here, since they may depend on vertices from
// other segments.
static void mm_transform_weighted_data_chunk(mms_data_chunk *d,
                                             mxs_vector *viewer, quat *qchild)
{
   int i;
   quat q,qid;
   mxs_matrix delta;
   mxs_vector v;

   quat_identity(&qid);

   // calculate the orientations based on vertex weight.
   for(i=0;i<d->verts;i++)
   {
      quat_slerp(&q,&qid,qchild,mmd_weights[d->weight_start+i]);
      quat_to_matrix(&delta,&q);
      mx_mat_mul_vec(&v, &delta, &mmd_vert_vecs[i+d->vert_start]);
      r3_transform_point(mmd_buff_point(i+d->vert_start), &v);
   }
   // use parent joint's orientation.  Can get fancy later if necessary
   if(mmd_light_cback)
   {
      handle_light_cback(d->verts, mmd_buff_point(d->vert_start),
                      mmd_vert_uvns + d->vert_start, 
                      mmd_vert_vecs + d->vert_start);
   }
   // set (u,v)'s
   set_uvs(d->verts,d->vert_start,&mmd_vert_uvns[d->vert_start]);
}


static void mm_transform_data_chunk(mms_data_chunk *d,mxs_vector *viewer)
{
   int j;

   if(!d->verts)
      return;

   r3_transform_block(d->verts, mmd_buff_point(d->vert_start),
                      &mmd_vert_vecs[d->vert_start]);

   // transform the normals, just need to do once
   for (j=d->pgon_start;j<d->pgon_start+d->pgons;j++) 
   {
      mmd_buff_norms[j] = mx_dot_vec(viewer,&mmd_norms[j]);
   }
   if(mmd_light_cback) 
   {
      handle_light_cback(d->verts, mmd_buff_point(d->vert_start),
                      mmd_vert_uvns+d->vert_start, mmd_vert_vecs+d->vert_start);
      
   }
   // set (u,v)'s
   set_uvs(d->verts,d->vert_start,&mmd_vert_uvns[d->vert_start]);
}

void mm_transform_stretchy_normals(mms_segment *s)
{
   int i, j;
   mms_mapping *map;
   mms_pgon *p;
   mms_data_chunk *d;

   // XXX THIS IS A HACK!  normal is only used to determine visibility, and
   // we don't have normal sharing, so just make buff value be really
   // negative if screen coords of triangle aren't oriented correctly.
   // TO DO: change mmd_buff_norms to be true/false buffer of whether poly
   // is facing.
   map=&mmd_mappings[s->map_start];
   for(i=0;i<s->smatsegs;i++,map++)
   {
      d = &mmd_smatsegs[*map].data;
      p = &mmd_pgons[d->pgon_start];

      for (j=d->pgon_start; j<d->pgon_start+d->pgons; p++,j++) 
      {
         grs_point *a = &mmd_buff_point(p->v[0])->grp;
         grs_point *b = &mmd_buff_point(p->v[1])->grp;
         grs_point *c = &mmd_buff_point(p->v[2])->grp;

         // If the triangle has some vertices in front of the camera,
         // and others not, we say it's visible and let the low-level
         // rendering routines backface cull.
         mmd_buff_norms[j] = -BIG_FLOAT;

         // We have to right-shift some numbers to fit our products into
         // 16-16 fixed ints.
         if (a->w >= 0.0 || b->w >= 0.0 || c->w >= 0.0) // punt if all behind
            if (a->w < 0.0 || a->w > 1.0
             || b->w < 0.0 || b->w > 1.0
             || c->w < 0.0 || c->w > 1.0
             || (fix_mul((b->sx - a->sx) >> 8, (c->sy - a->sy) >> 8)
               - fix_mul((b->sy - a->sy) >> 8, (c->sx - a->sx) >> 8) <= 0))
               mmd_buff_norms[j] = BIG_FLOAT;
      }
   }
}


static void _mm_transform_attachments(mxs_trans *segment_trans,
                                      int segment_index)
{
   int num_attachments = mmd_custom_data->num_attachments;

   if (num_attachments) {
      int i;
      mxs_trans *trans = mmd_buff_attach_trans;
      mms_attachment *attachment = mmd_custom_data->attachment_list;

      for (i = 0; i < num_attachments; ++i) {
         if (attachment->segment_index == segment_index)
            mx_mul_trans(trans, segment_trans, &attachment->relative_trans);

         ++trans;
         ++attachment;
      }
   }
}


void mm_transform_seg_any(mms_segment *s, int segment_index)
{
   int i;
   mms_mapping *map;
   mxs_trans trans;
   mxs_vector  *viewer;
   int num_smatsegs = s->smatsegs;
   int num_attachments = mmd_custom_data->num_attachments;

   if (!num_smatsegs && !num_attachments)
      return;

   // transform the viewer into object space now we can do dot prods
   // on pgon normals
   viewer = r3_get_view_in_obj();

   map=&mmd_mappings[s->map_start];

   if(s->flags&MMSEG_FLAG_STRETCHY)
   {
      quat q;

      stretchy_joint_cback(mmd_model, s->joint_id, &trans, &q);
      r3_start_object_trans(&trans);
      r3_start_block();
      for(i = 0; i < num_smatsegs; ++map, ++i)
      {
         mm_transform_weighted_data_chunk(&mmd_smatsegs[*map].data,viewer,&q);
      }

      r3_end_block();
      r3_end_object();
   } else
   {
      joint_pos_cback(mmd_model, s->joint_id, &trans);
      // XXX should make this a start and stop function?
      r3_start_object_trans(&trans);
      r3_start_block();
      for(i = 0; i < num_smatsegs; ++map, ++i)
      {
         mm_transform_data_chunk(&mmd_smatsegs[*map].data,viewer);
      }

      r3_end_block();
      r3_end_object();
   }

   if (num_attachments)
      _mm_transform_attachments(&trans, segment_index);
}


///////////////////////////////////////////////////////////////
// world-space rotation for mesh-ray intersection--does not deal
// w/(u,v)s, normals, or attachments, just vertices
static void mm_o2w_weighted_data_chunk(mms_data_chunk *d, quat *qchild)
{
   int i;
   quat q,qid;
   mxs_matrix delta;
   mxs_vector v;  // our vertex in object space, from the weighted quat

   quat_identity(&qid);

   // calculate the orientations based on vertex weight
   for (i = 0; i < d->verts; ++i) {
      quat_slerp(&q, &qid, qchild, mmd_weights[d->weight_start + i]);
      quat_to_matrix(&delta, &q);
      mx_mat_mul_vec(&v, &delta, &mmd_vert_vecs[d->vert_start + i]);
      r3_transform_o2w(&mmd_buff_point(d->vert_start + i)->p, &v);
   }
}


static void mm_o2w_data_chunk(mms_data_chunk *d)
{
   int i;

   if(!d->verts)
      return;

   for (i = 0; i < d->verts; ++i) {
      r3_transform_o2w(&mmd_buff_point(d->vert_start + i)->p,
                       &mmd_vert_vecs[d->vert_start + i]);
   }
}


void mm_o2w_seg_any(mms_segment *s)
{
   int i;
   mms_mapping *map;
   mxs_trans trans;
   int num_smatsegs = s->smatsegs;

   if (!num_smatsegs)
      return;

   map = &mmd_mappings[s->map_start];

   if(s->flags & MMSEG_FLAG_STRETCHY) {
      quat q;

      stretchy_joint_cback(mmd_model, s->joint_id, &trans, &q);
      r3_start_object_trans(&trans);
      r3_start_block();

      for(i = 0; i < num_smatsegs; ++map, ++i)
         mm_o2w_weighted_data_chunk(&mmd_smatsegs[*map].data, &q);

      r3_end_block();
      r3_end_object();
   } else {
      joint_pos_cback(mmd_model, s->joint_id, &trans);
      r3_start_object_trans(&trans);
      r3_start_block();

      for(i = 0; i < num_smatsegs; ++map, ++i)
         mm_o2w_data_chunk(&mmd_smatsegs[*map].data);

      r3_end_block();
      r3_end_object();
   }
}
