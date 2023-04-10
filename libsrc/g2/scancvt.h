// $Header: x:/prj/tech/libsrc/g2/RCS/scancvt.h 1.1 1998/03/03 12:13:30 KEVIN Exp $
#ifndef __SCANCVT_H
#define __SCANCVT_H

#include <g2spoint.h>

#define G2C_MAX_HEIGHT 768

extern void g2_reset_scan_buffer(void);
extern void g2_scan_convert(g2s_point *p1, g2s_point *p2);
extern void g2_get_scan_conversion(int *y_min, int *y_max, int (**xdata)[G2C_MAX_HEIGHT][2]);
extern void g2_reset_scan_buffer(void);

#endif
