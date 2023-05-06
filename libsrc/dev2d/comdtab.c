
#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*com_device_table[GDC_DEVICE_FUNCS])() =
{
	[GDC_INIT_DEVICE] =		com_init,
	[GDC_CLOSE_DEVICE] =	com_close,
	[GDC_SET_MODE] =		com_set_mode,
	[GDC_GET_MODE] =		com_get_mode,
	[GDC_SAVE_STATE] =		com_save_state,
	[GDC_RESTORE_STATE] =	com_restore_state,
	[GDC_STAT_HTRACE] =		com_stat_htrace,
	[GDC_STAT_VTRACE] =		com_stat_vtrace,
	[GDC_SET_PAL] =			com_set_pal,
	[GDC_GET_PAL] =			com_get_pal,
	[GDC_SET_WIDTH] =		com_set_width,
	[GDC_SET_FOCUS] =		com_set_focus,
	[GDC_GET_FOCUS] =		com_get_focus,
	[GDC_GET_RGB_BITMASK] =	com_get_rgb_bitmask,
};