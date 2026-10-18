#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*flat8_device_table[GDC_DEVICE_FUNCS])() =
{ 0 };

void init_flat8_device_table(void)
{
	flat8_device_table[GDC_INIT_DEVICE] = gr_null;
	flat8_device_table[GDC_CLOSE_DEVICE] = gr_null;
	flat8_device_table[GDC_SET_MODE] = flat8_set_mode;
	flat8_device_table[GDC_SAVE_STATE] = vga_save_state;
	flat8_device_table[GDC_RESTORE_STATE] = vga_restore_state;
	flat8_device_table[GDC_STAT_HTRACE] = vga_stat_htrace;
	flat8_device_table[GDC_STAT_VTRACE] = vga_stat_vtrace;
	flat8_device_table[GDC_SET_PAL] = vga_set_pal;
	flat8_device_table[GDC_GET_PAL] = vga_get_pal;
	flat8_device_table[GDC_SET_WIDTH] = vga_set_width;
	flat8_device_table[GDC_SET_FOCUS] = vga_set_focus;
	flat8_device_table[GDC_GET_FOCUS] = vga_get_focus;
	flat8_device_table[GDC_GET_RGB_BITMASK] = null_get_rgb_bitmask;
}
