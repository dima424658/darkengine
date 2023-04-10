/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/matmat.c $
 * $Revision: 1.7 $
 * $Author: kate $
 * $Date: 1997/11/20 13:31:53 $
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <matrix.h>

#define MX_VERSION

void mx_zero_mat(mxs_matrix *m)
{
   int i;
   for (i=0;i<9;++i) m->el[i] = 0;
}

// m = I
void mx_identity_mat(mxs_matrix *m)
{
   int i;
   for (i=1;i<8;++i) m->el[i] = 0;
   m->el[0] = 1.0;
   m->el[4] = 1.0;
   m->el[8] = 1.0;
}


// dest = src
void mx_copy_mat(mxs_matrix *dest,const mxs_matrix *src)
{
   memcpy(dest,src,sizeof(mxs_matrix));
}

// dest = m * s
void mx_scale_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_real s)
{
   int i;
   for (i=0;i<9;++i) dest->el[i] = m->el[i] * s;
}

// m *= s
void mx_scaleeq_mat(mxs_matrix *m,mxs_real s)
{
   int i;
   for (i=0;i<9;++i) m->el[i] *= s;
}

// dest = m/s
void mx_div_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_real s)
{
   int i;
   for (i=0;i<9;++i) dest->el[i] = m->el[i] / s;
}

// m /= s
void mx_diveq_mat(mxs_matrix *m,mxs_real s)
{
   int i;
   for (i=0;i<9;++i) m->el[i] /= s;
}

// return |m|
mxs_real mx_det_mat(const mxs_matrix *m)
{
   mxs_real d;
   d = m->el[0]*m->el[4]*m->el[8];
   d += m->el[3]*m->el[7]*m->el[2];
   d += m->el[6]*m->el[1]*m->el[5];

   d -= m->el[0]*m->el[7]*m->el[5];
   d -= m->el[3]*m->el[1]*m->el[8];
   d -= m->el[6]*m->el[4]*m->el[2];

   return d;
}

// dest = m^t
void mx_trans_mat(mxs_matrix *dest,const mxs_matrix *m)
{
   dest->el[0] = m->el[0];
   dest->el[1] = m->el[3];
   dest->el[2] = m->el[6];

   dest->el[3] = m->el[1];
   dest->el[4] = m->el[4];
   dest->el[5] = m->el[7];

   dest->el[6] = m->el[2];
   dest->el[7] = m->el[5];
   dest->el[8] = m->el[8];
}

// m = m^t
void mx_transeq_mat(mxs_matrix *m)
{
   mxs_real t;

   t = m->el[1];
   m->el[1] = m->el[3];
   m->el[3] = t;

   t = m->el[2];
   m->el[2] = m->el[6];
   m->el[6] = t;

   t = m->el[5];
   m->el[5] = m->el[7];
   m->el[7] = t;
}


// dest = m^-1, returns false if matrix degenerate
bool mx_inv_mat(mxs_matrix *dest,const mxs_matrix *m)
{
   int i,mi,j;
   mxs_real mv;
   mxs_matrix e;

   mx_identity_mat(&e);
   mx_copy_mat(dest,m);

   // Forward elimination
   // swap maximum to be pivot
   mi = 0;
   mv = fabs(dest->el[0]);
   if (fabs(dest->el[1]) > mv) {mi=1;mv=fabs(dest->el[1]);}
   if (fabs(dest->el[2]) > mv) mi=2;

 // swap max to pivot
   if (mi!=0) {
      mx_swaproweq_mat(&e,0,mi);
      mx_swaproweq_mat(dest,0,mi);
   }

   // eliminate rows 1 and 2
   for (j=1;j<3;++j) {
      mv = -dest->el[j]/dest->el[0];
      dest->el[0+j] = 0.0;
      dest->el[3+j] += mv*dest->el[3];
      dest->el[6+j] += mv*dest->el[6];
      for (i=0;i<9;i+=3) {
         e.el[i+j] += mv*e.el[i];
      }
   }

   // Forward elimination
   // swap maximum to be pivot
   if (fabs(dest->el[5]) > fabs(dest->el[4])) {
      mx_swaproweq_mat(&e,1,2);
      mx_swaproweq_mat(dest,1,2);
   }

   mv = -dest->el[5]/dest->el[4];
   dest->el[3+2] = 0.0;
   dest->el[6+2] += mv*dest->el[6+1];
   for (i=0;i<9;i+=3) {
      e.el[i+2] += mv*e.el[i+1];
   }

   // Now we've forward eliminated
   // backwards eliminate
   // eliminate rows 0 and 1
   for (j=0;j<2;++j) {
      mv = -dest->el[j+6]/dest->el[8];
      dest->el[j+6] = 0.0;
      for (i=0;i<9;i+=3) {
         e.el[i+j] += mv*e.el[i+2];
      }
   }

   // eliminate row 0
   mv = -dest->el[3]/dest->el[4];
   dest->el[3] = 0.0;
   for (i=0;i<9;i+=3) {
      e.el[i] += mv*e.el[i+1];
   }

   // divide by pivots
   for (j=0;j<3;++j) {
      for (i=0;i<9;i+=3) {
         e.el[i+j] /= dest->el[j*4];
      }
   }

   mx_copy_mat(dest,&e);

   return TRUE;
}

// m = m^-1, this is in place, so quite fast
bool mx_inveq_mat(mxs_matrix *m)
{
   bool yo;
   mxs_matrix t;
   yo = mx_inv_mat(&t,m);
   mx_copy_mat(m,&t);
   return yo;
}


// normalize each column of m
void mx_normcol_mat(mxs_matrix *dest,const mxs_matrix *m)
{
   mx_copy_mat(dest,m);
   mx_normcoleq_mat(dest);
}

void mx_normcoleq_mat(mxs_matrix *m)
{
   mx_normeq_vec((mxs_vector *)&(m->el[0]));
   mx_normeq_vec((mxs_vector *)&(m->el[3]));
   mx_normeq_vec((mxs_vector *)&(m->el[6]));
}

// normalize each row of m
void mx_normrow_mat(mxs_matrix *dest,const mxs_matrix *m)
{
   mx_copy_mat(dest,m);
   mx_normroweq_mat(dest);
}

void mx_normroweq_mat(mxs_matrix *m)
{
   mxs_real d;
   d = m->el[0]*m->el[0] + m->el[3]*m->el[3] + m->el[6]*m->el[6];
   d = sqrt(d);
   m->el[0] /= d;
   m->el[3] /= d;
   m->el[6] /= d;

   d = m->el[1]*m->el[1] + m->el[4]*m->el[4] + m->el[7]*m->el[7];
   d = sqrt(d);
   m->el[1] /= d;
   m->el[4] /= d;
   m->el[7] /= d;

   d = m->el[2]*m->el[2] + m->el[5]*m->el[5] + m->el[8]*m->el[8];
   d = sqrt(d);
   m->el[2] /= d;
   m->el[5] /= d;
   m->el[8] /= d;

}

// sanitize the matrix, make |m| = 1 no matter what,
// trying to preserve geometry
bool mx_sanitize_mat(mxs_matrix *dest,const mxs_matrix *m,mxs_real tol)
{
   mx_copy_mat(dest,m);
   return mx_sanitizeeq_mat(dest,tol);
}

bool mx_sanitizeeq_mat(mxs_matrix *m,mxs_real tol)
{

   mxs_real ubound = 1.0 + tol;
   mxs_real lbound = 1.0/(1.0 + tol);
   mxs_real orth[3];
   mxs_real mags[3];
   int i;
   bool prob = FALSE;

   for (i=0;i<3;++i) {
      mags[i] = mx_mag2_vec(&m->vec[i]);
      if (mags[i]>ubound || mags[i]<lbound) {
         prob = TRUE;
      }
      // % is dot prod, believe or not
      orth[i] = mx_dot_vec(&m->vec[i],&m->vec[(i+1)%3]);
      if (orth[i] > tol) {
         prob = TRUE;
      }
   }

   if (prob == FALSE) return FALSE;

   // catch zero or huge magnitudes
   for (i=0;i<3;++i) {
      if (mags[i] > 10 || mags[i] < .1) {
         mx_identity_mat(m);
         return TRUE;
      }
   }

   for (i=0;i<3;++i) {
      mx_diveq_vec(&m->vec[i],sqrt(mags[i]));
   }

   // this will usually fix it
   for (i=0;i<3;++i) {
      orth[i] = mx_dot_vec(&m->vec[i],&m->vec[(1+i)%3]);
      if (orth[i] > tol) {
         mx_identity_mat(m);
         return TRUE;
      }
   }

   return FALSE;
}

// dest = m1 x m2
void mx_mul_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2)
{
   dest->el[0] = m1->el[0]*m2->el[0] + m1->el[3]*m2->el[1] + m1->el[6]*m2->el[2];
   dest->el[1] = m1->el[1]*m2->el[0] + m1->el[4]*m2->el[1] + m1->el[7]*m2->el[2];
   dest->el[2] = m1->el[2]*m2->el[0] + m1->el[5]*m2->el[1] + m1->el[8]*m2->el[2];

   dest->el[3] = m1->el[0]*m2->el[3] + m1->el[3]*m2->el[4] + m1->el[6]*m2->el[5];
   dest->el[4] = m1->el[1]*m2->el[3] + m1->el[4]*m2->el[4] + m1->el[7]*m2->el[5];
   dest->el[5] = m1->el[2]*m2->el[3] + m1->el[5]*m2->el[4] + m1->el[8]*m2->el[5];

   dest->el[6] = m1->el[0]*m2->el[6] + m1->el[3]*m2->el[7] + m1->el[6]*m2->el[8];
   dest->el[7] = m1->el[1]*m2->el[6] + m1->el[4]*m2->el[7] + m1->el[7]*m2->el[8];
   dest->el[8] = m1->el[2]*m2->el[6] + m1->el[5]*m2->el[7] + m1->el[8]*m2->el[8];
}

// m1 = m1 x m2
void mx_muleq_mat(mxs_matrix *m1,const mxs_matrix *m2)
{
   mxs_matrix t;

   mx_mul_mat(&t,m1,m2);
   mx_copy_mat(m1,&t);
}

// dest = m1^t x m2
void mx_tmul_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2)
{
   dest->el[0] = m1->el[0]*m2->el[0] + m1->el[1]*m2->el[1] + m1->el[2]*m2->el[2];
   dest->el[1] = m1->el[3]*m2->el[0] + m1->el[4]*m2->el[1] + m1->el[5]*m2->el[2];
   dest->el[2] = m1->el[6]*m2->el[0] + m1->el[7]*m2->el[1] + m1->el[8]*m2->el[2];

   dest->el[3] = m1->el[0]*m2->el[3] + m1->el[1]*m2->el[4] + m1->el[2]*m2->el[5];
   dest->el[4] = m1->el[3]*m2->el[3] + m1->el[4]*m2->el[4] + m1->el[5]*m2->el[5];
   dest->el[5] = m1->el[6]*m2->el[3] + m1->el[7]*m2->el[4] + m1->el[8]*m2->el[5];

   dest->el[6] = m1->el[0]*m2->el[6] + m1->el[1]*m2->el[7] + m1->el[2]*m2->el[8];
   dest->el[7] = m1->el[3]*m2->el[6] + m1->el[4]*m2->el[7] + m1->el[5]*m2->el[8];
   dest->el[8] = m1->el[6]*m2->el[6] + m1->el[7]*m2->el[7] + m1->el[8]*m2->el[8];
}

// dest = m1 x m2^t
void mx_mult_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2)
{
   dest->el[0] = m1->el[0]*m2->el[0] + m1->el[3]*m2->el[3] + m1->el[6]*m2->el[6];
   dest->el[1] = m1->el[1]*m2->el[0] + m1->el[4]*m2->el[3] + m1->el[7]*m2->el[6];
   dest->el[2] = m1->el[2]*m2->el[0] + m1->el[5]*m2->el[3] + m1->el[8]*m2->el[6];

   dest->el[3] = m1->el[0]*m2->el[1] + m1->el[3]*m2->el[4] + m1->el[6]*m2->el[7];
   dest->el[4] = m1->el[1]*m2->el[1] + m1->el[4]*m2->el[4] + m1->el[7]*m2->el[7];
   dest->el[5] = m1->el[2]*m2->el[1] + m1->el[5]*m2->el[4] + m1->el[8]*m2->el[7];

   dest->el[6] = m1->el[0]*m2->el[2] + m1->el[3]*m2->el[5] + m1->el[6]*m2->el[8];
   dest->el[7] = m1->el[1]*m2->el[2] + m1->el[4]*m2->el[5] + m1->el[7]*m2->el[8];
   dest->el[8] = m1->el[2]*m2->el[2] + m1->el[5]*m2->el[5] + m1->el[8]*m2->el[8];
}


// m2 = m1^t x m2
// Note!  Assigns to m2, not m1
void mx_tmuleq_mat(const mxs_matrix *m1,mxs_matrix *m2)
{
   mxs_matrix t;

   mx_tmul_mat(&t,m1,m2);
   mx_copy_mat(m2,&t);
}


// dest = m1 .x m2
void mx_elmul_mat(mxs_matrix *dest,const mxs_matrix *m1,const mxs_matrix *m2)
{
   int i;
   for (i=0;i<9;++i) dest->el[i] = m1->el[i]*m2->el[i];
}

// m1 .x= m2
void mx_elmuleq_mat(mxs_matrix *m1,const mxs_matrix *m2)
{
   int i;
   for (i=0;i<9;++i) m1->el[i] *= m2->el[i];
}

// swap rows
void mx_swaprow_mat(mxs_matrix *dest,const mxs_matrix *m1,int n1,int n2)
{
   mx_copy_mat(dest,m1);
   mx_swaproweq_mat(dest,n1,n2);
}
void mx_swaproweq_mat(mxs_matrix *m1,int n1,int n2)
{
   mxs_real t;

   if (n1==n2) return;

   t = m1->el[n1];
   m1->el[n1] = m1->el[n2];
   m1->el[n2] = t;

   n1+=3;
   n2+=3;

   t = m1->el[n1];
   m1->el[n1] = m1->el[n2];
   m1->el[n2] = t;

   n1+=3;
   n2+=3;

   t = m1->el[n1];
   m1->el[n1] = m1->el[n2];
   m1->el[n2] = t;
}   

// swap cols
void mx_swapcol_mat(mxs_matrix *dest,const mxs_matrix *m1,int n1,int n2)
{
   mx_copy_mat(dest,m1);
   mx_swapcoleq_mat(dest,n1,n2);
}


void mx_swapcoleq_mat(mxs_matrix *m1,int n1,int n2)
{
   mxs_vector t;

   if (n1 == n2) return;

   mx_copy_vec(&t,&m1->vec[n1]);
   mx_copy_vec(&m1->vec[n1],&m1->vec[n2]);
   mx_copy_vec(&m1->vec[n2],&t);
}


void mx_prn_mat(const mxs_matrix *m)
{
   int j;

   for (j=0;j<3;++j) {
      printf("%f \t%f \t%f\n",m->el[j],m->el[j+3],m->el[j+6]);
   }
}
