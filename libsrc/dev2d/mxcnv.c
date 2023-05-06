#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <modex.h>

void (*gdd_default_modex_canvas_table[GDC_CANVAS_FUNCS])() =
{
	[GDC_UPIX] =			modex_upix8,
	[GDC_UPIX_EXPOSE] =		modex_upix8_expose,
	[GDC_UPIX8] =			modex_upix8,
	[GDC_UPIX8_EXPOSE] =	modex_upix8_expose,
	[GDC_UBITMAP] =			modex_ubitmap,
	[GDC_UBITMAP_EXPOSE] =	modex_ubitmap_expose,
	[GDC_UHLINE] =			modex_uhline,
	[GDC_UHLINE_EXPOSE] =	modex_uhline_expose,
	[GDC_UVLINE] =			modex_uvline,
	[GDC_UVLINE_EXPOSE] =	modex_uvline_expose,
};