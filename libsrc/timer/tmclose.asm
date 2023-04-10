;
; $Source: r:/prj/lib/src/timer/rcs/tmclose.asm $
; $Revision: 1.3 $
; $Author
; $Date: 1994/10/17 17:38:17 $
;
; Shutdown routine for timer handler.
;
; This file is part of the timer library.
;
; $Log: tmclose.asm $
; Revision 1.3  1994/10/17  17:38:17  lmfeeney
; renamed to _libt, deleted tmd_ticks
; 
; Revision 1.2  1994/02/02  11:46:06  kaboom
; Changed interrupt get\set code to use dos calls instead of dpmi.
; 
; Revision 1.1  1993/05/20  15:46:06  kaboom
; Initial revision
; 

.386

include type.inc
include dpmi.inc
include tmd.inc
include tmhnd.inc
include tmreg.inc

_DATA segment dword public use32 'DATA'
_DATA ends

_TEXT segment para public use32 'CODE'
; shutdown the timer library. restores the old timer interrupt vector,
; rate, and mode.
public tm_close_libt_
tm_close_libt_:
        push    ebx
        push    ecx
        push    edx
        ; set timer mode to squarewave generator and denominator to
        ; zero (65536).
        cli
        mov     al,TMM_SQUARE
        out     TMP_MODE,al
        xor     eax,eax
        out     TMP_CHAN0,al
        jmp     $+2
        mov     al,ah
        out     TMP_CHAN0,al
        sti

        mov     ax,2500h or TMD_INT
        mov     edx,d _tmd_old_prot_handler
        mov     ds,w _tmd_old_prot_handler+4
        int     21h
        mov     ax,_DATA
        mov     ds,ax

        dpmi_unlock_block tm_prot_handler_,_tmd_prot_handler_size

        ; restore old interrupt handler.
;        mov     cx,w _tmd_old_prot_handler+4
;        mov     edx,d _tmd_old_prot_handler
;        dpmi_set_prot_intvec TMD_INT,cx,edx
;        jc tc_bad_set
        ; release low memory buffer.
        dpmi_free_lowmem _tmd_lowmem_sel
        jc      tc_bad_free
        xor     eax,eax
tc_leave:
        pop     edx
        pop     ecx
        pop     ebx
        ret
tc_bad_set:
        mov     eax,1
        jmp     tc_leave
tc_bad_free:
        mov     eax,2
        jmp     tc_leave
_TEXT ends
end
