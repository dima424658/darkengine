//
// mploco.c -- Multiped locomotion interface functions.
//

#include <math.h>
#include <multiped.h>

//
// Returns heading normalized so 0 <= heading <= 2Pi.
// NOTE: Will probably fail if multiped is not upright, e.g. lying down,
// crawling, etc.
//

#define MAX_TILT_TOLERANCE 0.8

BOOL mp_get_heading(multiped * mp, float *heading)
{
   float result;
   mxs_vector v1, v2;

   mx_copy_vec(&v1, &mp->global_orient.vec[0]);

   if(v1.z>MAX_TILT_TOLERANCE)
      return FALSE;
   v1.z = 0;
   mx_normeq_vec(&v1);
   mx_unit_vec(&v2, 0);

   result = acos(mx_dot_vec(&v1, &v2));
   if (v1.y < 0)
   {
      result = MX_REAL_2PI - result;
   }

   *heading=result;
   return TRUE;
}

//

bool mp_set_heading(multiped * mp, float heading, float dt)
{
   bool result;
   mxs_vector v;
   mps_stack_node * node;
   mps_motion_node * mm = mp_get_main_motion(mp);

   if (mm)
   {
      // Cancel any bend in existing motions to avoid conflict.
      node = NULL;
      while (node = mp_list_traverse(&mp->main_motion, node))
      {
         if (node->type == MN_MOTION)
         {
            mps_motion_node * m = (mps_motion_node *) node;
            m->params.flags &= ~MP_BEND;
         }
      }
	
      mx_unit_vec(&v, 2);
      quat_create(&mp->target_heading, &v, heading);
      mp->th_time = 0;
      mp->th_duration = dt;

      result = TRUE;
   }
   else
   {
      // No main motions playing, do nothing.
      result = FALSE;
   }

   return result;
}

//

void mp_compute_loco_params(float * bend, float * stretch, multiped * mp, 
                            mps_start_info * info, mxs_vector * endpoint)
{
   int motion_num, start_frame;
   float d1, d2;
   mxs_vector heading, delta, v, offset;
   mps_motion * m;

   mx_sub_vec(&delta, endpoint, &mp->global_pos);
   delta.z = 0;
   mx_norm_vec(&v, &delta);

   mx_copy_vec(&heading, &mp->global_orient.vec[0]);
   heading.z = 0;
   mx_normeq_vec(&heading);
   *bend = acos(mx_dot_vec(&heading, &v));

   // Figure out direction of bend.
   mx_cross_vec(&v, &heading, &delta);
   if (v.z < 0)
   {
      *bend = -(*bend);
   }

   d1 = mx_mag_vec(&delta);

   if (info)
   {
      motion_num = info->motion_num;
      start_frame = info->start_frame;
   }
   else
   {
      mps_motion_node * mn = mp_get_main_motion(mp);
      if (mn)
      {
         motion_num = mn->handle;
         start_frame = (int) floor(mn->frame + 0.5);
      }
   }

   m = mp_motion_list + motion_num;
   mp_get_xlat_at_frame(&v, motion_num, m->info.num_frames - 1);
   mp_get_xlat_at_frame(&offset, motion_num, start_frame);

   mx_subeq_vec(&v, &offset);
   d2 = mx_mag_vec(&v);
   *stretch = d1 / d2;
}

//

void mp_start_locomotion(multiped * mp, mps_start_info * info, 
                         mxs_vector * endpoint)
{
   mps_motion_param p;

   mp_compute_loco_params(&p.bend, &p.stretch, mp, info, endpoint);
   p.flags = 0;
   if (fabs(p.bend) > 0.001)
   {
      p.flags |= MP_BEND;
   }
   if (fabs(p.stretch - 1.0) > 0.001)
   {
      p.flags |= MP_STRETCH;
   }

   info->params = &p;
   mp_start_motion(mp, info);
}

//

void mp_compute_endpoint(mxs_vector * endpoint, multiped * mp, 
                         mps_start_info * info)
{
   float angle;
   mxs_matrix mat;
   mxs_vector v1, v2;
   mps_motion_param * p;
   mps_motion * m;
   int start_frame, motion_num;

   // Get translational distance at start frame.
	
   if (info)
   {
      start_frame = info->start_frame;
      motion_num = info->motion_num;
      p = info->params;
   }
   else
   {
      mps_motion_node * mn = mp_get_main_motion(mp);
      if (mn)
      {
         start_frame = (int) floor(mn->frame + 0.5);
         motion_num = mn->handle;
         p = &mn->params;
      }
   }

   m = mp_motion_list + motion_num;

   mp_get_xlat_at_frame(&v1, motion_num, start_frame);
   mp_get_xlat_at_frame(&v2, motion_num, m->info.num_frames - 1);
   mx_subeq_vec(&v2, &v1);

   mp_get_heading(mp,&angle);
   mx_mk_rot_z_mat(&mat, mx_rad2ang(angle));

   if (p)
   {
      if (p->flags & MP_BEND)
      {
         mxs_matrix m2;
         mx_mk_rot_z_mat(&m2, mx_rad2ang(p->bend));
         mx_muleq_mat(&mat, &m2);
      }
      if (p->flags & MP_STRETCH)
      {
         mx_scaleeq_vec(&v2, p->stretch);
      }
   }

   mx_mat_muleq_vec(&mat, &v2);
   mx_add_vec(endpoint, &mp->global_pos, &v2);
}

//

void mp_sample_loco_path(multiped * mp, mps_start_info * info, int num_points, mxs_vector * points)
{
   int i, num_frames, start_frame, motion_num;
   float frame, dframe, angle;
   mxs_vector v, offset, *p;
   mps_motion * m;
   mps_motion_param * prm;
   mxs_matrix base_mat, mat;
   mps_motion_node * mn;
	
   if (info)
   {
      motion_num = info->motion_num;
      start_frame = info->start_frame;
      prm = info->params;
   }
   else
   {
      mn = mp_get_main_motion(mp);
      if (mn)
      {
         start_frame = (int) floor(mn->frame + 0.5);
         motion_num = mn->handle;
         prm = &mn->params;
      }
   }

   m = mp_motion_list + motion_num;
   num_frames = m->info.num_frames - 1 - start_frame;
   dframe = (float) num_frames / (float) num_points;

   frame = start_frame + dframe;

   mp_get_heading(mp,&angle);
   mx_mk_rot_z_mat(&base_mat, mx_rad2ang(angle));

   mp_get_xlat_at_frame(&offset, motion_num, start_frame);

   // The code in this loop copies the translational part of mp_evaluate_motion()
   // in mpstack.c.

   p = points;
   for (i = 0; i < num_points; i++, p++)
   {
      mp_get_xlat_at_frame(&v, motion_num, (int) floor(frame + 0.5));
      mx_subeq_vec(&v, &offset);

      if (prm)
      {
         if (prm->flags & MP_BEND)
         {
            mxs_matrix m2;
            float t = (frame - start_frame) / (m->info.num_frames - 1 - start_frame);
            angle = t * prm->bend;
	
            mx_mk_rot_z_mat(&m2, mx_rad2ang(angle));
            mx_mul_mat(&mat, &base_mat, &m2);
         }
         else
         {
            mx_copy_mat(&mat, &base_mat);
         }

         if (prm->flags & MP_STRETCH)
         {
            mx_scaleeq_vec(&v, prm->stretch);
         }
      }

      mx_mat_muleq_vec(&mat, &v);

      mx_add_vec(p, &mp->global_pos, &v);
      frame += dframe;
   }
}

// Assuming endpoint is in range, compute bend which will pass multiped
// through point.
void mp_compute_pass_through(float * bend, multiped * mp, 
                             mps_start_info * info, mxs_vector * endpoint)
{
   // Just like compute_bend, but scale motion vector so that when you get the
   // desired distance along it you'll be at the endpoint.
   int start_frame, motion_num;
   float d1, d2, theta, t;
   mxs_vector heading, delta, v, offset;
   mps_motion * m;
   mps_motion_node * mn;
	
   if (info)
   {
      start_frame = info->start_frame;
      motion_num = info->motion_num;
   }
   else
   {
      mn = mp_get_main_motion(mp);
      if (mn)
      {
         start_frame = (int) floor(mn->frame + 0.5);
         motion_num = mn->handle;
      }
   }
	
   m = mp_motion_list + motion_num;

   mx_sub_vec(&delta, endpoint, &mp->global_pos);
   delta.z = 0;
   mx_norm_vec(&v, &delta);

   mx_copy_vec(&heading, &mp->global_orient.vec[0]);
   heading.z = 0;
   mx_normeq_vec(&heading);
   theta = acos(mx_dot_vec(&heading, &v));

   // Figure out direction of bend.
   mx_cross_vec(&v, &heading, &delta);
   if (v.z < 0)
   {
      theta = -theta;
   }

   d1 = mx_mag_vec(&delta);
   mp_get_xlat_at_frame(&v, motion_num, m->info.num_frames - 1);
   mp_get_xlat_at_frame(&offset, motion_num, start_frame);
   mx_subeq_vec(&v, &offset);
   d2 = mx_mag_vec(&v);

// Now compute final bend.
   t = d1 / d2;
   *bend = theta + (theta / t) * (1.0 - t);
}


//

#define MP_TIMEOUT	50

//

void mp_compute_approach(float * bend, multiped * mp, mps_start_info * info,
                         mxs_vector * endpoint)
{
   bool done;
   float angle, b;
   mps_motion * m;
   mxs_vector delta, d, v, v2, offset, heading;
   mxs_matrix base_mat, mat;
   int i, start_frame, motion_num;

   // Now compute bend, using iterative method.

   mp_get_heading(mp,&angle);
   mx_mk_rot_z_mat(&base_mat, mx_rad2ang(angle));

   if (info)
   {
      start_frame = info->start_frame;
      motion_num = info->motion_num;
   }
   else
   {
      mps_motion_node * mn = mp_get_main_motion(mp);
      if (mn)
      {
         start_frame = (int) floor(mn->frame + 0.5);
         motion_num = mn->handle;
      }
   }

   m = mp_motion_list + motion_num;

   mp_get_xlat_at_frame(&offset, motion_num, start_frame);
   mp_get_xlat_at_frame(&v, motion_num, m->info.num_frames - 1);

   mx_subeq_vec(&v, &offset);
   mx_mat_muleq_vec(&base_mat, &v);

   // Now v is where the motion will end up with no bend. (relative to start pos)

   mx_sub_vec(&delta, endpoint, &mp->global_pos);
   delta.z = 0;
   mx_norm_vec(&d, &delta);

   mx_copy_vec(&heading, &mp->global_orient.vec[0]);
   heading.z = 0;
   mx_normeq_vec(&heading);

   // Choose half of angle between current heading and desired endpoint as initial
   // bend.
   b = acos(mx_dot_vec(&heading, &d)) / 2.0;
   mx_cross_vec(&v2, &heading, &d);
   if (v2.z < 0)
   {
      b = -b;
   }

   i = 0;
   done = FALSE;
   while (!done)
   {
      mxs_vector pos, h, diff, curr_end;

      // Find the end position and heading given bend b.
      mx_mk_rot_z_mat(&mat, mx_rad2ang(b));
      mx_mat_mul_vec(&pos, &mat, &v);

      mx_mk_rot_z_mat(&mat, mx_rad2ang(2.0 * b));
      mx_mat_mul_vec(&h, &mat, &heading);

      mx_add_vec(&curr_end, &pos, &mp->global_pos);
      mx_sub_vec(&diff, endpoint, &curr_end);

      diff.z = 0;
      mx_normeq_vec(&diff);

      angle = acos(mx_dot_vec(&h, &diff));
      if (fabs(angle) < 0.001)
      {
         done = TRUE;
      }
      else
      {
         mx_cross_vec(&v2, &h, &diff);
         if (v2.z < 0)
         {
            angle = -angle;
         }
			
         b += angle / 2.0;
      }

      i++;

      if (i > MP_TIMEOUT)
      {
         // I've never seen this thing iterate more than 10 times, but 
         // oscillatory weirdness is probably possible. -BB
         done = TRUE;
         Spew(MP_SRC, ("Timed out in mp_compute_approach().\n"));
      }
   }

   *bend = b;
}


//

int mp_motion_count_flags(int motion_num, int start_frame, uint search_flags)
{
   int i, result = 0;
   mps_motion * m = mp_motion_list + motion_num;
   mps_motion_flag *f = m->flags;

   for (i = 0; i < m->num_flags; i++, f++)
   {
      if ((f->flags & search_flags) && (f->frame>=start_frame))
      {
         result++;
      }
   }
   return result;
}

int mp_count_flags(multiped * mp, uint search_flags)
{
   mps_motion_node *mn;

   if ((mn = mp_get_main_motion(mp)) == NULL)
      return 0;
   return mp_motion_count_flags(mn->handle, mn->frame, search_flags);
}

int mp_motion_next_flags(int motion_num, int start_frame, uint search_flags)
{
   int i;
   mps_motion * m;
   mps_motion_flag * f;

   m = mp_motion_list + motion_num;
   f = m->flags;
   for (i=0; i<m->num_flags; i++, f++)
   {
      if ((f->flags&search_flags) && (f->frame>=start_frame))
      {
         return f->frame;
      }
   }
   return -1;
}

int mp_next_flags(multiped * mp, uint search_flags)
{
   mps_motion_node *mn;

   if ((mn = mp_get_main_motion(mp)) == NULL)
      return -1;
   return mp_motion_next_flags(mn->handle, mn->frame, search_flags);
}

int mp_motion_last_flags(int motion_num, int start_frame, uint search_flags)
{
   int i;
   mps_motion * m;
   mps_motion_flag * f;
   int last_frame = -1;

   m = mp_motion_list + motion_num;
   f = m->flags;
   for (i=0; i<m->num_flags; i++, f++)
   {
      if ((f->flags&search_flags) && (f->frame>=start_frame))
      {
         if (f->frame>last_frame)
            last_frame = f->frame;
      }
   }
   return last_frame;
}

int mp_last_flags(multiped * mp, uint search_flags)
{
   mps_motion_node *mn;

   if ((mn = mp_get_main_motion(mp)) == NULL)
      return -1;
   return mp_motion_last_flags(mn->handle, mn->frame, search_flags);
}

//

void mp_reset_loco_params(multiped * mp, float bend, float stretch)
{
   mps_motion_node * mn = mp_get_main_motion(mp);
	
   if (mn)
   {
      float angle;
      int frame = (int) floor(mn->frame + 0.5);
      mxs_matrix mat;

      // Need to effectively restart the motion by resetting base_orient, 
      // base_pos, start_frame, and offset to reflect current configuration.
      mp_get_xlat_at_frame(&mn->offset, mn->handle, frame);
      mn->start_frame = frame;
      mx_copy_vec(&mn->base_pos, &mp->global_pos);
	
      // We only want the initial heading (z rotation). 
      mp_get_heading(mp,&angle);
	
      mx_mk_rot_z_mat(&mat, mx_rad2ang(angle));
      quat_from_matrix(&mn->base_orient, &mat);	

      mn->params.flags |= MP_BEND | MP_STRETCH;
      mn->params.bend = bend;
      mn->params.stretch = stretch;
   }
}

bool mp_get_loco_params(multiped * mp, float *bend, float *stretch)
{
   mps_motion_node * mn = mp_get_main_motion(mp);
	
   if (mn)
   {
      *bend = mn->params.bend;
      *stretch = mn->params.stretch;
      return TRUE;
   }
   return FALSE;
}
