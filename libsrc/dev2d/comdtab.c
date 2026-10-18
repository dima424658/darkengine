
#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*com_device_table[GDC_DEVICE_FUNCS])() =
{ 0 };

void init_com_device_table(void)
{
	com_device_table[GDC_INIT_DEVICE] = com_init;
	com_device_table[GDC_CLOSE_DEVICE] = com_close;
	com_device_table[GDC_SET_MODE] = com_set_mode;
	com_device_table[GDC_GET_MODE] = com_get_mode;
	com_device_table[GDC_SAVE_STATE] = com_save_state;
	com_device_table[GDC_RESTORE_STATE] = com_restore_state;
	com_device_table[GDC_STAT_HTRACE] = com_stat_htrace;
	com_device_table[GDC_STAT_VTRACE] = com_stat_vtrace;
	com_device_table[GDC_SET_PAL] = com_set_pal;
	com_device_table[GDC_GET_PAL] = com_get_pal;
	com_device_table[GDC_SET_WIDTH] = com_set_width;
	com_device_table[GDC_SET_FOCUS] = com_set_focus;
	com_device_table[GDC_GET_FOCUS] = com_get_focus;
	com_device_table[GDC_GET_RGB_BITMASK] = com_get_rgb_bitmask;
}
