; $Header: r:/t2repos/thief2/libsrc/portold/ptfloat.asm,v 1.3 1996/12/16 15:15:24 buzzard Exp $
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

_u_offset dq 0
_v_offset dq 0
_two_to_52_power dd 059900000h

externdef _gda:qword
externdef _gdb:qword
externdef _gdc:qword
externdef _fdc:qword
externdef _u_step_data:qword
externdef _v_step_data:qword
externdef _one:qword
externdef float_buf:qword
externdef float_buf2:qword
externdef _two_to_52_power:dword
externdef _int_table:dword
externdef _u_offset:qword
externdef _v_offset:qword

_DATA	ends


_TEXT	segment para public USE32 'CODE'

extn tmap_float_uv_start_part_one
extn tmap_float_uv_start_part_two
extn tmap_float_uv_start_part_two_b
extn tmap_float_uv_start_part_two_c
extn tmap_float_uv_start
extn tmap_float_uv_iterate
extn tmap_float_uv_iterate_n
extn tmap_float_uv_cleanup

;
;
;   Compute U&V values using overlapped floating point ops
;
;

;
;   compute a single u,v value directly (no waiting),
;   and setup for next iterative step based on a passed-in
;   step size for c

tmap_float_uv_start_part_one:
         fld     _two_to_52_power

         fld     qword ptr [eax]

         fld     qword ptr 8[eax]

         fld     qword ptr 16[eax]    ;   c b a hack

         fld     _one     ;  1 c b a hack

         fld     st(1)    ;  c 1 c b a hack
         fxch    st(2)    ;  c 1 c b a hack

         fdiv             ;  z c b a hack
         fxch    st(3)    ;  a c b z hack

         ret

; pass in eax == integer to multiply dc by
tmap_float_uv_start_part_two:

    ; while waiting for the divide, prefetch
         cmp     eax,_int_table[eax*4]       ; fetch it
         lea     eax,_int_table[eax*4]       ; preaddress

         fmul    st,st(3) ; 1    u c b z hack
         fxch    st(1)    ;      c (1)u b z hack

         fld     st(0)    ; 2    c c (1)u b z hack
         fxch    st(3)    ;      b c (1)u c z hack

         fmulp   st(4),st ; 3    c (1)u c (3)v hack

         fld     _fdc     ;

         fmul    dword ptr [eax]  ; number of steps

         fadd             ;  4   (4)c' (1)u c (3)v hack
         fxch    st(1)    ;      (1)u (4)c' c (3)v hack

         fadd    _u_offset ; 5   (5)iu (4)c' c (3)v hack
         fxch    st(3)    ;      (3)v (4)c' c (5)iu hack

         fadd    _v_offset ; 6   (6)iv (4)c' c (5)iu hack
         fxch    st(1)    ;      (4)c' (6)iv c (5)iu hack

         fmul    st(2),st ; 7    (4)c' (6)iv (7)c*c' (5)iu hack
         fxch    st(1)    ;      (6)iv (4)c' (7)c*c' (5)iu hack

         fld     _one     ; 8    1 (6)iv (4)c' (7)c*c' (5)iu hack
         fxch    st(4)    ;      (5)iu (6)iv (4)c' (7)c*c' 1 hack

         fstp    float_buf ; 9   (6)iv c' (7)c*c' 1 hack

         fstp    float_buf2 ; 11  c' (7)c*c' 1 hack

         fxch    st(2)      ;     1 (7)c*c' c' hack

         fdivr              ;     zz c' hack
         ret
         
tmap_float_uv_start:
         call    tmap_float_uv_start_part_one
         lea     eax,_gdc
         call    tmap_float_uv_start_part_two
         ret

; compute final u,v values,
;  and start next computation going

tmap_float_uv_iterate:
        fld     st(0)    ;  zz zz c hack
        fxch    st(2)    ;  c zz zz hack

        fld     st(0)    ;  c c zz zz hack
        fxch    st(2)    ;  zz c c zz hack

        fmul    _u_step_data ; du c c zz hack
        fxch    st(1)    ;  c du c zz hack

        fadd    _gdc     ;  c' du c zz hack
        fxch    st(3)    ;  zz du c c' hack
 
        fmul    _v_step_data ; dv du c c' hack
        fxch    st(1)    ;  du dv c c' hack
        
        fadd    st,st(4) ;  idu dv c c' hack
        fxch    st(2)    ;  c dv idu c' hack

        fmul    st,st(3) ;  c*c' dv idu c' hack
        fxch    st(1)    ;  dv c*c' idu c' hack

        fadd    st,st(4) ;  idv c*c' idu c' hack
        fxch    st(2)    ;  idu c*c' idv c' hack

        fstp    float_buf ; c*c' idv c' hack

        fld     _one     ;  1 c*c' idv c hack
        fxch    st(2)    ;  idv c*c' 1 c hack

        fstp    float_buf2 ; c*c' 1 c' hack

        fdiv             ;  zz c hack

        ret

tmap_float_uv_iterate_n:
        fld     st(0)    ;  zz zz c hack
        fxch    st(2)    ;  c zz zz hack

        fld     st(0)    ;  c c zz zz hack
        fxch    st(2)    ;  zz c c zz hack

        fmul    _u_step_data ; du c c zz hack
        fxch    st(1)    ;  c du c zz hack

        fld     _fdc

        fmul    dword ptr _int_table[eax*4]

        fadd             ;  c' du c zz hack
        fxch    st(3)    ;  zz du c c' hack
 
        fmul    _v_step_data ; dv du c c' hack
        fxch    st(1)    ;  du dv c c' hack
        
        fadd    st,st(4) ;  idu dv c c' hack
        fxch    st(2)    ;  c dv idu c' hack

        fmul    st,st(3) ;  c*c' dv idu c' hack
        fxch    st(1)    ;  dv c*c' idu c' hack

        fadd    st,st(4) ;  idv c*c' idu c' hack
        fxch    st(2)    ;  idu c*c' idv c' hack

        fstp    float_buf ; c*c' idv c' hack

        fld     _one     ;  1 c*c' idv c hack
        fxch    st(2)    ;  idv c*c' 1 c hack

        fstp    float_buf2 ; c*c' 1 c' hack

        fdiv             ;  zz c hack

        ret

; clean up the stack
;  is this really necessary given the circular stack?
tmap_float_uv_cleanup:
         fstp     float_buf
         fstp     float_buf
         fstp     float_buf
         ret

_TEXT   ends

END
