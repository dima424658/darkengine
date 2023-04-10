;
; $Source: r:/prj/lib/src/timer/rcs/tmfast.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:46:43 $
;
; Routine to set timer rate to that of fastest process.
;
; This file is part of the timer library.
;
; $Log: tmfast.asm $
; Revision 1.1  1993/05/20  15:46:43  kaboom
; Initial revision
; 

.386

include tmd.inc
include tmdecl.inc
include tmflag.inc
include tmlow.inc
include tmrec.inc
d equ dword ptr

_TEXT segment dword public use32 'CODE'
; finds the fastest currently active timer process and sets the rate
; of the timer to the rate of that process.
public tm_find_fastest_
tm_find_fastest_:
        push    ebx
        push    ecx
        push    edx
        ; look for a process that's faster than the current timer rate.
        mov     ebx,_tmd_lowmem_start
        mov     ecx,ebx
        add     ebx,TML_RECLIST_START
        mov     edx,TML_RECLIST_SIZE
        mov     eax,[ecx+TML_DENOM]     ;save smallest denom in eax
tff_find:
        ; compare active denominators to eax.
        test    d [ebx+TMR_STATUS],TMF_ACTIVE
        jz      tff_next
        cmp     [ebx+TMR_DENOM],eax
        jae     tff_next
        mov     eax,[ebx+TMR_DENOM]
tff_next:
        add     ebx,TMR_SIZE
        dec     edx
        jnz     tff_find
        ; only call tm_set_rate if new rate is faster.
        cmp     [ecx+TML_DENOM],eax
        jbe     tff_leave
        call    tm_set_rate_
tff_leave:
        pop     edx
        pop     ecx
        pop     ebx
        ret
_TEXT ends
end
