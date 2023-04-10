;
; $Source: r:/prj/lib/src/timer/rcs/tmact.asm $
; $Revision: 1.1 $
; $Author
; $Date: 1993/05/20 15:45:49 $
;
; Routine to add another timer function.
;
; This file is part of the timer library.
;
; $Log: tmact.asm $
; Revision 1.1  1993/05/20  15:45:49  kaboom
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
; sets the active status bit for the process with the given id.
public tm_activate_process_
tm_activate_process_:
        push    ebx
        lea     ebx,[eax+4*eax]         ;ebx = tmd_lowmem_start+20*id
        shl     ebx,2
        add     ebx,_tmd_lowmem_start
        ; set the active status flag.
        or      d [ebx+TML_RECLIST_START+TMR_STATUS],TMF_ACTIVE
        xor     eax,eax
        pop     ebx
        ret
_TEXT ends
end
