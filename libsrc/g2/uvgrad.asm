; $Header: x:/prj/tech/libsrc/g2/RCS/uvgrad.asm 1.1 1998/04/28 14:32:12 KEVIN Exp $

.486
.nolist
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc
include macros.inc

include grs.inc
include g2spoint.inc
include plyparam.inc

DUX             equ G2S_POLY_PARAMS_DUX
DVX             equ G2S_POLY_PARAMS_DVX
DUY             equ G2S_POLY_PARAMS_DUY
DVY             equ G2S_POLY_PARAMS_DVY
P_DEST          equ G2S_POLY_PARAMS_P
U_SCALE         equ G2S_POLY_PARAMS_U_SCALE
V_SCALE         equ G2S_POLY_PARAMS_V_SCALE
YRIGHT          equ G2S_POLY_PARAMS_YRIGHT
YLEFT           equ G2S_POLY_PARAMS_YLEFT
YTOP            equ G2S_POLY_PARAMS_YTOP

.list
assume cs:_TEXT, ds:_DATA
_DATA segment
extd            _grd_canvas

convert_buf     dq 0
convert_buf2    dq 0
denom_u         dq 0
denom_v         dq 0
dy_2_0          dq 0
dy_2_1          dq 0
dx_2_0          dq 0
dx_2_1          dq 0

dx_20           dd 0
dy_20           dd 0
dx_21           dd 0
dy_21           dd 0

fix_unit        dd 65536.0
two_to_36       dd 51c00000h   ; convert float to 16.16 fix
_DATA ends


_TEXT segment
_FUNCDEF uv_triangle_gradients, 4
     push edi
     push esi

     push ebp
     mov  edi,ebx

     mov  ebx,eax
     mov  esi,edx

; ebx = V_TOP
; esi = V_RIGHT
; edi = V_LEFT
; ecx = triangle parameters

     mov  eax,G2S_POINT_SX[edi]
     mov  edx,G2S_POINT_SY[edi]
     sub  eax,G2S_POINT_SX[ebx]
     sub  edx,G2S_POINT_SY[ebx]

     mov  dx_20, eax
     mov  dy_20, edx

     mov  eax,G2S_POINT_SX[edi]
     mov  edx,G2S_POINT_SY[edi]
     sub  eax,G2S_POINT_SX[esi]
     sub  edx,G2S_POINT_SY[esi]

     mov  dx_21, eax
     mov  dy_21, edx

     fild       dx_20

     fild       dy_20         
     fxch       st(1)         ;x20 y20

     fild       dx_21
     fxch       st(2)         ;y20 x20 x21        

     fild       dy_21
     fxch       st(2)         ;x20 y20 y21 x21

     fstp       dx_2_0
     fstp       dy_2_0
     fstp       dy_2_1
     fst        dx_2_1

     ;compute dudx,dvdx

     ; compute the denominator and setup for gradients
     ; a = u2-u1; b = u2-u0; c= v2-v1; d = v2-v0

     fmul       dy_2_0                  ; x21y20
     fld        dx_2_0
     fmul       dy_2_1                  ; x20y21 x21y20
     fld        d G2S_POINT_U[edi]
     fsub       d G2S_POINT_U[esi]      ; a
     fxch       st(2)                   ; x21y20 x20y21 a
     fsubrp     st(1), st         
     fld        fix_unit                ; num denom a
     fld        d G2S_POINT_U[edi]
     fsub       d G2S_POINT_U[ebx]      ; b num denom a
     fxch       st(2)                   ; denom num b a
     fdivp      st(1), st               ; denom b a

     ; divide overlap
     mov  eax, G2S_POINT_SY[ebx]
     mov  edx, G2S_POINT_SY[esi]

     add  eax, 0ffffh
     add  edx, 0ffffh

     sar  edx, 16
     mov  ebp, G2S_POINT_SY[edi]

     sar  eax, 16
     mov  YRIGHT[ecx], edx

     mov  edx, _grd_canvas
     add  ebp, 0ffffh

     sar  ebp, 16
     mov  YTOP[ecx], eax

     mov  YLEFT[ecx], ebp
     mov  ebp, GRS_CANVAS_YTAB[edx]

     cmp  ebp, 0
     jne  p_dest_ytab

     mov  ebp, GRS_BITMAP_BITS[edx]
     mov  edx, GRS_BITMAP_ROW[edx]

     and  edx, 0ffffh
     imul edx

     add  eax, ebp
     jmp  p_dest_done

p_dest_ytab:
     mov  eax, [ebp + 4*eax]
     mov  edx, GRS_BITMAP_BITS[edx]

     add  eax, edx

p_dest_done:
     mov        P_DEST[ecx], eax
                                        ; OK, through w. divide overlap
                                        ; now finish gradient calculations
                                        ; * means x gradient, + means y gradient
     fld        st(0)
     fmul       d U_SCALE[ecx]
        fxch    st(1)
     fmul       d V_SCALE[ecx]          ; denom_v denom_u b a
     fld        st(3)
     fmul       dy_2_0                  ; *a denom_v denom_u b a
        fxch    st(2)

     fstp       denom_u
     fstp       denom_v                 ; *a b a
     fld        st(1)
     fmul       dy_2_1                  ; *b *a b a
     fld        d G2S_POINT_V[edi]
     fsub       d G2S_POINT_V[esi]      ; c *b *a b a
        fxch    st(4)
     fmul       dx_2_0                  ; +a *b *a b c
        fxch    st(3)
     fmul       dx_2_1                  ; +b *b *a +a c
        fxch    st(2)
     fsubrp     st(1), st               ; *(b-a) +b +a c
     fld        st(3)
     fmul       dy_2_0                  ; *c *(b-a) +b +a c
     fld        d G2S_POINT_V[edi]  
     fsub       d G2S_POINT_V[ebx]      ; d *c *(b-a) +b +a c
        fxch    st(4)
     fsubp      st(3),st                ; *c *(b-a) +(a-b) d c
        fxch    st(4)
     fmul       dx_2_0                  ; +c *(b-a) +(b-a) d *c
     fld        st(3)                                    
     fmul       dy_2_1                  ; *d +c *(b-a) +(b-a) d *c
        fxch    st(4)
     fmul       dx_2_1                  ; +d +c *(b-a) +(b-a) *d *c
        fxch    st(3)
     fmul       denom_u                 ; fuy +c *(b-a) +d *d *c
        fxch    st(5)
     fsubrp     st(4),st                ; +c *(b-a) +d *(d-c) fuy
     fsubp      st(2),st                ; *(b-a) +(c-d) *(d-c) fuy
     fmul       denom_u                 ; fux +(c-d) *(d-c) fuy
        fxch    st(2)
     fmul       denom_v                 ; fvx +(c-d) fux fuy
        fxch    st(1)
     fmul       denom_v                 ; fvy fvx fux fuy
        fxch    st(3)
     fadd       two_to_36               ; iuy fvx fux fvy
        fxch    st(2)
     fadd       two_to_36               ; iux fvx iuy fvy
        fxch    st(1)
     fadd       two_to_36               ; ivx iux iuv fvy
        fxch    st(3)
     fadd       two_to_36               ; ivy iux iuy ivx
        fxch    st(2)                   ; iuy iux ivy ivx
     fstp       convert_buf                   
     fstp       convert_buf2                  
     mov        eax, d convert_buf      ; iuy
     mov        edx, d convert_buf2     ; iux
     mov        DUY[ecx], eax
     mov        DUX[ecx], edx
     fstp       convert_buf
     fstp       convert_buf2
     mov        eax, d convert_buf      ; ivy
     mov        edx, d convert_buf2     ; ivx
     mov        DVY[ecx], eax
     mov        DVX[ecx], edx

     pop        ebp
     pop        esi
     pop        edi
     ret

_TEXT ends

END
