;
; $Source: r:/prj/lib/src/timer/rcs/tmrem.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:47:33 $
;
; Routine to remove a timer process.
;
; This file is part of the timer library.
;
; $Log: tmrem.asm $
; Revision 1.1  1993/05/20  15:47:33  kaboom
; Initial revision
; 

.386

include tmd.inc
include tmflag.inc
include tmlow.inc

_TEXT segment dword public use32 'CODE'
; takes id of timer process to remove.
public tm_remove_process_
tm_remove_process_:
        push    ebx
        push    ecx
        lea     ebx,[eax+4*eax]         ;ebx=tmd_lowmem+20*id
        shl     ebx,2
        add     ebx,_tmd_lowmem_start
        ; first check if this process is allocated.
        mov     ecx,[ebx+TML_RECLIST_START]
        test    ecx,TMF_INUSE
        jz      trf_leave
        ; if it is, clear its inuse _and_ active bits.
        and     ecx,not (TMF_INUSE or TMF_ACTIVE)
        mov     [ebx+TML_RECLIST_START],ecx
        dec     _tmd_installed_procs    ;one fewer process
trf_leave:
        pop     ecx
        pop     ebx
        ret
_TEXT ends
end
