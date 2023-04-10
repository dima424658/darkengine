; $Header: r:/t2repos/thief2/src/portal/ptmap.asm,v 1.5 1996/12/07 20:31:14 buzzard Exp $
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

        extd    _pt_buffer
        extd    _pt_lit_buffer
        extd _pt_tmap_mask,_pt_tmap_row,_pt_tmap_ptr
        extd _pt_row_table
        extd _pt_light
        extd _pt_dlight
        extd _pt_toggle
        extd _dither
        extd _scanline
        extd _pt_step_table
        extb _pt_arb_size

_DATA	segment	para public USE32 'DATA'

	align	8

float_buf dq 0
float_buf2 dq 0

dlight_8 dd 0
externdef dlight_8:dword

flat_flag dd 0

scanline_count dd 0

dest_save dd 0
count_save dd 0
pixel_save dd 0

save_eax dd 0
save_ebx dd 0
save_esi dd 0

pt_dest dd 0
externdef pt_dest:dword

start_len_table db 8,7,6,9,8,7,6,9

externdef _gda:qword
externdef _gdb:qword
externdef _gdc:qword
externdef _u_step_data:qword
externdef _v_step_data:qword
externdef _one:qword
externdef _dc_table:dword
externdef float_buf:qword
externdef float_buf2:qword

_DATA	ends


_TEXT	segment para public USE32 'CODE'


extn    compute_light_8
extn    compute_light_n
extn    ptmap_float_uv
extn    ptmap_float_duv
extn    ptmap_float_duv_b
extn    ptmap_float_duv_c
extn    ptmap_float_duv_d
extn    ptmap_setup_uv
extn    ptmap_setup_duv ; currently desupported
extn    ptmap_setup_uvduv


;  PTMAP_FLOAT_UV
;  get u,v values from our iterative process
;  and arrange them in registers for mapping

ptmap_float_uv:
        mov     ebx,dword ptr float_buf
        mov     ecx,dword ptr float_buf2

;    fall through


;  PTMAP_SETUP_UV
;  arrange u,v values into registers
;     ebx = u
;     ecx = v
;   we output into  ebx, esi, eax

ptmap_setup_uv:
        mov     al,_pt_arb_size
        cmp     al,0
        jne     pt_setup_arb_uv

        ror     ebx,16        ; align fraction high and integer low
        mov     eax,ecx

        shr     ecx,16
        mov     esi,ebx

        shl     eax,16
        mov     bh,cl

        mov     ecx,_pt_tmap_mask

        and     ebx,ecx       ; mask to stay wrapped in texture map
        ret

pt_setup_arb_uv:
        mov     eax,ecx              ;  eax = v
        mov     esi,_pt_tmap_row     ;  esi = row

        sar     eax,16               ;  eax = v_hi

        imul    eax,esi              ;  eax = v_hi * row

        shl     ecx,16               ;  ecx = v_lo
        mov     esi,ebx              ;  esi = u

        sar     ebx,16               ;  ebx = u_hi
        
        shl     esi,16               ;  esi = u_lo
        add     ebx,eax              ;  ebx = u_hi + v_hi*row

        mov     eax,ecx              ;  eax = v_lo
        mov     ecx,_pt_tmap_ptr     ;  ecx = texture

        add     ebx,ecx              ;  ebx = texture + u_hi + v_hi*row
        ret


;  PTMAP_FLOAT_DUV
;  get results of last FP computation
;  and use them to compute our deltas
;  and arrange them in registers

ptmap_float_duv:
        mov    dl,_pt_arb_size
        cmp    dl,0
        jnz    pt_float_arb_duv

        mov    edx,dword ptr float_buf
        mov    ebp,dword ptr float_buf2

        ror    edx,16

        shl    ebp,16
        mov    ecx,dword ptr float_buf2

        sar    ecx,16
        
        mov    dh,cl
        ret

; output:
;    pt_step_table[0] = ((dv_hi+1) * row) + (du_hi)
;    pt_step_table[1] = ((dv_hi * row) + (du_hi)

;  EAX  =  v_lo << 16
;  ESI  =  u_lo << 16
;  EBP  = dv_lo << 16
;  EDX  = du_lo << 16
;  EBX  = source + u_hi + v_hi * source_row

pt_float_arb_duv:
        push   eax
        mov    edx,dword ptr float_buf2      ; edx = dv

        sar    edx,16                        ; edx = dv_hi
        mov    eax,dword ptr float_buf       ; eax = du

        sar    eax,16                        ; eax = du_hi
        mov    ebp,_pt_tmap_row              ; ebp = row

        mov    edx,_pt_row_table[edx*4+32*4] ; edx = dv_hi * row

        add    edx,eax                       ; edx = dv_hi * row + du_hi
        pop    eax

        mov    _pt_step_table+4,edx          ; pt_step_table[1] = dv*row+du
        add    edx,ebp                       ; edx = (dv_hi+1) * row + du_hi

        mov    _pt_step_table,edx            ; pt_step_table[0] = ...
        mov    edx,dword ptr float_buf       ; edx = du

        shl    edx,16                        ; edx = du_lo
        mov    ebp,dword ptr float_buf2      ; ebp = dv

        shl    ebp,16                        ; ebp = dv_lo
        ret


;  PTMAP_SETUP_DUV
;  arrange du,dv values into registers
;     edx = du
;     ecx = dv
;  we output into edx, edx, ebp
ptmap_setup_duv:
        ror     edx,16
        mov     ebp,ecx

        ror     ecx,16
        
        shl     ebp,16
        mov     dh,cl

        ret


;  PTMAP_SETUP_UVDUV
;  arrange u,v,du,dv values into registers
;     ebx = u
;     eax = v
;     edx = du
;     ecx = dv

ptmap_setup_uvduv:
        ror     ecx,16

        ror     edx,16
        mov     ebp,ecx

        ror     ebx,16
        mov     dh,cl

        ror     eax,16
        mov     esi,ebx

        mov     ecx,_pt_tmap_mask
        mov     bh,al

        and     ebp,0ffff0000h
        and     ebx,ecx       ; mask to stay wrapped in texture map

        ret


;
;   compute lighting values
;

;  COMPUTE_LIGHT_N
;  computes n values of lighting found in al
;  preserves all registers except ecx

compute_light_n:
        push    ebx
        push    eax

        push    edi
        push    esi

        mov     esi,_dither

        test    esi,esi
        jnz     compute_light_dither_n

        mov     ebx,_pt_light

        ror     ebx,16
        mov     ecx,_pt_dlight

        shr     ecx,16
        mov     esi,_pt_dlight

        shl     esi,16
        lea     edi,_pt_lit_buffer

ptmap_cn_loop:
        mov     [edi],bl
        add     ebx,esi
        
        adc     ebx,ecx
        inc     edi

        dec     al
        jnz     ptmap_cn_loop

; clean up to compute final value
        ror     ebx,16
        mov     _pt_light,ebx

        pop     esi
        pop     edi

        pop     eax
        pop     ebx

        ret


compute_light_dither_n:
        push    edx
        mov     ebx,_pt_light

        mov     esi,_pt_dlight
        mov     ecx,_pt_toggle

        lea     edi,_pt_lit_buffer
        nop

ptmap_cdn_loop:
        mov     edx,ebx
        add     ebx,esi

        shr     edx,17
        xor     esi,ecx

        mov     [edi],dl
        inc     edi

        dec     al
        jnz     ptmap_cdn_loop

        mov     _pt_light,ebx
        mov     _pt_dlight,esi

        pop     edx
        pop     esi

        pop     edi
        pop     eax

        pop     ebx
        ret

         
;  COMPUTE_LIGHT_8
;  computes 8 values of lighting
;  stomps ebp,edx,ecx

compute_light_8:
        mov    ebp,_pt_light
        mov    edx,_pt_toggle

        test   edx,edx
        jnz    compute_light_dither_8

        mov    edx,_pt_dlight
        nop

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+1,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+2,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+3,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+4,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+5,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+6,cl

        mov    ecx,ebp
        shr    ecx,16
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+7,cl

        mov    _pt_light,ebp
        ret

compute_light_dither_8:
        push   eax
        mov    eax,_pt_dlight

        xor    edx,eax

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _pt_lit_buffer,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+1,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _pt_lit_buffer+2,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+3,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _pt_lit_buffer+4,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+5,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,eax
        mov    byte ptr _pt_lit_buffer+6,cl

        mov    ecx,ebp
        shr    ecx,17
        add    ebp,edx
        mov    byte ptr _pt_lit_buffer+7,cl

        mov    _pt_light,ebp
        pop    eax

        ret


_TEXT   ends

END
