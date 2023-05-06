#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*vesa_device_table[GDC_DEVICE_FUNCS])() =
{
	[GDC_INIT_DEVICE] =		gr_null,
	[GDC_CLOSE_DEVICE] =	vbe_close,
	[GDC_SET_MODE] =		vbe_set_mode,
	[GDC_GET_MODE] =		vbe_get_mode,
	[GDC_SAVE_STATE] =		save_gen_state,
	[GDC_RESTORE_STATE] =	restore_gen_state,
	[GDC_STAT_HTRACE] =		vga_stat_htrace,
	[GDC_STAT_VTRACE] =		vga_stat_vtrace,
	[GDC_SET_WIDTH] =		vbe_set_width,
	[GDC_GET_WIDTH] =		vbe_get_width,
	[GDC_SET_FOCUS] =		vbe_set_focus,
	[GDC_GET_FOCUS] =		vbe_get_focus,
	[GDC_GET_RGB_BITMASK] =	null_get_rgb_bitmask,
};