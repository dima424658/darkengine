/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matvec.c $
 * $Revision: 1.3 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:02 $
 */

#include <matrix.h>

#define MX_VERSION

// element wise mul of m by v to make different coordinate
// system
// dest = M .* v
void mx_mat_elmul_vec(mxs_matrix *dest,const mxs_matrix *m,const mxs_vector *v)
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
void mx_mat_elmuleq_vec(mxs_matrix *m,const mxs_vector *v)
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
void mx_mat_eltmul_vec(mxs_matrix *dest,const mxs_matrix *m,const mxs_vector *v)
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
void mx_mat_eltmuleq_vec(mxs_matrix *m,const mxs_vector *v)
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
void mx_mat_mul_vec(mxs_vector *dest,const mxs_matrix *m,const mxs_vector *v)
{
   dest->x = m->el[0]*v->x + m->el[3]*v->y + m->el[6]*v->z;
   dest->y = m->el[1]*v->x + m->el[4]*v->y + m->el[7]*v->z;
   dest->z = m->el[2]*v->x + m->el[5]*v->y + m->el[8]*v->z;
}

// v x= M, this is idiotic
void mx_mat_muleq_vec(const mxs_matrix *m,mxs_vector *v)
{
   mxs_vector t;
   mx_mat_mul_vec(&t,m,v);
   mx_copy_vec(v,&t);
}

// dest = M^t x v, this is for multing by inverse if unit
void mx_mat_tmul_vec(mxs_vector *dest,const mxs_matrix *m, const mxs_vector *v)
{
   dest->x = m->el[0]*v->x + m->el[1]*v->y + m->el[2]*v->z;
   dest->y = m->el[3]*v->x + m->el[4]*v->y + m->el[5]*v->z;
   dest->z = m->el[6]*v->x + m->el[7]*v->y + m->el[8]*v->z;
}

// v x= M^t, retarded
void mx_mat_tmuleq_vec(const mxs_matrix *m,mxs_vector *v)
{
   mxs_vector t;
   mx_mat_tmul_vec(&t,m,v);
   mx_copy_vec(v,&t);
}


