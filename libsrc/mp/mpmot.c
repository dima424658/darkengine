//
// mpmot.c -- Multiped motion start/stop/transition functions.
//

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <multiped.h>

//

// from mpinit.c
extern mps_component_xlat_callback_func mp_capture_component_xlat_func;
//

void mp_start_motion(multiped * mp, mps_start_info * info)
{
	if (info->motion_num < mp_num_motions)
	{
		uint sig;
		mps_motion * motion;
		mps_motion_node * m = mp_alloc_motion(mp, info);

		motion = mp_motion_list + m->handle;
		sig = motion->info.sig;

		if (QuerySigJoint(sig,mp->root_joint))
		{
			int i;
			uchar * map;
			mps_transition_node * t;

			if (!mp_list_count(&mp->main_motion))
			{
			// List was previously empty. Need to transition from whatever
			// current pose is to new motion.
				mps_motion_node * pose = mp_setup_current_pose(mp);
				mp_list_add(&mp->main_motion, (mps_stack_node *) pose);
				Spew(MP_SRC, ("list empty, using current pose.\n"));

			// Update joint map to reflect new motion.
				map = mp->joint_map;
				for (i = 0; i < mp->num_joints; i++, map++)
				{
					if (*map == 0xff)
					{
						*map = 0;
					}
				}
			}

		// Add new motion to list.
			mp_list_add(&mp->main_motion, (mps_stack_node *) m);

		// Add transition.
			Spew(MP_SRC, ("main transition.\n"));

			t = (mps_transition_node *) mp_alloc(sizeof(mps_transition_node), __FILE__, __LINE__);
				
			t->type = MN_TRANSITION;
			t->time = 0;
			t->duration = info->trans_duration;

			mp_list_add(&mp->main_motion, (mps_stack_node *) t);
		}
		else
		{
			int i, trans;
			mps_overlay * ov;

		// Overlay motion.
			ov = mp->overlays;
			trans = -1;
			for (i = 0; i < mp->num_overlays; i++, ov++)
			{
			// Maybe equality isn't what we're look for here, but rather some
			// measure of "closeness" as in greatest number of shared joints.
				if (ov->sig == sig)
				{
					trans = i;
					break;
				}
			}

			if (trans == -1)
			{
				if (mp->num_overlays < MAX_OVERLAY_MOTIONS)
				{
					uchar * map;
					int ov_idx = mp->num_overlays++;

					ov = &mp->overlays[ov_idx];
					ov->sig = sig;
					mp_list_add(&ov->list, (mps_stack_node *) m);
				// 
				// Starting a new overlay. Must set up transition parameters.
					ov->mode = -1;
					ov->time = 0;
					ov->duration = info->trans_duration;
					Spew(MP_SRC, ("starting overlay %d.\n", m->handle));

				// Update joint map;
					map = mp->joint_map;
					for (i = 0; i < mp->num_joints; i++, map++)
					{
						if (QuerySigJoint(sig, i))
						{
							*map = ov_idx + 1;
						}
					}
				}
				else
				{
					Spew(MP_SRC, ("Too many overlays, can't add motion %d.\n", m->handle));
				}
			}
			else
			{
			// Add transition.
				mps_transition_node * t;

				Spew(MP_SRC, ("overlay transition.\n"));

				mp_list_add(&ov->list, (mps_stack_node *) m);
	
				t = (mps_transition_node *) mp_alloc(sizeof(mps_transition_node), __FILE__, __LINE__);
   			t->type = MN_TRANSITION;
				t->time = 0;
				t->duration = info->trans_duration;

				mp_list_add(&ov->list, (mps_stack_node *) t);
			}
		}
      mp_setup_motion(mp,m); // finish setup. need to happed after listadd call, so that
                             // start callback will have been made (and hence data locked).
	}
	else
	{
		Spew(MP_SRC, ("Invalid motion specified: %d\n", info->motion_num));
	}
}

//

mps_motion_node * mp_alloc_motion(multiped * mp, mps_start_info * info)
{
	mps_motion_node * m = (mps_motion_node *) mp_alloc(sizeof(mps_motion_node), __FILE__, __LINE__);

	m->type = MN_MOTION;
	m->time = 0;
	m->handle = info->motion_num;
	m->start_frame = info->start_frame;
	m->frame = info->start_frame;
	m->callback = info->callback;
        m->callback_num = info->callback_num;
	if (m->callback || m->callback_num)
	{
		m->callback_frame = info->callback_frame;
	}

// Deal with parameters.
	if (info->params)
	{
		memcpy(&m->params, info->params, sizeof(mps_motion_param));
	}
	else
	{
		m->params.flags = 0;
	}

	m->targets = NULL;

	if (m->params.flags & MP_PLACE_EFFECTOR)
	{
	// Deal with effector placement.
		mp_compute_ep(mp, m);
	}
   return m;
}

//

void mp_setup_motion(multiped * mp, mps_motion_node *m)
{
	mps_motion * motion;
	motion = mp_motion_list + m->handle;

// Only deal with positional stuff for whole-body motions.
	if (QuerySigJoint(motion->info.sig,mp->root_joint))
	{
		float angle;
		mxs_matrix mat;

		if ((m->start_frame == 0) || (motion->info.type != MT_CAPTURE) || mp->frame_of_reference!=MFRT_REL_START)
		{
			mx_zero_vec(&m->offset);
		}
		else
		{
		// If we're not starting at frame 0, all the translational offsets
		// will be wrong, since they are offsets from frame 0. So we need to 
		// store the offset of the start frame which we can then subtract from 
		// each frame's translation.

			mp_get_xlat_at_frame(&m->offset, m->handle, m->start_frame);
		}
	
   	// Set motion's start position & orient
      if(mp->frame_of_reference==MFRT_GLOBAL)
      {
         mx_zero_vec(&m->base_pos);
         quat_identity(&m->base_orient);
      } else
      {
   		mx_copy_vec(&m->base_pos, &mp->global_pos);

   	   // We only want the initial heading (z rotation). 
			if(mp_get_heading(mp,&angle))
         {
   			mx_mk_rot_z_mat(&mat, mx_rad2ang(angle));
	   		quat_from_matrix(&m->base_orient, &mat);
         } else {
            // motion not upright.  don't try to be clever/helpful
            quat_identity(&m->base_orient);
         }
      }
	}

// Set flag index correctly.
	if (motion->num_flags)
	{
		mps_motion_flag * f	= motion->flags;
		m->flag_index = 0;

		while ((f->frame < m->start_frame) && (m->flag_index < motion->num_flags))
		{
			f++;
			m->flag_index++;
		}
	}
	else
	{
		m->flag_index = -1;
	}
}

//
// In order to make the  stop motion functions work in callbacks, we flag
// motions to be removed on next update. 
//
void mp_stop_motion(multiped * mp, int motion)
{
	bool found = FALSE;
	mps_stack_node * node = NULL;
	while (NULL!=(node = mp_list_traverse(&mp->main_motion, node)) && !found)
	{
		if (node->type == MN_MOTION)
		{
			mps_motion_node * mot = (mps_motion_node *) node;
			if (mot->handle == motion)
			{
				int i;
				uchar * map;

				mot->handle |= MP_KILL_FLAG;
				found = TRUE;
				mp->main_motion.flags = 1;

			// Update joint map;
				map = mp->joint_map;
				for (i = 0; i < mp->num_joints; i++, map++)
				{
					if (*map == 0)
					{
						*map = 0xff;
					}
				}
			}
		}
	}

	if (!found)
	{
		int i;
		mps_overlay * o = mp->overlays;

		for (i = 0; (i < mp->num_overlays) && !found; i++, o++)
		{
			node = NULL;
			while (NULL!=(node = mp_list_traverse(&o->list, node)) && !found)
			{
				if (node->type == MN_MOTION)
				{
					mps_motion_node * mot = (mps_motion_node *) node;
					if (mot->handle == motion)
					{
						int j;
						uchar * map;
						int idx = (mp_list_count(&mp->main_motion)) ? 0 : 0xff;

						mot->handle |= MP_KILL_FLAG;
						found = TRUE;
						o->list.flags = 1;

					// Update joint map.
						map = mp->joint_map;
						for (j = 0; j < mp->num_joints; j++, map++)
						{
							if (QuerySigJoint(o->sig, j))
							{
								*map = idx;
							}
						}
					}
				}
			}

		}
	}
}		

//
// Flag all motions to be killed on next call to mp_update().
//
void mp_stop_all_motions(multiped * mp)
{
	int i;
	mps_stack_node * node = NULL;
	mps_overlay * o = mp->overlays;

	if (mp_list_count(&mp->main_motion) > 0)
	{
		while (NULL!=(node = mp_list_traverse(&mp->main_motion, node)))
		{
			if (node->type == MN_MOTION)
			{
				mps_motion_node * mot = (mps_motion_node *) node;
				mot->handle |= MP_KILL_FLAG;
			}
		}
		mp->main_motion.flags = 1;
	}


	for (i = 0; i < mp->num_overlays; i++, o++)
	{
		if (mp_list_count(&o->list) > 0)
		{
			node = NULL;
			while (NULL!=(node = mp_list_traverse(&o->list, node)))
			{
				if (node->type == MN_MOTION)
				{
					mps_motion_node * mot = (mps_motion_node *) node;
					mot->handle |= MP_KILL_FLAG;
				}
			}
			o->list.flags = 1;
		}
	}

// Update joint map.
	memset(mp->joint_map, 0xff, mp->num_joints);
}

//

mps_motion_node * mp_setup_current_pose(multiped * mp)
{
	mps_motion_node * m = (mps_motion_node *) mp_alloc(sizeof(mps_motion_node), __FILE__, __LINE__);

	m->type = MN_MOTION;
	m->handle = -1;
	m->frame = -1;
	m->callback = NULL;
        m->callback_num = 0;
	m->params.flags = 0;

   // @HACK add extra "joint" for base/root orient
   // @HACK add mxs_vector for position
	m->targets = (quat *) mp_alloc((sizeof(quat) * (mp->num_joints+1))+sizeof(mxs_vector), __FILE__, __LINE__);

   // @HACK add extra "joint" for base/root orient
	memcpy(m->targets, mp->rel_orients, sizeof(quat) * (mp->num_joints+1));
   // @HACK add mxs_vector for position
   mx_copy_vec((mxs_vector *)(((quat *)(m->targets))+mp->num_joints+1),&mp->rel_xlat);

	return m;
}

//

mps_motion_node * mp_get_main_motion(multiped * mp)
{
// The motion node before the last transition is the "main" motion, because
// it's the one that we're ultimately transitioning into.

	mps_motion_node * result;
	mps_stack_node * node = mp->main_motion.tail;

	if (node)
	{
		if (node->type == MN_MOTION)
		{
			result = (mps_motion_node *) node;
		}
		else
		{
			result = (mps_motion_node *) node->prev;
		}
	}
	else
	{
		result = NULL;
	}
	
	return result;
}

//

void mp_get_xlat_at_frame(mxs_vector * xlat, int motion, int frame)
{
// BY CONVENTION, the root translation component motion, if present, 
// is ALWAYS FIRST. THIS DOESN'T DEAL WITH VIRTUAL MOTIONS.

	mps_motion * m = mp_motion_list + motion;
	mps_comp_motion * cm = m->components;
	if (cm->type == CM_TRANS)
	{
      mp_capture_component_xlat_func(NULL,&m->info,cm,frame,xlat);
	}
	else
	{
		mx_zero_vec(xlat);
	}
}

//
