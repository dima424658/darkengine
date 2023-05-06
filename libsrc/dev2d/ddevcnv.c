#include <icanvas.h>
#include <grnull.h>
#include <general.h>
#include <ddevblt.h>

void (*gdd_default_dispdev_canvas_table[GDC_CANVAS_FUNCS])() =
{
	[GDC_UBITMAP] =			dispdev_ubitmap,
	[GDC_UBITMAP_EXPOSE] =	dispdev_ubitmap_expose,
};