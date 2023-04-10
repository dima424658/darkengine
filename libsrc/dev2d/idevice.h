/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/idevice.h $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1998/03/12 10:41:01 $
 *
 * Symbolic constants for function table references.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __IDEVICE_H
#define __IDEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
   GDC_INIT_DEVICE,
   GDC_CLOSE_DEVICE,
   GDC_SET_MODE,
   GDC_GET_MODE,
   GDC_SAVE_STATE,
   GDC_RESTORE_STATE,
   GDC_STAT_HTRACE,
   GDC_STAT_VTRACE,
   GDC_SET_PAL,
   GDC_GET_PAL,
   GDC_SET_WIDTH,
   GDC_GET_WIDTH,
   GDC_SET_FOCUS,
   GDC_GET_FOCUS,
   GDC_SET_BANK,
   GDC_GET_RGB_BITMASK,
   GDC_DEVICE_FUNCS
};

#ifdef __cplusplus
};
#endif
#endif /* !__IDEVICE_H */
