
#ifndef __QT_H
#define __QT_H

//

#include <matrix.h>

// Quaternion structure. Access as (w, x, y, z), (s, v), or el[4].
//
typedef struct quat
{
	union 
	{
		struct
		{
			mxs_real	s;
			mxs_vector	v;
		};
		struct 
		{
			mxs_real w, x, y, z;
		};

		mxs_real el[4];
	};
} quat;

// Get a rotation matrix representation of the quaternion.
EXTERN void quat_to_matrix(mxs_matrix * m, quat * q);

// Given a rotation matrix, create an equivalent quaternion.
EXTERN void quat_from_matrix(quat * q, mxs_matrix * m);

// Given axis and angle, create quaternion.
EXTERN void quat_create(quat * q, mxs_vector * axis, float angle);

// dest = p * q.
// Remember, when concatenating quaternions, p * q means apply q first, then 
// p.
EXTERN void quat_mul(quat * dest, quat * p, quat * q);

// 'Scale' is misleading, this interpolates between an identity quaternion
// and the given quaternion, using the given scale as the interpolation
// parameter.
EXTERN void quat_scale(quat * dest, quat * q, float scale);

// Interpolates between q1 and q2.
EXTERN void quat_slerp(quat * dest, quat * q1, quat * q2, float t);

// *dst = *src.
EXTERN void quat_copy(quat * dest, quat * src);

EXTERN void quat_identity(quat * q);

// For debug purposes. A quaternion that represents a 3D rotation always
// has unit magnitude.
EXTERN float quat_mag(quat * q);

#endif

