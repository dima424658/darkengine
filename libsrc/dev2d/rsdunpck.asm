;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/rsdunpck.asm $
; $Revision: 1.3 $
; $Author: TOML $
; $Date: 1996/10/16 16:06:41 $
;
; Rsd unpacking into a bitmap where row=width.
;

.386

include cseg.inc
include rsdunpck.inc
include msmacros.inc

assume cs:_TEXT
_TEXT   segment

;takes eax=rsd source, edx=destination buffer.
;returns eax=final destination ptr.
;public gd_rsd8_unpack_
;        align   16
;gd_rsd8_unpack_:
_FUNCDEF gd_rsd8_unpack, 2
        align   16
        push    ecx
        push    esi
        push    edi
        mov     esi, eax
        mov     edi, edx
        xor     ecx, ecx
        xor     eax, eax
unpack_loop:
        mov     cl,[esi]        ;2
        cmp     cl,0            ;3
        jz      run             ;2
        cmp     cl,080h         ;3
        je      long_op         ;6
        ja      skip            ;6
;dump
        inc     esi             ;1
        memcpy  unpack_loop
        jmp     unpack_loop
        align   16
run:
        mov     cl,[esi+1]      ;3
        mov     al,[esi+2]      ;3
        add     esi,3           ;3
        memset  unpack_loop
        jmp     unpack_loop
        align   16
skip:
        and     ecx,07fh
        xor     eax,eax
        inc     esi
        memset  unpack_loop
        jmp     unpack_loop
long_op:
        mov     cx,[esi+1]
        cmp     ecx,08000h
        jae     long_run_or_dump
        or      ecx,ecx
        jz      rsd_done
;long skip
        xor     eax,eax
        add     esi,3
        memset  unpack_loop
        jmp     unpack_loop
long_run_or_dump:
        cmp     ecx,0C000h
        jae     long_run
;long dump
        and     ecx,07fffh
        add     esi,3
        memcpy  unpack_loop
        jmp     unpack_loop
long_run:
        and     ecx,03fffh
        mov     al,[esi+3]
        add     esi,4
        memset  unpack_loop
        jmp     unpack_loop
rsd_done:
        mov     eax,edi
        pop     edi
        pop     esi
        pop     ecx
        ret
_TEXT   ends
end
