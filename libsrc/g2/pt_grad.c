// $Header: x:/prj/tech/libsrc/g2/RCS/pt_grad.c 1.3 1997/11/21 14:00:56 KEVIN Exp $

#include <g2spoint.h>
#include <tmapd.h>

extern double g2pt_tmap_data[9];

static void do_grad(g2s_point *p0, g2s_point *p1, g2s_point *p2)
{
   double u0, u10, u20;
   double v0, v10, v20;
   double w0, w1, w2, w10, w20;
   double x10, x20, y10, y20, x0, y0;
   double recip;

   x0 = p0->sx; y0 = p0->sy;
   x10 = p1->sx - x0;
   x20 = p2->sx - x0;
   x0 *= (1.0/65536.0); // convert fix to int

   y10 = p1->sy - y0;
   y20 = p2->sy - y0;
   y0 *= (1.0/65536.0); // convert fix to int

   // one factor of 65536 to account for fixed pt screen coord; 
   recip = 65536.0 / (x10 * y20 - y10 * x20);

   w0 = p0->w;
   w1 = p1->w;
   w2 = p2->w;
   u0 = g2d_pp.u_scale*p0->u*w0;
   v0 = g2d_pp.v_scale*p0->v*w0;
   u10 = g2d_pp.u_scale*p1->u*w1 - u0;
   v10 = g2d_pp.v_scale*p1->v*w1 - v0;
   u20 = g2d_pp.u_scale*p2->u*w2 - u0;
   v20 = g2d_pp.v_scale*p2->v*w2 - v0;
   w10 = w1 - w0;
   w20 = w2 - w0;
   g2pt_tmap_data[3] = (y20*u10 - y10*u20)*recip;
   g2pt_tmap_data[4] = (y20*v10 - y10*v20)*recip;
   g2pt_tmap_data[5] = (y20*w10 - y10*w20)*recip;

   g2pt_tmap_data[6] = (x10*u20 - x20*u10)*recip;
   g2pt_tmap_data[7] = (x10*v20 - x20*v10)*recip;
   g2pt_tmap_data[8] = (x10*w20 - x20*w10)*recip;

   g2pt_tmap_data[0] = u0 - (g2pt_tmap_data[3] * x0 + g2pt_tmap_data[6] * y0);
   g2pt_tmap_data[1] = v0 - (g2pt_tmap_data[4] * x0 + g2pt_tmap_data[7] * y0);
   g2pt_tmap_data[2] = w0 - (g2pt_tmap_data[5] * x0 + g2pt_tmap_data[8] * y0);
}

void g2pt_poly_setup(int n, g2s_point **ppl)
{
   g2s_point *p0, *p1, *p2;
   float max_area = -1.0;

   if (n<3)
      return;

   
   if (n==3) {
      p0 = ppl[0];
      p1 = ppl[1];
      p2 = ppl[2];
   }
   else
   {  // pick set of three vertices that gives largest screen area
      int i,j,k;
      for (i=0; i<n-2; i++)
         for (j=i+1; j<n-1; j++)
            for (k=j+1; k<n; k++)
            {
               float area;
               float x10, x20, y10, y20;

               x10 = ppl[j]->sx - ppl[i]->sx;
               x20 = ppl[k]->sx - ppl[i]->sx;
               y10 = ppl[j]->sy - ppl[i]->sy;
               y20 = ppl[k]->sy - ppl[i]->sy;
               area = y20*x10 - y10*x20;
               area *= area;

               if (area > max_area)
               {
                  max_area = area;
                  p0=ppl[i], p1=ppl[j], p2=ppl[k];
               }
            }
   }
   do_grad(p0, p1, p2);
}
