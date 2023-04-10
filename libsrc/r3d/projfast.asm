; $Source: x:/prj/tech/libsrc/r3d/RCS/projfast.asm $
; $Revision: 1.8 $
; $Author: dc $
; $Date: 1997/01/05 22:51:04 $
;
; Project-space transformation/clip coding

.486
include type.inc
include cseg.inc
include thunks.inc
include r3spoint.inc
include ctxts.inc

	assume	ds:_DATA

_DATA	segment	para public USE32 'DATA'

	align	8

extern _r3d_x_off:dword
extern _r3d_y_off:dword
extern _r3d_x_off_24_8:dword
extern _r3d_y_off_24_8:dword
extern _r3d_c_w:dword
extern _r3d_c_h:dword
extern _r3d_c_w_24_8:dword
extern _r3d_c_h_24_8:dword
extern _r3d_x_clip:dword
extern _r3d_y_clip:dword
extern _r3d_near:dword
extern _r3d_fast_z:dword
extern _r3d_ccodes_or:dword
extern _r3d_glob:dword

x_off dq 0
y_off dq 0
tempbuf1 dq 0
tempbuf2 dq 0

two_to_52_power dd 059900000h
; we want to multiply by 256
; this means adding 8 to the exponent
; the number is:
;    0101 1001 1001 0000 0000 0000 0000 0000
;    sEEE EEEE EMMM MMMM MMMM MMMM MMMM MMMM
; exponent = 10110011
;          = 128 + 48 + 2 + 1

two_to_60_power dd 05d900000h

_DATA ends

_TEXT segment

_EXTFUNC mx_trans_mul_vec

;
;  r3_transform_block_proj_noclip
;
; do projectspace point transformation/projection
;
; C code snippet:
;  for (i=0; i < n; ++i) {
;     double w;
;     r3_trans_mul_vec(&dst->p, X2TRANS(&cx.o2c), src);
;     w = 1.0/dst->p.z; // w is 1/z;
;     dst->grp.sx = (int)(dst->p.x * w) + r3d_glob.x_off;
;     dst->grp.sy = (int)(dst->p.y * w) + r3d_glob.y_off;
;     dst->grp.w = w;
;     ++dst;
;     ++src;
;  }
; takes  eax = count,  edx = dest,  ebx = src, ecx = X2TRANS...
_FUNCDEF r3_transform_block_proj_noclip, 4

        push   esi
        push   edi

    ; swap around registers preparing for inner loop
        push   eax
        mov    eax,edx
        mov    edx,ecx
        pop    ecx

    ; we're going to use the add-a-big-number-instead-of-fist
    ; hack.  But we can fold them in to the adds we do as part
    ; of projection
        fld    two_to_52_power
        fild   _r3d_x_off
        fadd
        fstp   x_off
        fld    two_to_52_power
        fild   _r3d_y_off
        fadd
        fstp   y_off

r3_tbpn_top:
        ; we rely on the fact that the following
        ; function doesn't change any registers;
        ; this is only true of the asm implementation
        _CALLFUNC   mx_trans_mul_vec, 3

        ; w = 1.0 / dst->p.z;
        fld1

        fld    dword ptr [eax+R3S_POINT_Z]

        fdiv

        ; during the divide, preload
        ; our next vector and the rest
        ; of this point

        mov    esi,[eax+20]
        mov    esi,[ebx+12]
; removed by Doug since it faults MSVC memory layout
; really, this should be Stride Aware and should be Stride-4, probably?
;        mov    esi,[eax+40]
; really, the ebx one above probably should go too


        ; dst->grp.w = w;
        fst    dword ptr [eax+R3S_POINT_W]

        fld    st(0)

        fmul   dword ptr [eax+R3S_POINT_X]
        fxch   st(1)

        ; stall

        fmul   dword ptr [eax+R3S_POINT_Y]
        fxch   st(1)

        ; add 2^52+2^51 + r3d_x_off
        fadd   x_off
        fxch   st(1)

        ; ditto for y
        fadd   y_off
        ; no fxch because next instruction isn't FP

        add    ebx,12

        ; NB  we set flags on the previous instruction,
        ; don't stomp them before the jnz below!

        fxch   st(1)

        fstp   tempbuf1

        fstp   tempbuf2

        ; load fixed point sx,sy
        mov    esi,dword ptr tempbuf1
        mov    edi,dword ptr tempbuf2

        mov    [eax+R3S_POINT_SX],esi
        mov    [eax+R3S_POINT_SY],edi

        add    eax, _r3d_glob[R3S_GLOBAL_CONTEXT_CUR_STRIDE]
        dec    ecx        ; decrement loop count

        jnz    r3_tbpn_top

        pop    edi
        pop    esi

        ret

;
;  r3_transform_block_proj_clip
;
; do projectspace point transformation/projection & clip coding
;
; takes  eax = count,  edx = dest,  ebx = src, ecx = X2TRANS...
_FUNCDEF r3_transform_block_proj_clip, 4

        push   ebp
        push   esi
        push   edi

        push   eax
        mov    eax,edx
        mov    edx,ecx
        pop    ecx

        fld    two_to_60_power
        fild   _r3d_x_off
        fadd
        fstp   x_off
        fld    two_to_60_power
        fild   _r3d_y_off
        fadd
        fstp   y_off

r3_tbpc_top:
        ; we rely on the fact that the following
        ; function doesn't change any registers
        ; this is only true of the asm implementation
        _CALLFUNC   mx_trans_mul_vec, 3

        ; w = 1.0 / dst->p.z;
        fld1

        fld    dword ptr [eax+R3S_POINT_Z]

        fdiv

        ; during the divide, preload
        ; our next vector and the rest
        ; of this point

        mov    esi,[eax+20]
        mov    esi,[ebx+12]
; removed by Doug since it faults MSVC memory layout
; really, this should be Stride Aware and should be Stride-4, probably?
;        mov    esi,[eax+40]
; really, the ebx one above probably should go too

        ; dst->grp.w = w;
        fst    dword ptr [eax+R3S_POINT_W]

        fld    st(0)

        ; start computing sx,sy
        fmul   dword ptr [eax+R3S_POINT_X]
        fxch   st(1)

        fmul   dword ptr [eax+R3S_POINT_Y]
        fxch   st(1)

        fadd   x_off
        fxch   st(1)

        fadd   y_off

        ; load z into integer for fast compare
        ; and load "fast_z", which is a positive FP
        ; number representing whether it's safe to
        ; use fast (2d) clip coding
        mov    esi,[eax+R3S_POINT_Z]
        mov    edi,_r3d_fast_z

        ; we can integer compare them because one of
        ; them is definitely positive
        cmp    esi,edi
        jle    clip_slow

        push   ecx

        fstp   tempbuf2

        fstp   tempbuf1

        mov    esi,dword ptr tempbuf1
        mov    edi,dword ptr tempbuf2


        ; ESI = sx, EDI = sy

        ; TODO: should we offset these by half a pixel
        ; to fix the clip coding?  I think so.  Would only
        ; take one cycle to do.

        ; The following code uses some pretty gory algorithms
        ; to generate clip codes without branching.

        ; There are two central concepts.  First, if a value is
        ; negative, then the highest bit is set.  Thus, if we
        ; just shift it right by 31, then we have a clip code for
        ; it being negative.

        ; Second, if we compare against a max value, we generate
        ; a bunch of flags.  The easiest one to set a bit from is
        ; the carry flag, via sbb eax,eax.  However, this sets a
        ; bit if we borrowed in an _unsigned_ subtraction.  Thus,
        ; it sets a bit if (unsigned) x > max_x, which means it's
        ; set if (signed) x > max_x || x < 0.

        ; So we use the latter approach to set our flags, knowing
        ; that it will incorrectly set an "off_right" code when it's
        ; actually "off_left".  We fix this up by making "off_left"
        ; also set "off_right", and xoring.

        shl    esi,8            ; go from 24.8 to 16.16
        mov    ebp,_r3d_c_w_24_8  ; canvas width in 24.8

        shl    edi,8
        mov    [eax+R3S_POINT_SX],esi

        mov    [eax+R3S_POINT_SY],edi
        mov    esi,dword ptr tempbuf1

        mov    edi,dword ptr tempbuf2
        add    esi,128       ; offset location by half a pixel

        add    edi,128       ; offset location by half a pixel
        cmp    ebp,esi

        sbb    ebp,ebp        ; ebp = -1 if (esi > ebp or esi is negative)
        mov    ecx,_r3d_c_h_24_8

        shr    esi,30         ; if sign bit was set in esi, now esi = 3
        and    ebp,R3C_OFF_RIGHT

        ; at this point, esi = (R3C_OFF_LEFT | R3C_OFF_RIGHT) if it's off left
        ; and ebp = R3C_OFF_RIGHT if it's off left or off right

        xor    esi,ebp
        cmp    ecx,edi

        sbb    ecx,ecx        ; ecx = -1 if ecx>edi or edi is negative)

        shr    edi,28         ; edi & 4 if it was off top
        and    ecx,R3C_OFF_BOTTOM

        and    edi,12
        ; at this point, edi = (R3C_OFF_TOP | R3C_OFF_BOTTOM) if it's off top
        ; and ecx = R3C_OFF_BOTTOM if it's off top or off bottom
        xor    esi,ecx

        xor    esi,edi
        pop    ecx

        ; now write it out, and update r3d_ccodes_or
        mov    [eax+R3S_POINT_CCODES],esi
        mov    edi,_r3d_ccodes_or

        or     edi,esi

        mov    _r3d_ccodes_or,edi
        jmp    point_done

; straight-ahead version of above:
;       xor    ecx,ecx
;       mov    ebp,r3_c_w
;       cmp    esi,0
;       jge    not_left
;       mov    ecx,R3C_OFF_LEFT
; not_left:
;       cmp    esi,ebp
;       jle    not_right
;       or     ecx,R3C_OFF_RIGHT
; not_right:
;       mov    ebp,r3_c_h
;       cmp    edi,0
;       jge    not_top
;       or     ecx,R3C_OFF_TOP
; not_top:
;       cmp    edi,ebp
;       jle    not_bottom
;       or     ecx,R3C_OFF_BOTTOM
; not_bottom:
;       mov    [eax+R3S_POINT_CCODES],ecx
;       pop    ecx
;
;  this will often branch mispredict,
;  and since it has branches to branches
;  can do really horrible and wacky things
;  the no-branch version is always 9 cycles;
;  the above code is at best 7 cycles assuming
;  all branches taken and perfect prediction
        
;     } else {
;        // slow clipping (need two multiplies)
;        mxs_real iz = z * r3d_glob.x_clip;
;        mxs_real jz = z * r3d_glob.y_clip;
;        if (p->p.x < -iz) code = R3C_OFF_LEFT | R3C_BEHIND; else code = R3C_BEHIND;
;        if (p->p.x >  iz) code |= R3C_OFF_RIGHT;
;        if (p->p.y < -jz) code |= R3C_OFF_TOP;
;        if (p->p.y >  jz) code |= R3C_OFF_BOTTOM;
;     }

clip_slow:
        mov    edi,_r3d_near

        cmp    esi,edi
        jle    point_behind

        xor    edi,edi
        jmp    post_init_clipcode

point_behind:
        mov    edi,R3C_BEHIND
        nop

post_init_clipcode:
        mov    esi,eax

        fstp   tempbuf2

        fstp   tempbuf1

        mov    ebp,dword ptr tempbuf1
        mov    eax,dword ptr tempbuf2

        shl    ebp,8
        shl    eax,8
        mov    [esi+R3S_POINT_SX],ebp
        mov    [esi+R3S_POINT_SY],eax

        ; compute "iz" & "jz"
        fld   dword ptr [esi+R3S_POINT_Z]
        ;fld    st(0)
        fmul  _r3d_x_clip
        ;fxch   st(1)
        fld   dword ptr [esi+R3S_POINT_Z]
        fmul  _r3d_y_clip
        fxch  st(1)

        ; start comparing
        fcom   dword ptr [esi+R3S_POINT_X]    ; compare  iz ?? x
        fnstsw ax
        shr    ah,1      ; carry set if iz < x
        sbb    ebp,ebp
        and    ebp,R3C_OFF_RIGHT
        or     edi,ebp
        fldz
        fsubr
        fxch   st(1)
        fcom   dword ptr [esi+R3S_POINT_Y]
        fnstsw ax
        shr    ah,1      ; carry set if jz < y
        sbb    ebp,ebp
        and    ebp,R3C_OFF_BOTTOM
        or     edi,ebp
        fldz
        fsubr
        fxch   st(1)
        ; now we have -iz and -jz on the stack
        fcomp  dword ptr [esi+R3S_POINT_X]
        fnstsw ax
        and    ah,65     ; now if ah=0, -iz > x
        cmp    ah,1      ; sets carry if ah = 0
        sbb    ebp,ebp
        and    ebp,R3C_OFF_LEFT
        or     edi,ebp
        fcomp  dword ptr [esi+R3S_POINT_Y]
        fnstsw ax
        and    ah,65
        cmp    ah,1
        sbb    ebp,ebp
        and    ebp,R3C_OFF_TOP
        or     edi,ebp
        mov    ebp,_r3d_ccodes_or
        mov    [esi+R3S_POINT_CCODES],edi
        or     ebp,edi
        mov    _r3d_ccodes_or,ebp
        mov    eax,esi

point_done:
        add    eax, _r3d_glob[R3S_GLOBAL_CONTEXT_CUR_STRIDE]
        add    ebx,12

        dec    ecx
        jnz    r3_tbpc_top

        pop    edi
        pop    esi
        pop    ebp

        ret
_TEXT ends
end
