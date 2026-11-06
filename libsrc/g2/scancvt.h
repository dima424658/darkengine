// $Header: x:/prj/tech/libsrc/g2/RCS/scancvt.h 1.1 1998/03/03 12:13:30 KEVIN Exp $
#ifndef __SCANCVT_H
#define __SCANCVT_H

#include <g2spoint.h>

// The software texture mapper indexes its scan-conversion workspaces by the
// destination scanline. The original 768-row ceiling corrupts adjacent global
// data at modern resolutions such as 1920x1080. Keep the workspace at the
// maximum scanline height accepted by the renderer.
#define G2C_MAX_HEIGHT 8192

extern void g2_reset_scan_buffer(void);
extern void g2_scan_convert(g2s_point *p1, g2s_point *p2);
extern void g2_get_scan_conversion(int *y_min, int *y_max, int (**xdata)[G2C_MAX_HEIGHT][2]);
extern void g2_reset_scan_buffer(void);

#endif
