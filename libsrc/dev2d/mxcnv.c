#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <modex.h>

void (*gdd_default_modex_canvas_table[GDC_CANVAS_FUNCS])() =
{ 0 };

void init_gdd_default_modex_canvas_table(void)
{
	gdd_default_modex_canvas_table[GDC_UPIX] = modex_upix8;
	gdd_default_modex_canvas_table[GDC_UPIX_EXPOSE] = modex_upix8_expose;
	gdd_default_modex_canvas_table[GDC_UPIX8] = modex_upix8;
	gdd_default_modex_canvas_table[GDC_UPIX8_EXPOSE] = modex_upix8_expose;
	gdd_default_modex_canvas_table[GDC_UBITMAP] = modex_ubitmap;
	gdd_default_modex_canvas_table[GDC_UBITMAP_EXPOSE] = modex_ubitmap_expose;
	gdd_default_modex_canvas_table[GDC_UHLINE] = modex_uhline;
	gdd_default_modex_canvas_table[GDC_UHLINE_EXPOSE] = modex_uhline_expose;
	gdd_default_modex_canvas_table[GDC_UVLINE] = modex_uvline;
	gdd_default_modex_canvas_table[GDC_UVLINE_EXPOSE] = modex_uvline_expose;
}
