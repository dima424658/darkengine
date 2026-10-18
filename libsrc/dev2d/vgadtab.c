#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*vga_device_table[GDC_DEVICE_FUNCS])() =
{ 0 };

void init_vga_device_table(void)
{
	vga_device_table[GDC_INIT_DEVICE] = gr_null;
	vga_device_table[GDC_CLOSE_DEVICE] = gr_null;
	vga_device_table[GDC_SET_MODE] = vga_set_mode;
	vga_device_table[GDC_SAVE_STATE] = vga_save_state;
	vga_device_table[GDC_RESTORE_STATE] = vga_restore_state;
	vga_device_table[GDC_STAT_HTRACE] = vga_stat_htrace;
	vga_device_table[GDC_STAT_VTRACE] = vga_stat_vtrace;
	vga_device_table[GDC_SET_PAL] = vga_set_pal;
	vga_device_table[GDC_GET_PAL] = vga_get_pal;
	vga_device_table[GDC_SET_WIDTH] = vga_set_width;
	vga_device_table[GDC_SET_FOCUS] = vga_set_focus;
	vga_device_table[GDC_GET_FOCUS] = vga_get_focus;
	vga_device_table[GDC_GET_RGB_BITMASK] = null_get_rgb_bitmask;
}
