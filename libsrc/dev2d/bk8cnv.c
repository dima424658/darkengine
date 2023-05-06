#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <bank8.h>

void (*gdd_default_bank8_canvas_table[GDC_CANVAS_FUNCS])() =
{
	[GDC_UPIX] =			bank8_upix8,
	[GDC_UPIX_EXPOSE] =		bank8_upix8_expose,
	[GDC_UPIX8] =			bank8_upix8,
	[GDC_UPIX8_EXPOSE] =	bank8_upix8_expose,
	[GDC_UBITMAP] =			bank8_ubitmap,
	[GDC_UBITMAP_EXPOSE] =	bank8_ubitmap_expose,
	[GDC_UHLINE] =			bank8_uhline,
	[GDC_UHLINE_EXPOSE] =	bank8_uhline_expose,
	[GDC_UVLINE] =			bank8_uvline,
	[GDC_UVLINE_EXPOSE] =	bank8_uvline_expose,
};