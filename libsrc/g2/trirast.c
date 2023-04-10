// $Header: x:/prj/tech/libsrc/g2/RCS/trirast.c 1.5 1998/04/28 14:29:51 KEVIN Exp $
//
// Triangle rasterization shell.

#include <g2spoint.h>
#include <plyparam.h>



// takes presorted vertices...
int g2_triangle_raster(g2s_point *vtop, g2s_point *vright, g2s_point *vleft, g2s_poly_params *tp)
{
   int n;

   tp->grad_func(vtop, vright, vleft, tp);

   if (tp->yright>tp->yleft) {     // left elbow
      n = tp->yright-tp->ytop;

      tp->right_edge_func(vtop, vright, tp);
      if (tp->yleft>tp->ytop) {    // do we have a flat top?
         tp->left_edge_func(vtop, vleft, tp);
         tp->raster_func(tp, tp->yleft-tp->ytop);
      }
                           // can't have flat bottom since yright>yleft
      tp->left_edge_func(vleft, vright, tp);
      tp->raster_func(tp, tp->yright-tp->yleft);
   } else {                // right elbow
      n = tp->yleft-tp->ytop;
      if (n==0)
         return 0;

      tp->left_edge_func(vtop, vleft, tp);
      if (tp->yright>tp->ytop) {   // do we have a flat top?
         tp->right_edge_func(vtop, vright, tp);
         tp->raster_func(tp, tp->yright-tp->ytop);
      }
      if (tp->yleft>tp->yright) {  // do we have a flat bottom?
         tp->right_edge_func(vright, vleft, tp);
         tp->raster_func(tp, tp->yleft-tp->yright);
      }
   }
   return n;
}

// sorts vertices and calls triangle_raster()
int g2_triangle_raster_sort(g2s_point *v0, g2s_point *v1, g2s_point *v2, g2s_poly_params *tp)
{
   if (v0->sy < v1->sy) {
      if (v0->sy < v2->sy)
         return g2_triangle_raster(v0, v1, v2, tp);
      return g2_triangle_raster(v2, v0, v1, tp);
   }
   if (v1->sy < v2->sy)
      return g2_triangle_raster(v1, v2, v0, tp);
   return g2_triangle_raster(v2, v0, v1, tp);
}
