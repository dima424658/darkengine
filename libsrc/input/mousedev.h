/*
 * $Source: x:/prj/tech/libsrc/input/RCS/mousedev.h $
 * $Revision: 1.4 $
 * $Author: TOML $
 * $Date: 1996/09/23 16:58:52 $
 *
 * device dependant mouse stuff
 */

#ifndef __MOUSEDEV_H
#define __MOUSEDEV_H

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

int mouse_dev_init(short xsize, short ysize);
int mouse_dev_shutdown(void);
errtype mouse_dev_put_xy(short x, short y);
errtype mouse_dev_constrain_xy(short xl, short yl, short xh, short yh);
errtype mouse_dev_set_rate(short xr, short yr, short thold);
errtype mouse_dev_get_rate(short* xr, short* yr, short* thold);
errtype mouse_dev_read_state(mouse_state *pMouseState);
errtype mouse_dev_extremes( short *xmin, short *ymin, short *xmax, short *ymax );

#ifdef __cplusplus
}
#endif  // cplusplus

#endif // __MOUSEDEV_H
