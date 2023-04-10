// $Header: x:/prj/tech/libsrc/dev2d/RCS/pal.h 1.3 1998/01/27 16:40:17 JAEMZ Exp $
// Prototypes and macros for palette manipulation routines.

#ifndef __PAL_H
#define __PAL_H

#ifdef __cplusplus
extern "C" {
#endif

extern void gr_set_pal (int start, int n, uchar *pal_data);
extern void gr_get_pal (int start, int n, uchar *pal_data);
extern void gr_blend_pal (fix degree, ulong color, int start,
                           int n, uchar *old_pal, uchar *new_pal);
extern void gr_bw_pal (fix degree, int start, int n, uchar *old_pal, uchar *new_pal);
extern void gr_set_default_pal(uchar *pal);

#ifdef __cplusplus
};
#endif
#endif /* !__PAL_H */



