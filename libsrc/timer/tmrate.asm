;
; $Source: r:/prj/lib/src/timer/rcs/tmrate.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:47:22 $
;
; Timer rate set routine.
;
; This file is part of the timer library.
;
; $Log: tmrate.asm $
; Revision 1.1  1993/05/20  15:47:22  kaboom
; Initial revision
; 

.386

include tmd.inc
include tmlow.inc
include tmreg.inc

_TEXT segment dword public use32 'CODE'
; takes divisor in eax
; sets the rate of the PIT.
public tm_set_rate_
tm_set_rate_:
        push    ebx
        mov     ebx,_tmd_lowmem_start
        mov     [ebx+TML_NEW_DENOM],eax
        cli
        out     TMP_CHAN0,al
        mov     al,ah
        out     TMP_CHAN0,al
        sti
        pop     ebx
        ret
_TEXT ends
end
