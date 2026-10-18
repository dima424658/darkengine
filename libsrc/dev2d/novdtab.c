#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*no_video_device_table[GDC_DEVICE_FUNCS])() =
{ 0 };

void init_no_video_device_table(void)
{
	no_video_device_table[GDC_INIT_DEVICE] = null_device;
	no_video_device_table[GDC_CLOSE_DEVICE] = null_device;
	no_video_device_table[GDC_SET_MODE] = null_set_mode;
	no_video_device_table[GDC_GET_MODE] = null_get_mode;
	no_video_device_table[GDC_SAVE_STATE] = null_state;
	no_video_device_table[GDC_RESTORE_STATE] = null_state;
	no_video_device_table[GDC_STAT_HTRACE] = null_trace;
	no_video_device_table[GDC_STAT_VTRACE] = null_trace;
	no_video_device_table[GDC_SET_WIDTH] = null_width;
	no_video_device_table[GDC_GET_WIDTH] = null_width;
	no_video_device_table[GDC_GET_FOCUS] = null_get_focus;
	no_video_device_table[GDC_GET_RGB_BITMASK] = null_get_rgb_bitmask;
}
