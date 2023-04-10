/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matvec.c $
 * $Revision: 1.3 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:02 $
 */

#include <matrixd.h>

#define MXD_VERSION

// element wise mul of m by v to make different coordinate
// system
// dest = M .* v
void mxd_mat_elmul_vec(mxds_matrix *dest,const mxds_matrix *m,const mxds_vector *v)
{
  dest->el[0] = m->el[0] * v->x;
  dest->el[1] = m->el[1] * v->x;
  dest->el[2] = m->el[2] * v->x;

  dest->el[3] = m->el[3] * v->y;
  dest->el[4] = m->el[4] * v->y;
  dest->el[5] = m->el[5] * v->y;

  dest->el[6] = m->el[6] * v->z;
  dest->el[7] = m->el[7] * v->z;
  dest->el[8] = m->el[8] * v->z;
}

// M .*= v
void mxd_mat_elmuleq_vec(mxds_matrix *m,const mxds_vector *v)
{
  m->el[0] *= v->x;
  m->el[1] *= v->x;
  m->el[2] *= v->x;

  m->el[3] *= v->y;
  m->el[4] *= v->y;
  m->el[5] *= v->y;

  m->el[6] *= v->z;
  m->el[7] *= v->z;
  m->el[8] *= v->z;
}

// dest = v^t .* M
void mxd_mat_eltmul_vec(mxds_matrix *dest,const mxds_matrix *m,const mxds_vector *v)
{
  dest->el[0] = m->el[0] * v->x;
  dest->el[3] = m->el[3] * v->x;
  dest->el[6] = m->el[6] * v->x;

  dest->el[1] = m->el[1] * v->y;
  dest->el[4] = m->el[4] * v->y;
  dest->el[7] = m->el[7] * v->y;

  dest->el[2] = m->el[2] * v->z;
  dest->el[5] = m->el[5] * v->z;
  dest->el[8] = m->el[8] * v->z;
}

// M .*= v^t
void mxd_mat_eltmuleq_vec(mxds_matrix *m,const mxds_vector *v)
{
  m->el[0] *= v->x;
  m->el[3] *= v->x;
  m->el[6] *= v->x;

  m->el[1] *= v->y;
  m->el[4] *= v->y;
  m->el[7] *= v->y;

  m->el[2] *= v->z;
  m->el[5] *= v->z;
  m->el[8] *= v->z;
}

// dest = M x v
void mxd_mat_mul_vec(mxds_vector *dest,const mxds_matrix *m,const mxds_vector *v)
{
   dest->x = m->el[0]*v->x + m->el[3]*v->y + m->el[6]*v->z;
   dest->y = m->el[1]*v->x + m->el[4]*v->y + m->el[7]*v->z;
   dest->z = m->el[2]*v->x + m->el[5]*v->y + m->el[8]*v->z;
}

// v x= M, this is idiotic
void mxd_mat_muleq_vec(const mxds_matrix *m,mxds_vector *v)
{
   mxds_vector t;
   mxd_mat_mul_vec(&t,m,v);
   mxd_copy_vec(v,&t);
}

// dest = M^t x v, this is for multing by inverse if unit
void mxd_mat_tmul_vec(mxds_vector *dest,const mxds_matrix *m, const mxds_vector *v)
{
   dest->x = m->el[0]*v->x + m->el[1]*v->y + m->el[2]*v->z;
   dest->y = m->el[3]*v->x + m->el[4]*v->y + m->el[5]*v->z;
   dest->z = m->el[6]*v->x + m->el[7]*v->y + m->el[8]*v->z;
}

// v x= M^t, retarded
void mxd_mat_tmuleq_vec(const mxds_matrix *m,mxds_vector *v)
{
   mxds_vector t;
   mxd_mat_tmul_vec(&t,m,v);
   mxd_copy_vec(v,&t);
}


