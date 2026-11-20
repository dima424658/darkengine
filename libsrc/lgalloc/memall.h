// Core allocation helpers used by the engine.

#ifndef MEMALL_H
#define MEMALL_H

#include <stddef.h>
#include <stdlib.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void *(*f_malloc)(size_t size);
extern void *(*f_realloc)(void *p, size_t size);
extern void (*f_free)(void *p);
extern size_t (*f_msize)(void *p);

#define MSize(p) ((*f_msize)(p))
#define Malloc(size) ((*f_malloc)(size))
#define Realloc(p, size) ((*f_realloc)((p), (size)))
#define Free(p) ((*f_free)(p))

void *CallocNorm(size_t size);
#define Calloc(size) CallocNorm(size)

typedef struct
{
   void *baseptr;
   long sz;
   void *topptr;
} MemStack;

void MemStackInit(MemStack *ms);
void *MemStackAlloc(MemStack *ms, long size);
void *MemStackRealloc(MemStack *ms, void *ptr, long newsize);
bool MemStackFree(MemStack *ms, void *ptr);

#ifdef __cplusplus
}
#endif

#endif
