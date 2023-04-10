// $Header: x:/prj/tech/libsrc/dev2d/RCS/pal_.h 1.1 1998/01/27 12:17:50 JAEMZ Exp $
// Prototypes and macros for palette manipulation routines.

#ifndef __PAL__H
#define __PAL__H

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_PAL_CBACKS 2

// All the types of cbacks must be here
#define PAL_CONTRAST 0
#define PAL_GAMMA 1


// These must be able to have dst == src
typedef void (*palSettingCback)(uchar *dst,uchar *src,int start,int n);

extern palSettingCback grd_pal_callbacks[NUM_PAL_CBACKS];

#ifdef __cplusplus
};
#endif
#endif /* !__PAL__H */

