
#include <dev2d.h>
#include <icanvas2.h>
#include <general.h>
#include <flat16.h>

void (*g2d_default_flat16_canvas_table[G2C_TYPES])() =
{
	[G2C_ULINE_EXPOSE] =	gen_uline_expose,
	[G2C_ULINE] =			gen_uline,
	[G2C_USCALE_EXPOSE] =	gen_uscale_expose,
	[G2C_USCALE] =			gen_uscale,
	[G2C_SCALE_EXPOSE] =	gen_scale_expose,
	[G2C_SCALE] =			gen_scale,
	[G2C_UPOLY] =			gen_upoly_setup,
	[G2C_USPOLY] =			gen_uspoly_setup,
	[G2C_ULMAP] =			gen_ulmap_setup,
	[G2C_UPMAP] =			flat16_upmap_setup,
	[G2C_LIT_ULMAP] =		flat16_lit_ulmap_setup,
	[G2C_LIT_UPMAP] =		flat16_lit_upmap_setup,
};