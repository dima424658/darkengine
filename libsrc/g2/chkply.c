/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/chkply.c $
 * $Revision: 1.3 $
 * $Author: KEVIN $
 * $Date: 1996/12/20 11:52:08 $
 *
 * Procedures to check polygon extent.
 *
 * This file is part of the g2 library.
 */

#include <polyint.h>

/* I suppose this might belong in an include file, */
/* but I can't imagine any other routines using it. */
extern int gri_check_edge_y (grs_vertex **p_new, grs_vertex **p_old, long *h_buf);
extern int gri_check_edge_y_update (grs_vertex **p_new, grs_vertex **p_old, long *h_buf);

/* checks polgon y_min as a function of x against h_buf and updates h_buf. */
/* returns 0 if h_buf unchanged; 1 otherwise. */
int gr_check_poly_y_min(int n, grs_vertex **vpl,long *h_buf)
{
   int x,x_min,x_max;
   int pix_drawn=0;
   grs_vertex **p_prev,**p_top, **p;

   poly_find_x_extrema_retval(x_min,x_max,p_top,vpl,n,pix_drawn);

   p_prev=p=p_top;
   x=x_min;
   while (x!=x_max) {
      int x_next=fix_cint((*p)->x);
      do {
         if (x_next==x) p_prev=p;
         if (--p < vpl) p=vpl+n-1;
      } while ((x_next=fix_cint((*p)->x))<=x);
      if (gri_check_edge_y(p,p_prev,h_buf))
         return 2;
      x=x_next;
   }
   x=x_min;
   p_prev=p=p_top;
   while (x!=x_max) {
      int x_next=fix_cint((*p)->x);
      do {
         if (x_next==x) p_prev=p;
         if (++p >= vpl+n) p=vpl;
      } while ((x_next=fix_cint((*p)->x))<=x);
      pix_drawn|=gri_check_edge_y_update(p,p_prev,h_buf);
      x=x_next;
   }
   return pix_drawn; //either 0 or 1
}


