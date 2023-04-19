//		Memall.C		Memory Allocation module
//		Rex E. Bradford (REX)
//
//		Memall provides a very simple mechanism for "installable memory
//		allocators".  The default allocator is malloc()/realloc()/free().
//		Clients may install and de-install their own handlers, in a pushdown
//		stack so that old allocators can be re-activated when the new allocator
//		is no longer needed.
//
//		To install a new allocator set, call:
//
//			MemPushAllocator(f_malloc, f_realloc, f_free);
//
//		To deinstall an allocator set, call:
//
//			MemPopAllocator();
//
//		A set of allocators is provided which features automatic checking for
//		allocation failure.  A convenience routine is provided to install
//		these:
//
//			MemCheckOn(bool hard);	// will Error() or Warn() based on flag
//
//		To turn checking off, use:
//
//			MemCheckOff();
//
//		Also, a set of routines to malloc, realloc, and free conventional
//		(below 1 Mb) memory blocks is provided.  Since the regular memory
//		allocation system can usurp conventional memory, systems which need
//		conventional memory should allocate it early on in a program.
// @TBD (toml 07-10-97): this code needs modernization


/*
* $Header: x:/prj/tech/libsrc/lgalloc/RCS/memall.c 1.16 1997/07/15 21:27:56 TOML Exp $
* $log$
*/

#include <string.h>
#include <dos.h>
#include <memall.h>
#include <mallocdb.h>
#include <dbg.h>

#pragma code_seg("lgalloc")

//	Allocator set structure

typedef struct {
	void *(*func_malloc)(size_t size);				// allocator func
	void *(*func_realloc)(void *p, size_t size);	// realloc func
	void (*func_free)(void *p);						// de-allocator func
	size_t (*func_msize)(void *p);
	void *(*func_malloc_db)(size_t size, const char *, int);
	void *(*func_realloc_db)(void *p, size_t size, const char *, int);	// realloc func
	void (*func_free_db)(void *p, const char *, int);						// de-allocator func
} MemAllocSet;

//	Allocator set stack

#define MAX_ALLOCATORS 4
static MemAllocSet memAllocStack[MAX_ALLOCATORS] = {
	{ malloc, realloc, free },
};
static int memIndexAllocStack = 0;

//	Current allocator ptrs

void *(*f_malloc)(size_t size) = malloc;
void *(*f_realloc)(void *p, size_t size) = realloc;
void (*f_free)(void *p) = free;
size_t (*f_msize)(void *p) = _msize;
#ifndef SHIP
void *(*f_malloc_db)(size_t size, const char *, int) = malloc_db;
void *(*f_realloc_db)(void *p, size_t size, const char *, int) = realloc_db;
void (*f_free_db)(void *p, const char *, int) = free_db;
#endif

//	Miscellaneous

static bool memHardCheck;		// if checking on, use Error or Warning?

#define INT_DPMI 0x31			// intr for Dos Protected Mode Interface

//	Internal prototypes

void *MallocChecked(size_t size);
void *ReallocChecked(void *p, size_t size);

// Jacobson, 10/18.
//	These are the functions that the checked allocs point to.
// We are going to avoid using MallocPrev.

static void *(*checked_malloc)(size_t size) = malloc;
static void *(*checked_realloc)(void *p, size_t size) = realloc;
static void (*checked_free)(void *p) = free;

//	--------------------------------------------------------------
//		SETTING, PUSHING, & POPPING ALLOCATOR SETS
//	--------------------------------------------------------------
//
//	MemSetAllocator() sets the current allocator set.
//
//		fm  = ptr to allocator function
//		ff  = ptr to free function
//		fr  = ptr to realloc function

void MemSetAllocator2(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p),
	size_t (*fms)(void *p))
{
	MemAllocSet *pmas;

	pmas = &memAllocStack[memIndexAllocStack];
	f_malloc = pmas->func_malloc = fm;
	f_realloc = pmas->func_realloc = fr;
	f_free = pmas->func_free = ff;
	f_msize = pmas->func_msize = fms;
}

void MemSetAllocator(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p))
{
    MemSetAllocator2(fm, fr, ff, NULL);
}

//	--------------------------------------------------------------
//
//	MemPushAllocator() pushes old allocators, sets new one.
//
//		fm  = ptr to allocator function
//		ff  = ptr to free function
//		fr  = ptr to realloc function
//
//	Returns: 0 if successful, -1 if allocations stack full

int MemPushAllocator2(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p),
	size_t (*fms)(void *p))
{
	if (memIndexAllocStack >= (MAX_ALLOCATORS - 1))
		return(-1);

	++memIndexAllocStack;
	MemSetAllocator2(fm, fr, ff, fms);
	return(0);
}

int MemPushAllocator(void *(*fm)(size_t size),
	void *(*fr)(void *p, size_t size), void (*ff)(void *p))
{
    return MemPushAllocator2(fm, fr, ff, NULL);
}

//	---------------------------------------------------------------
//
//	MemPopAllocator() pops most recent allocator.
//
//	Returns: 0 if successful, -1 if allocations stack underflow

int MemPopAllocator()
{
	MemAllocSet *pmas;

	if (memIndexAllocStack <= 0)
		return(-1);

	--memIndexAllocStack;
	pmas = &memAllocStack[memIndexAllocStack];
	f_malloc = pmas->func_malloc;
	f_realloc = pmas->func_realloc;
	f_free = pmas->func_free;
	f_msize = pmas->func_msize;
	return(0);
}

//	---------------------------------------------------------------
//		CALLOC - NONDEBUG VERSION
//	---------------------------------------------------------------
//
//	CallocNorm() allocates with Malloc(), then clears to 0.
//
//		size = # bytes to allocate and clear

void *CallocNorm(size_t size)
{
	void *p = (*f_malloc)(size);
	if (p)
		memset(p, 0, size);
	return(p);
}

//	---------------------------------------------------------------
//		SPEW VERSIONS
//	---------------------------------------------------------------
//
//	MallocSpew() does Malloc() and spews.

#ifndef SHIP

void *MallocSpew(size_t size, const char *file, int line)
{
	void *p;
    p = (*f_malloc_db)(size, file, line);

	Spew(DSRC_LG_Memall, ("Malloc:  p: 0x%x  size: %d  (file: %s line: %d)\n",
		p, size, file, line));
	return(p);
}

//	---------------------------------------------------------------
//
//	ReallocSpew() does Realloc() and spews.

void *ReallocSpew(void *p, size_t size, const char *file, int line)
{
	void *pnew;
	pnew = (*f_realloc_db)(p,size,file,line);

	Spew(DSRC_LG_Memall, ("Realloc: p: 0x%x  pold: 0x%x  size: %d  (file: %s line: %d)\n",
		pnew, p, size, file, line));
	return(pnew);
}

//	---------------------------------------------------------------
//
//	FreeSpew() does Free() and spews.

void FreeSpew(void *p, const char *file, int line)
{
    (*f_free_db)(p, file, line);
	Spew(DSRC_LG_Memall, ("Free:    p: 0x%x  (file: %s line: %d)\n",
		p, file, line));
}

//	---------------------------------------------------------------
//
//	CallocSpew() does Calloc() and spews.

void *CallocSpew(size_t size, const char *file, int line)
{
    void *p;
	p = (*f_malloc_db)(size, file, line);
	if (p)
		memset(p, 0, size);
	Spew(DSRC_LG_Memall, ("Calloc:  p: 0x%x  size: %d  (file: %s line: %d)\n",
		p, size, file, line));
	return(p);
}

#endif

#if 0
//	---------------------------------------------------------------
//		CHECKED ALLOCATION
//	---------------------------------------------------------------
//
//	MemCheckOn() turns on memory checking.
//
//		hard = if TRUE, do hard error on alloc fail, else do warning

#ifdef DBG_ON
static int checked_on_stack = 0;
#endif

void MemCheckOn(bool hard)
{
   // This will choke if we've got two checked mallocs on the stack.
#ifdef DBG_ON
   Assrt(checked_on_stack == 0);
   checked_on_stack = 1;
#endif

   // Store off the current malloc, alloc, and free.

   checked_malloc  = f_malloc;
   checked_realloc = f_realloc;
   checked_free    = f_free;

   // Now push on the checked allocators

	MemPushAllocator(MallocChecked, ReallocChecked, f_free);

	memHardCheck = hard;
}

//	---------------------------------------------------------------
//
//	MemCheckOff() turns off memory checking.

void MemCheckOff()
{
   // Doesn't make sense if we don't have the checked stuff on.

#ifdef DBG_ON
   Assrt(checked_on_stack == 1);
   checked_on_stack = 0;
#endif

	MemPopAllocator();
}
#endif

//	----------------------------------------------------------
//		CONVENTIONAL MEMORY ALLOCATION
//	----------------------------------------------------------
//
//	MallocConvMemBlock() allocates conventional memory.  It
//	returns a protected mode ptr as well as filling in a useful
//	structure, or returns NULL if unable to get the memory.
//
//		size = size of memory block in bytes
//		pcmb = ptr to ConvMemBlock structure (see res.h)
//
//	Returns: far ptr to block in low memory, or NULL

#ifndef WIN32
void far *MallocConvMemBlock(ushort size, ConvMemBlock *pcmb)
{
	union REGS regs;

//	Use DPMI to get the memory

	regs.x.eax = 0x0100;
	regs.x.ebx = (size + 15) >> 4;
	int386(INT_DPMI, &regs, &regs);
	if (regs.x.cflag)
		return(NULL);

//	Fill in our ConvMemBlock struct, return protected ptr

	pcmb->realSeg = regs.w.ax;
	pcmb->protSel = regs.w.dx;
//	pcmb->protPtr = MK_FP(pcmb->protSel, 0);	// this is the non-flat memory way
	pcmb->protPtr = (void *)(pcmb->realSeg << 4);
	return(pcmb->protPtr);
}
#endif


//	----------------------------------------------------------
//
//	ReallocConvMemBlock() resizes a conventional memory block.
//
//		pcmb    = ptr to ConvMemBlock structure (see res.h)
//		newsize = new size in bytes
//
//	Returns: far ptr to realloc'ed block
#ifndef WIN32
void far *ReallocConvMemBlock(ConvMemBlock *pcmb, ushort newsize)
{
	union REGS regs;
	long realAddr;

	regs.x.eax = 0x0102;
	regs.w.bx = (newsize + 15) >> 4;
	regs.w.dx = pcmb->protSel;
	int386(INT_DPMI, &regs, &regs);
	if (regs.x.cflag)
		return(NULL);

	regs.x.eax = 0x0006;
	regs.w.bx = pcmb->protSel;
	int386(INT_DPMI, &regs, &regs);
	realAddr = (((long) regs.w.cx) << 16) + regs.w.dx;
	pcmb->realSeg = realAddr >> 4;
//	pcmb->protPtr = MK_FP(pcmb->protSel, 0);	// this is the non-flat memory way
	pcmb->protPtr = (void *)(pcmb->realSeg << 4);
	return(pcmb->protPtr);
}
#endif

//	---------------------------------------------------------
//
//	FreeConvMemBlock() frees a conventional memory block.
//
//		pcmb = ptr to ConvMemBlock structure (see res.h)
//
//	Returns: 0 if successful, -1 if free failed

#ifndef WIN32
int FreeConvMemBlock(ConvMemBlock *pcmb)
{
	union REGS regs;

	regs.x.eax = 0x0101;
	regs.w.dx = pcmb->protSel;
	int386(INT_DPMI, &regs, &regs);
	if (regs.x.cflag)
		return(-1);
	return(0);
}
#endif

//	---------------------------------------------------------------
//		INTERNAL ROUTINES
//	---------------------------------------------------------------
//
//	MallocChecked() calls the previously installed allocator, and
//		checks for NULL.  If underlying malloc failed, does hard error.
//
//		size = # bytes to allocate
//
//	Returns: ptr to memory block.

void *MallocChecked(size_t size)
{
	void *p;

	p = checked_malloc(size);
	if (p == NULL)
		{
		if (memHardCheck)
			Error(1, "MallocChecked: out of memory allocating %d bytes\n", size);
		else
			Warning(("MallocChecked: returning NULL (%d bytes requested)\n", size));
		}

	return(p);
}

//	----------------------------------------------------------------
//
//	ReallocChecked() calls the previously installed allocator, and
//		checks for NULL.  If underlying realloc failed, does hard error.
//
//		p    = ptr to existing block
//		size = new size
//
//	Returns: ptr to realloc'ed block.

void *ReallocChecked(void *p, size_t size)
{
	void *pnew;

	pnew = checked_realloc(p, size);
	if (pnew == NULL)
		{
		if (memHardCheck)
			Error(1, "ReallocChecked: out of memory reallocing %d bytes\n", size);
		else
			Warning(("ReallocChecked: returning NULL (%d bytes requested)\n", size));
		}

	return(pnew);
}


