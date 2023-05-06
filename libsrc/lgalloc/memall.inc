
option expr32
option casemap:none

IFNDEF	memall_inc
memall_inc	EQU		1

; Begin of file memall.h
; 		Memall.H		Memory allocator
; 		Rex E. Bradford (REX)
; 
; * $Header: x:/prj/tech/libsrc/lgalloc/RCS/memall.inc 1.2 1997/07/10 17:13:17 TOML Exp $
; * $Log: memall.inc $
;Revision 1.2  1997/07/10  17:13:17  TOML
;*** empty log message ***
;
;Revision 1.1  1995/05/25  06:59:40  LMFEENEY
;Initial revision
;
;  * Revision 1.12  1993/12/20  13:40:05  ept
;  * Added MemStackRealloc.
;  * 
;  * Revision 1.11  1993/09/30  18:36:51  rex
;  * Added prototypes for memgrow.c stuff (heap grow, lock, unlock)
;  * 
;  * Revision 1.10  1993/09/13  12:40:12  dfan
;  * ptr and size were reserved words in assembler
;  * 
;  * Revision 1.9  1993/09/13  11:11:13  dfan
;  * Add memstack stuff
;  * 
;  * Revision 1.8  1993/08/11  18:44:12  rex
;  * Changed Calloc() to macro, so can do spew
;  * 
;  * Revision 1.7  1993/08/11  17:30:40  rex
;  * Added Spew() versions of malloc/realloc/free, and made uppercase macro
;  * versions point to them when DBG is on
;  * 
;  * Revision 1.6  1993/04/16  12:59:55  matt
;  * Added some void's to functions that took no args to get around h2inc bug.
;  * 
;  * Revision 1.5  1993/04/13  16:05:22  rex
;  * Added prototypes for MemCheckOn() and MemCheckOff()
;  * 
;  * Revision 1.4  1993/03/24  12:02:44  matt
;  * More asm header junk
;  * 
;  * Revision 1.3  1993/03/16  15:12:39  matt
;  * Added junk for H2INC translation to assembly header
;  * 
;  * Revision 1.2  1993/02/05  17:38:07  rex
;  * Added Calloc() prototype
;  * 
;  * Revision 1.1  1993/01/29  09:47:55  rex
;  * Initial revision
;  * 
;  * Revision 1.3  1993/01/18  12:18:38  rex
;  * Changed interface to new standard we all agreed on
;  * 
;  * Revision 1.2  1993/01/14  09:41:02  rex
;  * Decided Malloc() should never fail, changed interface.
;  * 
;  * Revision 1.1  1993/01/12  17:54:54  rex
;  * Initial revision
;  * 
; 
	INCLUDE	malloc.inc

	INCLUDE	types.inc

; 	Setting, pushing, & popping allocator sets
@proto_memall47		TYPEDEF		PROTO C :size_t
@t_memall1		TYPEDEF		PTR @proto_memall47
@proto_memall48		TYPEDEF		PROTO C :PTR , :size_t
@t_memall2		TYPEDEF		PTR @proto_memall48
@proto_memall49		TYPEDEF		PROTO C :PTR 
@t_memall3		TYPEDEF		PTR @proto_memall49
@proto_memall50		TYPEDEF		PROTO C :@t_memall1, :@t_memall2, :@t_memall3
MemSetAllocator		PROTO		@proto_memall50

@proto_memall51		TYPEDEF		PROTO C :size_t
@t_memall4		TYPEDEF		PTR @proto_memall51
@proto_memall52		TYPEDEF		PROTO C :PTR , :size_t
@t_memall5		TYPEDEF		PTR @proto_memall52
@t_memall6		TYPEDEF		PTR 
@proto_memall53		TYPEDEF		PROTO C :@t_memall4, :@t_memall5, :@t_memall6
MemPushAllocator		PROTO		@proto_memall53

@proto_memall54		TYPEDEF		PROTO C 
MemPopAllocator		PROTO		@proto_memall54

; 	Allocating, reallocating, & freeing memory
@proto_memall55		TYPEDEF		PROTO C :size_t
EXTERNDEF		C f_malloc:PTR @proto_memall55

@proto_memall56		TYPEDEF		PROTO C :PTR , :size_t
EXTERNDEF		C f_realloc:PTR @proto_memall56

@proto_memall57		TYPEDEF		PROTO C :PTR 
EXTERNDEF		C f_free:PTR @proto_memall57

@proto_memall58		TYPEDEF		PROTO C :size_t
CallocNorm		PROTO		@proto_memall58

; if translating, include assembly macros 
; this will translate to 'include memmacro.inc'
	INCLUDE	memmacro.inc

; 	Memory checking
@proto_memall59		TYPEDEF		PROTO C :bool
MemCheckOn		PROTO		@proto_memall59

@proto_memall60		TYPEDEF		PROTO C 
MemCheckOff		PROTO		@proto_memall60

; 	Heap management (memgrow.c)
@proto_memall61		TYPEDEF		PROTO C :SWORD
MemGrowHeap		PROTO		@proto_memall61

EXTERNDEF		C MemLockHeap: PTR PTR

EXTERNDEF		C MemUnlockHeap: PTR PTR

; 	Calling previous (underlying) allocators (only from top level!)
; 	Allocating conventional memory
; 	Caveat: since Malloc() can grab conventional memory, necessary
; 	conventional memory blocks should be grabbed early in program.
;  real mode segment to conventional mem block
;  protected mode selector for conv mem block
@t_memall7		TYPEDEF		FAR PTR 
;  protected mode ptr to mem block
@tag_memall0		STRUCT 1t
realSeg		ushort		?
protSel		ushort		?
protPtr		@t_memall7		?
@tag_memall0		ENDS

ConvMemBlock		TYPEDEF		@tag_memall0

@proto_memall62		TYPEDEF		PROTO C :ushort, :PTR ConvMemBlock
MallocConvMemBlock		PROTO		@proto_memall62

;  alloc
@proto_memall63		TYPEDEF		PROTO C :PTR ConvMemBlock, :ushort
ReallocConvMemBlock		PROTO		@proto_memall63

;  resize
@proto_memall64		TYPEDEF		PROTO C :PTR ConvMemBlock
FreeConvMemBlock		PROTO		@proto_memall64

;  free low memory block
; ////////////////////////////
; 
;  Dealing with a large block of memory as a stack for easy allocation
; 
;  Rationale: often, routines want some large amount of memory, and know they
;  will throw it away when they're done.  If you use a MemStack for this memory,
;  you don't have to worry about fragmenting the heap.
; 
;  To use, declare a MemStack.  Malloc n bytes of memory, put the resulting
;  pointer in baseptr and n in size, and call MemStackInit().  Then use
;  MemStackAlloc() and MemStackFree() to grab and release memory.  You must
;  free memory in the reverse order of allocating it, as this is a stack.
;  Note that you can have multiple MemStacks if you feel like it.
@t_memall8		TYPEDEF		PTR 
;  pointer to bottom of stack
;  size of stack in bytes
@t_memall9		TYPEDEF		PTR 
;  pointer to current top of stack (next free byte)
@tag_memall1		STRUCT 1t
baseptr		@t_memall8		?
sz		SDWORD		?
topptr		@t_memall9		?
@tag_memall1		ENDS

MemStack		TYPEDEF		@tag_memall1

@proto_memall65		TYPEDEF		PROTO C :PTR MemStack
MemStackInit		PROTO		@proto_memall65

@proto_memall66		TYPEDEF		PROTO C :PTR MemStack, :SDWORD
MemStackAlloc		PROTO		@proto_memall66

@proto_memall67		TYPEDEF		PROTO C :PTR MemStack, :PTR , :SDWORD
MemStackRealloc		PROTO		@proto_memall67

@proto_memall68		TYPEDEF		PROTO C :PTR MemStack, :PTR 
MemStackFree		PROTO		@proto_memall68

; End of file memall.h

ENDIF

