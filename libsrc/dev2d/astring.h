// Portable aligned-copy compatibility interface.

#ifndef __ASTRING_H
#define __ASTRING_H

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void memcpya(void *d, void *s, int n, void *a);
extern void memcpy_align_src(void *d, void *s, int n);
extern void memcpy_align_dst(void *d, void *s, int n);
extern void memcpy_cache_dst(void *d, void *s, int n);
extern void memcpy_by_byte(void *d, void *s, int n);

/* The alignment hint was used only by the old hand-written x86 loop. */
static __inline void memcpya_(void *d, const void *s, int n, const void *a)
{
   (void)a;
   if (n > 0)
      memcpy(d, s, (size_t)n);
}

#ifdef __cplusplus
}
#endif

#endif
