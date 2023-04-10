;                                                     
; $Source: x:/prj/tech/libsrc/g2/RCS/chkedgeb.asm $
; $Revision: 1.2 $
; $Author: TOML $
; $Date: 1996/10/16 16:07:33 $
;
; Procedures to check sloppy edge extent.
;
; This file is part of the g2 library.
;

.386
include cseg.inc
include dseg.inc
include fix.inc
include plytyp.inc
include type.inc

assume ds:_DATA
_DATA segment
_DATA ends

_TEXT segment

        align 16
; eax new vertex, edx old vertex, ebx ptr to min y list
; returns eax=1 if height buffer is changed, 0 otherwise.
public gri_check_edge_y_
gri_check_edge_y_:
        pushad                                  ;1
        mov     edi,[eax]                       ;2
        mov     esi,[edx]                       ;2

        mov     ecx,GRS_VERTEX_X[edi]           ;3 x_new
        mov     ebp,GRS_VERTEX_X[esi]           ;3 x_old
        mov     edx,ecx                         ;2
        add     edx,0ffffh                      ;6
        sar     edx,010h                        ;3
        lea     ebx,[ebx+4*edx]                 ;3
        mov     d ds:nyn_read-4,ebx             ;5
        sub     ecx,ebp                         ;2 delta_x 
        mov     ebx,ebp
        neg     ebp
        and     ebp,0ffffh
        add     ebx,0ffffh
        sar     ebx,010h
        sub     edx,ebx
        jle     nyn_end                         ;2
        mov     esi,GRS_VERTEX_Y[esi]           ;3 
        mov     eax,GRS_VERTEX_Y[edi]           ;2 y_new
        mov     edi,edx                         ;2
        sal     edi,2                           ;3
        neg     edi                             ;2 edi=p_offset
        sub     eax,esi                         ;2 delta_y
        jz      sce_skip_div                    ;2
        cdq                                     ;1
        mov     ebx,edx                         ;2
        inc     ebx                             ;1
        fixdiv  ecx                             ;2
        add     eax,ebx                         ;2
        add     esi,ebx                         ;2
sce_skip_div:
        mov     ecx,eax                         ;2
        add     esi,0ffffh
        fixmul  ebp
        add     esi,eax
        mov     ebx,esi                         ;2
        sal     ebx,010h                        ;6 ebx=frac_y
        sar     esi,010h                        ;3 esi=floor_y
        mov     edx,ecx                         ;2
        sal     ecx,010h                        ;3
        sar     edx,010h                        ;3
        sub     ebx,ecx                         ;2 fixup for pre-increment
        sbb     esi,edx                         ;2
        mov     ebp,4                           ;4
        jmp     nyn_loop                     ;2


;eax=dummy
;ebx=frac_y
;ecx=frac_dy
;edx=floor_dy
;esi=floor_y
;edi=p_offset
;ebp=4

        align   16
nyn_loop:
        add     ebx,ecx                 ;2
        mov     eax,0deadbeefh[edi]     ;6
nyn_read:
        adc     esi,edx                 ;2
        cmp     eax,esi                 ;2
        jg      nyn_punt                ;2
        add     edi,ebp                 ;2
        jnz     nyn_loop                ;2

nyn_end:
        popad
        xor     eax,eax
        ret
nyn_punt:
        popad
        mov     eax,1
        ret

_TEXT ends
end














                               
