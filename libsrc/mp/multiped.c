#include "multiped.h"
#include "motstruc.h"
#include <stdio.h>
#include <math.h>
#include <lg.h>
#include <schedstr.h>

//#include <appbiped.h>
#include <motion.h>

#define QSCALE	0.5

//

extern char * Jnt[];

//

mpm_motion *mpd_motion_table;
int mpd_max_mtab = 0;

void mp_old_init(int max_mtab)
{
	int i;

	mpd_motion_table=(mpm_motion *)Malloc(max_mtab*sizeof(mpm_motion));

	//DbgSetLogFile(MP_SRC, "mpout.txt");

	for (i = 0; i < max_mtab; i++)
	{
		mpd_motion_table[i].handle=MP_MOT_INVALID;
	}

	mpd_max_mtab = max_mtab;
}

void mp_old_close()
{
   Free(mpd_motion_table);
   mpd_max_mtab = 0;
}

// this is clearly imbecilic, but we don't really know what motion loading should 
// do yet.
int mp_old_load_motion(Id motID)
{
	int result;
	int i = 0;

	while ((i < mpd_max_mtab) && (mpd_motion_table[i].handle != MP_MOT_INVALID))
	{
		i++;
	}

	if (i == mpd_max_mtab)
	{
		result = MP_MOT_INVALID;
	}
	else 
	{
		if (ResExtract(motID, (void *) &mpd_motion_table[i]))
		{
			mpd_motion_table[i].handle = i;
			result = i;
		}
	}

	return result;
}

//

float MotionWeight = 1.0;

//
extern int DBG_flag;
//

#if 0
//
// THIS IS THE UPDATE FUNCTION FOR LG-BIPED DATA.
//
void mp_old_update(multiped * mp, float dt, mxs_vector * pos, mxs_matrix * orient)
{
	int i, res_id, res_index, norm_id, bend, frame;
	mpm_cmot_handle * cm;
	unsigned char * motion_data;
	effec_data * ed;
	effec_capture * ec;
	limb_data * ld;
	limb_capture * lc;
	trunk_data * td;
	trunk_capture * tc;
	hand_capture * hc;
	mxs_vector * dst;
	mxs_vector v1, v2;
	mxs_matrix m1, m2, uppertorso_offset, lowerab_direc;
	mxs_real r1, r2, r3, r4;
	int num_torsos = 0;
	int num_limbs = 0;
	int num_effectors = 0;
	mps_motion_node * node;

	int torsos[10];
	int limbs[10];
	int effectors[10];

	int num_frames;
	float frame_inc;

	mpm_motion * m;

	if (mp_list_count(&mp->main_motion))
	{
		//m = &mpd_motion_table[mp->motions[0].handle];
		node = (mps_motion_node *) mp_list_traverse(&mp->main_motion, NULL);
		m = &mpd_motion_table[node->handle];
		num_frames = m->info.num_frames;

 		frame_inc = ((float) m->info.frequency) * dt / 1000.0;
		node->frame += frame_inc;
		frame = (int) floor(node->frame + 0.5);

		if (frame < m->info.num_frames)
		{
			res_id = REFID(m->component->ref);
	
			Spew(MP_SRC, ("\nnew frame\n\n"));
	
		// Loop through component motions, finding torsos, limbs, and effectors.
	
			cm = m->component;
			for (i = 0; i < MAX_COMPONENTS; i++, cm++)
			{
				if ((cm->ctype > MPCT_NULL) && (cm->ctype <= MPCT_HAND))
				{
					res_index = REFINDEX(cm->ref);
	
					motion_data = (unsigned char *) RefLock(cm->ref);
	
					switch (cm->ctype)
					{
						case MPCT_TORSO:
							torsos[num_torsos++] = i;
							break;
	
						case MPCT_LIMB:
	
							limbs[num_limbs++] = i;
							break;
	
						case MPCT_EFFECTOR:
						case MPCT_HAND:
	
							effectors[num_effectors++] = i;
							break;
					}
	
	
					RefUnlock(cm->ref);
				}
			}
	
			for (i = 0; i < num_torsos; i++)
			{
				cm = m->component + torsos[i];
	
				motion_data = (unsigned char *) RefLock(cm->ref);
	
				//Spew(MP_SRC, ("torso\n"));
	
				td = (trunk_data *) motion_data;
				tc = (trunk_capture *) (motion_data + sizeof(trunk_data));
				tc += frame;
	
	  		// pelvis and torso orientations.
	  			mx_mul_mat(&m1, orient, &tc->base_orient);			 
				mx_copy_mat(&lowerab_direc, &m1);
	  			mx_copy_vec(&mp->norm[NORM_PELV], &m1.vec[1]);
	
				mx_copy_mat(&uppertorso_offset, &tc->ab_orient);
	  			mx_mul_mat(&m2, &m1, &tc->ab_orient);
	  			mx_copy_vec(&mp->norm[NORM_UPTORS], &m2.vec[1]);
	
			// butt position.
				mx_mat_mul_vec(&v1, orient, &tc->base);
	
				//mx_add_vec(&mp->joints[BUTT], pos, &v1);
				mp->joints[BUTT].x = node->base_pos.x + v1.x;
				mp->joints[BUTT].y = node->base_pos.y + v1.y;
				mp->joints[BUTT].z = v1.z;
	
				mx_copy_vec(pos, &mp->joints[BUTT]);
	
			// abdomen position.
				mx_unit_vec(&v1, 2);
				mx_scaleeq_vec(&v1, td->ab_per * td->spine_len);
				mx_mat_muleq_vec(&m1, &v1);
				mx_add_vec(&mp->joints[ABDOMEN], &mp->joints[BUTT], &v1);
	
			// neck.
	
				mx_unit_vec(&v1, 2);
				mx_scaleeq_vec(&v1, (1 - td->ab_per) * td->spine_len);
				mx_mat_muleq_vec(&m2, &v1);
				mx_add_vec(&mp->joints[NECK], &mp->joints[ABDOMEN], &v1);
	
			// shoulders.
	
				mx_copy_vec(&v1, &tc->lshldr);
				mx_mat_muleq_vec(&m2, &v1);
				mx_add_vec(&mp->joints[LSHLDR], &mp->joints[ABDOMEN], &v1);
				mx_copy_vec(&v1, &tc->rshldr);
				mx_mat_muleq_vec(&m2, &v1);
				mx_add_vec(&mp->joints[RSHLDR], &mp->joints[ABDOMEN], &v1);
	
			// hips.
	
				mx_unit_vec(&v1, 1);
				mx_scaleeq_vec(&v1, td->hip_width / 2);
				mx_mat_muleq_vec(&m1, &v1);
				mx_scale_vec(&v2, &v1, -1);
				mx_add_vec(&mp->joints[LHIP], &mp->joints[BUTT], &v1);
				mx_add_vec(&mp->joints[RHIP], &mp->joints[BUTT], &v2);
	
				RefUnlock(cm->ref);
			}
	
			for (i = 0; i < num_limbs; i++)
			{
				cm = m->component + limbs[i];
				motion_data = (unsigned char *) RefLock(cm->ref);
	
				ld = (limb_data *) motion_data;
				lc = (limb_capture *) (motion_data + sizeof(limb_data));
				lc += frame;
	
				//Spew(MP_SRC, ("limb: %s -> %s\n", Jnt[ld->base_joint_id], Jnt[ld->end_joint_id]));
			
				switch (ld->base_joint_id)
				{
					case LSHLDR:
						dst = mp->joints + LELBOW;
						mx_mul_mat(&m1, &lowerab_direc, &uppertorso_offset);
						norm_id = NORM_LARM;
						bend = 0;
						break;
	
					case RSHLDR:
						dst = mp->joints + RELBOW;
						mx_mul_mat(&m1, &lowerab_direc, &uppertorso_offset);
						norm_id = NORM_RARM;
						bend = 0;
						break;
	
					case LHIP:
						dst = mp->joints + LKNEE;
						mx_copy_mat(&m1, &lowerab_direc);
						norm_id = NORM_LLEG;
						bend = 1;
						break;
	
					case RHIP:
						dst = mp->joints + RKNEE;
						mx_copy_mat(&m1, &lowerab_direc);
						norm_id = NORM_RLEG;
						bend = 1;
						break;
				}
	
				mx_mat_mul_vec(&v1, &m1, &lc->e_data);
				mx_add_vec(&mp->joints[ld->end_joint_id], &mp->joints[ld->base_joint_id], &v1);
	
				mx_mat_mul_vec(&mp->norm[norm_id], &m1, &lc->n_data);
	
			// solve for joint position.
	
				mx_sub_vec(&v1, &mp->joints[ld->end_joint_id], &mp->joints[ld->base_joint_id]);
				r1 = mx_dot_vec(&v1, &v1);
				r2 = sqrt(r1);
	
				r3 = ld->len1 * ld->len1 - ld->len2 * ld->len2;
				r4 = ((r3/r1) + 1.0) / 2.0;
	
				dst->x = mp->joints[ld->base_joint_id].x + v1.x * r4;
				dst->y = mp->joints[ld->base_joint_id].y + v1.y * r4;
				dst->z = mp->joints[ld->base_joint_id].z + v1.z * r4;
	
				mx_cross_vec(&v2, &mp->norm[norm_id], &v1);
				r4 = (r3/r2 + r2) / 2.0;
				r4 = sqrt((ld->len1 * ld->len1) - (r4 * r4)) / r2;
				if (bend)
				{
					r4 = -r4;
				}
	
				dst->x += v2.x * r4;
				dst->y += v2.y * r4;
				dst->z += v2.z * r4;
			
				RefUnlock(cm->ref);
			}
	
			for (i = 0; i < num_effectors; i++)
			{
				cm = m->component + effectors[i];
				motion_data = (unsigned char *) RefLock(cm->ref);
	
				ed = (effec_data *) motion_data;
	
				if (cm->ctype == MPCT_HAND)
				{
					hc = (hand_capture *) (motion_data + sizeof(effec_data));
					hc += frame;
					ec = &hc->effec_table;
				}
				else
				{
					ec = (effec_capture *) (motion_data + sizeof(effec_data));
					ec += frame;
				}
	
				//Spew(MP_SRC, ("effector: %s -> %s\n", Jnt[ed->base_joint_id], Jnt[ed->eff_pos_id]));
	
				dst = mp->joints + ed->eff_pos_id;
	
				switch (ed->eff_pos_id)
				{
					case HEAD:
						norm_id = NORM_HEAD;
						break;
	
					case LFINGER:
						norm_id = NORM_LHAND;
						break;
	
					case RFINGER:
						norm_id = NORM_RHAND;
						break;
	
					case LTOE:
						norm_id = NORM_LLEG;
						break;
	
					case RTOE:
						norm_id = NORM_RLEG;
						break;
	
					default:
						Spew(MP_SRC, ("neglected effector\n"));
						break;
				}
				mx_copy_mat(&m1, &ec->e_data);
				mx_mul_mat(&m2, &uppertorso_offset, &m1);
				mx_mul_mat(&m1, &lowerab_direc, &m2);
				mx_copy_vec(&v1, &m1.vec[0]);
				mx_scaleeq_vec(&v1, ed->len1);
				mx_add_vec(&mp->joints[ed->eff_pos_id], &mp->joints[ed->base_joint_id], &v1);
				mx_copy_vec(&mp->norm[norm_id], &m1.vec[1]);
	
				if (norm_id == NORM_HEAD)
				{
					//Spew(MP_SRC, ("head normal: %f, %f, %f\n", fix_from_float(m1.vec[1].x), fix_from_float(m1.vec[1].y), fix_from_float(m1.vec[1].z)));
				}
	
				RefUnlock(cm->ref);
			}
		/*
			mp->motions[0].frame++;
			if (mp->motions[0].frame >= num_frames)
			{
				//mp->motion = -1;
				mp->num_active_motions = 0;
			}
		*/
		}
	}
}
#endif

//
/*
void mp_start_motion(multiped * mp, unsigned int motion_num, mxs_vector * start_pos, mxs_matrix * start_orient)
{
	mpm_motion * m = &mpd_qmotion_table[motion_num];

	//mx_mat2ang(start_orient, &m->info.start_orient);
//mx_copy_mat(start_orient, &m->info.start_orient);
	//mx_identity_mat(start_orient);

	mp->motion = motion_num;
	mp->frame = 0;
	mx_copy_vec(&mp->base_pos, start_pos);

	
}

//

void mp_stop_all_motions(multiped * mp)
{
	mp->motion = -1;
}
*/
//


