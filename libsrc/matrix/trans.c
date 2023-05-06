/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/trans.c $
 * $Revision: 1.6 $
 * $Author: kate $
 * $Date: 1997/11/20 13:32:08 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrix.h>

// #define MX_VERSION

// so that MXD_VERSION will use the C version, MX_VERSION the ASM
#ifdef MX_VERSION
#define USE_ASM
#endif

// identity transform
void mx_identity_trans(mxs_trans *t)
{
   mx_zero_vec(&t->vec);
   mx_identity_mat(&t->mat);
}

// copy transform
void mx_copy_trans(mxs_trans *dest,const mxs_trans *src)
{
   memcpy(dest,src,sizeof(mxs_trans));
}


// invert transform that is unitary, dest != src
void mx_transpose_trans(mxs_trans *dest,const mxs_trans *src)
{
   // transpose matrix
   mx_trans_mat(&dest->mat,&src->mat);

   // m' = -inv(M)m
   mx_mat_mul_vec(&dest->vec,&dest->mat,&src->vec);
   mx_negeq_vec(&dest->vec);
}   


// multiply 2 transforms, like pushing t2 onto a tform stack
// dest = t1 * t2
void mx_mul_trans(mxs_trans *dest,const mxs_trans *t1,const mxs_trans *t2)
{
   // dest->mat = t1->mat * t2->mat
   mx_mul_mat(&dest->mat,&t1->mat,&t2->mat);

   // M1m2 + m1
   mx_mat_mul_vec(&dest->vec,&t1->mat,&t2->vec);
   mx_addeq_vec(&dest->vec,&t1->vec);
}

// transpose pre multiply 2 transforms
// for instance, putting view onto a stack
// dest = t1^t * t2
void mx_tmul_trans(mxs_trans *dest,const mxs_trans *t1,const mxs_trans *t2)
{
   mxs_vector tmp;

   // M = M1^t * M2
   mx_tmul_mat(&dest->mat,&t1->mat,&t2->mat);

   // m = M1^t(m2-m1)
   mx_sub_vec(&tmp,&t2->vec,&t1->vec);
   mx_mat_tmul_vec(&dest->vec,&t1->mat,&tmp);
}


// transpose post multiply 2 transforms
// dest = t1 * t2^t
void mx_mult_trans(mxs_trans *dest,const mxs_trans *t1,const mxs_trans *t2)
{
   mxs_vector tmp;

   // D = T1 * T2^t
   mx_mult_mat(&dest->mat,&t1->mat,&t2->mat);

   // d = t1 - Dt2
   mx_mat_mul_vec(&tmp,&dest->mat,&t2->vec);
   mx_sub_vec(&dest->vec,&t1->vec,&tmp);
}

#ifndef USE_ASM
// multiply tform by vector
// dest = t * v
void mx_trans_mul_vec(mxs_vector *dest,const mxs_trans *t,const mxs_vector *v)
{
   // M1v + m1
   mx_mat_mul_vec(dest,&t->mat,v);
   mx_addeq_vec(dest,&t->vec);
}
#endif
                             
// inverse multiply tform by vector
// dest = t^t * v
void mx_trans_tmul_vec(mxs_vector *dest,const mxs_trans *t,const mxs_vector *v)
{
   mxs_vector tmp;
   mx_sub_vec(&tmp,v,&t->vec);
   mx_mat_tmul_vec(dest,&t->mat,&tmp);
}

// print one out
void mx_prn_trans(const mxs_trans *t)
{
   int j;
   const mxs_matrix *m=&t->mat;
   const mxs_vector *v=&t->vec;

   for (j=0;j<3;++j) {
      printf("%f \t%f \t%f \t: %f\n",m->el[j],m->el[j+3],m->el[j+6],v->el[j]);
   }
}
