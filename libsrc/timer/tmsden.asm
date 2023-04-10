;
; $Source: r:/prj/lib/src/timer/rcs/tmsden.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:47:42 $
;
; Routine to set the denominator for a given process.
;
; This file is part of the timer library.
;
; $Log: tmsden.asm $
; Revision 1.1  1993/05/20  15:47:42  kaboom
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
; takes process id in eax, denominator in edx.
; sets the magic timer denominator of the given process.
public tm_set_process_denom_
tm_set_process_denom_:
        ; first check if this is a legal process number.
        cmp     eax,TML_RECLIST_SIZE
        jae     tspd_bad_id
        push    ebx
        push    ecx
        lea     ebx,[eax+4*eax]         ;ebx=tmd_lowmem_start+20*id
        shl     ebx,2
        add     ebx,_tmd_lowmem_start
        ; now modify process' denominator and recheck to find the
        ; fastest process.
        cli
        mov     [ebx+TML_RECLIST_START+TMR_DENOM],edx
        call    tm_find_fastest_
        sti
        xor     eax,eax                 ;no errors
tspd_leave:
        pop     ecx
        pop     ebx
        ret
tspd_bad_id:
        ; just return, no regs saved or modified.
        mov     eax,1
        ret
_TEXT ends
end
