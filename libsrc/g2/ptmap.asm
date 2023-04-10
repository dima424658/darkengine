; $Header: x:/prj/tech/libsrc/g2/RCS/ptmap.asm 1.3 1998/03/23 14:56:36 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptmap.asm
;   support functions


.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc

	.list

	assume	cs:_TEXT, ds:_DATA

        extd _g2pt_buffer
        extd _g2pt_lit_buffer
        extd _g2pt_tmap_mask,_g2pt_tmap_row,_g2pt_tmap_ptr
        extd _g2pt_row_table
        extd _g2pt_light
        extd _g2pt_dlight
        extd _g2pt_toggle
        extd _g2pt_dither
        extd _g2pt_step_table
        extb _g2pt_arb_size

        extq _g2pt_float_buf
        extq _g2pt_float_buf2
        extd _g2pt_dest


_DATA	segment	para public USE32 'DATA'

	align	8

_g2pt_float_buf         dq 0
_g2pt_float_buf2        dq 0
_g2pt_dest              dd 0

_DATA	ends


_TEXT	segment para public USE32 'CODE'


extn    g2pt_compute_light_8
extn    g2pt_compute_light_n

;
;   compute lighting values
;

;  g2pt_compute_light_N
;  computes n values of lighting found in ecx
;  preserves all registers except ebx, ebp and edx

g2pt_compute_light_n:
        push    ecx
        push    edi

;        mov     ebp,_g2pt_dither
;
;        test    ebp,ebp
;        jnz     g2pt_compute_light_dither_n
;
        mov     ebx,_g2pt_light
        lea     edi,_g2pt_lit_buffer

        ror     ebx,16
        mov     edx,_g2pt_dlight

        shr     edx,16
        mov     ebp,_g2pt_dlight

        shl     ebp,16

g2ptmap_cn_loop:
        mov     [edi],bl
        add     ebx,ebp
        
        adc     ebx,edx
        inc     edi

        dec     ecx
        jnz     g2ptmap_cn_loop

; clean up to compute final value
        ror     ebx,16
        pop     edi

        mov     _g2pt_light,ebx
        pop     ecx

        ret

;  g2pt_compute_light_8
;  computes 8 values of lighting
;  stomps ebp,edx,ebx

g2pt_compute_light_8:
        mov    ebp,_g2pt_light

;        mov    edx,_g2pt_toggle        ;shouldn't this be _g2pt_dither?
;
;        test   edx,edx
;        jnz    g2pt_compute_light_dither_8

        mov    edx,_g2pt_dlight
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+1,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+2,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+3,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+4,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+5,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+6,bl
        mov    ebx,ebp

        shr    ebx,16
        add    ebp,edx

        mov    byte ptr _g2pt_lit_buffer+7,bl
        mov    _g2pt_light,ebp

        ret

; dithering versions: currently unsupported

if 0
g2pt_compute_light_dither_n:
        push    edx
        mov     ebx,_g2pt_light

        mov     esi,_g2pt_dlight
        mov     ecx,_g2pt_toggle

        lea     edi,_g2pt_lit_buffer
        nop

g2ptmap_cdn_loop:
        mov     edx,ebx
        add     ebx,esi

        shr     edx,17
        xor     esi,ecx

        mov     [edi],dl
        inc     edi

        dec     al
        jnz     g2ptmap_cdn_loop

        mov     _g2pt_light,ebx
        mov     _g2pt_dlight,esi

        pop     edx
        pop     esi

        pop     edi
        pop     eax

        pop     ebx
        ret
         
g2pt_compute_light_dither_8:
        push   eax
        mov    eax,_g2pt_dlight

        xor    edx,eax

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _g2pt_lit_buffer,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _g2pt_lit_buffer+1,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _g2pt_lit_buffer+2,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _g2pt_lit_buffer+3,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _g2pt_lit_buffer+4,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _g2pt_lit_buffer+5,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _g2pt_lit_buffer+6,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _g2pt_lit_buffer+7,cl

        mov    _g2pt_light,ebp
        pop    eax

        ret
endif

if 0

; the following register gronking routines are no longer needed...

extn    g2tmap_float_uv
extn    g2tmap_float_duv
extn    g2tmap_float_duv_b
extn    g2tmap_float_duv_c
extn    g2tmap_float_duv_d
extn    g2ptmap_setup_uv
extn    g2ptmap_setup_duv ; currently desupported
extn    g2ptmap_setup_uvduv


;  g2tmap_float_UV
;  get u,v values from our iterative process
;  and arrange them in registers for mapping

g2tmap_float_uv:
        mov     ebx,dword ptr _g2pt_float_buf
        mov     ecx,dword ptr _g2pt_float_buf2

;    fall through


;  g2ptmap_SETUP_UV
;  arrange u,v values into registers
;     ebx = u
;     ecx = v
;   we output into  ebx, esi, eax

g2ptmap_setup_uv:
        mov     al,_g2pt_arb_size
        cmp     al,0
        jne     g2pt_setup_arb_uv

        ror     ebx,16        ; align fraction high and integer low
        mov     eax,ecx

        shr     ecx,16
        mov     esi,ebx

        shl     eax,16
        mov     bh,cl

        mov     ecx,_g2pt_tmap_mask

        and     ebx,ecx       ; mask to stay wrapped in texture map
        ret

g2pt_setup_arb_uv:
        mov     eax,ecx              ;  eax = v
        mov     esi,_g2pt_tmap_row     ;  esi = row

        sar     eax,16               ;  eax = v_hi

        imul    eax,esi              ;  eax = v_hi * row

        shl     ecx,16               ;  ecx = v_lo
        mov     esi,ebx              ;  esi = u

        sar     ebx,16               ;  ebx = u_hi
        
        shl     esi,16               ;  esi = u_lo
        add     ebx,eax              ;  ebx = u_hi + v_hi*row

        mov     eax,ecx              ;  eax = v_lo
        mov     ecx,_g2pt_tmap_ptr     ;  ecx = texture

        add     ebx,ecx              ;  ebx = texture + u_hi + v_hi*row
        ret

;  g2tmap_float_DUV
;  get results of last FP computation
;  and use them to compute our deltas
;  and arrange them in registers

g2tmap_float_duv:
        mov    dl,_g2pt_arb_size
        cmp    dl,0
        jnz    g2pt_float_arb_duv

        mov    edx,dword ptr _g2pt_float_buf
        mov    ebp,dword ptr _g2pt_float_buf2

        ror    edx,16

        shl    ebp,16
        mov    ecx,dword ptr _g2pt_float_buf2

        sar    ecx,16
        
        mov    dh,cl
        ret

; output:
;    g2pt_step_table[0] = ((dv_hi+1) * row) + (du_hi)
;    g2pt_step_table[1] = ((dv_hi * row) + (du_hi)

;  EAX  =  v_lo << 16
;  ESI  =  u_lo << 16
;  EBP  = dv_lo << 16
;  EDX  = du_lo << 16
;  EBX  = source + u_hi + v_hi * source_row

g2pt_float_arb_duv:
        push   eax
        mov    edx,dword ptr _g2pt_float_buf2      ; edx = dv

        sar    edx,16                        ; edx = dv_hi
        mov    eax,dword ptr _g2pt_float_buf       ; eax = du

        sar    eax,16                        ; eax = du_hi
        mov    ebp,_g2pt_tmap_row              ; ebp = row

        mov    edx,_g2pt_row_table[edx*4+32*4] ; edx = dv_hi * row

        add    edx,eax                       ; edx = dv_hi * row + du_hi
        pop    eax

        mov    _g2pt_step_table+4,edx          ; g2pt_step_table[1] = dv*row+du
        add    edx,ebp                       ; edx = (dv_hi+1) * row + du_hi

        mov    _g2pt_step_table,edx            ; g2pt_step_table[0] = ...
        mov    edx,dword ptr _g2pt_float_buf       ; edx = du

        shl    edx,16                        ; edx = du_lo
        mov    ebp,dword ptr _g2pt_float_buf2      ; ebp = dv

        shl    ebp,16                        ; ebp = dv_lo
        ret

;  g2ptmap_SETUP_DUV
;  arrange du,dv values into registers
;     edx = du
;     ecx = dv
;  we output into edx, edx, ebp
g2ptmap_setup_duv:
        ror     edx,16
        mov     ebp,ecx

        ror     ecx,16
        
        shl     ebp,16
        mov     dh,cl

        ret

;  g2ptmap_SETUP_UVDUV
;  arrange u,v,du,dv values into registers
;     ebx = u
;     eax = v
;     edx = du
;     ecx = dv

g2ptmap_setup_uvduv:
        ror     ecx,16

        ror     edx,16
        mov     ebp,ecx

        ror     ebx,16
        mov     dh,cl

        ror     eax,16
        mov     esi,ebx

        mov     ecx,_g2pt_tmap_mask
        mov     bh,al

        and     ebp,0ffff0000h
        and     ebx,ecx       ; mask to stay wrapped in texture map

        ret

endif

_TEXT   ends

END
