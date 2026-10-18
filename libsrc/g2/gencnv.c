#include <dev2d.h>
#include <icanvas2.h>
#include "general.h"

void (*g2d_default_gen_canvas_table[G2C_TYPES])() =
{ 0 };

void init_g2d_default_gen_canvas_table(void)
{
	g2d_default_gen_canvas_table[G2C_ULINE_EXPOSE] = gen_uline_expose;
	g2d_default_gen_canvas_table[G2C_ULINE] = gen_uline;
	g2d_default_gen_canvas_table[G2C_USCALE_EXPOSE] = gen_uscale_expose;
	g2d_default_gen_canvas_table[G2C_USCALE] = gen_uscale;
	g2d_default_gen_canvas_table[G2C_SCALE_EXPOSE] = gen_scale_expose;
	g2d_default_gen_canvas_table[G2C_SCALE] = gen_scale;
	g2d_default_gen_canvas_table[G2C_UPOLY] = gen_upoly_setup;
	g2d_default_gen_canvas_table[G2C_USPOLY] = gen_uspoly_setup;
	g2d_default_gen_canvas_table[G2C_ULMAP] = gen_ulmap_setup;
	g2d_default_gen_canvas_table[G2C_UPMAP] = gen_upmap_setup;
	g2d_default_gen_canvas_table[G2C_LIT_ULMAP] = gen_lit_ulmap_setup;
	g2d_default_gen_canvas_table[G2C_LIT_UPMAP] = gen_lit_upmap_setup;
}
