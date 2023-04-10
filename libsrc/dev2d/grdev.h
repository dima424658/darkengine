// $Header: x:/prj/tech/libsrc/dev2d/RCS/grdev.h 1.3 1998/03/12 10:40:48 KEVIN Exp $
// Macros for table driven device driver functions.

#ifndef __GRDEV_H
#define __GRDEV_H
#include <grd.h>
#include <idevice.h>
#include <state.h>

#ifdef __cplusplus
extern "C" {
#endif

/* here are the definitions for all the table driven function. */
#define gr_init_device \
   ((int (*)(grs_sys_info *_info))grd_device_table[GDC_INIT_DEVICE])

#define gr_close_device \
   ((int (*)(grs_sys_info *_info))grd_device_table[GDC_CLOSE_DEVICE])

#define gr_set_screen_mode \
   ((int (*)(int mode, int clear))grd_device_table[GDC_SET_MODE])

#define gr_get_screen_mode \
   ((int (*)(void))grd_device_table[GDC_GET_MODE])

#define gr_save_state \
   ((int (*)(VideoState *vidStatePtr))grd_device_table[GDC_SAVE_STATE])

#define gr_restore_state \
   ((int (*)(VideoState *vidStatePtr))grd_device_table[GDC_RESTORE_STATE])

#define gr_stat_htrace \
   ((int (*)(void))grd_device_table[GDC_STAT_HTRACE])

#define gr_stat_vtrace \
   ((int (*)(void))grd_device_table[GDC_STAT_VTRACE])

#define gr_set_screen_pal \
   ((void (*)(int start,int n,uchar *pal_data))grd_device_table[GDC_SET_PAL])

#define gr_get_screen_pal \
   ((void (*)(int start,int n,uchar *pal_data))grd_device_table[GDC_GET_PAL])

#define gr_set_width \
   ((int (*)(int w))grd_device_table[GDC_SET_WIDTH])

#define gr_get_width \
   ((int (*)(void))grd_device_table[GDC_GET_WIDTH])

#define gr_set_focus \
   ((void (*)(int x,int y))grd_device_table[GDC_SET_FOCUS])

#define gr_get_focus \
   ((void (*)(int *x,int *y))grd_device_table[GDC_GET_FOCUS])

#define gr_get_screen_rgb_bitmask \
   ((void (*)(grs_rgb_bitmask *bitmask))grd_device_table[GDC_GET_RGB_BITMASK])

#ifdef __cplusplus
};
#endif
#endif /* !__GRDEV_H */
