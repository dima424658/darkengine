;
; $Source: r:/prj/lib/src/timer/rcs/tminit.asm $
; $Revision: 1.4 $
; $Author
; $Date: 1994/10/17 17:37:39 $
;
; Initialization routine for timer handler.
;
; This file is part of the timer library.
;
; $Log: tminit.asm $
; Revision 1.4  1994/10/17  17:37:39  lmfeeney
; renamed to _libt, deleted tmd_ticks
; 
; Revision 1.3  1994/02/02  11:45:44  kaboom
; Changed interrupt get\set code to use dos calls instead of dpmi.
; 
; Revision 1.2  1993/07/01  15:28:34  kaboom
; Now clears high 16 bits of register after dpmi alloc lowmem call.
; 
; Revision 1.1  1993/05/20  15:46:59  kaboom
; Initial revision
; 

.386

include type.inc
include dpmi.inc
include tmd.inc
include tmdecl.inc
include tmflag.inc
include tmhnd.inc
include tmlow.inc
include tmreg.inc
include tmrec.inc
include tmftab.inc

_DATA segment dword public use32 'DATA'
_DATA ends

_TEXT segment para public use32 'CODE'
; start up the timer handler.  allocate the low memory buffer and
; install protected mode interrupt handler.
public tm_init_libt_
tm_init_libt_:
        push    ebx
        push    ecx
        push    edx
        push    esi
        push    edi
	; set function table
	mov 	ebx,p _tm_ftab_libt
	mov 	_tm_ftab,ebx
        ; allocate low memory buffer.
        mov     bx,(TML_LOWBUF_SIZE+15)/16
        dpmi_alloc_lowmem bx
        jc      ti_bad_alloc
        ; save selector & convert real segment to a pointer.
        mov     _tmd_lowmem_sel,dx
        and     eax,00000ffffh
        shl     eax,4
        mov     _tmd_lowmem_start,eax
        ; zero out new low buffer.
        mov     esi,eax
        mov     edi,eax
        mov     ecx,(TML_LOWBUF_SIZE+3)/4
        xor     eax,eax
        rep     stosd

        mov     ax,3500h or TMD_INT
        int     21h
        mov     w _tmd_old_prot_handler+4,es
        mov     d _tmd_old_prot_handler,ebx
        mov     ax,_DATA
        mov     es,ax

        ; save old interrupt vector addresses.
;        dpmi_get_prot_intvec TMD_INT
;        mov     w _tmd_old_prot_handler+4,cx
;        mov     d _tmd_old_prot_handler,edx
;        dpmi_get_real_intvec TMD_INT
;        shl     ecx,10h
;        or      cx,dx
;        mov     d [esi+TML_OLD_REAL_HANDLER],ecx
        ; initialize low buffer globals.
        mov     eax,TMD_DEF_DENOM
        mov     [esi+TML_DENOM],eax
        mov     [esi+TML_NEW_DENOM],eax
        ; set up bios chain record.
        mov     d [esi+TML_RECLIST_LAST+TMR_STATUS],TMF_INUSE or TMF_ACTIVE
        mov     [esi+TML_RECLIST_LAST+TMR_COUNT],eax
        mov     [esi+TML_RECLIST_LAST+TMR_DENOM],eax
        mov     d [esi+TML_RECLIST_LAST+TMR_PROT_FUNC],tph_chain_
        mov     [esi+TML_RECLIST_LAST+TMR_REAL_FUNC],ecx

; no tmd_ticks here
;        add     esi,TML_TICKS
;        mov     _tmd_ticks,esi

        ; set new interrupt vector.
;        dpmi_set_prot_intvec TMD_INT,cs,tm_prot_handler_
;        jc      ti_bad_prot_hook

        dpmi_lock_block tm_prot_handler_,_tmd_prot_handler_size

        mov     ax,2500h or TMD_INT
        mov     edx,p tm_prot_handler_
        mov     bx,_TEXT
        mov     ds,bx
        int     21h
        mov     bx,_DATA
        mov     ds,bx

        ; set timer to mode 2, rate generator.
        cli
        mov     al,TMM_RATEGEN
        out     TMP_MODE,al
        xor     eax,eax
        out     TMP_CHAN0,al
        jmp     $+2
        mov     al,ah
        out     TMP_CHAN0,al
        sti
        xor     eax,eax
ti_leave:
        pop     edi
        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        ret
ti_bad_alloc:
        mov     eax,1
        jmp     ti_leave
ti_bad_prot_hook:
        mov     eax,2
        jmp     ti_leave
_TEXT ends
end
