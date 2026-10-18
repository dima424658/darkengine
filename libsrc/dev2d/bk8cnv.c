#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <bank8.h>

void (*gdd_default_bank8_canvas_table[GDC_CANVAS_FUNCS])() =
{ 0 };

void init_gdd_default_bank8_canvas_table(void)
{
	gdd_default_bank8_canvas_table[GDC_UPIX] = bank8_upix8;
	gdd_default_bank8_canvas_table[GDC_UPIX_EXPOSE] = bank8_upix8_expose;
	gdd_default_bank8_canvas_table[GDC_UPIX8] = bank8_upix8;
	gdd_default_bank8_canvas_table[GDC_UPIX8_EXPOSE] = bank8_upix8_expose;
	gdd_default_bank8_canvas_table[GDC_UBITMAP] = bank8_ubitmap;
	gdd_default_bank8_canvas_table[GDC_UBITMAP_EXPOSE] = bank8_ubitmap_expose;
	gdd_default_bank8_canvas_table[GDC_UHLINE] = bank8_uhline;
	gdd_default_bank8_canvas_table[GDC_UHLINE_EXPOSE] = bank8_uhline_expose;
	gdd_default_bank8_canvas_table[GDC_UVLINE] = bank8_uvline;
	gdd_default_bank8_canvas_table[GDC_UVLINE_EXPOSE] = bank8_uvline_expose;
}
