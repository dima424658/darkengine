;
; $Source: r:/prj/lib/src/timer/rcs/tmd.asm $
; $Revision: 1.2 $
; $Author
; $Date: 1994/10/17 17:34:23 $
;
; Timer system global variables.
;
; This file is part of the timer library.
;
; $Log: tmd.asm $
; Revision 1.2  1994/10/17  17:34:23  lmfeeney
; tmd_ticks removed to tm_realtime
; 
; Revision 1.1  1993/05/20  15:46:18  kaboom
; Initial revision
; 

.386

include tmd.inc
include tmlow.inc

_DATA segment dword public use32 'DATA'
; selector for shared low memory buffer.
_tmd_lowmem_sel         dw ?

; pointer to start of shared low memory buffer.
_tmd_lowmem_start       dd ?

; pointer to old protected mode int 8 handler.
_tmd_old_prot_handler   df ?

; how many processes we have running.
_tmd_installed_procs    dd ?

_DATA ends
end
