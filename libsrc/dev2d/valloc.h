// $Header: x:/prj/tech/libsrc/dev2d/RCS/valloc.h 1.2 1997/01/22 12:54:26 KEVIN Exp $

#ifndef __VALLOC_H
#define __VALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

extern bool vMonitor (uchar **p);
extern void vUnmonitor (uchar **p);
extern bool vAlloc (uchar **p, int w, int h);
extern void vFree (uchar **p);

#ifdef __cplusplus
};
#endif
#endif /* !__VALLOC_H */
