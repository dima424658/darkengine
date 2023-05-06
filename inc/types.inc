
option expr32
option casemap:none

IFNDEF	types_inc
types_inc	EQU		1

; Begin of file types.h
; 
;  * $Source: t:/prj/tech/hsrc/RCS/types.inc $
;  * $Revision: 1.1 $
;  * $Author: LMFEENEY $
;  * $Date: 1995/05/25 15:55:49 $
;  *
;  * extra typedefs and macros for use by all code.
;  *
;  * $Log: types.inc $
;Revision 1.1  1995/05/25  15:55:49  LMFEENEY
;Initial revision
;
;  * Revision 1.2  1993/09/28  01:12:47  kaboom
;  * Converted #include "xxx" to #include <xxx> for watcom.
;  * 
;  * Revision 1.1  1993/03/19  18:19:27  matt
;  * Initial revision
;  
; don't redefine byte in assembly header
;  these are convenience typedefs so we don't always have to keep typing
;    `unsigned.' 
uchar		TYPEDEF		BYTE

ushort		TYPEDEF		WORD

uint		TYPEDEF		WORD

ulong		TYPEDEF		DWORD

bool		TYPEDEF		BYTE

ubyte		TYPEDEF		BYTE

NULL		EQU		0t
;  !NULL 
TRUE		EQU		1t
;  !TRUE 
FALSE		EQU		0t
;  !FALSE 
;  !__TYPES_H 
; End of file types.h

ENDIF

