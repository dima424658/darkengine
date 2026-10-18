#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*win32_device_table[GDC_DEVICE_FUNCS])() =
{ 0 };

void init_win32_device_table(void)
{
	win32_device_table[GDC_SET_MODE] = win32_set_mode;
	win32_device_table[GDC_SET_PAL] = win32_set_pal;
	win32_device_table[GDC_GET_PAL] = win32_get_pal;
}
