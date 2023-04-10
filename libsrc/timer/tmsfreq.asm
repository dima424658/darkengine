;
; $Source: r:/prj/lib/src/timer/rcs/tmsfreq.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:47:54 $
;
; Routine to set the frequency for a given process.
;
; This file is part of the timer library.
;
; $Log: tmsfreq.asm $
; Revision 1.1  1993/05/20  15:47:54  kaboom
; Initial revision
; 

.386

include tmd.inc
include tmdecl.inc
include tmflag.inc
include tmlow.inc
include tmrec.inc
include tmreg.inc
d equ dword ptr

_TEXT segment dword public use32 'CODE'
; takes process id in eax, frequency in Hz in edx.
public tm_set_process_freq_
tm_set_process_freq_:
        ; first check if this is a legal process number.
        cmp     eax,TML_RECLIST_SIZE
        jae     tspf_bad_id
        push    ebx
        push    ecx
        push    edx
        mov     ecx,eax                 ;ecx=process id
        ; calculate process' new denominator from frequency.
        mov     ebx,edx                 ;eax=TMD_FREQ/frequency
        mov     eax,TMD_FREQ
        xor     edx,edx
        div     ebx
        lea     ebx,[ecx+4*ecx]         ;ebx=tmd_lowmem_start+20*id
        shl     ebx,2
        add     ebx,_tmd_lowmem_start
        ; now modify process' denominator and recheck to find the
        ; fastest process.
        cli
        mov     [ebx+TML_RECLIST_START+TMR_DENOM],eax
        call    tm_find_fastest_
        sti
        xor     eax,eax                 ;no errors
tspf_leave:
        pop     edx
        pop     ecx
        pop     ebx
        ret
tspf_bad_id:
        mov     eax,1                   ;return error
        ret
_TEXT ends
end
