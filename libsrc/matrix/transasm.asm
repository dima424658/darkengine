; $Source: x:/prj/tech/libsrc/matrix/RCS/transasm.asm $
; $Revision: 1.1 $
; $Author: buzzard $
; $Date: 1996/10/21 19:59:48 $
;
; Project-space transformation/clip coding

.486
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc

_TEXT segment

; takes eax = destvec, edx = transform, ebx = srcvec
;dest->x = t->mat.el[0]*v->x + t->mat.el[3]*v->y + t->mat.el[6]*v->z+t->vec.x;
;dest->y = t->mat.el[1]*v->x + t->mat.el[4]*v->y + t->mat.el[7]*v->z+t->vec.y;
;dest->z = t->mat.el[2]*v->x + t->mat.el[5]*v->y + t->mat.el[8]*v->z+t->vec.z;

_FUNCDEF mx_trans_mul_vec, 3

     ; x# means a value which will be added to compute x,
     ; computed at tick #

     ; p# is an intermediate value with no good name
     ; p is always the result of a load, and hence has
     ; no latency requirements on usage

        fld   dword ptr [ebx]       ;  1  p1

        fmul  dword ptr [edx]       ;  2  x2

        fld   dword ptr [ebx+4]     ;  3  p3 x2

        fmul  dword ptr [edx+12]    ;  4  x4 x2

        fld   dword ptr [ebx+8]     ;  5  p5 x4 x2

        fmul  dword ptr [edx+24]    ;  6  x6 x4 x2
        fxch  st(2)                 ;     x2 x4 x6

        fadd                        ;  7  x7 x6
        fxch  st(1)                 ;     x6 x7

        fld   dword ptr [ebx]       ;  8  p8 x6 x7

        fmul  dword ptr [edx+4]     ;  9  y9 x6 x7
        fxch  st(2)                 ;     x7 x6 y9

        fadd  dword ptr [edx+36]    ; 10  x10 x6 y9

        fld   dword ptr [ebx+4]     ; 11  p11 x10 x6 y9

        fmul  dword ptr [edx+16]    ; 12  y12 x10 x6 y9
        fxch  st(2)                 ;     x6 x10 y12 y9

        fadd                        ; 13  x13 y12 y9

        fld   dword ptr [ebx+8]     ; 14  p14 x13 y12 y9

        fmul  dword ptr [edx+28]    ; 15  y15 x13 y12 y9
        fxch  st(2)                 ;     y12 x13 y15 y9

        fadd  dword ptr [edx+40]    ; 16  y16 x13 y15 y9
        fxch  st(1)                 ;     x13 y16 y15 y9

        fstp  dword ptr [eax]       ; 17  y16 y15 y9

        fld   dword ptr [ebx]       ; 19  p19 y16 y15 y9

        fmul  dword ptr [edx+8]     ; 20  z20 y16 y15 y9
        fxch  st(3)                 ;     y9 y16 y15 z20

        fadd                        ; 21  y21 y15 z20

        fld   dword ptr [ebx+4]     ; 22  p22 y21 y15 z20

        fmul  dword ptr [edx+20]    ; 23  z23 y21 y15 z20
        fxch  st(2)                 ;     y15 y21 z23 z20

        fadd                        ; 24  y24 z23 z20
        fxch  st(2)                 ;     z20 z23 y24

        fld   dword ptr [ebx+8]     ; 25  p25 z20 z23 y24

        fmul  dword ptr [edx+32]    ; 26  z26 z20 z23 y24
        fxch  st(2)                 ;     z23 z20 z26 y24

        fadd                        ; 27  z27 z26 y24
        fxch  st(1)                 ;     z26 z27 y24

        ; stall
        ; now, on this cycle, instead of stalling, we could
        ; store y24.  Then on cycle 30, we could do the next
        ; zadd (to z26).  Then we couldn't do the next add
        ; till cycle 33.  So this looks one cycle faster.

        fadd  dword ptr [edx+44]    ; 29  z29 z27 y24
        fxch  st(2)                 ;     y24 z27 z29

        fstp  dword ptr [eax+4]     ; 30  z27 z29

        fadd                        ; 32  z32

        ; stall

        fstp  dword ptr [eax+8]
        ret

_TEXT ends
end
