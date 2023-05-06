#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*no_video_device_table[GDC_DEVICE_FUNCS])() =
{
	[GDC_INIT_DEVICE] =		null_device,
	[GDC_CLOSE_DEVICE] =	null_device,
	[GDC_SET_MODE] =		null_set_mode,
	[GDC_GET_MODE] =		null_get_mode,
	[GDC_SAVE_STATE] =		null_state,
	[GDC_RESTORE_STATE] =	null_state,
	[GDC_STAT_HTRACE] =		null_trace,
	[GDC_STAT_VTRACE] =		null_trace,
	[GDC_SET_WIDTH] =		null_width,
	[GDC_GET_WIDTH] =		null_width,
	[GDC_GET_FOCUS] =		null_get_focus,
	[GDC_GET_RGB_BITMASK] =	null_get_rgb_bitmask,
};