; $Header: x:/prj/tech/libsrc/g2/RCS/ptpersp.asm 1.2 1997/09/30 12:06:39 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptpersp.asm
;
; perspective-correct outer (per 8 pixel) loops

.486
	.nolist

        include type.inc
	include	macros.inc
        include thunks.inc

	.list

	assume	cs:_TEXT, ds:_DATA

        extd  _g2pt_tmap_mask
        extd  _g2pt_tmap_ptr
        extd  _g2pt_buffer
        extd  _g2pt_lit_buffer

        extd  _g2pt_light
        extd  _g2pt_dlight

        extd  _g2pt_func_n
        extd  _g2pt_func_8
        extd  _g2pt_func_n_flat
        extd  _g2pt_func_8_flat
        extd  _g2pt_func_perspective_core
        extd  _g2pt_func_perspective_run
        extd  _g2pt_dlight_8

        extd  _g2pt_dest
        extd  _g2pt_toggle

        extq  _g2pt_float_buf
        extq  _g2pt_float_buf2
        extq  _one

_DATA	segment	dword public USE32 'DATA'

	align	4

flat_flag dd 0

pixel_count dd 0
scanline_count dd 0

dest_save dd 0
count_save dd 0
pixel_save dd 0

save_eax dd 0
save_ebx dd 0
save_esi dd 0

start_len_table db 8,7,6,9,8,7,6,9


_DATA	ends


_TEXT	segment para public USE32 'CODE'

extn    g2tmap_float_uv_start_part_one
extn    g2tmap_float_uv_start_part_two
extn    g2tmap_float_uv_start
extn    g2tmap_float_uv_iterate
extn    g2tmap_float_uv_iterate_n
extn    g2tmap_float_uv_cleanup

extn    g2ptmap_perspective_run_asm
;extn    g2ptmap_perspective_run
;extn    g2pt_lit_perspective_run
;extn    g2pt_unlit_perspective_run
;extn    g2pt_lit_perspective_core
;extn    g2pt_unlit_perspective_core
extn    g2pt_compute_light_n
extn    g2pt_compute_light_8
extn    g2tmap_float_uv
extn    g2tmap_float_duv


 _FUNCDEF g2ptmap_perspective_run,3
        push    ebp
        push    esi
        push    edi
        push    eax
        push    ebx
        push    ecx
        push    edx
        mov     esi,eax
        mov     eax,edx
        mov     edi,ebx
        jmp     [_g2pt_func_perspective_run]

 g2ptmap_perspective_run_asm:
        push    ebp
        push    esi
        push    edi
        push    eax
        push    ebx
        push    ecx
        push    edx
        jmp     [_g2pt_func_perspective_run]

;  g2pt_LIT_PERSPECTIVE_RUN
;
;     esi = info table
;     eax = pointer to floating (a,b,c)
;     edi = dest

 _FUNCDEF_ASM g2pt_lit_perspective_run
        call    g2tmap_float_uv_start_part_one

; now we have a floating point divide running in
; the background while we set up for our other stuff

; compute length of leading run from starting position

        mov     ecx,_g2pt_dlight
        mov     eax,_g2pt_toggle

        xor     eax,ecx

        add     ecx,eax
        mov     eax,edi

        shl     ecx,2
;  we're going to compute a starting length as follows:
;  bottom bits of edi:   0  1  2  3  4  5  6  7
;  starting length:      8  7  6  9  8  7  6  9
        and     eax,7

        mov     _g2pt_dlight_8,ecx
        mov     ecx,esi

        mov     count_save,ecx

;  if we don't have at least one aligned run of 8,
; we're a "short" run.  Actually testing for this is
; complicated, but basically our worst case is 9 pixels
; on the front, 8 pixels aligned, and then 5 pixels at
; the end, which is 22 pixels

        cmp     ecx,23
        jge     g2pt_lpr_long_run

; there are two kinds of short runs.  one kind of short
; run is so short that we just do it all by itself.  The other
; kind is long enough that it needs to be subdivided.

        cmp     ecx,12
        jl      g2pt_lpr_short_run

; we need to subdivide.   what we'll do is leverage the 8-pixel
; unrolled stuff but we won't bother being aligned (still faster
; than not using it at all).
; rather than do any work, we'll just skip the "leading run" code
; for long runs, and jump straight into where it does 8 pixels at
; a time.  Then the trailing code support for it will handle
; the remaining pixels

        mov     eax,8                        ; the first step will be by 8
        call    g2tmap_float_uv_start_part_two ; so use the precomputed one
        mov     eax,esi                      ; get the full size
        sub     ecx,1
        sub     eax,1
        shr     ecx,3                        ; compute the # of 8-byte blocks
        and     eax,7                        ; compute the remainder
        add     eax,1
        mov     pixel_save,eax
        mov     count_save,ecx
        mov     esi, d [_g2pt_float_buf]
        mov     eax, d [_g2pt_float_buf2]
        jmp     plp_entry

; lets just do a single run by itself
g2pt_lpr_short_run:

        mov     eax,ecx
        mov     count_save,ecx

        dec     eax

        call    g2tmap_float_uv_start_part_two

        mov     esi, d [_g2pt_float_buf]
        mov     eax, d [_g2pt_float_buf2]

        call    g2tmap_float_uv_iterate

        call    g2pt_compute_light_n

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_n]

        call    g2tmap_float_uv_cleanup

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     edi
        pop     esi
        pop     ebp

        ret

g2pt_lpr_long_run:
        mov     al,start_len_table[eax]

        mov     ecx,eax          ; save our initial length

        call    g2tmap_float_uv_start_part_two

        mov     eax,ecx          ; restore our initial length
        mov     ecx,count_save

        sub     ecx,eax      ; less the leading run

        mov     pixel_save,ecx
        sub     ecx,6        ; our extra run will be at least 6 pixels

        shr     ecx,3

        mov     count_save,ecx

        shl     ecx,3

        sub     pixel_save,ecx

        mov     ecx,eax      ; put leading run count back in ecx

; ok, we're all set to go and do the leading run

        mov     esi, d _g2pt_float_buf
        mov     eax, d _g2pt_float_buf2

        call    g2tmap_float_uv_iterate

        call    g2pt_compute_light_n

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_n]

plp_entry:
        mov     ecx,count_save

        dec     ecx
        jz      plp_post

        mov     _g2pt_dest,edi

        call    [_g2pt_func_perspective_core]

plp_post:

; do the last run of 8, and start
; a divide for some other length
        mov     ecx, eax

        mov     eax,pixel_save
        dec     eax

        call    g2tmap_float_uv_iterate_n

        mov     eax, ecx

        call    g2pt_compute_light_8

        mov     _g2pt_dest,edi

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_8]

        mov     ecx, eax                ; save v
        mov     eax,pixel_save
        
        cmp     eax,0
        je      plp_done

        call    g2tmap_float_uv_iterate

        mov     eax, ecx                ; restore v
        mov     ecx, pixel_save         ; restore count

        call    g2pt_compute_light_n

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_n]

plp_done:
        call    g2tmap_float_uv_cleanup

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     edi
        pop     esi
        pop     ebp
        ret


; the main inner loop which does 8 pixels at a time
; and overlaps a computation of the stepsize for the
; NEXT 8 pixels at a time; this is modularized out
; so we can instantiate ultra-fast ones for particular
; types (which is only a win if we have big polys)

 _FUNCDEF_ASM g2pt_lit_perspective_core
        mov     count_save,ecx
        call    g2tmap_float_uv_iterate

        call    g2pt_compute_light_8

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_8]

        mov     _g2pt_dest,edi

        mov     ecx,count_save

        dec     ecx
        jnz     g2pt_lit_perspective_core_

        ret




;  g2pt_UNLIT_PERSPECTIVE_RUN
;
;     esi = info table
;     eax = pointer to floating (a,b,c)
;     edi = dest

 _FUNCDEF_ASM g2pt_unlit_perspective_run
        call    g2tmap_float_uv_start_part_one

; now we have a floating point divide running in
; the background while we set up for our other stuff

; compute length of leading run from starting position

        mov     eax,edi

;  we're going to compute a starting length as follows:
;  bottom bits of edi:   0  1  2  3  4  5  6  7
;  starting length:      8  7  6  9  8  7  6  9
        and     eax,7
        mov     ecx,esi

        mov     count_save,ecx

;  if we don't have at least one aligned run of 8,
; we're a "short" run.  Actually testing for this is
; complicated, but basically our worst case is 9 pixels
; on the front, 8 pixels aligned, and then 5 pixels at
; the end, which is 22 pixels

        cmp     ecx,23
        jge     g2pt_pr_long_run

; there are two kinds of short runs.  one kind of short
; run is so short that we just do it all by itself.  The other
; kind is long enough that it needs to be subdivided.

        cmp     ecx,12
        jl      g2pt_pr_short_run

; we need to subdivide.   what we'll do is leverage the 8-pixel
; unrolled stuff but we won't bother being aligned (still faster
; than not using it at all).
; rather than do any work, we'll just skip the "leading run" code
; for long runs, and jump straight into where it does 8 pixels at
; a time.  Then the trailing code support for it will handle
; the remaining pixels

        mov     eax,8                        ; the first step will be by 8
        call    g2tmap_float_uv_start_part_two ; so use the precomputed one
        mov     eax,esi                      ; get the full size
        sub     ecx,1
        sub     eax,1
        shr     ecx,3                        ; compute the # of 8-byte blocks
        and     eax,7                        ; compute the remainder
        add     eax,1
        mov     pixel_save,eax
        mov     count_save,ecx
        mov     esi, d _g2pt_float_buf
        mov     eax, d _g2pt_float_buf2
        jmp     pp_entry

; lets just do a single run by itself
g2pt_pr_short_run:

        mov     eax,ecx

        dec     eax

        call    g2tmap_float_uv_start_part_two

        mov     esi, d _g2pt_float_buf
        mov     eax, d _g2pt_float_buf2

        call    g2tmap_float_uv_iterate

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_n]

        call    g2tmap_float_uv_cleanup

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     edi
        pop     esi
        pop     ebp
        ret

g2pt_pr_long_run:
        mov     al,start_len_table[eax]

        mov     ecx,eax          ; save our initial length

        call    g2tmap_float_uv_start_part_two

        mov     eax,ecx          ; restore our initial length
        mov     ecx,count_save

        sub     ecx,eax      ; less the leading run

        mov     pixel_save,ecx
        sub     ecx,6        ; our extra run will be at least 6 pixels

        shr     ecx,3

        mov     count_save,ecx

        shl     ecx,3

        sub     pixel_save,ecx

        mov     ecx,eax      ; put leading run count back in ecx
        mov     esi, d _g2pt_float_buf

        mov     eax, d _g2pt_float_buf2

; ok, we're all set to go and do the leading run

        call    g2tmap_float_uv_iterate

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_n]

pp_entry:
        mov     ecx,count_save

        dec     ecx
        jz      pp_post

        mov     _g2pt_dest,edi

        call    [_g2pt_func_perspective_core]

pp_post:
        mov     ecx, eax        ; save v

; do the last run of 8, and start
; a divide for some other length

        mov     eax,pixel_save

        dec     eax

        call    g2tmap_float_uv_iterate_n

        mov     eax, ecx        ; restore v
        mov     _g2pt_dest,edi

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        mov     ecx, 1

        call    [_g2pt_func_8]

        mov     ecx,pixel_save
        
        cmp     ecx,0
        je      pp_done

        call    g2tmap_float_uv_iterate

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_n]

pp_done:
        call    g2tmap_float_uv_cleanup

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     edi
        pop     esi
        pop     ebp
        ret

; the main inner loop which does 8 pixels at a time
; and overlaps a computation of the stepsize for the
; NEXT 8 pixels at a time; this is modularized out
; so we can instantiate ultra-fast ones for particular
; types (which is only a win if we have big polys)

 _FUNCDEF_ASM g2pt_unlit_perspective_core
        mov     count_save,ecx

        call    g2tmap_float_uv_iterate

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        call    [_g2pt_func_8]

        mov     _g2pt_dest,edi
        mov     ecx,count_save

        dec     ecx
        jnz     g2pt_unlit_perspective_core_

        ret

_TEXT   ends

END
