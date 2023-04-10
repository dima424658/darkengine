/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/screen.h $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1997/01/08 16:49:34 $
 *
 * Screen handling prototypes.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __SCREEN_H
#define __SCREEN_H
#include <grs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern grs_screen *gr_alloc_screen (int w, int h);
extern void gr_free_screen (grs_screen *s);
extern void gr_set_screen (grs_screen *s);

#ifdef __cplusplus
};
#endif
#endif /* !__SCREEN_H */
