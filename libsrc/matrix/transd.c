/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/trans.c $
 * $Revision: 1.6 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:08 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrixd.h>

#define MXD_VERSION

// so that MXD_VERSION will use the C version, MX_VERSION the ASM
#ifdef MX_VERSION
#define USE_ASM
#endif

// identity transform
void mxd_identity_trans(mxds_trans *t)
{
   mxd_zero_vec(&t->vec);
   mxd_identity_mat(&t->mat);
}

// copy transform
void mxd_copy_trans(mxds_trans *dest,const mxds_trans *src)
{
   memcpy(dest,src,sizeof(mxds_trans));
}


// invert transform that is unitary, dest != src
void mxd_transpose_trans(mxds_trans *dest,const mxds_trans *src)
{
   // transpose matrix
   mxd_trans_mat(&dest->mat,&src->mat);

   // m' = -inv(M)m
   mxd_mat_mul_vec(&dest->vec,&dest->mat,&src->vec);
   mxd_negeq_vec(&dest->vec);
}   


// multiply 2 transforms, like pushing t2 onto a tform stack
// dest = t1 * t2
void mxd_mul_trans(mxds_trans *dest,const mxds_trans *t1,const mxds_trans *t2)
{
   // dest->mat = t1->mat * t2->mat
   mxd_mul_mat(&dest->mat,&t1->mat,&t2->mat);

   // M1m2 + m1
   mxd_mat_mul_vec(&dest->vec,&t1->mat,&t2->vec);
   mxd_addeq_vec(&dest->vec,&t1->vec);
}

// transpose pre multiply 2 transforms
// for instance, putting view onto a stack
// dest = t1^t * t2
void mxd_tmul_trans(mxds_trans *dest,const mxds_trans *t1,const mxds_trans *t2)
{
   mxds_vector tmp;

   // M = M1^t * M2
   mxd_tmul_mat(&dest->mat,&t1->mat,&t2->mat);

   // m = M1^t(m2-m1)
   mxd_sub_vec(&tmp,&t2->vec,&t1->vec);
   mxd_mat_tmul_vec(&dest->vec,&t1->mat,&tmp);
}


// transpose post multiply 2 transforms
// dest = t1 * t2^t
void mxd_mult_trans(mxds_trans *dest,const mxds_trans *t1,const mxds_trans *t2)
{
   mxds_vector tmp;

   // D = T1 * T2^t
   mxd_mult_mat(&dest->mat,&t1->mat,&t2->mat);

   // d = t1 - Dt2
   mxd_mat_mul_vec(&tmp,&dest->mat,&t2->vec);
   mxd_sub_vec(&dest->vec,&t1->vec,&tmp);
}

#ifndef USE_ASM
// multiply tform by vector
// dest = t * v
void mxd_trans_mul_vec(mxds_vector *dest,const mxds_trans *t,const mxds_vector *v)
{
   // M1v + m1
   mxd_mat_mul_vec(dest,&t->mat,v);
   mxd_addeq_vec(dest,&t->vec);
}
#endif
                             
// inverse multiply tform by vector
// dest = t^t * v
void mxd_trans_tmul_vec(mxds_vector *dest,const mxds_trans *t,const mxds_vector *v)
{
   mxds_vector tmp;
   mxd_sub_vec(&tmp,v,&t->vec);
   mxd_mat_tmul_vec(dest,&t->mat,&tmp);
}

// print one out
void mxd_prn_trans(const mxds_trans *t)
{
   int j;
   const mxds_matrix *m=&t->mat;
   const mxds_vector *v=&t->vec;

   for (j=0;j<3;++j) {
      printf("%f \t%f \t%f \t: %f\n",m->el[j],m->el[j+3],m->el[j+6],v->el[j]);
   }
}
