#include <grd.h>
#include <bitmap.h>
#include <idevice.h>

#define gri_set_focus \
   ((void (*)(int x, int y))grd_device_table[GDC_GET_FOCUS])

void gr_set_focus(int x, int y)
{
   gri_set_focus(x, y);

   grd_screen->x = x;
   grd_screen->y = y;
   grd_visible_canvas->bm.bits = grd_screen_canvas->bm.bits;
   gr_calc_sub_bitmap(&grd_visible_canvas->bm, x, y);
}

