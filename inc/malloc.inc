
option expr32
option casemap:none

IFNDEF	malloc_inc
malloc_inc	EQU		1

; Begin of file p:\w95\h\malloc.h
; 
;  *  malloc.h	Memory allocation functions
;  *
;  *  Copyright by WATCOM International Corp. 1988-1993.  All rights reserved.
;  
size_t		TYPEDEF		WORD

NULL		EQU		000000000h
@proto_malloc0		TYPEDEF		PROTO C :size_t
alloca		PROTO		@proto_malloc0

@proto_malloc1		TYPEDEF		PROTO C :size_t
__doalloca		PROTO		@proto_malloc1

@proto_malloc2		TYPEDEF		PROTO C 
stackavail		PROTO		@proto_malloc2

_HEAPOK		EQU		0t
;  heap isn't initialized 
_HEAPEMPTY		EQU		1t
;  heap header is corrupted 
_HEAPBADBEGIN		EQU		2t
;  heap entry is corrupted 
_HEAPBADNODE		EQU		3t
;  end of heap entries (_heapwalk) 
_HEAPEND		EQU		4t
;  invalid heap entry pointer (_heapwalk) 
_HEAPBADPTR		EQU		5t
_USEDENTRY		EQU		0t
_FREEENTRY		EQU		1t
@t_malloc0		TYPEDEF		PTR 
;  heap pointer 
;  heap entry size 
;  heap entry 'in-use' flag 
_heapinfo		STRUCT 1t
_pentry		@t_malloc0		?
_size		size_t		?
_useflag		SWORD		?
_heapinfo		ENDS

_HEAPINFO		TYPEDEF		_heapinfo

@proto_malloc3		TYPEDEF		PROTO C :SWORD
_heapenable		PROTO		@proto_malloc3

@proto_malloc4		TYPEDEF		PROTO C 
_heapchk		PROTO		@proto_malloc4

@proto_malloc5		TYPEDEF		PROTO C 
_nheapchk		PROTO		@proto_malloc5

@proto_malloc6		TYPEDEF		PROTO C 
_fheapchk		PROTO		@proto_malloc6

@proto_malloc7		TYPEDEF		PROTO C :WORD
_heapset		PROTO		@proto_malloc7

@proto_malloc8		TYPEDEF		PROTO C :WORD
_nheapset		PROTO		@proto_malloc8

@proto_malloc9		TYPEDEF		PROTO C :WORD
_fheapset		PROTO		@proto_malloc9

@proto_malloc10		TYPEDEF		PROTO C :PTR _heapinfo
_heapwalk		PROTO		@proto_malloc10

@proto_malloc11		TYPEDEF		PROTO C :PTR _heapinfo
_nheapwalk		PROTO		@proto_malloc11

@proto_malloc12		TYPEDEF		PROTO C :PTR _heapinfo
_fheapwalk		PROTO		@proto_malloc12

@proto_malloc13		TYPEDEF		PROTO C 
_heapgrow		PROTO		@proto_malloc13

@proto_malloc14		TYPEDEF		PROTO C 
_nheapgrow		PROTO		@proto_malloc14

@proto_malloc15		TYPEDEF		PROTO C 
_fheapgrow		PROTO		@proto_malloc15

@proto_malloc16		TYPEDEF		PROTO C 
_heapmin		PROTO		@proto_malloc16

@proto_malloc17		TYPEDEF		PROTO C 
_nheapmin		PROTO		@proto_malloc17

@proto_malloc18		TYPEDEF		PROTO C 
_fheapmin		PROTO		@proto_malloc18

@proto_malloc19		TYPEDEF		PROTO C 
_heapshrink		PROTO		@proto_malloc19

@proto_malloc20		TYPEDEF		PROTO C 
_nheapshrink		PROTO		@proto_malloc20

@proto_malloc21		TYPEDEF		PROTO C 
_fheapshrink		PROTO		@proto_malloc21

@proto_malloc22		TYPEDEF		PROTO C :size_t
__nmemneed		PROTO		@proto_malloc22

@proto_malloc23		TYPEDEF		PROTO C :size_t
__fmemneed		PROTO		@proto_malloc23

@proto_malloc24		TYPEDEF		PROTO C :size_t, :size_t
_fcalloc		PROTO		@proto_malloc24

@proto_malloc25		TYPEDEF		PROTO C :size_t, :size_t
_ncalloc		PROTO		@proto_malloc25

@proto_malloc26		TYPEDEF		PROTO C :PTR , :size_t
_expand		PROTO		@proto_malloc26

@proto_malloc27		TYPEDEF		PROTO C :PTR , :size_t
_fexpand		PROTO		@proto_malloc27

@proto_malloc28		TYPEDEF		PROTO C :PTR , :size_t
_nexpand		PROTO		@proto_malloc28

@proto_malloc29		TYPEDEF		PROTO C :PTR 
_ffree		PROTO		@proto_malloc29

@proto_malloc30		TYPEDEF		PROTO C :size_t
_fmalloc		PROTO		@proto_malloc30

@proto_malloc31		TYPEDEF		PROTO C :size_t
_freect		PROTO		@proto_malloc31

@proto_malloc32		TYPEDEF		PROTO C :SDWORD, :size_t
halloc		PROTO		@proto_malloc32

@proto_malloc33		TYPEDEF		PROTO C :PTR 
hfree		PROTO		@proto_malloc33

@proto_malloc34		TYPEDEF		PROTO C :PTR 
_nfree		PROTO		@proto_malloc34

@proto_malloc35		TYPEDEF		PROTO C :size_t
_nmalloc		PROTO		@proto_malloc35

@proto_malloc36		TYPEDEF		PROTO C :PTR , :size_t
_nrealloc		PROTO		@proto_malloc36

@proto_malloc37		TYPEDEF		PROTO C :PTR , :size_t
_frealloc		PROTO		@proto_malloc37

@proto_malloc38		TYPEDEF		PROTO C :PTR 
_msize		PROTO		@proto_malloc38

@proto_malloc39		TYPEDEF		PROTO C :PTR 
_nmsize		PROTO		@proto_malloc39

@proto_malloc40		TYPEDEF		PROTO C :PTR 
_fmsize		PROTO		@proto_malloc40

@proto_malloc41		TYPEDEF		PROTO C 
_memavl		PROTO		@proto_malloc41

@proto_malloc42		TYPEDEF		PROTO C 
_memmax		PROTO		@proto_malloc42

@proto_malloc43		TYPEDEF		PROTO C :size_t, :size_t
calloc		PROTO		@proto_malloc43

@proto_malloc44		TYPEDEF		PROTO C :PTR 
free		PROTO		@proto_malloc44

@proto_malloc45		TYPEDEF		PROTO C :size_t
malloc		PROTO		@proto_malloc45

@proto_malloc46		TYPEDEF		PROTO C :PTR , :size_t
realloc		PROTO		@proto_malloc46

; End of file p:\w95\h\malloc.h

ENDIF

