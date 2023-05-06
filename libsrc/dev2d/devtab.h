/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/devtab.h $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/08/29 09:44:31 $
 *
 * Declaration for device table list.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __DEVTAB_H
#define __DEVTAB_H

#ifdef __cplusplus
extern "C" {
#endif

extern void (*no_video_device_table[])();
extern void (*flat8_device_table[])();
extern void (*win32_device_table[])();
extern void (*com_device_table[])();
extern void (*vga_device_table[])();
extern void (*vesa_device_table[])();

#ifdef __cplusplus
};
#endif
#endif /* !__DEVTAB_H */
