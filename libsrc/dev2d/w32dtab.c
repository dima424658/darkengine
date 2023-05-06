#include <grnull.h>
#include <idevice.h>
#include <dtabfcn.h>

void (*win32_device_table[GDC_DEVICE_FUNCS])() =
{
	[GDC_SET_MODE] =	win32_set_mode,
	[GDC_SET_PAL] =		win32_set_pal,
	[GDC_GET_PAL] =		win32_get_pal,
};