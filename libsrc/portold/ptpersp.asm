; $Header: r:/t2repos/thief2/libsrc/portold/ptpersp.asm,v 1.9 1997/02/07 15:30:27 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptpersp.asm
;
; perspective-correct outer (per 8 pixel) loops

.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc
        include thunks.inc

	.list

	assume	cs:_TEXT, ds:_DATA

        extd  _pt_tmap_mask
        extd  _pt_tmap_ptr
        extd  _pt_buffer
        extd  _pt_lit_buffer

        extd  _pt_light
        extd  _pt_dlight

        extd  _pt_func_n
        extd  _pt_func_8
        extd  _pt_func_n_flat
        extd  _pt_func_8_flat
        extd  _pt_func_perspective_core
        extd  _pt_func_perspective_run
        extd  dlight_8

        extd  pt_dest
        extd  _pt_toggle

_DATA	segment	dword public USE32 'DATA'

	align	4

two_to_52_power dd 059900000h

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

externdef _gda:qword
externdef _gdb:qword
externdef _gdc:qword
externdef _u_step_data:qword
externdef _v_step_data:qword
externdef _one:qword
externdef float_buf:qword
externdef float_buf2:qword

_DATA	ends


_TEXT	segment para public USE32 'CODE'

extn    tmap_float_uv_start_part_one
extn    tmap_float_uv_start_part_two
extn    tmap_float_uv_start
extn    tmap_float_uv_iterate
extn    tmap_float_uv_iterate_n
extn    tmap_float_uv_cleanup

extn    ptmap_perspective_run_asm
;extn    ptmap_perspective_run
;extn    pt_lit_perspective_run
;extn    pt_unlit_perspective_run
;extn    pt_lit_perspective_core
;extn    pt_unlit_perspective_core
extn    compute_light_n
extn    compute_light_8
extn    ptmap_float_uv
extn    ptmap_float_duv


 _FUNCDEF ptmap_perspective_run,3
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
        jmp     [_pt_func_perspective_run]

 ptmap_perspective_run_asm:
        push    ebp
        push    esi
        push    edi
        push    eax
        push    ebx
        push    ecx
        push    edx
        jmp     [_pt_func_perspective_run]

;  PT_LIT_PERSPECTIVE_RUN
;
;     esi = info table
;     eax = pointer to floating (a,b,c)
;     edi = dest

 _FUNCDEF_ASM pt_lit_perspective_run
        call    tmap_float_uv_start_part_one

; now we have a floating point divide running in
; the background while we set up for our other stuff

; compute length of leading run from starting position

        mov     ecx,_pt_dlight
        mov     eax,_pt_toggle

        xor     eax,ecx

        add     ecx,eax
        mov     eax,edi

        shl     ecx,2
;  we're going to compute a starting length as follows:
;  bottom bits of edi:   0  1  2  3  4  5  6  7
;  starting length:      8  7  6  9  8  7  6  9
        and     eax,7

        mov     dlight_8,ecx
        mov     ecx,esi

        mov     count_save,ecx

;  if we don't have at least one aligned run of 8,
; we're a "short" run.  Actually testing for this is
; complicated, but basically our worst case is 9 pixels
; on the front, 8 pixels aligned, and then 5 pixels at
; the end, which is 22 pixels

        cmp     ecx,23
        jge     pt_lpr_long_run

; there are two kinds of short runs.  one kind of short
; run is so short that we just do it all by itself.  The other
; kind is long enough that it needs to be subdivided.

        cmp     ecx,12
        jl      pt_lpr_short_run

; we need to subdivide.   what we'll do is leverage the 8-pixel
; unrolled stuff but we won't bother being aligned (still faster
; than not using it at all).
; rather than do any work, we'll just skip the "leading run" code
; for long runs, and jump straight into where it does 8 pixels at
; a time.  Then the trailing code support for it will handle
; the remaining pixels

        mov     eax,8                        ; the first step will be by 8
        call    tmap_float_uv_start_part_two ; so use the precomputed one
        mov     eax,esi                      ; get the full size
        sub     ecx,1
        sub     eax,1
        shr     ecx,3                        ; compute the # of 8-byte blocks
        and     eax,7                        ; compute the remainder
        add     eax,1
        mov     pixel_save,eax
        mov     count_save,ecx
        call    ptmap_float_uv               ; get the initial step values
        mov     save_eax,eax                 ; load our tmapping state and go
        mov     save_ebx,ebx
        mov     save_esi,esi
        jmp     plp_entry

; lets just do a single run by itself
pt_lpr_short_run:

        mov     eax,ecx
        mov     count_save,ecx

        dec     eax

        call    tmap_float_uv_start_part_two

        call    ptmap_float_uv   ; let's check out the starting values

        mov     al,byte ptr count_save  ; put initial run count in bottom bits of al

        mov     save_ebx,ebx
        mov     save_esi,esi
        mov     save_eax,eax

        call    tmap_float_uv_iterate

        mov     eax,count_save      ; get count into al
        call    compute_light_n

        call    ptmap_float_duv
        
        mov     eax,save_eax
        mov     ebx,save_ebx
        mov     esi,save_esi

        lea     ecx,_pt_lit_buffer
        call    [_pt_func_n]

        call    tmap_float_uv_cleanup

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     edi
        pop     esi
        pop     ebp

        ret

pt_lpr_long_run:
        mov     al,start_len_table[eax]

        mov     ecx,eax          ; save our initial length

        call    tmap_float_uv_start_part_two

        mov     eax,ecx          ; restore our initial length
        mov     ecx,count_save

        sub     ecx,eax      ; less the leading run

        mov     pixel_save,ecx
        sub     ecx,6        ; our extra run will be at least 6 pixels

        shr     ecx,3

        mov     count_save,ecx

        shl     ecx,3

        sub     pixel_save,ecx

        mov     edx,eax      ; put leading run count back in ecx
        call    ptmap_float_uv

        mov     al,dl         ; put initial run count in bottom bits of al
        mov     ecx,edx

        mov     save_eax,eax

        mov     save_ebx,ebx
        mov     save_esi,esi

; ok, we're all set to go and do the leading run

        call    tmap_float_uv_iterate

        mov     eax,ecx       ; get count into al
        call    compute_light_n

        call    ptmap_float_duv
        
        mov     eax,save_eax
        mov     ebx,save_ebx
        mov     esi,save_esi

        lea     ecx,_pt_lit_buffer
        call    [_pt_func_n]

        mov     save_eax,eax

        mov     save_ebx,ebx
        mov     save_esi,esi

plp_entry:
        mov     ecx,count_save

        dec     ecx
        jz      plp_post

        mov     eax,save_eax
        mov     ebx,save_ebx

        mov     esi,save_esi
        mov     pt_dest,edi

        call    [_pt_func_perspective_core]

        mov     save_eax,eax
        mov     save_ebx,ebx

        mov     save_esi,esi
        nop

plp_post:

; do the last run of 8, and start
; a divide for some other length

        mov     eax,pixel_save
        dec     eax

        call    tmap_float_uv_iterate_n

        call    compute_light_8

        call    ptmap_float_duv

        mov     pt_dest,edi

        mov     eax,save_eax
        mov     ebx,save_ebx

        mov     esi,save_esi
        call    [_pt_func_8]

        mov     save_eax,eax
        mov     save_ebx,ebx

        mov     save_esi,esi
        
        mov     eax,pixel_save
        
        cmp     eax,0
        je      plp_done

        call    tmap_float_uv_iterate

        call    compute_light_n

        call    ptmap_float_duv

        mov     ecx,pixel_save
        mov     eax,save_eax

        mov     al,cl
        mov     ebx,save_ebx

        mov     esi,save_esi

        lea     ecx,_pt_lit_buffer
        call    [_pt_func_n]

plp_done:
        call    tmap_float_uv_cleanup

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

 _FUNCDEF_ASM pt_lit_perspective_core
        mov     count_save,ecx
        call    tmap_float_uv_iterate

        call    compute_light_8

        call    ptmap_float_duv

        mov     eax,save_eax
        mov     ebx,save_ebx

        mov     esi,save_esi
        call    [_pt_func_8]

        mov     pt_dest,edi
        mov     save_esi,esi

        mov     save_eax,eax
        mov     save_ebx,ebx

        mov     ecx,count_save

        dec     ecx
        jnz     pt_lit_perspective_core_

        ret




;  PT_UNLIT_PERSPECTIVE_RUN
;
;     esi = info table
;     eax = pointer to floating (a,b,c)
;     edi = dest

 _FUNCDEF_ASM pt_unlit_perspective_run
        call    tmap_float_uv_start_part_one

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
        jge     pt_pr_long_run

; there are two kinds of short runs.  one kind of short
; run is so short that we just do it all by itself.  The other
; kind is long enough that it needs to be subdivided.

        cmp     ecx,12
        jl      pt_pr_short_run

; we need to subdivide.   what we'll do is leverage the 8-pixel
; unrolled stuff but we won't bother being aligned (still faster
; than not using it at all).
; rather than do any work, we'll just skip the "leading run" code
; for long runs, and jump straight into where it does 8 pixels at
; a time.  Then the trailing code support for it will handle
; the remaining pixels

        mov     eax,8                        ; the first step will be by 8
        call    tmap_float_uv_start_part_two ; so use the precomputed one
        mov     eax,esi                      ; get the full size
        sub     ecx,1
        sub     eax,1
        shr     ecx,3                        ; compute the # of 8-byte blocks
        and     eax,7                        ; compute the remainder
        add     eax,1
        mov     pixel_save,eax
        mov     count_save,ecx
        call    ptmap_float_uv               ; get the initial step values
        mov     save_eax,eax                 ; load our tmapping state and go
        mov     save_ebx,ebx
        mov     save_esi,esi
        jmp     pp_entry

; lets just do a single run by itself
pt_pr_short_run:

        mov     eax,ecx
        mov     count_save,ecx

        dec     eax

        call    tmap_float_uv_start_part_two

        call    ptmap_float_uv   ; let's check out the starting values

        mov     al,byte ptr count_save  ; put initial run count in bottom bits of al

        mov     save_ebx,ebx
        mov     save_esi,esi
        mov     save_eax,eax

        call    tmap_float_uv_iterate

        mov     eax,count_save      ; get count into al
        call    ptmap_float_duv
        
        mov     eax,save_eax
        mov     ebx,save_ebx
        mov     esi,save_esi

        call    [_pt_func_n]

        call    tmap_float_uv_cleanup

        pop     edx
        pop     ecx
        pop     ebx
        pop     eax
        pop     edi
        pop     esi
        pop     ebp
        ret

pt_pr_long_run:
        mov     al,start_len_table[eax]

        mov     ecx,eax          ; save our initial length

        call    tmap_float_uv_start_part_two

        mov     eax,ecx          ; restore our initial length
        mov     ecx,count_save

        sub     ecx,eax      ; less the leading run

        mov     pixel_save,ecx
        sub     ecx,6        ; our extra run will be at least 6 pixels

        shr     ecx,3

        mov     count_save,ecx

        shl     ecx,3

        sub     pixel_save,ecx

        mov     edx,eax      ; put leading run count back in ecx
        call    ptmap_float_uv

        mov     ecx,edx
        mov     al,dl         ; put initial run count in bottom bits of al

        mov     save_ebx,ebx
        mov     save_eax,eax
        mov     save_esi,esi

; ok, we're all set to go and do the leading run

        call    tmap_float_uv_iterate

        call    ptmap_float_duv
        
        mov     eax,save_eax
        mov     ebx,save_ebx
        mov     esi,save_esi

        call    [_pt_func_n]

        mov     save_eax,eax

        mov     save_ebx,ebx
        mov     save_esi,esi

pp_entry:
        mov     ecx,count_save

        dec     ecx
        jz      pp_post

        mov     eax,save_eax
        mov     ebx,save_ebx

        mov     esi,save_esi
        mov     pt_dest,edi

        call    [_pt_func_perspective_core]

        mov     save_eax,eax
        mov     save_ebx,ebx

        mov     save_esi,esi
        nop

pp_post:

; do the last run of 8, and start
; a divide for some other length

        mov     eax,pixel_save
        dec     eax

        call    tmap_float_uv_iterate_n

        call    ptmap_float_duv

        mov     pt_dest,edi

        mov     eax,save_eax
        mov     ebx,save_ebx

        mov     esi,save_esi
        call    [_pt_func_8]

        mov     save_eax,eax
        mov     save_ebx,ebx

        mov     save_esi,esi
        
        mov     eax,pixel_save
        
        cmp     eax,0
        je      pp_done

        call    tmap_float_uv_iterate

        call    ptmap_float_duv

        mov     ecx,pixel_save
        mov     eax,save_eax

        mov     al,cl
        mov     ebx,save_ebx

        mov     esi,save_esi

        call    [_pt_func_n]

pp_done:
        call    tmap_float_uv_cleanup

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

 _FUNCDEF_ASM pt_unlit_perspective_core
        mov     count_save,ecx
        call    tmap_float_uv_iterate

        call    ptmap_float_duv

        mov     eax,save_eax
        mov     ebx,save_ebx

        mov     esi,save_esi
        call    [_pt_func_8]

        mov     pt_dest,edi
        mov     save_esi,esi

        mov     save_eax,eax
        mov     save_ebx,ebx

        mov     ecx,count_save

        dec     ecx
        jnz     pt_unlit_perspective_core_

        ret


_TEXT   ends

END
