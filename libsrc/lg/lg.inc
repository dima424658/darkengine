
option expr32
option casemap:none

IFNDEF	lg_inc
lg_inc	EQU		1

; Begin of file lg.h
; 		LG.H		Looking Glass Over-Arching Master Control Header File
; 
; 
; * $Header: r:/prj/lib/src/lg/rcs/lg.inc 1.1 1995/05/25 07:00:48 LMFEENEY Exp $
; * $Log: lg.inc $
;Revision 1.1  1995/05/25  07:00:48  LMFEENEY
;Initial revision
;
;  * Revision 1.3  1993/08/06  11:00:11  rex
;  * Removed libdbg.h
;  * 
;  * Revision 1.2  1993/03/19  18:20:22  rex
;  * Added RCS header
;  * 
; 
	INCLUDE	types.inc

	INCLUDE	memall.inc

	INCLUDE	dbg.inc

; End of file lg.h

ENDIF

