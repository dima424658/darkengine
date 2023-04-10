;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/rsdblt8.asm $
; $Revision: 1.2 $
; $Author: TOML $
; $Date: 1996/10/16 16:06:17 $
;
; Rsd unpacking into a bitmap where row=width.
;

.386

include cseg.inc
include dseg.inc
include type.inc
include rsdblt.inc
include msmacros.inc

assume ds:_DATA
_DATA segment
_DATA ends

_TEXT segment

;takes eax=rsd_source, edx=dest, ebx=dest_row, ecx=src_width
_FUNCDEF flat8_rsd8_blit, 4
        pushad
        mov     edi,edx
        mov     esi,eax
        xor     edx,edx
        mov     edx,ecx
        xor     ecx,ecx
        sub     ebx,edx
        je      flat_loop

        mov     ebp,edx
        jmp     check_loop

check_rsd_done:
        popad
        ret

check_loop:
        mov     cl,[esi]        ;2
        cmp     cl,0            ;3
        jz      check_run       ;2
        cmp     cl,080h         ;3
        je      check_long_op   ;6
;aligned here
        ja      check_skip      ;6
;dump
        inc     esi             ;1
        sub     edx,ecx
        jle     check_wrap_dump
check_end_dump:
        memcpy  check_loop
        jmp     check_loop
check_wrap_dump:
        add     ecx,edx         ;ecx=edx
        memcpy
        add     edi,ebx
        mov     ecx,edx
        neg     ecx
        add     edx,ebp
        jle     check_wrap_dump
        jmp     check_end_dump

        align   16
check_run:
        mov     cl,[esi+1]      ;3
        mov     al,[esi+2]      ;3
        add     esi,3           ;3
        sub     edx,ecx
        jle     check_wrap_run
check_end_run:
        memset  check_loop
        jmp     check_loop
check_wrap_run:
        add     ecx,edx         ;ecx=edx
        memset
        add     edi,ebx
        mov     ecx,edx
        neg     ecx
        add     edx,ebp
        jle     check_wrap_run
        jmp     check_end_run

        align   16
check_skip:
        and     ecx,07fh
        inc     esi
        add     edi,ecx
        sub     edx,ecx
        ja      check_loop
check_wrap_skip:
        add     edi,ebx
        add     edx,ebp
        jle     check_wrap_skip
        xor     ecx,ecx
        jmp     check_loop


check_long_op:
        mov     cx,[esi+1]
        cmp     ecx,08000h
        jae     check_long_run_or_dump
        or      ecx,ecx
        jz      flat_rsd_done
;long skip
        add     esi,3
        add     edi,ecx
        sub     edx,ecx
        jle     check_wrap_skip
        xor     ecx,ecx
        jmp     check_loop
check_long_run_or_dump:
        cmp     ecx,0C000h
        jae     check_long_run
;long dump
        and     ecx,07fffh
        add     esi,3
        sub     edx,ecx
        jle     check_wrap_dump
        memcpy  check_loop
        jmp     check_loop
check_long_run:
        and     ecx,03fffh
        mov     al,[esi+3]
        add     esi,4
        sub     edx,ecx
        jle     check_wrap_run
        memset  check_loop
        jmp     check_loop

flat_rsd_done:
        popad
        ret

        align 16
flat_loop:
        mov     cl,[esi]        ;2
        cmp     cl,0            ;3
        jz      flat_run        ;2
        cmp     cl,080h         ;3
        je      flat_long_op    ;6
        ja      flat_skip       ;6
;dump
        inc     esi             ;1
        memcpy  flat_loop
        jmp     flat_loop

        align   16
flat_run:
        mov     cl,[esi+1]      ;3
        mov     al,[esi+2]      ;3
        add     esi,3           ;3
        memset  flat_loop
        jmp     flat_loop

        align   16
flat_skip:
        and     ecx,07fh
        add     edi,ecx
        inc     esi
        jmp     flat_loop
flat_long_op:
        mov     cx,[esi+1]
        cmp     ecx,08000h
        jae     flat_long_run_or_dump
        or      ecx,ecx
        jz      flat_rsd_done
;long skip
        add     esi,3
        add     edi,ecx
        xor     ecx,ecx
        jmp     flat_loop
flat_long_run_or_dump:
        cmp     ecx,0C000h
        jae     flat_long_run
;long dump
        and     ecx,07fffh
        add     esi,3
        memcpy  flat_loop
        jmp     flat_loop
flat_long_run:
        and     ecx,03fffh
        mov     al,[esi+3]
        add     esi,4
        memset  flat_loop
        jmp     flat_loop
_TEXT ends
end
