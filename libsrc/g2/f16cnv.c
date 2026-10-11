
#include <dev2d.h>
#include <icanvas2.h>
#include "general.h"
#include "flat16.h"

void (*g2d_default_flat16_canvas_table[G2C_TYPES])() =
{ 0 };

void init_g2d_default_flat16_canvas_table(void)
{
	g2d_default_flat16_canvas_table[G2C_ULINE_EXPOSE] = gen_uline_expose;
	g2d_default_flat16_canvas_table[G2C_ULINE] = gen_uline;
	// A 16-bit destination needs the format-aware dispatch table. Hardware
	// textures are commonly uploaded as BMT_FLAT16; the generic table only
	// handles 8-bit/RSD sources and otherwise dispatches through a null slot.
	g2d_default_flat16_canvas_table[G2C_USCALE_EXPOSE] = flat16_uscale_expose;
	g2d_default_flat16_canvas_table[G2C_USCALE] = flat16_uscale;
	g2d_default_flat16_canvas_table[G2C_SCALE_EXPOSE] = flat16_scale_expose;
	g2d_default_flat16_canvas_table[G2C_SCALE] = flat16_scale;
	g2d_default_flat16_canvas_table[G2C_UPOLY] = gen_upoly_setup;
	g2d_default_flat16_canvas_table[G2C_USPOLY] = gen_uspoly_setup;
	g2d_default_flat16_canvas_table[G2C_ULMAP] = gen_ulmap_setup;
	g2d_default_flat16_canvas_table[G2C_UPMAP] = flat16_upmap_setup;
	g2d_default_flat16_canvas_table[G2C_LIT_ULMAP] = flat16_lit_ulmap_setup;
	g2d_default_flat16_canvas_table[G2C_LIT_UPMAP] = flat16_lit_upmap_setup;
}
