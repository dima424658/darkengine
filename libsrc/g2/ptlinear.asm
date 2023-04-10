; $Header: x:/prj/tech/libsrc/g2/RCS/ptlinear.asm 1.2 1998/03/23 14:56:29 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptlinear.asm
;
; linear outer (per 8 pixel) loops


.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc
        include thunks.inc

	.list

	assume	cs:_TEXT, ds:_DATA

        extd  _g2pt_tmap_mask
        extd  _g2pt_tmap_ptr
        extd  _g2pt_buffer
        extd  _g2pt_lit_buffer

        extd  _g2pt_light
        extd  _g2pt_dlight

        extd   _g2pt_func_n
        extd   _g2pt_func_8

        extd   _g2pt_dest

_DATA	segment	dword public USE32 'DATA'

	align	4

dlight_8 dd 0

pixel_count dd 0
scanline_count dd 0

dest_save dd 0
count_save dd 0
pixel_save dd 0

save_ebp dd 0
save_edx dd 0

_DATA	ends


_TEXT	segment para public USE32 'CODE'

;extn    g2ptmap_run_
;extn    g2ptmap_flat_run_
;extn    g2ptmap_lit_run_
;extn    g2ptmap_setup_uv
extn    g2ptmap_setup_duv
extn    g2ptmap_setup_uvduv
extn    g2pt_compute_light_n
extn    g2pt_compute_light_8

;  g2ptmap_AFFINE_DUV
;     eax = DU
;     edx = DV

  _FUNCDEF g2ptmap_affine_duv,2
      mov   save_edx,eax
      mov   save_ebp,edx

      ret


;     edx = DU
;     ecx = DV

;  g2ptmap_RUN
;
;  old parms:
;     esi = count
;     eax = V
;     ebx = U
;     edi = dest
;
;  new parms:
;     eax = dest
;     edx = count
;     ebx = U
;     ecx = V

  _FUNCDEF g2ptmap_run,4
        push    ebp
        push    esi
        push    edi

        mov     pixel_count,edx

        mov     edi,eax
        mov     eax,ecx

        mov     edx,save_edx
        mov     ebp,save_ebp

        mov     esi,ebx
        mov     ecx,pixel_count

; check if whole thing is short

        cmp     ecx,16
        jg      pr_handle_long_case

        call    [_g2pt_func_n]

        pop     edi
        pop     esi
        pop     ebp

        ret

pr_handle_long_case:

; now pick up a leading run of 1-7 pixels
; the length is equal to 8-(edi&7), except
; we want 0 if edi&7 is 0.  This is just
; (-edi)&7

        mov     ecx,edi

        neg     ecx

        and     ecx,7
        jz      pr_no_leading_run

; stuff length of leading run

        sub     pixel_count,ecx     ; SLOW!
        call    [_g2pt_func_n]
        
pr_no_leading_run:

        mov     ecx,pixel_count
        mov     _g2pt_dest,edi

        shr     ecx,3

pr_pixel_8_loop:
        push    ecx
        call    [_g2pt_func_8]

        pop     ecx
        mov     _g2pt_dest,edi

        mov     edx,save_edx
        mov     ebp,save_ebp

        dec     ecx
        jnz     pr_pixel_8_loop

        mov     ecx,pixel_count

        and     ecx,7
        jz      pr_no_trailing_run

        call    [_g2pt_func_n]

pr_no_trailing_run:
        pop     edi
        pop     esi
        pop     ebp

        ret


;
;  g2ptmap_LIT_RUN
;
;    This has two entry points,
;    one for C and one for assembly.
;
; This is broken under MSVC, cause it
; takes 6 parameters.  Can we not change
; this calling convention, like CDECL it
; or pass in a structure pointer or something
; or even write a separate entry point
; for MSVC and ifdef it?
; BROKEN BROKEN BROKEN
;
;  old parms:
;     edi: dest
;     esi: count
;     ebx: u
;     eax: v
;     edx: du
;     ecx: dv
;
;  new parms:
;     eax = dest
;     edx = count
;     ebx = U
;     ecx = V


   _FUNCDEF g2ptmap_lit_run,4
        push    ebp
        push    esi
        push    edi
        mov     pixel_count,edx

        mov     edi,eax
        mov     eax,ecx

        mov     esi,ebx
        mov     ecx,pixel_count

        cmp     ecx,16
        jg      pl_handle_long_case

        call    g2pt_compute_light_n

        mov     edx,save_edx     ; du,dv
        mov     ebp,save_ebp

        call    [_g2pt_func_n]

        pop     edi
        pop     esi
        pop     ebp
        ret

pl_handle_long_case:

; now pick up a leading run of 1-7 pixels
; the length is equal to 8-(edi&7), except
; we want 0 if edi&7 is 0.  This is just
; (-edi)&7

        mov     ecx,edi

        neg     ecx

        and     ecx,7
        jz      pl_no_leading_run

; stuff length of leading run

        sub     pixel_count,ecx     ; SLOW!
        call    g2pt_compute_light_n

        mov     edx,save_edx
        mov     ebp,save_ebp

        call    [_g2pt_func_n]
        
pl_no_leading_run:

        mov     ecx,_g2pt_dlight

        shl     ecx,3
 
        mov     dlight_8,ecx
        mov     ecx,pixel_count

        shr     ecx,3

pl_pixel_8_loop:
        mov     count_save,ecx
        call    g2pt_compute_light_8

        mov     edx,save_edx
        mov     ebp,save_ebp

        mov     _g2pt_dest,edi
        call    [_g2pt_func_8]

        mov     ecx,count_save

        dec     ecx
        jnz     pl_pixel_8_loop

cleanup:
        mov     ecx,pixel_count

        and     ecx,7
        jz      pl_no_trailing_run

        call    g2pt_compute_light_n

        mov     edx,save_edx
        mov     ebp,save_ebp

        call    [_g2pt_func_n]

pl_no_trailing_run:
        pop     edi
        pop     esi
        pop     ebp
        ret

_TEXT   ends

END
