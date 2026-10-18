#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <flat8.h>

void (*gdd_default_flat8_canvas_table[GDC_CANVAS_FUNCS])() =
{ 0 };

void init_gdd_default_flat8_canvas_table(void)
{
	gdd_default_flat8_canvas_table[GDC_UPIX] = flat8_upix8;
	gdd_default_flat8_canvas_table[GDC_UPIX_EXPOSE] = flat8_upix8_expose;
	gdd_default_flat8_canvas_table[GDC_UPIX8] = flat8_upix8;
	gdd_default_flat8_canvas_table[GDC_UPIX8_EXPOSE] = flat8_upix8_expose;
	gdd_default_flat8_canvas_table[GDC_UPIX16] = flat8_upix16;
	gdd_default_flat8_canvas_table[GDC_UPIX16_EXPOSE] = flat8_upix16_expose;
	gdd_default_flat8_canvas_table[GDC_LPIX] = flat8_lpix8;
	gdd_default_flat8_canvas_table[GDC_LPIX_EXPOSE] = flat8_lpix8_expose;
	gdd_default_flat8_canvas_table[GDC_LPIX8] = flat8_lpix8;
	gdd_default_flat8_canvas_table[GDC_LPIX8_EXPOSE] = flat8_lpix8_expose;
	gdd_default_flat8_canvas_table[GDC_UGPIX8] = flat8_ugpix8;
	gdd_default_flat8_canvas_table[GDC_UGPIX8_EXPOSE] = flat8_ugpix8_expose;
	gdd_default_flat8_canvas_table[GDC_UBITMAP] = flat8_ubitmap;
	gdd_default_flat8_canvas_table[GDC_UBITMAP_EXPOSE] = flat8_ubitmap_expose;
	gdd_default_flat8_canvas_table[GDC_UHLINE] = flat8_uhline;
	gdd_default_flat8_canvas_table[GDC_UHLINE_EXPOSE] = flat8_uhline_expose;
	gdd_default_flat8_canvas_table[GDC_UVLINE] = flat8_uvline;
	gdd_default_flat8_canvas_table[GDC_UVLINE_EXPOSE] = flat8_uvline_expose;
}
