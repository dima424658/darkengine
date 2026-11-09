#include <string.h>
#include <memall.h>
#include <platform_services.h>

#pragma code_seg("lgalloc")

void *(*f_malloc)(size_t size) = malloc;
void *(*f_realloc)(void *p, size_t size) = realloc;
void (*f_free)(void *p) = free;
size_t (*f_msize)(void *p) = PlatformAllocationSize;

void *CallocNorm(size_t size)
{
   void *p = (*f_malloc)(size);
   if (p)
      memset(p, 0, size);
   return p;
}
