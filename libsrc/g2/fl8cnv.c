#include <dev2d.h>
#include <icanvas2.h>
#include "general.h"
#include "flat8.h"

void (*g2d_default_flat8_canvas_table[G2C_TYPES])() =
{ 0 };

void init_g2d_default_flat8_canvas_table(void)
{
	g2d_default_flat8_canvas_table[G2C_ULINE_EXPOSE] = g2_flat8_uline_expose;
	g2d_default_flat8_canvas_table[G2C_ULINE] = g2_flat8_uline;
	g2d_default_flat8_canvas_table[G2C_USCALE_EXPOSE] = flat8_uscale_expose;
	g2d_default_flat8_canvas_table[G2C_USCALE] = flat8_uscale;
	g2d_default_flat8_canvas_table[G2C_SCALE_EXPOSE] = flat8_scale_expose;
	g2d_default_flat8_canvas_table[G2C_SCALE] = flat8_scale;
	g2d_default_flat8_canvas_table[G2C_UPOLY] = gen_upoly_setup;
	g2d_default_flat8_canvas_table[G2C_USPOLY] = flat8_uspoly_setup;
	g2d_default_flat8_canvas_table[G2C_ULMAP] = flat8_ulmap_setup;
	g2d_default_flat8_canvas_table[G2C_UPMAP] = flat8_upmap_setup;
	g2d_default_flat8_canvas_table[G2C_LIT_ULMAP] = flat8_lit_ulmap_setup;
	g2d_default_flat8_canvas_table[G2C_LIT_UPMAP] = flat8_lit_upmap_setup;
}
