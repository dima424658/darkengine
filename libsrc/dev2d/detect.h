/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/detect.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:53:14 $
 *
 * Prototypes for detection routines.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __DETECT_H
#define __DETECT_H

#include <grs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int win32_detect(grs_sys_info *info);
extern int flat8_detect(grs_sys_info *info);
extern int vga_detect(grs_sys_info *info);

#ifdef __cplusplus
};
#endif
#endif /* !__DETECT_H */
