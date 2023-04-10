;
; $Source: r:/prj/lib/src/timer/rcs/tmphnd.asm $
; $Revision: 1.3 $
; $Author
; $Date: 1994/10/17 17:43:13 $
;
; Shutdown routine for timer handler.
;
; This file is part of the timer library.
;
; $Log: tmphnd.asm $
; Revision 1.3  1994/10/17  17:43:13  lmfeeney
; bug/fix, need to save eax between call's, use low mem
; 
; Revision 1.2  1994/02/02  11:43:09  kaboom
; Added handler size variable for locking.
; 
; Revision 1.1  1993/05/20  15:47:08  kaboom
; Initial revision
; 

.386

include type.inc
include pic.inc
include tmd.inc
include tmflag.inc
include tmlow.inc
include tmrec.inc

_DATA segment dword public use32 'DATA'
public _tmd_prot_handler_size
_tmd_prot_handler_size   dd tm_prot_handler_end-tm_prot_handler_
_DATA ends

; protected mode irq0 handler.  polls all currently installed timer
; routines and calls them if necessary.
_TEXT segment dword public use32 'CODE'
public tm_prot_handler_
tm_prot_handler_:
        ; fire up the handler---allow furthur interrupts, stack used
        ; registers, set up the data segment.
        sti
        push    eax
        push    ebx
        push    ds
        push    ecx
        push    edx
        push    esi
        mov     ax,_DATA
        mov     ds,ax
        mov     eax,_tmd_lowmem_start
        mov     esi,eax
        ; if the frequency has been changed since last time, it will
        ; only take effect after this interrupt has been handled.
        mov     ebx,[eax+TML_DENOM]
        mov     edx,[eax+TML_NEW_DENOM]
        mov     [eax+TML_DENOM],edx
        ; load regs for loop.
        mov     edx,TML_RECLIST_SIZE
        add     eax,TML_RECLIST_START
tph_process_timer:
        ; check if we need to process this timer.
        test    d [eax+TMR_STATUS],TMF_ACTIVE
        jz      tph_next_timer
        ; adjust count for this timer & call func if needed.
        mov     ecx,[eax+TMR_COUNT]
        cmp     ebx,[eax+TMR_DENOM]
        ja      tph_next_timer
        sub     ecx,ebx
        ja      tph_save_count
        add     ecx,[eax+TMR_DENOM]     ;time to call
        mov     [esi+TML_CUR_REC],eax
        call    d [eax+TMR_PROT_FUNC]
        mov     eax,[esi+TML_CUR_REC]
tph_save_count:
        mov     [eax+TMR_COUNT],ecx
tph_next_timer:
        ; advance to the next timer.
        add     eax,TMR_SIZE
        dec     edx
        jnz     tph_process_timer
tph_leave:
        ; clear stack of everything but eax.
        pop     esi
        pop     edx
        pop     ecx
        pop     ds
        pop     ebx
        ; finish off irq and return.
        cli
        mov     al,ICC_EOI
        out     ICP_COMM,al
        pop     eax
        iretd

        ; called from timer func loop to chain to previous handler.
public tph_chain_
tph_chain_:
        ; since we don't return, we have to set the count here.
        mov     [eax+TMR_COUNT],ecx
        ; clear stack except for last 2 regs & load previous handler.
        add     esp,4                   ;punt return address
        pop     esi
        pop     edx
        pop     ecx
        movzx   eax,w _tmd_old_prot_handler+4
        mov     ebx,d _tmd_old_prot_handler
        pop     ds
        ; poke handler's address into our return address and return.
        xchg    ebx,[esp]
        xchg    eax,[esp+4]
        cli
        retf
        nop
tm_prot_handler_end:
_TEXT ends
end
