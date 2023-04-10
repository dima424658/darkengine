;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/rsdblt16.asm $
; $Revision: 1.4 $
; $Author: TOML $
; $Date: 1997/01/10 12:51:14 $
;
; Rsd unpacking into a bitmap where row=width.
;

.386

include cseg.inc
include dseg.inc
include type.inc
include msmacros.inc

assume ds:_DATA
_DATA segment
src_width       dd ?
row_end_delta   dd ?
memset_8to16    dd ?
memcpy_8to16    dd ?
_DATA ends

_TEXT segment

;takes eax=memset routine, edx=memcpy routine
_FUNCDEF flat16_rsd8_blit_init, 2
        mov     memset_8to16, eax
        mov     memcpy_8to16, edx
        ret

;takes eax=rsd_source, edx=dest, ebx=dest_row, ecx=src_width
_FUNCDEF flat16_rsd8_blit, 4
        push    ebp
        push    edi

        push    esi
        mov     edi, edx

        sub     ebx, ecx
        mov     esi, eax

        mov     src_width, ecx
        sub     ebx, ecx

        mov     row_end_delta, ebx
        xor     ebx, ebx

;       esi = soure
;       edi = dest
;       eax, edx = scratch
;       ebx = rsd token; count
;       ecx = src pix remaining this row


check_loop:
        mov     bl, [esi]
        cmp     bl, 0
        jz      check_run
        cmp     bl, 080h
        je      check_long_op
        ja      check_skip

;dump
        inc     esi
        sub     ecx, ebx
        jle     check_wrap_dump
check_end_dump:
        mov     eax, edi
        mov     edx, esi
        lea     edi, [edi + 2*ebx]
        add     esi, ebx
        _CALLFUNCPTR    memcpy_8to16, 3
        xor     ebx, ebx
        jmp     check_loop
check_wrap_dump:
        add     ebx, ecx        ;ebx = pix remaining this row
        mov     eax, edi
        mov     edx, esi
        lea     edi, [edi + 2*ebx]
        add     esi, ebx
        _CALLFUNCPTR    memcpy_8to16, 3
        mov     ebx, ecx
        add     edi, row_end_delta  ;advance to next row
        neg     ebx             ;ebx = pix left to draw
        add     ecx, src_width      ;ecx = pix remaining this row
        jle     check_wrap_dump ;if ecx<= 0, draw entire row and continue
        jmp     check_end_dump  ;otherwise draw remaining and get next token

check_run:
        mov     al, [esi+2]
        mov     bl, [esi+1]

        add     esi, 3
        and     eax, 0ffh

        sub     ecx, ebx
        jle     check_wrap_run
check_end_run:
        mov     edx, edi
        lea     edi, [edi + 2*ebx]
        _CALLFUNCPTR    memset_8to16, 3
        xor     ebx, ebx
        jmp     check_loop
check_wrap_run:
        add     ebx, ecx         ;ecx=edx
        mov     edx, edi
        lea     edi, [edi + 2*ebx]
        push    eax
        _CALLFUNCPTR    memset_8to16, 3
        pop     eax
        add     edi, row_end_delta
        mov     ebx, ecx
        neg     ebx
        add     ecx, src_width
        jle     check_wrap_run
        jmp     check_end_run

check_skip:
        and     ebx,07fh
        inc     esi
        lea     edi, [edi + 2*ebx]
        sub     ecx, ebx
        ja      check_loop
check_wrap_skip:
        add     edi, row_end_delta
        add     ecx, src_width
        jle     check_wrap_skip
        xor     ebx, ebx
        jmp     check_loop

check_long_op:
        mov     bx, [esi+1]
        cmp     ebx, 08000h
        jae     check_long_run_or_dump
        or      ebx, ebx
        jz      check_rsd_done
;long skip
        add     esi, 3
        lea     edi, [edi + 2*ebx]
        sub     ecx, ebx
        jle     check_wrap_skip
        xor     ebx, ebx
        jmp     check_loop
check_long_run_or_dump:
        cmp     ebx, 0C000h
        jae     check_long_run
;long dump
        and     ebx, 03fffh
        add     esi, 3
        sub     ecx, ebx
        jle     check_wrap_dump
        jmp     check_end_dump
check_long_run:
        and     ebx, 03fffh
        mov     al, [esi+3]
        add     esi, 4
        sub     ecx, ebx
        jle     check_wrap_run
        jmp     check_end_run

check_rsd_done:
        pop     esi
        pop     edi

        pop     ebp
        ret

_TEXT ends
end
