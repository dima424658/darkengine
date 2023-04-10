//
// qt.c -- Quaternion functions.
//

#include <dbg.h>
#include <math.h>
#include <qt.h>

//

//#define QT_DEBUG

#ifdef QT_DEBUG

int DBG_flag = 0;
#define TOLERANCE 0.00001

int TracePositive;

#endif

//

//
// We need to be able to access matrix elements by (row, column),
// which mxs_matrix doesn't have.
//
typedef struct q_mat
{
	union
	{
		mxs_matrix	m;
		mxs_real	mat[3][3];
	};
} q_mat;

//

void quat_to_matrix(mxs_matrix * m, quat * q)
{
	float s, xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;
	q_mat * qm = (q_mat *) m;

#ifdef QT_DEBUG
	quat q2;
	float r1, r2;
#endif

	//s = 2.0 / (q->w*q->w + q->x*q->x + q->y*q->y + q->z*q->z);
// Magnitude should always be 1, so forget the division.
	s = 2.0;

	xs = q->x * s;
	ys = q->y * s;
	zs = q->z * s;

	wx = q->w * xs;
	wy = q->w * ys;
	wz = q->w * zs;

	xx = q->x * xs;
	xy = q->x * ys;
	xz = q->x * zs;
	yy = q->y * ys;
	yz = q->y * zs;
	zz = q->z * zs;

	qm->mat[0][0] = 1.0 - (yy + zz);
	qm->mat[1][0] = xy + wz;
	qm->mat[2][0] = xz - wy;

	qm->mat[0][1] = xy - wz;
	qm->mat[1][1] = 1.0 - (xx + zz);
	qm->mat[2][1] = yz + wx;

	qm->mat[0][2] = xz + wy;
	qm->mat[1][2] = yz - wx;
	qm->mat[2][2] = 1.0 - (xx + yy);

#ifdef QT_DEBUG

	if (!DBG_flag)		// DBG_flag is for re-entrancy check.
	{
		r1 = quat_mag(q);
		r2 = mx_det_mat(m);
		if (fabs(r1 - r2) > TOLERANCE)
		{
			printf("quat_to_matrix(): Magnitude error\n");
		}

		DBG_flag = 1;
		quat_from_matrix(&q2, m);
	
		if ((fabs(q->w - q2.w) > TOLERANCE) ||
			(fabs(q->x - q2.x) > TOLERANCE) ||
			(fabs(q->y - q2.y) > TOLERANCE) ||
			(fabs(q->z - q2.z) > TOLERANCE))
		{
			//Spew(MP_SRC, ("quat_to_matrix() error\n"));
			printf("quat_to_matrix() error (%d)\n", TracePositive);
		}
	
		DBG_flag = 0;
	}
#endif
}

//

static int nxt[3] = {1, 2, 0};

//

void quat_from_matrix(quat * q, mxs_matrix * m)
{
	float trace, s;
	int i, j, k;
	q_mat * qm = (q_mat *) m;
#ifdef QT_DEBUG
	mxs_matrix m2;
#endif

	trace = qm->mat[0][0] + qm->mat[1][1] + qm->mat[2][2];
	if (trace > 0)
	{
		s = sqrt(trace + 1.0);
		q->w = s * 0.5;
		s = 0.5 / s;

		q->x = (qm->mat[2][1] - qm->mat[1][2]) * s;
		q->y = (qm->mat[0][2] - qm->mat[2][0]) * s;
		q->z = (qm->mat[1][0] - qm->mat[0][1]) * s;

#ifdef QT_DEBUG
		TracePositive = 1;
#endif
	}
	else
	{
		i = 0;
		if (qm->mat[1][1] > qm->mat[0][0])
		{
			i = 1;
		}
		if (qm->mat[2][2] > qm->mat[i][i])
		{
			i = 2;
		}

		j = nxt[i];
		k = nxt[j];

		s = sqrt( (qm->mat[i][i] - (qm->mat[j][j]+qm->mat[k][k])) + 1.0);

		q->el[i+1] = s * 0.5;
		s = 0.5 / s;
		q->w = (qm->mat[k][j] - qm->mat[j][k]) * s;
		q->el[j+1] = (qm->mat[j][i] + qm->mat[i][j]) * s;
		q->el[k+1] = (qm->mat[k][i] + qm->mat[i][k]) * s;

#ifdef QT_DEBUG
		TracePositive = 0;
#endif
	}

#ifdef QT_DEBUG
	if (!DBG_flag)
	{
		DBG_flag = 1;
		quat_to_matrix(&m2, q);
	
		for (i = 0; i < 9; i++)
		{
			if (m->el[i] != m2.el[i])
			{
				//Spew(MP_SRC, ("quat_from_matrix() error\n"));
				printf("quat_from_matrix() error\n");
				break;
			}
		}
	
		DBG_flag = 0;
	}
#endif
}

//

void quat_create(quat * q, mxs_vector * axis, float angle)
{
	float sine;
	float half_angle = angle / 2.0;

	q->s = cos(half_angle);

	mx_norm_vec(&q->v, axis);
	sine = sin(half_angle);
	mx_scaleeq_vec(&q->v, sine);
}

//

void quat_mul(quat * dest, quat * p, quat * q)
{
	mxs_vector v1, v2, v3;

	dest->s = p->s * q->s - mx_dot_vec(&p->v, &q->v);
	
	mx_scale_vec(&v1, &q->v, p->s);
	mx_scale_vec(&v2, &p->v, q->s);
	mx_cross_vec(&v3, &p->v, &q->v);

	mx_add_vec(&dest->v, &v1, &v2);
	mx_addeq_vec(&dest->v, &v3);
}

//

static quat IdentityQuat = {1, 0, 0, 0};

//

//
// Scaling in this case doesn't mean scaling each element of the
// quaternion, but blending with the identity quaternion.
//
//
void quat_scale(quat * dest, quat * q, float scale)
{
	quat_slerp(dest, &IdentityQuat, q, scale);
}

//

#define EPSILON	0.00001

//
// Do the spherical linear interpolation thing.
//
#define MX_REAL_HALF_PI	(MX_REAL_PI / 2.0)

void quat_slerp(quat * dest, quat * q1, quat * q2, float t)
{
	float cos_omega, omega, sin_omega, s1, s2;
	quat sum, dif, qs;

// Make sure we're taking the shorter arc.

	sum.w = q1->w + q2->w;
	sum.x = q1->x + q2->x;
	sum.y = q1->y + q2->y;
	sum.z = q1->z + q2->z;

	dif.w = q1->w - q2->w;
	dif.x = q1->x - q2->x;
	dif.y = q1->y - q2->y;
	dif.z = q1->z - q2->z;

	s1 = dif.w * dif.w + dif.x * dif.x + dif.y * dif.y + dif.z * dif.z;
	s2 = sum.w * sum.w + sum.x * sum.x + sum.y * sum.y + sum.z * sum.z;

	if (s1 > s2)
	{
		qs.w = -q2->w;
		qs.x = -q2->x;
		qs.y = -q2->y;
		qs.z = -q2->z;
	}
	else
	{
		quat_copy(&qs, q2);
	}

	cos_omega = q1->w * qs.w + q1->x * qs.x + q1->y * qs.y + q1->z * qs.z;

// Check for cases where rotations are nearly opposite, which causes numerical
// weirdness.
	if ((1.0 + cos_omega) > EPSILON)
	{
	// Check for case where rotations are very close, which also causes 
	// weirdness. 
		if ((1.0 - cos_omega) > EPSILON)
		{
			omega = acos(cos_omega);
			sin_omega = sin(omega);
	
			s1 = sin((1.0 - t) * omega) / sin_omega;
			s2 = sin(t * omega) / sin_omega;
		}
		else
		{
		// Too close, do plain old linear interpolation.
			s1 = 1.0 - t;
			s2 = t;
		}

		dest->w = s1 * q1->w + s2 * qs.w;
		dest->x = s1 * q1->x + s2 * qs.x;
		dest->y = s1 * q1->y + s2 * qs.y;
		dest->z = s1 * q1->z + s2 * qs.z;
	}
	else
	{
		dest->w = q1->z;
		dest->x = -q1->y;
		dest->y = q1->x;
		dest->z = -q1->w;

		s1 = sin((1.0 - t) * MX_REAL_HALF_PI);
		s2 = sin(t * MX_REAL_HALF_PI);
//#define MP_SRC	DBGSRC(31, 1)
		dest->x = s1 * q1->x + s2 * dest->x;
		dest->y = s1 * q1->y + s2 * dest->y;
		dest->z = s1 * q1->z + s2 * dest->z;
//		Spew(MP_SRC, ("using reverse quat interpolation.\n"));
	}

}

//

void quat_copy(quat * dst, quat * src)
{
	dst->w = src->w;
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}

//

void quat_identity(quat * q)
{
	q->w = 1;
	q->x = 0;
	q->y = 0;
	q->z = 0;
}

//

float quat_mag(quat * q)
{
	float s = q->w * q->w + mx_dot_vec(&q->v, &q->v);
	return sqrt(s);
}

//
