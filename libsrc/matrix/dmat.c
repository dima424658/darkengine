/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/dmat.c $
 * $Revision: 1.1 $
 * $Author: JAEMZ $
 * $Date: 1997/03/31 00:06:21 $
 *
 * Double matrix implementation
 * 
 */

#include <string.h>
#include <math.h>

#include <dmat.h>

#include <lg.h>

// Returns zerod matrix
dms_matrix *dm_alloc(int col,int row)
{
   dms_matrix *d;

   AssertMsg1(col>0,"Column size is %d",col);
   AssertMsg1(row>0,"Row size is %d",row);

   d = (dms_matrix *)Malloc(sizeof(dms_matrix)+col*row*sizeof(double));
   d->col = col;
   d->row = row;

   return d;
}

// Creates a matrix from float array, defined column wise
dms_matrix *dm_float_create(int col,int row,float *f)
{
   dms_matrix *m;
   int i=col*row;

   m = dm_alloc(col,row);

   while(i) {
      --i;
      m->el[i]=f[i];
   }
   return m;
}

// Creates a matrix from double array, defined column wise
dms_matrix *dm_double_create(int col,int row,double *d)
{
   dms_matrix *m;

   m = dm_alloc(col,row);

   memcpy(m->el,d,sizeof(double)*row*col);
   return m;
}

dms_matrix *dm_zero(int col,int row)
{
   dms_matrix *d;

   d = dm_alloc(col,row);
   memset(d->el,0,sizeof(double)*col*row);
   return d;
}



dms_matrix *dm_identity(int col,int row)
{
   dms_matrix *d;
   int i;

   d = dm_zero(col,row);

   for (i=0;i<row && i<col;++i) {
      DM_EL(d,i,i) = 1.0;
   }
   return d;
}


void dm_free(dms_matrix *m)
{
   Free(m);
}


// m^t
dms_matrix *dm_transpose(dms_matrix *m)
{
   dms_matrix *d;
   int i,j;

   d = dm_alloc(m->row,m->col);

   for (i=0;i<m->col;++i) {
      for (j=0;j<m->row;++j) {
         DM_EL(d,j,i) = DM_EL(m,i,j);
      }
   }
   return d;
}

// d = m1 * m2;
dms_matrix *dm_multiply(dms_matrix *m1,dms_matrix *m2)
{
   int i,j,k;
   dms_matrix *d;
   double *d1,*d2,*d3;

   AssertMsg2(m1->col == m2->row,"m1 has %d cols, m2 has %d rows\n",m1->col,m2->row);

   d = dm_alloc(m2->col,m1->row);

   d2 = m2->el;
   d3 = d->el;

   for (i=0;i<d->col;++i) {
      d1 = m1->el;
      for (j=0;j<d->row;++j) {
         double a = 0;
         double *dd1=d1;
         double *dd2=d2;
         for (k=0;k<m2->row;++k) {
            a += *dd1 * *dd2;
            dd2++;
            dd1 += m1->row;
         }
         *d3 = a;
         d3++;
         d1++;
      }
      d2 = d2+m2->row;
   }

   return d;
}

// Swaps two rows in place
void dm_swaprow(dms_matrix *m,int r1,int r2)
{
   double tmp;
   int i;

   if (r1==r2) return;

   for (i=0;i<m->col;++i) {
      tmp = DM_EL(m,i,r1);
      DM_EL(m,i,r1) = DM_EL(m,i,r2);
      DM_EL(m,i,r2) = tmp;
   }
}

static double col_max(double *d,int num,int *mret)
{
   int mi;
   int j;
   double mv;
   double tmp;

   mi = 0;
   mv = fabs(d[0]);

   for (j=1;j<num;++j) {
      tmp = fabs(d[j]);
      if (tmp > mv) {
         mi = j;
         mv = tmp;
      }
   }

   *mret = mi;
   return mv;
}



// d = m^-1
dms_matrix *dm_inverse(dms_matrix *m1)
{
   int i,mi,j,piv;
   double mv;
   dms_matrix *e;
   dms_matrix *dest;
   double *col;
   double rat;

   // this is actual dest
   e = dm_identity(m1->col,m1->row);

   dest = dm_double_create(m1->col,m1->row,m1->el);

   for (piv = 0;piv < dest->row-1;++piv) {

      // Forward elimination
      // swap maximum to be pivot
      col = DM_COL(dest,piv) + piv;
      mv = col_max(col,dest->row - piv,&mi);
      mi += piv;

      dm_swaprow(dest,piv,mi);
      dm_swaprow(e,piv,mi);

      if (mv==0) {
         AssertMsg(mv!=0,"Pivot zero, no solution\n");
         dm_print(dest);
      }

      // eliminate rows beneath it
      for (j=piv+1;j<dest->row;++j) {

         rat = -DM_EL(dest,piv,j)/mv;

         // zero first one
         DM_EL(dest,piv,j) = 0.0;
         // go through the other columns
         for (i=piv+1;i<dest->col;++i) {
            DM_EL(dest,i,j) += rat*DM_EL(dest,i,piv);
         }
         for (i=0;i<dest->col;++i) {
            DM_EL(e,i,j) += rat*DM_EL(e,i,piv);
         }
      }
   }



   // Now we've forward eliminated
   // backwards eliminate
   // eliminate rows 0 and 1 from row 2
   for (piv=dest->row-1;piv>0;--piv) {

      // go through the rows
      for (j=0;j<piv;++j) {
                     
         mv = -DM_EL(dest,piv,j)/DM_EL(dest,piv,piv);

         // set pivot column to zero
         DM_EL(dest,piv,j) = 0.0;
         for (i=0;i<dest->col;i++) {
            DM_EL(e,i,j) += mv * DM_EL(e,i,piv);
         }


      }
   }

   // divide by pivots
   for (j=0;j<dest->row;++j) {
      for (i=0;i<dest->col;++i) {
         DM_EL(e,i,j) /= DM_EL(dest,j,j);
      }
   }

   dm_free(dest);

   return e;
}

// Outputs a formatted array
void dm_output(FILE *f,dms_matrix *m)
{
   int i,j;

   fprintf(f,"%d x %d\n",m->col,m->row);

   for (j=0;j<m->row;++j) {
      for (i=0;i<m->col;++i) {
         fprintf(f,"%f \t",DM_EL(m,i,j));
      }
      fprintf(f,"\n");
   }
}

