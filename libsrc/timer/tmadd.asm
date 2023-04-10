;
; $Source: r:/prj/lib/src/timer/rcs/tmadd.asm $
; $Revision: 1.3 $
; $Author
; $Date: 1994/10/17 17:41:18 $
;
; Routine to add another timer function.
;
; This file is part of the timer library.
;
; $Log: tmadd.asm $
; Revision 1.3  1994/10/17  17:41:18  lmfeeney
; name change for compatibility, tm_add_process is table lookup
; 
; Revision 1.2  1993/05/21  12:43:12  kaboom
; Fixed incorrect order of pops,
; 
; Revision 1.1  1993/05/20  15:46:00  kaboom
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
; arguments:
;   eax: pointer to protected mode timer function.
;   edx: segment/offset of real mode function.
;   ebx: frequency denominator.
; returns:
;   eax: id of new timer or -1 if error.
public TF_tm_add_process_
TF_tm_add_process_:
        push    ecx
        push    esi
        push    edi
        ; check if we have a space for a new process.
        cmp     _tmd_installed_procs,TML_RECLIST_SIZE
        jge     taf_no_slots
        inc     d _tmd_installed_procs
        mov     ecx,_tmd_lowmem_start   ;save ptr to low buf
        ; find the first available process slot.
        mov     esi,ecx                 ;esi->process 0's record
        add     esi,TML_RECLIST_START
        xor     edi,edi                 ;edi=current process' id
taf_find_free:
        ; loop until we find a slot with INUSE flag=0.
        test    d [esi+TMR_STATUS],TMF_INUSE
        jz      taf_found_free
        add     esi,TMR_SIZE
        inc     edi
        cmp     edi,TML_RECLIST_SIZE
        jne     taf_find_free
taf_found_free:
        push    edi                     ;save process id
        ; initialize fields of new process' record.
        mov     d [esi+TMR_STATUS],TMF_INUSE or TMF_ACTIVE
        mov     [esi+TMR_COUNT],ebx
        mov     [esi+TMR_DENOM],ebx
        mov     [esi+TMR_REAL_FUNC],edx
        mov     [esi+TMR_PROT_FUNC],eax
        call    tm_find_fastest_
taf_same_rate:
        pop     eax                     ;get id of new process
taf_leave:
        pop     edi
        pop     esi
        pop     ecx
        ret
taf_no_slots:
        mov     eax,-1
        jmp     taf_leave
_TEXT ends
end
