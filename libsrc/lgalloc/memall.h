//		Memall.H		Memory allocator
//		Rex E. Bradford (REX)
/*
 * $Header: x:/prj/tech/libsrc/lgalloc/RCS/memall.h 1.22 1997/10/15 11:52:16 TOML Exp $
 */

// @TBD (toml 07-10-97): this code needs modernization

// Jacobson:
// Note: Right now, the current code base only supports the following:
// 1) Push any number of allocators you wish except MemCheckOn or heapdraw
// 2) Optionally call MemCheckOn
// 3) Optionally call hd_init to start heapdraw.
// 4) If called hd_init, call hd_close to stop heapdraw.
// 5) If called MemCheckOn, call MemCheckOff.

#ifndef MEMALL_H
#define MEMALL_H

#include <malloc.h>
#include "types.h"

//	Setting, pushing, & popping allocator sets

#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus

#if 0
void MemSetAllocator(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p));
void MemSetAllocator2(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p),
	size_t (*fms)(void *p));
int MemPushAllocator(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p));
int MemPushAllocator2(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p),
	size_t (*fms)(void *p));
int MemPopAllocator(void);
#endif

//	Allocating, reallocating, & freeing memory

extern void *(*f_malloc)(size_t size);
extern void *(*f_realloc)(void *p, size_t size);
extern void (*f_free)(void *p);
extern size_t (*f_msize)(void *p);
#ifndef SHIP
extern void *(*f_malloc_db)(size_t size, const char *, int);
extern void *(*f_realloc_db)(void *p, size_t size, const char *, int);
extern void (*f_free_db)(void *p, const char *, int);
#endif

#define MSize(p) ((*f_msize)(p))

#ifdef DBG_ON

void *MallocSpew(size_t size, const char *file, int line);
void *ReallocSpew(void *p, size_t size, const char *file, int line);
void FreeSpew(void *p, const char *file, int line);
void *CallocSpew(size_t size, const char *file, int line);

extern const char * g_pszMemCurrentFile;
extern int          g_MemCurrentLine;

#define Malloc(size) MallocSpew(size,__FILE__,__LINE__)
#define Realloc(p,size) ReallocSpew(p,size,__FILE__,__LINE__)
#define Free(p) FreeSpew(p,__FILE__,__LINE__)
#define Calloc(size) CallocSpew(size,__FILE__,__LINE__)

#if defined(__cplusplus) && !defined(SHIP)
inline void * operator new (size_t size, const char * pszFileName, int line)
{
    return MallocSpew(size, pszFileName, line);
}

#if defined(__WATCOMC__)
inline void * operator new [](size_t size, const char * pszFileName, int line)
{
    return MallocSpew(size, pszFileName, line);
}
#endif

#define MEMALL_DEBUG_NEW new(__FILE__, __LINE__)
#endif

#else

void *CallocNorm(size_t size);

#define Malloc(size) (*f_malloc)(size)
#define Realloc(p,size) (*f_realloc)(p,size)
#define Free(p) (*f_free)(p)
#define Calloc(size) CallocNorm(size)

#if !defined(DEBUG_NEW) && defined(__cplusplus) && !defined(SHIP)
#define DEBUG_NEW new
#endif

#endif

#ifdef __cplusplus
}
#endif //cplusplus


#ifdef _H2INC					//if translating, include assembly macros
#include "memmacro.h"		//this will translate to 'include memmacro.inc'
#endif
#ifdef __cplusplus
extern "C"  {
#endif  // cplusplus


//	Memory checking

void MemCheckOn(bool hard);
void MemCheckOff(void);

//	Heap management (memgrow.c)

int MemGrowHeap(int wantK);
void MemLockHeap();
void MemUnlockHeap();

#if !defined(_WIN32)

//	Allocating conventional memory
//	Caveat: since Malloc() can grab conventional memory, necessary
//	conventional memory blocks should be grabbed early in program.

typedef struct {
	ushort realSeg;		// real mode segment to conventional mem block
	ushort protSel;		// protected mode selector for conv mem block
	void far *protPtr;	// protected mode ptr to mem block
} ConvMemBlock;

void far *MallocConvMemBlock(ushort size, ConvMemBlock *pcmb);		// alloc
void far *ReallocConvMemBlock(ConvMemBlock *pcmb, ushort newsize); // resize
int FreeConvMemBlock(ConvMemBlock *pcmb);		// free low memory block

#endif

//////////////////////////////
//
// Dealing with a large block of memory as a stack for easy allocation
//
// Rationale: often, routines want some large amount of memory, and know they
// will throw it away when they're done.  If you use a MemStack for this memory,
// you don't have to worry about fragmenting the heap.
//
// To use, declare a MemStack.  Malloc n bytes of memory, put the resulting
// pointer in baseptr and n in size, and call MemStackInit().  Then use
// MemStackAlloc() and MemStackFree() to grab and release memory.  You must
// free memory in the reverse order of allocating it, as this is a stack.
// Note that you can have multiple MemStacks if you feel like it.

typedef struct
{
   void *baseptr; // pointer to bottom of stack
   long  sz;      // size of stack in bytes
   void *topptr;  // pointer to current top of stack (next free byte)
}
MemStack;

void MemStackInit (MemStack *ms);
void *MemStackAlloc (MemStack *ms, long size);
void *MemStackRealloc (MemStack *ms, void *ptr, long newsize);
bool MemStackFree (MemStack *ms, void *ptr);


#ifdef __cplusplus
}
#endif  // cplusplus

#endif

