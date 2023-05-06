#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*vga_device_table[GDC_DEVICE_FUNCS])() =
{
	[GDC_INIT_DEVICE] =		gr_null,
	[GDC_CLOSE_DEVICE] =	gr_null,
	[GDC_SET_MODE] =		vga_set_mode,
	[GDC_SAVE_STATE] =		vga_save_state,
	[GDC_RESTORE_STATE] =	vga_restore_state,
	[GDC_STAT_HTRACE] =		vga_stat_htrace,
	[GDC_STAT_VTRACE] =		vga_stat_vtrace,
	[GDC_SET_PAL] =			vga_set_pal,
	[GDC_GET_PAL] =			vga_get_pal,
	[GDC_SET_WIDTH] =		vga_set_width,
	[GDC_SET_FOCUS] =		vga_set_focus,
	[GDC_GET_FOCUS] =		vga_get_focus,
	[GDC_GET_RGB_BITMASK] =	null_get_rgb_bitmask,
};