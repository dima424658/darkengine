//
// abvm.c -- Sample virtual motions. 
//

#include <math.h>
#include <appbiped.h>

//

bool abp_head_track(multiped * mp, mps_motion_info * m, int frame, quat * rot,
					mxs_vector * trans)
{
	float yaw, pitch;
	mxs_vector d, v1, v2;
	mxs_matrix m1, m2, m3;
	appbiped * ab = (appbiped *) mp->app_ptr;
	bool result = TRUE;

	mx_sub_vec(&d, &ab->target, &ab->joints[HEAD]);

// Get yaw.
	mx_copy_vec(&v1, &d);
	v1.z = 0;
	mx_normeq_vec(&v1);
	mx_unit_vec(&v2, 0);
	yaw = acos(mx_dot_vec(&v1, &v2));
	if (v1.y < 0)
	{
		yaw = -yaw;
	}

// Get pitch.
	mx_norm_vec(&v1, &d);
	mx_copy_vec(&v2, &v1);
	v2.z = 0;
	mx_normeq_vec(&v2);
	pitch = acos(mx_dot_vec(&v1, &v2));
	if (v1.z > 0)
	{
		pitch = -pitch;
	}

	mx_mk_rot_z_mat(&m1, mx_rad2ang(yaw));
	mx_rot_y_mat(&m3, &m1, mx_rad2ang(pitch));
	
// Through a strange combination of circumstances, the rotation matrix for
// for the neck is such that +X points toward the middle of the head, +Y 
// points out the left ear, and -Z points out the nose.
	mx_copy_vec(&m1.vec[0], &m3.vec[2]);
	mx_copy_vec(&m1.vec[1], &m3.vec[1]);
	mx_neg_vec(&m1.vec[2], &m3.vec[0]);

	mx_inv_mat(&m2, &ab->orients[ABDOMEN]);
	mx_mul_mat(&m3, &m2, &m1);

	quat_from_matrix(&rot[NECK], &m3);

	return result;
}

//

