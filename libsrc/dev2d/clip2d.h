/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/clip.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:52:00 $
 *
 * Prototypes for 2d clippers, constants for clipping codes.
 *
 * This file is part of the dev2d library.
 */

#ifndef __CLIP2D_H
#define __CLIP2D_H

#ifdef __cplusplus
extern "C" {
#endif

/* clip codes. */
#define CLIP_NONE    0
#define CLIP_LEFT    1
#define CLIP_TOP     2
#define CLIP_RIGHT   4
#define CLIP_BOT     8
#define CLIP_ALL     16

#ifdef __cplusplus
};
#endif
#endif /* !__CLIP2D_H */
