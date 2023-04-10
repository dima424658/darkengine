;
; $Source: r:/prj/lib/src/timer/rcs/tmdeact.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:46:32 $
;
; Routine to remove a timer process.
;
; This file is part of the timer library.
;
; $Log: tmdeact.asm $
; Revision 1.1  1993/05/20  15:46:32  kaboom
; Initial revision
; 

.386

include tmd.inc
include tmflag.inc
include tmlow.inc
include tmrec.inc
d equ dword ptr

_TEXT segment dword public use32 'CODE'
; takes process id in eax.
; clears the active status bit for the process with the given id.
public tm_deactivate_process_
tm_deactivate_process_:
        push    ebx
        lea     ebx,[eax+4*eax]         ;ebx = tmd_lowmem_start+20*id
        shl     ebx,2
        add     ebx,_tmd_lowmem_start
        and     d [ebx+TML_RECLIST_START+TMR_STATUS],not TMF_ACTIVE
        pop     ebx
        ret
_TEXT ends
end

