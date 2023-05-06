;
; $Source: s:/prj/tech/libsrc/dev2d/RCS/rsdblt.inc $
; $Revision: 1.1 $
; $Author: KEVIN $
; $Date: 1996/04/10 16:25:58 $
;
; macros for rsd unpacking.
;

ifndef RSDUNPCK_INC
RSDUNPCK_INC equ 1
endif

include type.inc

;takes ecx=count, edi=destination, al=arg
memset  macro   ret_lab:=<foo>
local   wacky,end
        cmp     ecx,16
        jae     wacky
        rep stosb
;I wish I knew a better way to do this
IFDIFI <ret_lab>, <foo>
        jmp     ret_lab         ;up to 6
ELSE
        jmp     end
ENDIF

        align   16
wacky:
        push    edx             ;1
        mov     ah,al           ;2
        mov     edx,eax         ;2
        shl     eax,16          ;3
        mov     ax,dx           ;3
        mov     edx,ecx         ;2
        mov     ecx,edi         ;2
        neg     ecx             ;2
        and     ecx,3           ;3
        sub     edx,ecx         ;2
        rep stosb               ;2
        mov     ecx,edx         ;2
        shr     ecx,2           ;3
        rep stosd               ;2
        mov     ecx,edx         ;2
        pop     edx             ;1
        and     ecx,3           ;3
        rep stosb               ;2
end:        
endm

;this could probably be fixed so it was faster than the
;above, but I'm punting for now.
memsetl macro ret_lab:=<foo>
local   wacky,loop,loop_end,end
        cmp     ecx,16          ;3
        jae     wacky           ;2
        rep stosb               ;2
;I wish I knew a better way to do this
IFDIFI <ret_lab>, <foo>
        jmp     ret_lab         ;up to 6
ELSE
        jmp     end
ENDIF
        align   16
wacky:
        mov     ah,al
        mov     edx,eax
        shl     eax,16
        or      eax,edx
        mov     edx,ecx
        mov     ecx,edi
        neg     ecx
        and     ecx,3
        sub     edx,ecx
        rep stosb
        
        mov     ecx,edx
        and     ecx,3
        push    ecx
        mov     ecx,edx
        and     ecx,0111100b
        add     edi,ecx
        shr     ecx,2
        shr     edx,6
        lea     ecx,[2*ecx+ecx]
        neg     ecx
        add     ecx,loop_end
        jmp     ecx
        align   16
loop:
        add     edi,64         ;2
n=-64
repeat 16
        mov     [edi+n],eax    ;3*16
        n = n+4
endm
loop_end:        
        sub     edx,1          ;2
        jns     loop           ;2
        pop     ecx            ;2
        rep stosb              ;2
end:
endm

;takes ecx=count, edi=destination, esi=source
memcpy  macro   ret_lab:=<foo>
local   wacky,end
        cmp     ecx,016
        jae     wacky
        rep movsb
;I wish I knew a better way to do this
IFDIFI <ret_lab>, <foo>
        jmp     ret_lab         ;up to 6
ELSE
        jmp     end
ENDIF
        align   16
wacky:
        push    edx
        mov     edx,ecx
        mov     ecx,edi
        neg     ecx
        and     ecx,3
        sub     edx,ecx
        rep movsb
        mov     ecx,edx
        shr     ecx,2
        rep movsd
        mov     ecx,edx
        pop     edx
        and     ecx,3
        rep movsb
end:        
endm

