
#include <dev2d.h>
#include <flat16.h>

void(*gdd_default_flat16_canvas_table[GDC_CANVAS_FUNCS])() = {
	[GDC_UPIX] =			flat16_upix16,
	[GDC_UPIX_EXPOSE] =		flat16_upix16_expose,
	[GDC_UPIX8] =			flat16_upix8,
	[GDC_UPIX8_EXPOSE] =	flat16_upix8_expose,
	[GDC_UPIX16] =			flat16_upix16,
	[GDC_UPIX16_EXPOSE] =	flat16_upix16_expose,
	[GDC_LPIX] =			flat16_lpix16,
	[GDC_LPIX_EXPOSE] =		flat16_lpix16_expose,
	[GDC_LPIX8] =			flat16_lpix8,
	[GDC_LPIX8_EXPOSE] =	flat16_lpix8_expose,
	[GDC_LPIX16] =			flat16_lpix16,
	[GDC_LPIX16_EXPOSE] =	flat16_lpix16_expose,
	[GDC_UGPIX8] =			flat16_ugpix8,
	[GDC_UGPIX8_EXPOSE] =	flat16_ugpix8_expose,
	[GDC_UGPIX16] =			flat16_ugpix16,
	[GDC_UGPIX16_EXPOSE] =	flat16_ugpix16_expose,
	[GDC_UBITMAP] =			flat16_ubitmap,
	[GDC_UBITMAP_EXPOSE] =	flat16_ubitmap_expose,
	[GDC_UHLINE] =			flat16_uhline,
	[GDC_UHLINE_EXPOSE] =	flat16_uhline_expose,
	[GDC_UVLINE] =			flat16_uvline,
	[GDC_UVLINE_EXPOSE] =	flat16_uvline_expose
};