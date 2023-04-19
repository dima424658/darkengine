//
// abmp.c -- Stuff that deals with the inner workings of the multiped library,
// but needs to know more about multiped layout, etc.
//

#include <appbiped.h>
#include <math.h>


void abp_compute_arm_normal(mxs_vector* n, multiped* mp, limb* l, mxs_vector* end_pos);
void abp_compute_leg_normal(mxs_vector* n, multiped* mp, limb* l, mxs_vector* end_pos);

//

void mp_mirror_motion(quat * rot, mxs_vector * xlat, uint sig)
{
	int i;
	quat temp, * q = rot;

// Go through and reflect each quaternion about the x-axis and reverse angles.
	for (i = 0; i < NUM_JOINTS; i++, q++)
	{
		if (QuerySigJoint(sig, i))
		{
			q->w = -q->w;
			q->y = -q->y;
		}
	}

// Now we need to swap left and right limbs.

	if (QuerySigJoint(sig, LSHLDR) && QuerySigJoint(sig, RSHLDR))
	{
		quat_copy(&temp, &rot[LSHLDR]);
		quat_copy(&rot[LSHLDR], &rot[RSHLDR]);
		quat_copy(&rot[RSHLDR], &temp);
	}

	if (QuerySigJoint(sig, LELBOW) && QuerySigJoint(sig, RELBOW))
	{
		quat_copy(&temp, &rot[LELBOW]);
		quat_copy(&rot[LELBOW], &rot[RELBOW]);
		quat_copy(&rot[RELBOW], &temp);
	}

	if (QuerySigJoint(sig, LWRIST) && QuerySigJoint(sig, RWRIST))
	{
		quat_copy(&temp, &rot[LWRIST]);
		quat_copy(&rot[LWRIST], &rot[RWRIST]);
		quat_copy(&rot[RWRIST], &temp);
	}

	if (QuerySigJoint(sig, LHIP) && QuerySigJoint(sig, RHIP))
	{
		quat_copy(&temp, &rot[LHIP]);
		quat_copy(&rot[LHIP], &rot[RHIP]);
		quat_copy(&rot[RHIP], &temp);
	}

	if (QuerySigJoint(sig, LKNEE) && QuerySigJoint(sig, RKNEE))
	{
		quat_copy(&temp, &rot[LKNEE]);
		quat_copy(&rot[LKNEE], &rot[RKNEE]);
		quat_copy(&rot[RKNEE], &temp);
	}

	if (QuerySigJoint(sig, LANKLE) && QuerySigJoint(sig, RANKLE))
	{
		quat_copy(&temp, &rot[LANKLE]);
		quat_copy(&rot[LANKLE], &rot[RANKLE]);
		quat_copy(&rot[RANKLE], &temp);
	}

// Reflect translation about the x-axis.
	xlat->y = -xlat->y;
}

//

void abp_compute_limb_normal(mxs_vector * normal, multiped * mp, limb * l, mxs_vector * end_pos)
{
	switch (l->joint_id[0])
	{
		case LSHLDR:
		case RSHLDR:
			abp_compute_arm_normal(normal, mp, l, end_pos);
			break;

		case LHIP:
		case RHIP:
			abp_compute_leg_normal(normal, mp, l, end_pos);
			break;
	}
}

//

#define EPSILON 0.001

void abp_compute_arm_normal(mxs_vector * n, multiped * mp, limb * l, mxs_vector * end_pos)
{
	quat q;
	mxs_matrix m;
	mxs_vector across, n1, n2, prj;
	float t, angle;

// For any wrist position, there's about a 90-degree range of elbow 
// orientations. We choose one somewhere in the middle.

	mx_norm_vec(&prj, end_pos);

	if (l->joint_id[0] == LSHLDR)
	{
		mx_sub_vec(&across, &mp->joints[RSHLDR], &mp->joints[LSHLDR]);
		mx_normeq_vec(&across);
		mx_sub_vec(&n2, &mp->joints[NECK], &mp->joints[BUTT]);
		mx_normeq_vec(&n2);
		mx_cross_norm_vec(&n1, &n2, &prj);
	}
	else
	{

		mx_sub_vec(&across, &mp->joints[LSHLDR], &mp->joints[RSHLDR]);
		mx_normeq_vec(&across);
		mx_sub_vec(&n2, &mp->joints[BUTT], &mp->joints[NECK]);
		mx_normeq_vec(&n2);
		mx_cross_norm_vec(&n1, &prj, &n2);
	}


// Desired normal is between n1 and n2. The further you reach across your body,
// e.g. the left hand near the right shoulder, the more "up" the normal gets. 
// The further you reach out to the side and behind you, the more "across" the
// normal gets. Use angle between "across" and "end_pos" to determine where to 
// interpolate between n1 and n2.
//
// Obviously this is not physically/physiologically accurate, but it seems to
// generate reasonable normals.

// Compute interpolation parameter. We've defined things such that end_pos is
// in the plane defined by "across" and "out".
	angle = acos(mx_dot_vec(&prj, &across));

// Get normal.
	if (l->joint_id[0] == LSHLDR)
	{
		t = angle / 2.0;
		quat_create(&q, end_pos, t);
		quat_to_matrix(&m, &q);
		mx_mat_mul_vec(n, &m, &n2);
	}
	else
	{
		t = (MX_REAL_PI - angle) / 2.0;
		quat_create(&q, end_pos, t);
		quat_to_matrix(&m, &q);
		mx_mat_mul_vec(n, &m, &n1);
	}
}

//

void abp_compute_leg_normal(mxs_vector * n, multiped * mp, limb * l, mxs_vector * end_pos)
{
	mxs_vector v1, v2;

	mx_sub_vec(&v1, &mp->joints[RHIP], &mp->joints[LHIP]);
	mx_cross_norm_vec(&v2, &v1, end_pos);
	mx_cross_norm_vec(n, &v2, end_pos);
}

//


