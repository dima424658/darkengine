
#include <dev2d.h>
#include <flat16.h>

void(*gdd_default_flat16_canvas_table[GDC_CANVAS_FUNCS])() = { 0 };

void init_gdd_default_flat16_canvas_table(void)
{
	gdd_default_flat16_canvas_table[GDC_UPIX] = flat16_upix16;
	gdd_default_flat16_canvas_table[GDC_UPIX_EXPOSE] = flat16_upix16_expose;
	gdd_default_flat16_canvas_table[GDC_UPIX8] = flat16_upix8;
	gdd_default_flat16_canvas_table[GDC_UPIX8_EXPOSE] = flat16_upix8_expose;
	gdd_default_flat16_canvas_table[GDC_UPIX16] = flat16_upix16;
	gdd_default_flat16_canvas_table[GDC_UPIX16_EXPOSE] = flat16_upix16_expose;
	gdd_default_flat16_canvas_table[GDC_LPIX] = flat16_lpix16;
	gdd_default_flat16_canvas_table[GDC_LPIX_EXPOSE] = flat16_lpix16_expose;
	gdd_default_flat16_canvas_table[GDC_LPIX8] = flat16_lpix8;
	gdd_default_flat16_canvas_table[GDC_LPIX8_EXPOSE] = flat16_lpix8_expose;
	gdd_default_flat16_canvas_table[GDC_LPIX16] = flat16_lpix16;
	gdd_default_flat16_canvas_table[GDC_LPIX16_EXPOSE] = flat16_lpix16_expose;
	gdd_default_flat16_canvas_table[GDC_UGPIX8] = flat16_ugpix8;
	gdd_default_flat16_canvas_table[GDC_UGPIX8_EXPOSE] = flat16_ugpix8_expose;
	gdd_default_flat16_canvas_table[GDC_UGPIX16] = flat16_ugpix16;
	gdd_default_flat16_canvas_table[GDC_UGPIX16_EXPOSE] = flat16_ugpix16_expose;
	gdd_default_flat16_canvas_table[GDC_UBITMAP] = flat16_ubitmap;
	gdd_default_flat16_canvas_table[GDC_UBITMAP_EXPOSE] = flat16_ubitmap_expose;
	gdd_default_flat16_canvas_table[GDC_UHLINE] = flat16_uhline;
	gdd_default_flat16_canvas_table[GDC_UHLINE_EXPOSE] = flat16_uhline_expose;
	gdd_default_flat16_canvas_table[GDC_UVLINE] = flat16_uvline;
	gdd_default_flat16_canvas_table[GDC_UVLINE_EXPOSE] = flat16_uvline_expose;
}
