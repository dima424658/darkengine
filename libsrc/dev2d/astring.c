// $Header: x:/prj/tech/libsrc/dev2d/RCS/astring.c 1.2 1997/02/07 11:20:46 KEVIN Exp $
// Aligned string copy routines.

#include <astring.h>
void memcpya(void *d, void *s, int n, void *a)
{
   memcpya_(d, s, n, a);
}

void memcpy_align_src(void *d, void *s, int n)
{
   memcpya_(d, s, n, s);
}

void memcpy_align_dst(void *d, void *s, int n)
{
   memcpya_(d, s, n, d);
}

// this is rather pathetic...
void memcpy_by_byte(void *d, void *s, int n)
{
   int i;
   for (i=0; i<n; i++)
      ((uchar *)d)[i]=((uchar *)s)[i];
}
