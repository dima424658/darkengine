; $Header: x:/prj/tech/libsrc/fix/RCS/fixdiv.asm 1.3 1996/10/16 16:08:23 TOML Exp $
;
; Fixed point division with no overflows guaranteed.

.386
include type.inc
include cseg.inc
include msmacros.inc

_TEXT segment

; takes eax = m0, edx = m1, ebx = denominator
_FUNCDEF fix_mul_div_safe, 3
        imul    edx

        push    ebx
        push    edx

        push    eax
        test    ebx,ebx

        jns     fmd_ebx_ok
        neg     ebx

fmd_ebx_ok:
        test    edx,edx
        jns     fmd_edx_ok

        neg     edx

fmd_edx_ok:
        shl     edx,1

        shr     eax,31

        or      edx,eax
        pop     eax

        cmp     ebx,edx
        pop     edx

        pop     ebx
        ja      fmd_no_overflow

        xor     edx,ebx
        mov     eax,07fffffffh

        sar     edx,31

        sub     eax,edx

        sub     eax,edx

        ret

fmd_no_overflow:
        idiv    ebx

        ret

; takes eax=numerator, edx=denominator
_FUNCDEF fix_div_safe, 2
        push    ebx
        mov     ebx,eax

        sar     ebx,15          ;ebx = numerator high 17 bits
        push    edx             ; push denom

        shl     eax,16
        push    ebx             ; push num high

        test    ebx,ebx
        jns     fd_ebx_ok

        neg     ebx

fd_ebx_ok:
        test    edx,edx
        jns     fd_edx_ok

        neg     edx

fd_edx_ok:
        cmp     edx,ebx
        pop     edx             ;now put edx = num high

        pop     ebx             ;        ebx = divisor
        ja      fd_no_overflow

        xor     edx,ebx
        mov     eax,07fffffffh

        sar     edx,31
        pop     ebx

        sub     eax,edx

        sub     eax,edx

        ret

fd_no_overflow:
        sar     edx,1

        idiv    ebx

        pop     ebx

        ret

_TEXT ends
end
