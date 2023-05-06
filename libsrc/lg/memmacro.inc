;
; $Source: r:/prj/lib/src/lg/rcs/memmacro.inc $
; $Revision: 1.1 $
; $Author: matt $
; $Date: 1993/03/24 12:01:30 $
;
; Macros for doing memory allocates using Rex's malloc system  
;
; $Log: memmacro.inc $
; Revision 1.1  1993/03/24  12:01:30  matt
; Initial revision
; 
; 
;

;allocate memory through function pointer
_malloc	macro
	call	[f_malloc]
	endm

;free memory through function pointer
_free	macro
	call	[f_free]
	endm

;reallocate memory through function pointer
_realloc	macro
	call	[f_realloc]
	endm

