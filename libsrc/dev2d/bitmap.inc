;
; $Source: s:/prj/tech/libsrc/dev2d/RCS/bitmap.inc $
; $Revision: 1.2 $
; $Author: KEVIN $
; $Date: 1996/04/11 09:28:11 $
;
; Symbolic constants for bitmap types&flags.
;
; This file is part of the dev2d library.
;

ifndef bitmap_inc
bitmap_inc equ 1

; bitmap types. 
BMT_MODEX       equ     0t  
BMT_MONO        equ     1t
BMT_FLAT8       equ     2t
BMT_FLAT16      equ     3t
BMT_RSD8        equ     4t
BMT_FLAT24      equ     5t
BMT_BANK8       equ     6t
BMT_BANK16      equ     7t
BMT_BANK24      equ     8t
BMT_TLUC8       equ     9t
BMT_TYPES       equ     10t


;  bitmap flags. 
BMF_TRANS       equ     1t       
BMF_TLUC8       equ     2t       
                                 
endif   ;ndef bitmap_inc         
                                 
                                 
                                 
                                 
                                 
                                 
                                 
