/*
 * $Header: x:/prj/tech/libsrc/timer/RCS/tminit.h 1.8 1996/12/10 21:16:06 TONY Exp $
 */

#ifndef _TMINIT_H
#define _TMINIT_H

#if defined(AIL_SOUND)
#define tm_init tm_init_AIL
#define tm_close tm_close_AIL

EXTERN int tm_init_AIL (void);
EXTERN int tm_close_AIL (void);
#else

#define tm_init tm_init_libt
#define tm_close tm_close_libt

EXTERN int tm_init_libt (void);
EXTERN int tm_close_libt (void);

#endif

#endif /* _TMINIT_H */

