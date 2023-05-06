;
; $Source: r:/prj/lib/src/dpmi/rcs/dpmi.inc $
; $Revision: 1.6 $
; $Author: ept $
; $Date: 1995/04/13 11:32:22 $
;
; Macro definitions and data structures for DPMI calls.
;

ifndef __DPMI_INC
__DPMI_INC equ 1
DPMI_INT        equ 31h

; format of real mode register structure for DPMI calls.
DPMIS_REG_EDI   equ 0
DPMIS_REG_ESI   equ 4
DPMIS_REG_EBP   equ 8
; reserved
DPMIS_REG_EBX   equ 10h
DPMIS_REG_EDX   equ 14h
DPMIS_REG_ECX   equ 18h
DPMIS_REG_EAX   equ 1ch
DPMIS_REG_FLAGS equ 20h
DPMIS_REG_ES    equ 22h
DPMIS_REG_DS    equ 24h
DPMIS_REG_FS    equ 26h
DPMIS_REG_GS    equ 28h
; ip reserved       2ah
; cs reserved       2ch
DPMIS_REG_SP    equ 2eh
DPMIS_REG_SS    equ 30h
DPMIS_REG_SIZE  equ 32h

; all DPMI calls exit with the carry set if there is an error.  if
; there is an error, an extended error code is sometimes returned in
; ax.  here are the definitions for the extended error codes that may
; be returned by the DPMI calls defined in this file.
DPMIE_UNSUP_FUNC        equ     8001h   ;unsupported function
DPMIE_INVL_STATE        equ     8002h   ;invalid state
DPMIE_SYS_INTEG         equ     8003h   ;system integrity
DPMIE_DEADLOCK          equ     8004h   ;deadlock
DPMIE_REQ_CANCEL        equ     8005h   ;request cancelled
DPMIE_RES_UNAVAUL       equ     8010h   ;resource unavailable
DPMIE_DESC_UNAVAIL      equ     8011h   ;descriptor unavailable
DPMIE_LIN_UNAVAIL       equ     8012h   ;linear address unavailable
DPMIE_PHYS_UNAVAIL      equ     8013h   ;physical memory unavailable
DPMIE_BACK_UNAVAIL      equ     8014h   ;backing store unavailable
DPMIE_CALL_UNAVAIL      equ     8015h   ;callback unavailable
DPMIE_HAND_UNAVAIL      equ     8016h   ;handle unavailable
DPMIE_LOCK_COUNT        equ     8017h   ;lock count exceeded
DPMIE_RES_OWNED         equ     8018h   ;resource owned exclusively
DPMIE_RES_SHARE         equ     8019h   ;resource owned shared
DPMIE_INVL_VALUE        equ     8021h   ;invalid value
DPMIE_INVL_SELECTOR     equ     8022h   ;invalid selector
DPMIE_INVL_HANDLE       equ     8023h   ;invalid handle
DPMIE_INVL_CALL         equ     8024h   ;invalid callback
DPMIE_INVL_LINADR       equ     8025h   ;invalid linear address
DPMIE_INVL_REQ          equ     8026h   ;request not supported

dpmi_get_base macro sel
ifdifi <sel>,<bx>
        mov     bx,sel
endif
        mov     ax,6
        int     DPMI_INT
        endm

; allocate specified number of paragraphs of low memory.  returns new
; base selector in dx and real segment in ax.  if there is an error,
; ax has the DOS error code and bx has how many paragraphs are free.
; thrashes ax,bx,dx.
dpmi_alloc_lowmem macro paragraphs
ifdifi <paragraphs>,<bx>
        mov     bx,paragraphs
endif
        mov     ax,0100h
        int     DPMI_INT
        endm

; free block of low memory associated with the given base selector.
; if error, code is returned in ax.
; thrashes ax,dx.
dpmi_free_lowmem macro selector
ifdifi <selector>,<dx>
        mov     dx,selector
endif
        mov     ax,0101h
        int     DPMI_INT
        endm

; resize a block of low memory specified by <selector> to <size>.
; if error, ax is DOS error code, bx is paragraphs free.
; thrashes ax,bx,dx.
dpmi_resize_lowmem macro size,selector
ifdifi <size>,<bx>
        mov     bx,size
endif
ifdifi <selector>,<dx>
        mov     dx,selector
endif
        mov     ax,0102h
        int     DPMI_INT
        endm

; returns the segment:offset of the real mode interrupt handler for
; <n_int> in cx:dx.  this call always succeeds, they say.
; thrashes ax,bl,cx,dx.
dpmi_get_real_intvec macro n_int
ifdifi <n_int>,<bl>
        mov     bl,n_int
endif
        mov     ax,0200h
        int     DPMI_INT
        endm

; sets the real-mode interrupt handler for <n_int> to <h_seg>:<h_off>.
; if error, no code is returned.
; thrashes ax,bl,cx,dx.
dpmi_set_real_intvec macro n_int,h_seg,h_off
ifdifi <n_int>,<bl>
        mov     bl,n_int
endif
ifdifi <h_seg>,<cx>
        mov     cx,h_seg
endif
ifdifi <h_off>,<dx>
        mov     dx,h_off
endif
        mov     ax,0201h
        int     DPMI_INT
        endm

; returns the selector:offset of the handler for processor exception
; number <n_exc>.  returns an error if n_exc is not from 0-1f or if this
; function isn't implemented (as is the case with dos4gw).
; thrashes ax,bl,cx,edx.
dpmi_get_except_vec macro n_exc
ifdifi <n_exc>,<bl>
        mov     bl,n_exc
endif
        mov     ax,0202h
        int     DPMI_INT
        endm

; sets the exception handler for processor exception number <n_exc> to be
; the routine at <h_sel>:<h_off>.
; thrashes ax,bl,cx,edx.
dpmi_set_except_vec macro n_exc,h_sel,h_off
ifdifi <n_exc>,<bl>
        mov     bl,n_exc
endif
ifdifi <h_sel>,<cx>
        mov     cx,h_sel
endif
ifdifi <h_off>,<edx>
        mov     edx,h_off
endif
        mov     ax,0203h
        int     DPMI_INT
        endm

; returns the selector:offset of the protected mode interrupt handler
; for int <n_int> in cx:edx.  this call always succeeds.
; thrashes ax,bl,cx,edx.
dpmi_get_prot_intvec macro n_int
ifdifi <n_int>,<bl>
        mov     bl,n_int
endif
        mov     ax,0204h
        int     DPMI_INT
        endm

; sets the protected mode interrupt handler for int <n_int> to be at
; <h_sel>:<h_off>.  if error, no code is returned.
; thrashes ax,bl,cx,edx.
dpmi_set_prot_intvec macro n_int,h_sel,h_off
ifdifi <n_int>,<bl>
        mov     bl,n_int
endif
ifdifi <h_sel>,<cx>
        mov     cx,h_sel
endif
ifdifi <h_off>,<edx>
        mov     edx,h_off
endif
        mov     ax,0205h
        int     DPMI_INT
        endm

; simulates a real mode interrupt.  copies <s_words> words off the
; protected mode stack to the real mode stack.  loads the registers
; with the values in the buffer at <b_sel>:<b_off>.  calls interrupt
; <n_int> after switching to real mode.
; thrashes ax,bl,cx,edi, and es.
dpmi_real_interrupt macro n_int,s_words,b_sel,b_off
ifdifi <n_int>,<bl>
        mov     bl,n_int
endif
ifdifi <s_words>,<cx>
        mov     cx,s_words
endif
ifdifi <b_sel>,<es>
        mov     es,b_sel
endif
ifdifi <b_off>,<edi>
        mov     edi,b_off
endif
        mov     ax,0300h
        xor     bh,bh
        int     DPMI_INT
        endm

; locks a range of linear addresses from being paged by the dpmi host.
; the pages which include the memory block at linear address <adr> of
; length <siz> bytes are locked into memory if the host virtual memory
; is supported by the host.
; args: siz can't be in ebx or ecx
; thrashes: ax,ebx,ecx,esi,edi.
; errors: DPMIE_PHYS_UVAIL, DPMIE_LOCK_COUNT, DPMIE_INVL_LINADR
dpmi_lock_block macro adr,siz
ifdifi <adr>,<ebx>
        mov     ebx,adr
endif
ifdifi <adr>,<ecx>
        mov     ecx,ebx
endif
        shr     ebx,010h
        and     ecx,0ffffh
ifdifi <siz>,<esi>
        mov     esi,siz
endif
ifdifi <siz>,<edi>
        mov     edi,esi
endif
        shr     esi,010h
        and     edi,0ffffh
        mov     ax,0600h
        int     DPMI_INT
        endm

; unlocks a previously lock range of linear addresses.
dpmi_unlock_block macro adr,siz
; args: siz can't be in ebx or ecx
; thrashes: ax,ebx,ecx,esi,edi.
; errors: DPMIE_INVL_STATE, DPMIE_INVL_LINADR
ifdifi <adr>,<ebx>
        mov     ebx,adr
endif
ifdifi <adr>,<ecx>
        mov     ecx,ebx
endif
        shr     ebx,16
        and     ebx,0ffffh
ifdifi <siz>,<esi>
        mov     esi,siz
endif
ifdifi <siz>,<edi>
        mov     edi,esi
endif
        shr     esi,16
        and     edi,0ffffh
        mov     ax,0601h
        int     DPMI_INT
        endm

; map a physical address to a linear address

dpmi_map_physical_mem macro adr,siz,val
; args default to ecx,edi
; value defaults to ebx
; thrashes: ax,ebx,ecx,edi,esi
; errors: DPMIE_SYS_INTEG, DPMIE_INVL_VALUE

ifdifi <adr>,<ecx>
        mov     ecx,adr
endif
ifdifi <siz>,<edi>
        mov     edi,siz
endif
        mov     ebx,ecx
        mov     esi,edi
        shr     ebx,10h
        shr     esi,10h
        mov     ax,0800h
        int     DPMI_INT
        jc      @F
        shl     ebx,10h         ; bx:cx contains linear address
        mov     bx,cx
        add     ebx,0           ; to clear the carry flag
ifdifi <val>,<ebx>
        mov     val,ebx
endif
@@:
        endm

; free the physical adress previously mapped

dpmi_free_physical_mem macro adr
; arg defaults to ecx
; thrashes: ax,ebx,ecx
; errors: DPMIE_INVL_LINADR

ifdifi <adr>,<ecx>
        mov     ecx,adr
endif
        mov     ebx,ecx
        shr     ebx,10h
        mov      ax,0801h
        int     DPMI_INT

        endm

endif ;ndef __DPMI_INC
