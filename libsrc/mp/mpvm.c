//
// mpvm.c -- Multiped virtual motion/effector placement stuff.
//

#include <math.h>
#include <string.h>
#include <multiped.h>

//

extern quat mp_rot[4][MAX_JOINTS];

//

static mxs_vector	SaveJoints[MAX_JOINTS];
static mxs_matrix	SaveOrients[MAX_JOINTS];
static quat			SaveRelOrients[MAX_JOINTS];
static mxs_vector	SaveGlobalPos;
static mxs_matrix	SaveGlobalOrient;

//

void mp_save_mp_state(multiped * mp)
{
	memcpy(SaveJoints, mp->joints, sizeof(mxs_vector) * mp->num_joints);
	memcpy(SaveOrients, mp->orients, sizeof(mxs_matrix) * mp->num_joints);
	memcpy(SaveRelOrients, mp->rel_orients, sizeof(quat) * mp->num_joints);
	mx_copy_vec(&SaveGlobalPos, &mp->global_pos);
	mx_copy_mat(&SaveGlobalOrient, &mp->global_orient);
}

//

void mp_restore_mp_state(multiped * mp)
{
	memcpy(mp->joints, SaveJoints, sizeof(mxs_vector) * mp->num_joints);
	memcpy(mp->orients, SaveOrients, sizeof(mxs_matrix) * mp->num_joints);
	memcpy(mp->rel_orients, SaveRelOrients, sizeof(quat) * mp->num_joints);
	mx_copy_vec(&mp->global_pos, &SaveGlobalPos);
	mx_copy_mat(&mp->global_orient, &SaveGlobalOrient);
}

//

void mp_compute_ep(multiped * mp, mps_motion_node * m)
{
	mps_motion_node * main_motion;
	mps_motion * motion;
	mps_effector_place * ep = &m->params.ep;
	quat * q;
	int i;

	if (ep->absolute)
	{
	// save multiped joint positions.
		mp_save_mp_state(mp);
	
	// Figure out where the base joint will be at the time of placement. Need 
	// to account for bend, stretch, time scaling, etc...
	
		motion = mp_motion_list + m->handle;
	
	// Get main full-body motion.
		if (QuerySigJoint(motion->info.sig,mp->root_joint))
		{
			main_motion = m;
		}
		else
		{
			main_motion = mp_get_main_motion(mp);
		}
	
		if (main_motion)
		{
		// Evaluate motion at time of placement.
			limb * l = ep->l;
			mp_get_config_at_time(mp, main_motion, ep->placement_time, 
								  l->joint_id[l->num_segments]);
		}
	}

	mp_solve_limb(mp, ep, &mp_rot[1]);

	if (ep->stick)
	{
		if (main_motion)
		{
			limb * l = ep->l;
			mp_get_config_at_time(mp, main_motion, 
								  ep->placement_time + ep->placement_duration,
								  l->joint_id[l->num_segments]);
		}

		mp_solve_limb(mp, ep, &mp_rot[2]);

	// Alloc targets for rotations 1 and 2, copy.
		m->targets = (quat *) mp_alloc(sizeof(quat) * ep->l->num_segments * 2, __FILE__, __LINE__);
		q = m->targets;
		for (i = 0; i < ep->l->num_segments; i++, q++)
		{
			quat_copy(q, &mp_rot[1][i]);
			quat_copy(q + ep->l->num_segments, &mp_rot[2][i]);
		}
	}
	else
	{
	// Alloc targets for rotations 1, copy.
		m->targets = (quat *) mp_alloc(sizeof(quat) * ep->l->num_segments, __FILE__, __LINE__);
	
		q = m->targets;
		for (i = 0; i < ep->l->num_segments; i++, q++)
		{
			quat_copy(q, &mp_rot[1][i]);
		}
	}

	if (ep->absolute)
	{
	// Restore multiped state.
		mp_restore_mp_state(mp);
	}
}

//

void mp_get_config_at_time(multiped * mp, mps_motion_node * m, float time, 
						   int stop_joint)
{
	mps_motion * motion = mp_motion_list + m->handle;
	float fo = motion->info.freq / 1000.0 * time;
	mxs_vector v;
	float save_frame;

	save_frame = m->frame;

	if (m->params.flags & MP_DURATION_SCALE)
	{
		fo /= m->params.duration_scalar;
	}

	m->frame += fo;
	m->frame = __min(m->frame, motion->info.num_frames - 1);

	mp_evaluate_motion(mp, m, &mp_rot[0], &v);
	mp_apply_motion(mp, &mp_rot[0], &v, stop_joint);

	m->frame = save_frame;
}

void mp_get_config_at_frame(multiped * mp, mps_motion_node * m, float frame_offset)
{
	mps_motion * motion = mp_motion_list + m->handle;
	mxs_vector v;
	float save_frame;

	save_frame = m->frame;

	m->frame += frame_offset;
	m->frame = __min(m->frame, motion->info.num_frames - 1);

	mp_evaluate_motion(mp, m, &mp_rot[0], &v);
	mp_apply_motion(mp, &mp_rot[0], &v, -1);

	m->frame = save_frame;
}

//

void mp_solve_limb(multiped * mp, mps_effector_place * ep, quat * rot)
{
	mxs_vector v1, v2, normal, seg[2];

	mx_copy_vec(&v1, &mp->joints[ep->l->joint_id[0]]);

	if (ep->absolute)
	{
		mx_sub_vec(&v2, &ep->pos, &v1);
	}
	else
	{
		mx_copy_vec(&v2, &ep->pos);
	}

// Compute the normal. App provides this function.
	if (mp_compute_limb_normal)
	{
		mp_compute_limb_normal(&normal, mp, ep->l, &v2);
	}
	else
	{
		Spew(MP_SRC, ("App must set mp_compute_limb_normal function pointer.\n"));
	}

// Now find the mid joint's position based on the end position and normal.
	mp_compute_limb_pos(&v1, ep->l, &v2, &normal, ep->l->bend);

// Now compute rotations which correspond to given limb configuration.
	mx_copy_vec(&seg[0], &v1);
	mx_sub_vec(&seg[1], &v2, &v1);

	if (ep->use_orient)
	{
		mp_compute_limb_rot(rot, &mp->orients[mp->torsos[ep->l->torso_id].joint], ep->l, seg, &normal, &ep->orient);
	}
	else
	{
		mp_compute_limb_rot(rot, &mp->orients[mp->torsos[ep->l->torso_id].joint], ep->l, seg, &normal, NULL);
	}
	
#ifdef TEST_PLACEMENT
	TestRot(mp, ep, rot);
#endif
}

//
// Give a limb, end position relative to base position, and normal to plane
// containing base, mid, and end joints, compute the mid joint position
// (relative to base).
//
void mp_compute_limb_pos(mxs_vector * mid, limb * l, mxs_vector * end, mxs_vector * normal, int bend)
{
	mxs_vector v1, v2, v3;
	float dist, dist_squared, r3, r4;
	float limb_length, upper, lower;

// Get total limb length. There's no point in trying to pretend this routine 
// is general. Include effector length in lower segment for arms, not legs.

	upper = l->seg_len[0];
	if (bend)
	{
		lower = l->seg_len[1];
	}
	else
	{
		lower = l->seg_len[1] + l->seg_len[2];
	}
	limb_length = upper + lower;

	mx_copy_vec(&v1, end);
	dist = mx_mag_vec(&v1);
	if (dist > limb_length)
	{
		mx_scaleeq_vec(&v1, limb_length / dist);
		dist = mx_mag_vec(&v1);
	}

	dist_squared = dist * dist;
	r3 = upper * upper - lower * lower;
	r4 = ((r3 / dist_squared) + 1.0) / 2.0;

	mx_scale_vec(&v2, &v1, r4);
	mx_copy_vec(mid, &v2);

	mx_cross_vec(&v3, normal, &v1);
	r4 = (r3 / dist + dist) / 2.0;
	r4 = sqrt(upper * upper - r4 * r4) / dist;
	if (bend)
	{
		r4 = -r4;
	}

	mx_scale_vec(&v2, &v3, r4);
	mx_addeq_vec(mid, &v2);
}

//

void mp_compute_limb_rot(quat * rot, mxs_matrix * parent, limb * l, 
						 mxs_vector * pos, mxs_vector * normal, 
						 mxs_matrix * end_orient)
{
	int i;
	mxs_vector dx, dy, dz;
	mxs_vector * p = pos;
	mxs_matrix m_inv, cur, transform, parent_chain;
	quat * dst = rot;

	mx_copy_mat(&parent_chain, parent);
	mx_copy_vec(&dy, normal);

	for (i = 0; i < l->num_segments - 1; i++, p++, dst++)
	{
	// Build the current matrix.
		mx_norm_vec(&dx, p);
		mx_cross_vec(&dz, &dx, &dy);

		mx_copy_vec(&cur.vec[0], &dx);
		mx_copy_vec(&cur.vec[1], &dy);
		mx_copy_vec(&cur.vec[2], &dz);

		mx_inv_mat(&m_inv, &parent_chain);
		mx_mul_mat(&transform, &m_inv, &cur);

	// Save off transform somewhere.

		quat_from_matrix(dst, &transform);

	// Build up the parent chain.
		mx_muleq_mat(&parent_chain, &transform);
	}

// Enforce the end_orient. No problemo, since effector orientation (twist) is
// independent of other segments. Simply figure out what to multiply the wrist
// orientation by to get end_orient.
	
	if (end_orient)
	{
		mx_inv_mat(&m_inv, &parent_chain);
		mx_mul_mat(&transform, &m_inv, end_orient);
		quat_from_matrix(dst, &transform);
	}
	else
	{
		quat_identity(dst);
	}
}

//

#ifdef TEST_PLACEMENT

void TestRot(multiped * mp, mps_effector_place * ep, quat * rot)
{
	limb * l = ep->l;
	mxs_vector v1, v2, v3;
	mxs_matrix m1, m2, m3;

	mx_copy_vec(&v1, &mp->joints[l->joint_id[0]]);

	quat_to_matrix(&m1, &rot[0]);
	mx_mul_mat(&m2, &mp->orients[mp->torsos[l->torso_id].joint], &m1);

	mx_scale_vec(&v2, &l->seg[0], l->seg_len[0]);
	mx_mat_muleq_vec(&m2, &v2);

	mx_addeq_vec(&v1, &v2);
printf("elbow: %g, %g, %g\n", v1.x, v1.y, v1.z);

	quat_to_matrix(&m3, &rot[1]);
	mx_mul_mat(&m1, &m2, &m3);

	mx_scale_vec(&v2, &l->seg[1], l->seg_len[1]);
	mx_mat_muleq_vec(&m1, &v2);

	mx_addeq_vec(&v1, &v2);
printf("wrist: %g, %g, %g\n", v1.x, v1.y, v1.z);

	quat_to_matrix(&m2, &rot[2]);
	mx_mul_mat(&m3, &m1, &m2);

	mx_scale_vec(&v2, &l->seg[2], l->seg_len[2]);
	mx_mat_muleq_vec(&m3, &v2);
	mx_addeq_vec(&v1, &v2);
printf("end: %g, %g, %g\n", v1.x, v1.y, v1.z);
}

#endif

void mp_forecast_joint_pos(multiped *mp, mps_start_info *info, float frame, 
                           int joint, mxs_vector *pos)
{
   mps_motion_node *motion;

   if (info)
   {
      motion = mp_alloc_motion(mp, info);
      mp_setup_motion(mp,motion);
   }
      
   else
      motion = mp_get_main_motion(mp);
   if (motion)
   {
      // Save the multiped state
      mp_save_mp_state(mp);
      // Evaluate motion at time of placement.
      // TODO: this should probably be able to just evaluate the config
      // of the requested joint using the new mp_apply_motion func
      mp_get_config_at_frame(mp, motion, frame-motion->frame);
      // Get the joint position at contact frame
      *pos = mp->joints[joint];  
      // restore the mp state
      mp_restore_mp_state(mp);
   }
   if (info && motion)
      mp_free(motion);
}
//

