#include <genrast.h>

/* The x86 version computed the same generic UV gradients with fewer cycles. */
void uv_triangle_gradients(g2s_point *p0, g2s_point *p1, g2s_point *p2,
                           g2s_poly_params *params)
{
   gen_triangle_gradients(p0, p1, p2, params);
}

void iuv_triangle_gradients(g2s_point *p0, g2s_point *p1, g2s_point *p2,
                            g2s_poly_params *params)
{
   gen_triangle_gradients(p0, p1, p2, params);
}
