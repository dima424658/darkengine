; $Header: x:/prj/tech/libsrc/g2/RCS/ptfloat.asm 1.1 1997/09/23 12:41:38 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptfloat.asm
;
; Floating point routines to compute (u,v) and (du,dv)
; from (a,b,c) and (c*da - a*dc, c*db - b*dc)

.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc

	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	para public USE32 'DATA'

	align	8

_g2pt_u_offset          dq 0
_g2pt_v_offset          dq 0
_g2pt_two_to_52_power   dd 059900000h

extq _g2pt_gda
extq _g2pt_gdb
extq _g2pt_gdc
extq _g2pt_fdc
extq _g2pt_u_step_data
extq _g2pt_v_step_data
extq _g2pt_one
extq _g2pt_float_buf
extq _g2pt_float_buf2
extq _g2pt_u_offset
extq _g2pt_v_offset

extd _g2pt_two_to_52_power
extd _g2pt_int_table

_DATA	ends


_TEXT	segment para public USE32 'CODE'

extn g2tmap_float_uv_start_part_one
extn g2tmap_float_uv_start_part_two
extn g2tmap_float_uv_start_part_two_b
extn g2tmap_float_uv_start_part_two_c
extn g2tmap_float_uv_start
extn g2tmap_float_uv_iterate
extn g2tmap_float_uv_iterate_n
extn g2tmap_float_uv_cleanup

;
;
;   Compute U&V values using overlapped floating point ops
;
;

;
;   compute a single u,v value directly (no waiting),
;   and setup for next iterative step based on a passed-in
;   step size for c

g2tmap_float_uv_start_part_one:
         fld     _g2pt_two_to_52_power

         fld     qword ptr [eax]

         fld     qword ptr 8[eax]

         fld     qword ptr 16[eax]    ;   c b a hack

         fld     _g2pt_one     ;  1 c b a hack

         fld     st(1)    ;  c 1 c b a hack
         fxch    st(2)    ;  c 1 c b a hack

         fdiv             ;  z c b a hack
         fxch    st(3)    ;  a c b z hack

         ret

; pass in eax == integer to multiply dc by
g2tmap_float_uv_start_part_two:

    ; while waiting for the divide, prefetch
         cmp     eax,_g2pt_int_table[eax*4]       ; fetch it
         lea     eax,_g2pt_int_table[eax*4]       ; preaddress

         fmul    st,st(3) ; 1    u c b z hack
         fxch    st(1)    ;      c (1)u b z hack

         fld     st(0)    ; 2    c c (1)u b z hack
         fxch    st(3)    ;      b c (1)u c z hack

         fmulp   st(4),st ; 3    c (1)u c (3)v hack

         fld     _g2pt_fdc     ;

         fmul    dword ptr [eax]  ; number of steps

         fadd             ;  4   (4)c' (1)u c (3)v hack
         fxch    st(1)    ;      (1)u (4)c' c (3)v hack

         fadd    _g2pt_u_offset ; 5   (5)iu (4)c' c (3)v hack
         fxch    st(3)    ;      (3)v (4)c' c (5)iu hack

         fadd    _g2pt_v_offset ; 6   (6)iv (4)c' c (5)iu hack
         fxch    st(1)    ;      (4)c' (6)iv c (5)iu hack

         fmul    st(2),st ; 7    (4)c' (6)iv (7)c*c' (5)iu hack
         fxch    st(1)    ;      (6)iv (4)c' (7)c*c' (5)iu hack

         fld     _g2pt_one     ; 8    1 (6)iv (4)c' (7)c*c' (5)iu hack
         fxch    st(4)    ;      (5)iu (6)iv (4)c' (7)c*c' 1 hack

         fstp    _g2pt_float_buf ; 9   (6)iv c' (7)c*c' 1 hack

         fstp    _g2pt_float_buf2 ; 11  c' (7)c*c' 1 hack

         fxch    st(2)      ;     1 (7)c*c' c' hack

         fdivr              ;     zz c' hack
         ret
         
g2tmap_float_uv_start:
         call    g2tmap_float_uv_start_part_one
         lea     eax,_g2pt_gdc
         call    g2tmap_float_uv_start_part_two
         ret

; compute final u,v values,
;  and start next computation going

g2tmap_float_uv_iterate:
        fld     st(0)    ;  zz zz c hack
        fxch    st(2)    ;  c zz zz hack

        fld     st(0)    ;  c c zz zz hack
        fxch    st(2)    ;  zz c c zz hack

        fmul    _g2pt_u_step_data ; du c c zz hack
        fxch    st(1)    ;  c du c zz hack

        fadd    _g2pt_gdc     ;  c' du c zz hack
        fxch    st(3)    ;  zz du c c' hack
 
        fmul    _g2pt_v_step_data ; dv du c c' hack
        fxch    st(1)    ;  du dv c c' hack
        
        fadd    st,st(4) ;  idu dv c c' hack
        fxch    st(2)    ;  c dv idu c' hack

        fmul    st,st(3) ;  c*c' dv idu c' hack
        fxch    st(1)    ;  dv c*c' idu c' hack

        fadd    st,st(4) ;  idv c*c' idu c' hack
        fxch    st(2)    ;  idu c*c' idv c' hack

        fstp    _g2pt_float_buf ; c*c' idv c' hack

        fld     _g2pt_one     ;  1 c*c' idv c hack
        fxch    st(2)    ;  idv c*c' 1 c hack

        fstp    _g2pt_float_buf2 ; c*c' 1 c' hack

        fdiv             ;  zz c hack

        ret

g2tmap_float_uv_iterate_n:
        fld     st(0)    ;  zz zz c hack
        fxch    st(2)    ;  c zz zz hack

        fld     st(0)    ;  c c zz zz hack
        fxch    st(2)    ;  zz c c zz hack

        fmul    _g2pt_u_step_data ; du c c zz hack
        fxch    st(1)    ;  c du c zz hack

        fld     _g2pt_fdc

        fmul    dword ptr _g2pt_int_table[eax*4]

        fadd             ;  c' du c zz hack
        fxch    st(3)    ;  zz du c c' hack
 
        fmul    _g2pt_v_step_data ; dv du c c' hack
        fxch    st(1)    ;  du dv c c' hack
        
        fadd    st,st(4) ;  idu dv c c' hack
        fxch    st(2)    ;  c dv idu c' hack

        fmul    st,st(3) ;  c*c' dv idu c' hack
        fxch    st(1)    ;  dv c*c' idu c' hack

        fadd    st,st(4) ;  idv c*c' idu c' hack
        fxch    st(2)    ;  idu c*c' idv c' hack

        fstp    _g2pt_float_buf ; c*c' idv c' hack

        fld     _g2pt_one     ;  1 c*c' idv c hack
        fxch    st(2)    ;  idv c*c' 1 c hack

        fstp    _g2pt_float_buf2 ; c*c' 1 c' hack

        fdiv             ;  zz c hack

        ret

; clean up the stack
;  is this really necessary given the circular stack?
g2tmap_float_uv_cleanup:
         fstp     _g2pt_float_buf
         fstp     _g2pt_float_buf
         fstp     _g2pt_float_buf
         ret

_TEXT   ends

END
