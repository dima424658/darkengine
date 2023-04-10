; $Header: x:/prj/tech/libsrc/g2/RCS/ptmap_8.asm 1.9 1998/04/06 15:41:52 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptmap_8.asm
;
; 8-pixel loops which write to _g2pt_buffer;
; filters transparent, translucent,
; and generic all write to edi

.486
	.nolist

        include thunks.inc
        include type.inc
	include	macros.inc
        include grs.inc

extd    _pixpal
extd    _grd_light_table, _grd_ltab816

extd    _g2pt_tmap_ptr,_g2pt_tmap_mask
extd    _g2pt_clut, _g2pt_tluc_table, _g2pt_light_table
extd    _g2pt_buffer, _g2pt_lit_buffer
extd    _g2pt_light, _g2pt_dlight, _g2pt_dlight_8
extd    _g2pt_step_table

extd    _g2pt_unlit_8_chain
extd    _g2pt_pallit_8_chain
extd    _g2pt_palflat_8_chain
extd    _g2pt_lit_8_chain
extd    _g2pt_clut_8_chain
extd    _g2pt_transp_8_chain
extd    _g2pt_tluc_8_chain
extd    _g2pt_generic_8_chain
extd    _g2pt_generic_8_2_chain
extd    _g2pt_dest

extq    _g2pt_float_buf
extq    _g2pt_float_buf2
extq    _g2pt_gdc
extq    _g2pt_u_step_data
extq    _g2pt_v_step_data
extq    _g2pt_one

	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'

	align	4

_g2pt_dlight_8 dd 0
dummy_buffer dd 0, 0, 0, 0

count_save dd 0
dest_save  dd 0

u8_last_mask_u         dd 0deadbeefh
u8_last_mask_v         dd 0deadbeefh
u8_last_p_src          dd 0deadbeefh
                       
l8_last_mask_u         dd 0deadbeefh
l8_last_mask_v         dd 0deadbeefh
l8_last_p_src          dd 0deadbeefh
l8_last_light_table    dd 0deadbeefh

u8_16_last_mask_u      dd 0deadbeefh
u8_16_last_mask_v      dd 0deadbeefh
u8_16_last_p_src       dd 0deadbeefh
                       
l8_16_last_mask_u      dd 0deadbeefh
l8_16_last_mask_v      dd 0deadbeefh
l8_16_last_p_src       dd 0deadbeefh
l8_16_last_light_table dd 0deadbeefh

u8_last_shift_v        db 0ach
l8_last_shift_v        db 0ach
u8_16_last_shift_v     db 0ach
l8_16_last_shift_v     db 0ach

_DATA	ends


_TEXT	segment para public USE32 'CODE'

; import
extn    g2tmap_float_uv_iterate
extn    g2pt_dest_pallit_n_
extn    g2pt_unlit_arb_n_

; export
;extn    g2pt_unlit_8,g2pt_unlit_8_setup
;extn    g2pt_palflat_8,g2pt_palflat_8_setup,g2pt_palflat_direct_8
;extn    g2pt_lit_8,g2pt_lit_8_setup
;extn    g2pt_pallit_8,g2pt_pallit_store_8
;extn    g2pt_clut_8
;extn    g2pt_pallit_clut_store_8
;extn    g2pt_transp_8
;extn    g2pt_tluc_8
;extn    g2pt_generic_8, g2pt_generic_8_2
;extn    g2pt_generic_preload_8, g2pt_generic_preload_8_2
;extn    g2pt_dest_pallit_8
;extn    g2pt_unlit_arb_8
;extn    g2pt_pallit_perspective_core
;extn    g2pt_unlit_perspective_unrolled


;  first we have all of the texture mappers themselves
;
;  Basically, all of these mappers may have extra state
;  in self-modified things or in global variables, but
;  since they're called there can't be any other state
;  preserved.  They are also free to trash the other
;  registers not listed:

;  If the tmapper must read the source (or not write
;  some of it), then the source/destination is g2pt_dest.  We only
;  have this for the outer filters: transparent, translucent,
;  and generic (which just is a wrapper that redirects to
;  g2pt_dest).

;  Lighting is stored in a static buffer.
;  These routines are not C callable and therefore
;  don't preserve EBP.



; eax = 16.16 v
; esi = 16.16 u

   _FUNCDEF_ASM g2pt_unlit_8

        mov     ebx, esi
        mov     edi, eax

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_0:

        shr     edi, 0ach
g2pt_u8_shift_v_0:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_0:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_0:
        mov     edi, eax

        mov     b _g2pt_buffer, cl
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_1:

        shr     edi, 0ach
g2pt_u8_shift_v_1:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_1:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_1:
        mov     edi, eax

        mov     b _g2pt_buffer+1, ch
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_2:

        shr     edi, 0ach
g2pt_u8_shift_v_2:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_2:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_2:
        mov     edi, eax

        mov     b _g2pt_buffer+2, cl
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_3:

        shr     edi, 0ach
g2pt_u8_shift_v_3:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_3:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_3:
        mov     edi, eax

        mov     b _g2pt_buffer+3, ch
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_4:

        shr     edi, 0ach
g2pt_u8_shift_v_4:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_4:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_4:
        mov     edi, eax

        mov     b _g2pt_buffer+4, cl
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_5:

        shr     edi, 0ach
g2pt_u8_shift_v_5:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_5:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_5:
        mov     edi, eax

        mov     b _g2pt_buffer+5, ch
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_6:

        shr     edi, 0ach
g2pt_u8_shift_v_6:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_6:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_6:
        mov     edi, eax

        mov     b _g2pt_buffer+6, cl
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_u8_mask_v_7:

        shr     edi, 0ach
g2pt_u8_shift_v_7:
        and     ebx, 0deadbeefh
g2pt_u8_mask_u_7:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_u8_p_src_7:

        mov     b _g2pt_buffer+7, ch
        jmp     [_g2pt_unlit_8_chain]

; setup
;
; takes pointer to bitmap in eax

  _FUNCDEF g2pt_unlit_8_setup,1
        push    esi
        push    edi

        push    ebx
        push    edx

        push    ecx
        mov     ebx, GRS_BITMAP_BITS[eax]

        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

;eax = mask_u, edx = mask_v, ebx = p_src, cl = shift_v
        mov     esi, u8_last_mask_u
        mov     edi, u8_last_mask_v

        cmp     esi, eax
        jz      g2pt_u8_skip_mask_u

        mov     d ds:[g2pt_u8_mask_u_0-4], eax
        mov     d ds:[g2pt_u8_mask_u_1-4], eax
        mov     d ds:[g2pt_u8_mask_u_2-4], eax
        mov     d ds:[g2pt_u8_mask_u_3-4], eax
        mov     d ds:[g2pt_u8_mask_u_4-4], eax
        mov     d ds:[g2pt_u8_mask_u_5-4], eax
        mov     d ds:[g2pt_u8_mask_u_6-4], eax
        mov     d ds:[g2pt_u8_mask_u_7-4], eax
        mov     u8_last_mask_u, eax
g2pt_u8_skip_mask_u:
        cmp     edi, edx
        jz      g2pt_u8_skip_mask_v

        mov     d ds:[g2pt_u8_mask_v_0-4], edx
        mov     d ds:[g2pt_u8_mask_v_1-4], edx
        mov     d ds:[g2pt_u8_mask_v_2-4], edx
        mov     d ds:[g2pt_u8_mask_v_3-4], edx
        mov     d ds:[g2pt_u8_mask_v_4-4], edx
        mov     d ds:[g2pt_u8_mask_v_5-4], edx
        mov     d ds:[g2pt_u8_mask_v_6-4], edx
        mov     d ds:[g2pt_u8_mask_v_7-4], edx
        mov     u8_last_mask_v, edx
g2pt_u8_skip_mask_v:

        mov     eax, u8_last_p_src
        mov     dl, u8_last_shift_v

        cmp     eax, ebx
        jz      g2pt_u8_skip_p_src

        mov     d ds:[g2pt_u8_p_src_0-4], ebx
        mov     d ds:[g2pt_u8_p_src_1-4], ebx
        mov     d ds:[g2pt_u8_p_src_2-4], ebx
        mov     d ds:[g2pt_u8_p_src_3-4], ebx
        mov     d ds:[g2pt_u8_p_src_4-4], ebx
        mov     d ds:[g2pt_u8_p_src_5-4], ebx
        mov     d ds:[g2pt_u8_p_src_6-4], ebx
        mov     d ds:[g2pt_u8_p_src_7-4], ebx
        mov     u8_last_p_src, ebx
g2pt_u8_skip_p_src:
        cmp     dl, cl
        jz      g2pt_u8_skip_shift_v

        mov     b ds:[g2pt_u8_shift_v_0-1], cl
        mov     b ds:[g2pt_u8_shift_v_1-1], cl
        mov     b ds:[g2pt_u8_shift_v_2-1], cl
        mov     b ds:[g2pt_u8_shift_v_3-1], cl
        mov     b ds:[g2pt_u8_shift_v_4-1], cl
        mov     b ds:[g2pt_u8_shift_v_5-1], cl
        mov     b ds:[g2pt_u8_shift_v_6-1], cl
        mov     b ds:[g2pt_u8_shift_v_7-1], cl
        mov     u8_last_shift_v, cl
g2pt_u8_skip_shift_v:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret



   _FUNCDEF_ASM g2pt16_unlit_8

        mov     ebx, esi
        mov     edi, eax

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_0:

        shr     edi, 0ach
g2pt16_u8_shift_v_0:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_0:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_0:
        mov     edi, eax

        mov     w _g2pt_buffer, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_1:

        shr     edi, 0ach
g2pt16_u8_shift_v_1:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_1:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_1:
        mov     edi, eax

        mov     w _g2pt_buffer+2, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_2:

        shr     edi, 0ach
g2pt16_u8_shift_v_2:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_2:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_2:
        mov     edi, eax

        mov     w _g2pt_buffer+4, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_3:

        shr     edi, 0ach
g2pt16_u8_shift_v_3:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_3:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_3:
        mov     edi, eax

        mov     w _g2pt_buffer+6, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_4:

        shr     edi, 0ach
g2pt16_u8_shift_v_4:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_4:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_4:
        mov     edi, eax

        mov     w _g2pt_buffer+8, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_5:

        shr     edi, 0ach
g2pt16_u8_shift_v_5:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_5:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_5:
        mov     edi, eax

        mov     w _g2pt_buffer+10, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_6:

        shr     edi, 0ach
g2pt16_u8_shift_v_6:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_6:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_6:
        mov     edi, eax

        mov     w _g2pt_buffer+12, cx
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt16_u8_mask_v_7:

        shr     edi, 0ach
g2pt16_u8_shift_v_7:
        and     ebx, 0deadbeefh
g2pt16_u8_mask_u_7:

        add     esi, edx
        add     eax, ebp

        mov     cx, 0deadbeefh[edi + 2*ebx]
g2pt16_u8_p_src_7:

        mov     w _g2pt_buffer+14, cx
        jmp     [_g2pt_unlit_8_chain]

; setup
;
; takes pointer to bitmap in eax

  _FUNCDEF g2pt16_unlit_8_setup,1
        push    esi
        push    edi

        push    ebx
        push    edx

        push    ecx
        mov     ebx, GRS_BITMAP_BITS[eax]

        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

;eax = mask_u, edx = mask_v, ebx = p_src, cl = shift_v
        mov     esi, u8_16_last_mask_u
        mov     edi, u8_16_last_mask_v

        cmp     esi, eax
        jz      g2pt16_u8_skip_mask_u

        mov     d ds:[g2pt16_u8_mask_u_0-4], eax
        mov     d ds:[g2pt16_u8_mask_u_1-4], eax
        mov     d ds:[g2pt16_u8_mask_u_2-4], eax
        mov     d ds:[g2pt16_u8_mask_u_3-4], eax
        mov     d ds:[g2pt16_u8_mask_u_4-4], eax
        mov     d ds:[g2pt16_u8_mask_u_5-4], eax
        mov     d ds:[g2pt16_u8_mask_u_6-4], eax
        mov     d ds:[g2pt16_u8_mask_u_7-4], eax
        mov     u8_16_last_mask_u, eax
g2pt16_u8_skip_mask_u:
        cmp     edi, edx
        jz      g2pt16_u8_skip_mask_v

        mov     d ds:[g2pt16_u8_mask_v_0-4], edx
        mov     d ds:[g2pt16_u8_mask_v_1-4], edx
        mov     d ds:[g2pt16_u8_mask_v_2-4], edx
        mov     d ds:[g2pt16_u8_mask_v_3-4], edx
        mov     d ds:[g2pt16_u8_mask_v_4-4], edx
        mov     d ds:[g2pt16_u8_mask_v_5-4], edx
        mov     d ds:[g2pt16_u8_mask_v_6-4], edx
        mov     d ds:[g2pt16_u8_mask_v_7-4], edx
        mov     u8_16_last_mask_v, edx
g2pt16_u8_skip_mask_v:

        mov     eax, u8_16_last_p_src
        mov     dl, u8_16_last_shift_v

        cmp     eax, ebx
        jz      g2pt16_u8_skip_p_src

        mov     d ds:[g2pt16_u8_p_src_0-4], ebx
        mov     d ds:[g2pt16_u8_p_src_1-4], ebx
        mov     d ds:[g2pt16_u8_p_src_2-4], ebx
        mov     d ds:[g2pt16_u8_p_src_3-4], ebx
        mov     d ds:[g2pt16_u8_p_src_4-4], ebx
        mov     d ds:[g2pt16_u8_p_src_5-4], ebx
        mov     d ds:[g2pt16_u8_p_src_6-4], ebx
        mov     d ds:[g2pt16_u8_p_src_7-4], ebx
        mov     u8_16_last_p_src, ebx
g2pt16_u8_skip_p_src:
        cmp     dl, cl
        jz      g2pt16_u8_skip_shift_v

        mov     b ds:[g2pt16_u8_shift_v_0-1], cl
        mov     b ds:[g2pt16_u8_shift_v_1-1], cl
        mov     b ds:[g2pt16_u8_shift_v_2-1], cl
        mov     b ds:[g2pt16_u8_shift_v_3-1], cl
        mov     b ds:[g2pt16_u8_shift_v_4-1], cl
        mov     b ds:[g2pt16_u8_shift_v_5-1], cl
        mov     b ds:[g2pt16_u8_shift_v_6-1], cl
        mov     b ds:[g2pt16_u8_shift_v_7-1], cl
        mov     u8_16_last_shift_v, cl
g2pt16_u8_skip_shift_v:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret



  _FUNCDEF_ASM g2pt_lit_8
        xor     ecx, ecx
        mov     ebx, esi                ; u->u_temp

        mov     edi, eax                ; v->v_temp
        mov     ch, b _g2pt_lit_buffer

        shr     ebx, 16
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_0:

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_0:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_0:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_0:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_0:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_1:

        mov     b _g2pt_buffer, cl      ; write pixel
        mov     ch, b _g2pt_lit_buffer+1

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_1:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_1:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_1:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_1:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_2:

        mov     b _g2pt_buffer+1, cl    ; write pixel
        mov     ch, b _g2pt_lit_buffer+2

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_2:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_2:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_2:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_2:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_3:

        mov     b _g2pt_buffer+2, cl    ; write pixel
        mov     ch, b _g2pt_lit_buffer+3

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_3:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_3:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_3:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_3:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_4:

        mov     b _g2pt_buffer+3, cl    ; write pixel
        mov     ch, b _g2pt_lit_buffer+4

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_4:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_4:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_4:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_4:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_5:

        mov     b _g2pt_buffer+4, cl    ; write pixel
        mov     ch, b _g2pt_lit_buffer+5

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_5:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_5:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_5:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_5:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_6:

        mov     b _g2pt_buffer+5, cl    ; write pixel
        mov     ch, b _g2pt_lit_buffer+6

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_6:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_6:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_6:
        mov     ebx, esi                ; u->u_temp

        mov     edi, eax                ; v->v_temp
        shr     ebx, 16

        mov     cl, 0deadbeefh[ecx]
g2pt_l8_light_table_6:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt_l8_mask_v_7:

        shr     edi, 0ach               ; shift v_temp
g2pt_l8_shift_v_7:
        and     ebx, 0deadbeefh
g2pt_l8_mask_u_7:

        mov     b _g2pt_buffer+6, cl    ; write pixel
        mov     ch, b _g2pt_lit_buffer+7

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt_l8_p_src_7:
        add     esi, edx                ; inc u

        add     eax, ebp                ; inc v
        mov     cl, 0deadbeefh[ecx]     ; AGI
g2pt_l8_light_table_7:

        mov     b _g2pt_buffer+7, cl    ; write pixel
        jmp     [_g2pt_lit_8_chain]


; setup
  _FUNCDEF g2pt_lit_8_setup,1
        push    esi
        push    edi

        push    ebx
        push    edx

        push    ecx
        mov     ebx, GRS_BITMAP_BITS[eax]

        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

;eax = mask_u, edx = mask_v, ebx = p_src, cl = shift_v
        mov     esi, l8_last_mask_u
        mov     edi, l8_last_mask_v

        cmp     esi, eax
        jz      g2pt_l8_skip_mask_u

        mov     d ds:[g2pt_l8_mask_u_0-4], eax
        mov     d ds:[g2pt_l8_mask_u_1-4], eax
        mov     d ds:[g2pt_l8_mask_u_2-4], eax
        mov     d ds:[g2pt_l8_mask_u_3-4], eax
        mov     d ds:[g2pt_l8_mask_u_4-4], eax
        mov     d ds:[g2pt_l8_mask_u_5-4], eax
        mov     d ds:[g2pt_l8_mask_u_6-4], eax
        mov     d ds:[g2pt_l8_mask_u_7-4], eax
        mov     l8_last_mask_u, eax
g2pt_l8_skip_mask_u:
        cmp     edi, edx
        jz      g2pt_l8_skip_mask_v

        mov     d ds:[g2pt_l8_mask_v_0-4], edx
        mov     d ds:[g2pt_l8_mask_v_1-4], edx
        mov     d ds:[g2pt_l8_mask_v_2-4], edx
        mov     d ds:[g2pt_l8_mask_v_3-4], edx
        mov     d ds:[g2pt_l8_mask_v_4-4], edx
        mov     d ds:[g2pt_l8_mask_v_5-4], edx
        mov     d ds:[g2pt_l8_mask_v_6-4], edx
        mov     d ds:[g2pt_l8_mask_v_7-4], edx
        mov     l8_last_mask_v, edx
g2pt_l8_skip_mask_v:

        mov     eax, l8_last_p_src
        mov     dl, l8_last_shift_v

        cmp     eax, ebx
        jz      g2pt_l8_skip_p_src

        mov     d ds:[g2pt_l8_p_src_0-4], ebx
        mov     d ds:[g2pt_l8_p_src_1-4], ebx
        mov     d ds:[g2pt_l8_p_src_2-4], ebx
        mov     d ds:[g2pt_l8_p_src_3-4], ebx
        mov     d ds:[g2pt_l8_p_src_4-4], ebx
        mov     d ds:[g2pt_l8_p_src_5-4], ebx
        mov     d ds:[g2pt_l8_p_src_6-4], ebx
        mov     d ds:[g2pt_l8_p_src_7-4], ebx
        mov     l8_last_p_src, ebx
g2pt_l8_skip_p_src:
        cmp     dl, cl
        jz      g2pt_l8_skip_shift_v

        mov     b ds:[g2pt_l8_shift_v_0-1], cl
        mov     b ds:[g2pt_l8_shift_v_1-1], cl
        mov     b ds:[g2pt_l8_shift_v_2-1], cl
        mov     b ds:[g2pt_l8_shift_v_3-1], cl
        mov     b ds:[g2pt_l8_shift_v_4-1], cl
        mov     b ds:[g2pt_l8_shift_v_5-1], cl
        mov     b ds:[g2pt_l8_shift_v_6-1], cl
        mov     b ds:[g2pt_l8_shift_v_7-1], cl
        mov     l8_last_shift_v, cl
g2pt_l8_skip_shift_v:

        mov     ebx, l8_last_light_table
        mov     edx, _grd_light_table

        cmp     ebx, edx
        jz      g2pt_l8_skip_light_table

        mov     d ds:[g2pt_l8_light_table_0-4], edx
        mov     d ds:[g2pt_l8_light_table_1-4], edx
        mov     d ds:[g2pt_l8_light_table_2-4], edx
        mov     d ds:[g2pt_l8_light_table_3-4], edx
        mov     d ds:[g2pt_l8_light_table_4-4], edx
        mov     d ds:[g2pt_l8_light_table_5-4], edx
        mov     d ds:[g2pt_l8_light_table_6-4], edx
        mov     d ds:[g2pt_l8_light_table_7-4], edx
        mov     l8_last_light_table, edx
g2pt_l8_skip_light_table:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret



  _FUNCDEF_ASM g2pt16_lit_8
        xor     ecx, ecx
        mov     ebx, esi                ; u->u_temp

        mov     edi, eax                ; v->v_temp
        mov     ch, b _g2pt_lit_buffer

        shr     ebx, 16
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_0:

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_0:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_0:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_0:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_0:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_1:

        mov     w _g2pt_buffer, cx      ; write pixel
        mov     ch, b _g2pt_lit_buffer+1

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_1:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_1:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_1:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_1:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_2:

        mov     w _g2pt_buffer+2, cx    ; write pixel
        mov     ch, b _g2pt_lit_buffer+2

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_2:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_2:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_2:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_2:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_3:

        mov     w _g2pt_buffer+4, cx    ; write pixel
        mov     ch, b _g2pt_lit_buffer+3

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_3:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_3:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_3:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_3:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_4:

        mov     w _g2pt_buffer+6, cx    ; write pixel
        mov     ch, b _g2pt_lit_buffer+4

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_4:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_4:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_4:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_4:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_5:

        mov     w _g2pt_buffer+8, cx    ; write pixel
        mov     ch, b _g2pt_lit_buffer+5

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_5:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_5:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_5:
        mov     ebx, esi                ; u->u_temp

        shr     ebx, 16
        mov     edi, eax                ; v->v_temp

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_5:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_6:

        mov     w _g2pt_buffer+10, cx    ; write pixel
        mov     ch, b _g2pt_lit_buffer+6

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_6:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_6:

        add     esi, edx                ; inc u
        add     eax, ebp                ; inc v

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_6:
        mov     ebx, esi                ; u->u_temp

        mov     edi, eax                ; v->v_temp
        shr     ebx, 16

        mov     cx, 0deadbeefh[2*ecx]
g2pt16_l8_light_table_6:
        and     edi, 0deadbeefh         ; mask v_temp
g2pt16_l8_mask_v_7:

        shr     edi, 0ach               ; shift v_temp
g2pt16_l8_shift_v_7:
        and     ebx, 0deadbeefh
g2pt16_l8_mask_u_7:

        mov     w _g2pt_buffer+12, cx    ; write pixel
        mov     ch, b _g2pt_lit_buffer+7

        mov     cl, 0deadbeefh[edi+ebx] ; read pixel
g2pt16_l8_p_src_7:
        add     esi, edx                ; inc u

        add     eax, ebp                ; inc v
        mov     cx, 0deadbeefh[2*ecx]     ; AGI
g2pt16_l8_light_table_7:

        mov     w _g2pt_buffer+14, cx    ; write pixel
        jmp     [_g2pt_lit_8_chain]


; setup
  _FUNCDEF g2pt16_lit_8_setup,1
        push    esi
        push    edi

        push    ebx
        push    edx

        push    ecx
        mov     ebx, GRS_BITMAP_BITS[eax]

        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

;eax = mask_u, edx = mask_v, ebx = p_src, cl = shift_v
        mov     esi, l8_16_last_mask_u
        mov     edi, l8_16_last_mask_v

        cmp     esi, eax
        jz      g2pt16_l8_skip_mask_u

        mov     d ds:[g2pt16_l8_mask_u_0-4], eax
        mov     d ds:[g2pt16_l8_mask_u_1-4], eax
        mov     d ds:[g2pt16_l8_mask_u_2-4], eax
        mov     d ds:[g2pt16_l8_mask_u_3-4], eax
        mov     d ds:[g2pt16_l8_mask_u_4-4], eax
        mov     d ds:[g2pt16_l8_mask_u_5-4], eax
        mov     d ds:[g2pt16_l8_mask_u_6-4], eax
        mov     d ds:[g2pt16_l8_mask_u_7-4], eax
        mov     l8_16_last_mask_u, eax
g2pt16_l8_skip_mask_u:
        cmp     edi, edx
        jz      g2pt16_l8_skip_mask_v

        mov     d ds:[g2pt16_l8_mask_v_0-4], edx
        mov     d ds:[g2pt16_l8_mask_v_1-4], edx
        mov     d ds:[g2pt16_l8_mask_v_2-4], edx
        mov     d ds:[g2pt16_l8_mask_v_3-4], edx
        mov     d ds:[g2pt16_l8_mask_v_4-4], edx
        mov     d ds:[g2pt16_l8_mask_v_5-4], edx
        mov     d ds:[g2pt16_l8_mask_v_6-4], edx
        mov     d ds:[g2pt16_l8_mask_v_7-4], edx
        mov     l8_16_last_mask_v, edx
g2pt16_l8_skip_mask_v:

        mov     eax, l8_16_last_p_src
        mov     dl, l8_16_last_shift_v

        cmp     eax, ebx
        jz      g2pt16_l8_skip_p_src

        mov     d ds:[g2pt16_l8_p_src_0-4], ebx
        mov     d ds:[g2pt16_l8_p_src_1-4], ebx
        mov     d ds:[g2pt16_l8_p_src_2-4], ebx
        mov     d ds:[g2pt16_l8_p_src_3-4], ebx
        mov     d ds:[g2pt16_l8_p_src_4-4], ebx
        mov     d ds:[g2pt16_l8_p_src_5-4], ebx
        mov     d ds:[g2pt16_l8_p_src_6-4], ebx
        mov     d ds:[g2pt16_l8_p_src_7-4], ebx
        mov     l8_16_last_p_src, ebx
g2pt16_l8_skip_p_src:
        cmp     dl, cl
        jz      g2pt16_l8_skip_shift_v

        mov     b ds:[g2pt16_l8_shift_v_0-1], cl
        mov     b ds:[g2pt16_l8_shift_v_1-1], cl
        mov     b ds:[g2pt16_l8_shift_v_2-1], cl
        mov     b ds:[g2pt16_l8_shift_v_3-1], cl
        mov     b ds:[g2pt16_l8_shift_v_4-1], cl
        mov     b ds:[g2pt16_l8_shift_v_5-1], cl
        mov     b ds:[g2pt16_l8_shift_v_6-1], cl
        mov     b ds:[g2pt16_l8_shift_v_7-1], cl
        mov     l8_16_last_shift_v, cl
g2pt16_l8_skip_shift_v:

        mov     ebx, l8_16_last_light_table
        mov     edx, _grd_ltab816

        cmp     ebx, edx
        jz      g2pt16_l8_skip_light_table

        mov     d ds:[g2pt16_l8_light_table_0-4], edx
        mov     d ds:[g2pt16_l8_light_table_1-4], edx
        mov     d ds:[g2pt16_l8_light_table_2-4], edx
        mov     d ds:[g2pt16_l8_light_table_3-4], edx
        mov     d ds:[g2pt16_l8_light_table_4-4], edx
        mov     d ds:[g2pt16_l8_light_table_5-4], edx
        mov     d ds:[g2pt16_l8_light_table_6-4], edx
        mov     d ds:[g2pt16_l8_light_table_7-4], edx
        mov     l8_16_last_light_table, edx
g2pt16_l8_skip_light_table:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret



; NEXT WE HAVE THE 8-BYTE FILTERS

; these are called with identical
; parameters as above, and have to
; process things without stomping
; registers

;
;   8 pixels of transparency
;  edi is the destination

  _FUNCDEF_ASM g2pt_transp_8
        mov    edi,_g2pt_dest
        push   ebx

        push   eax
        mov    eax,_g2pt_buffer

; if all four pixels are transparent, don't do anything more

        test   eax,eax
        jz     g2pt_t8_long_1

; check for all four non-zero
        mov    ecx,eax
        and    eax,07f7f7f7fh

        sub    eax,01010101h

        and    eax,080808080h
        jnz    g2pt_t8_byte_test_1

        mov    [edi],ecx
        jmp    g2pt_t8_long_1

g2pt_t8_byte_test_1:
        cmp    cl,0
        je     g2pt_t8_byte_1

        mov    [edi],cl
        nop

g2pt_t8_byte_1:
        cmp    ch,0
        je     g2pt_t8_byte_2

        mov    1[edi],ch
        nop

g2pt_t8_byte_2:
        shr    ecx,16

        cmp    cl,0
        je     g2pt_t8_byte_3

        mov    2[edi],cl
        nop

g2pt_t8_byte_3:
        cmp    ch,0
        je     g2pt_t8_byte_4

        mov    3[edi],ch
        nop

g2pt_t8_byte_4:
g2pt_t8_long_1:

        mov    eax,_g2pt_buffer+4
        nop

; if all four pixels are transparent, don't do anything more

        test   eax,eax
        jz     g2pt_t8_long_2

        mov    ecx,eax
        and    eax,07f7f7f7fh

        sub    eax,001010101h

        and    eax,080808080h
        jnz    g2pt_t8_byte_test_2

        mov    [edi+4],ecx
        jmp    g2pt_t8_long_2

g2pt_t8_byte_test_2:
        cmp    cl,0
        je     g2pt_t8_byte_5

        mov    4[edi],cl
        nop

g2pt_t8_byte_5:
        cmp    ch,0
        je     g2pt_t8_byte_6

        mov    5[edi],ch
        nop

g2pt_t8_byte_6:
        shr    ecx,16

        cmp    cl,0
        je     g2pt_t8_byte_7

        mov    6[edi],cl
        nop

g2pt_t8_byte_7:
        cmp    ch,0
        je     g2pt_t8_byte_8

        mov    7[edi],ch
        nop

g2pt_t8_byte_8:
g2pt_t8_long_2:

        pop    eax
        pop    ebx

        add    edi,8
        ret


  _FUNCDEF_ASM g2pt816_transp_8
        push   edx
        mov    edi, _g2pt_dest

        push   ebx
        mov    edx, _pixpal

        push   eax
        mov    eax, _g2pt_buffer

; if all four pixels are transparent, don't do anything more

        test   eax,eax
        jz     g2pt816_t8_long_1

        xor    ebx, ebx
        xor    ecx, ecx

        mov    bl, al
        mov    cl, ah

g2pt816_t8_byte_0:
        cmp    al,0
        je     g2pt816_t8_byte_1

        mov    bx, [edx + ebx*2]

        mov    [edi],bx

g2pt816_t8_byte_1:
        cmp    ah,0
        je     g2pt816_t8_byte_2

        mov    cx, [edx + ecx*2]

        mov    2[edi],cx

g2pt816_t8_byte_2:
        shr    eax,16
        jz     g2pt816_t8_long_1

        xor    ebx, ebx
        xor    ecx, ecx

        mov    bl, al
        mov    cl, ah

        cmp    al,0
        je     g2pt816_t8_byte_3

        mov    bx, [edx + 2*ebx]

        mov    4[edi],bx

g2pt816_t8_byte_3:
        cmp    ah,0
        je     g2pt816_t8_byte_4

        mov    cx, [edx + 2*ecx]

        mov    6[edi],cx

g2pt816_t8_byte_4:
g2pt816_t8_long_1:

        mov    eax,_g2pt_buffer+4
        nop

; if all four pixels are transparent, don't do anything more

        test   eax,eax
        jz     g2pt816_t8_long_2


        xor    ebx, ebx
        xor    ecx, ecx

        mov    bl, al
        mov    cl, ah

        cmp    al,0
        je     g2pt816_t8_byte_5

        mov    bx, [edx + 2*ebx]

        mov    8[edi],bx

g2pt816_t8_byte_5:
        cmp    ah,0
        je     g2pt816_t8_byte_6

        mov    cx, [edx + 2*ecx]

        mov    10[edi],cx

g2pt816_t8_byte_6:
        shr    eax,16
        jz     g2pt816_t8_long_2

        xor    ebx, ebx
        xor    ecx, ecx

        mov    bl, al
        mov    cl, ah

        cmp    al,0
        je     g2pt816_t8_byte_7

        mov    bx, [edx + 2*ebx]

        mov    12[edi],bx

g2pt816_t8_byte_7:
        cmp    ah,0
        je     g2pt816_t8_byte_8

        mov    cx, [edx + 2*ecx]

        mov    14[edi],cx

g2pt816_t8_byte_8:
g2pt816_t8_long_2:

        pop    eax
        pop    ebx

        pop    edx
        add    edi,16
        ret


;   8 pixels of translucency
;  this must come last in chain since it needs to read from real buffer

  _FUNCDEF_ASM g2pt_tluc_8
        ; save the final destination; we don't want to
        ; write temp data there in case it's the screen
        ; so we redirect to the buffer

        push   eax

        push   ebx
        push   esi

        mov    edi,_g2pt_dest
        mov    esi,_g2pt_tluc_table

        xor    eax,eax
        xor    ebx,ebx

        xor    ecx,ecx
        xor    edx,edx

        mov    al,2[edi]
        mov    bl,3[edi]

        mov    ah,b _g2pt_buffer+2
        mov    bh,b _g2pt_buffer+3

        mov    cl,0[edi]
        mov    dl,1[edi]

        mov    al,[esi+eax]
        mov    ch,b _g2pt_buffer+0

        mov    ah,[esi+ebx]
        mov    dh,b _g2pt_buffer+1

        shl    eax,16
        mov    cl,[esi+ecx]

        mov    ch,[esi+edx]
        mov    dl,5[edi]

        or     eax,ecx
        mov    cl,4[edi]

        mov    [edi],eax
        mov    ch,b _g2pt_buffer+4

        xor    eax,eax
        mov    dh,b _g2pt_buffer+5

        mov    al,6[edi]
        mov    cl,[esi+ecx]

        mov    bl,7[edi]
        mov    ah,b _g2pt_buffer+6

        mov    bh,b _g2pt_buffer+7
        mov    ch,[esi+edx]

        mov    al,[esi+eax]

        mov    ah,[esi+ebx]
        pop    esi

        shl    eax,16
        pop    ebx

        or     eax,ecx

        mov    4[edi],eax
        add    edi,8

        pop    eax
        ret

  _FUNCDEF_ASM g2pt_pal816_8
        push   esi
        push   eax

        mov    edi, _g2pt_dest
        mov    esi, _pixpal

        xor    eax, eax
        xor    ebx, ebx

        xor    ecx, ecx
        xor    edx, edx

        mov    al, b _g2pt_buffer
        mov    bl, b _g2pt_buffer+1

        mov    cl, b _g2pt_buffer+2
        mov    dl, b _g2pt_buffer+3

        mov    ax, [esi + 2*eax]
        mov    bx, [esi + 2*ebx]

        mov    cx, [esi + 2*ecx]
        mov    dx, [esi + 2*edx]

        mov    [edi],   ax
        mov    [edi+2], bx

        mov    [edi+4], cx
        mov    [edi+6], dx

        xor    eax, eax
        xor    ebx, ebx

        xor    ecx, ecx
        xor    edx, edx

        mov    al, b _g2pt_buffer+4
        mov    bl, b _g2pt_buffer+5

        mov    cl, b _g2pt_buffer+6
        mov    dl, b _g2pt_buffer+7

        mov    ax, [esi + 2*eax]
        mov    bx, [esi + 2*ebx]

        mov    cx, [esi + 2*ecx]
        mov    dx, [esi + 2*edx]

        mov    [edi+8],  ax
        mov    [edi+10], bx

        mov    [edi+12], cx
        mov    [edi+14], dx

        pop    eax
        pop    esi

        add    edi,16
        ret

  _FUNCDEF_ASM g2pt_generic_8

        mov    edi,_g2pt_dest
        mov    ecx,_g2pt_buffer

        mov    edx,_g2pt_buffer+4
        mov    [edi],ecx

        mov    4[edi],edx
        add    edi,8

        ret


  _FUNCDEF_ASM g2pt16_generic_8

        mov    edi,_g2pt_dest
        mov    ecx,_g2pt_buffer

        mov    edx,_g2pt_buffer+4
        mov    [edi],ecx

        mov    ecx,_g2pt_buffer+8
        mov    4[edi],edx

        mov    edx,_g2pt_buffer+12
        mov    8[edi],ecx

        mov    12[edi],edx
        add    edi,16

        ret


  _FUNCDEF_ASM g2pt_generic_preload_8
        mov    edi,_g2pt_dest

        mov    ecx,_g2pt_buffer
        mov    edx,_g2pt_buffer+4

        mov    [edi],ecx
        mov    4[edi],edx

        mov    ecx,8[edi]
        add    edi,8

        ret

  _FUNCDEF_ASM g2pt_clut_8
        push   esi
        push   eax

        mov    esi,_g2pt_clut
        nop

        xor    ecx,ecx
        xor    eax,eax

        xor    ebx,ebx
        xor    edx,edx

        mov    al,b _g2pt_buffer
        mov    bl,b _g2pt_buffer+1

        mov    cl,b _g2pt_buffer+2
        mov    dl,b _g2pt_buffer+3

        mov    al,[esi+eax]
        mov    bl,[esi+ebx]

        mov    b _g2pt_buffer,al
        mov    cl,[esi+ecx]

        mov    b _g2pt_buffer+1,bl
        mov    dl,[esi+edx]

        mov    b _g2pt_buffer+2,cl
        mov    al,b _g2pt_buffer+4

        mov    b _g2pt_buffer+3,dl
        mov    bl,b _g2pt_buffer+5

        mov    cl,b _g2pt_buffer+6
        mov    dl,b _g2pt_buffer+7

        mov    al,[esi+eax]
        mov    bl,[esi+ebx]

        mov    b _g2pt_buffer+4,al
        mov    cl,[esi+ecx]

        mov    b _g2pt_buffer+5,bl
        mov    dl,[esi+edx]

        mov    b _g2pt_buffer+6,cl
        mov    b _g2pt_buffer+7,dl

        pop    eax
        pop    esi

        jmp    [_g2pt_clut_8_chain]


if 0
;
;  PALETTE FLATLIT 8-RUN
;
;     cycle 0:   write bl   add  ebp
;     cycle 1:   write bh                                    write al
;     cycle 2:   copy ebx->ecx           al = [ecx]
;     cycle 3:   mask ecx                add  eax

  _FUNCDEF_ASM g2pt_palflat_8
        mov    ecx,ebp
        mov    edi,_g2pt_tmap_ptr

        mov    ch,b _g2pt_lit_buffer

        add    eax,ecx
        mov    ebp,ecx

        mov    ah,[edi+ebx]

        adc    bh,dh
        add    esi,edx

        adc    bl,dl

        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_1:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_2:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+1,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_3:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+2,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_4:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+3,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_5:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+4,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_6:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+5,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_pf8_mask_7:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+6,ah

        and    ebx,0deadbeefh
g2pt_pf8_mask_8:
        mov    ah,[edi+ecx]

        push   eax
        add    eax,ebp

        and    ebp,0ffff0000h
        mov    b _g2pt_buffer+7,ah

        pop    eax
        jmp    [_g2pt_palflat_8_chain]


; setup
  _FUNCDEF g2pt_palflat_8_setup,0
        push   eax
        push   ebx

        mov    eax,_g2pt_tmap_mask
        mov    ebx,d ds:g2pt_pf8_mask_1-4

        cmp    eax,ebx
        je     g2pt_pf8_setup_done

        mov    d ds:g2pt_pf8_mask_1-4,eax
        mov    d ds:g2pt_pf8_mask_2-4,eax
        mov    d ds:g2pt_pf8_mask_3-4,eax
        mov    d ds:g2pt_pf8_mask_4-4,eax
        mov    d ds:g2pt_pf8_mask_5-4,eax
        mov    d ds:g2pt_pf8_mask_6-4,eax
        mov    d ds:g2pt_pf8_mask_7-4,eax
        mov    d ds:g2pt_pf8_mask_8-4,eax

        mov    d ds:g2pt_if8_mask_1-4,eax
        mov    d ds:g2pt_if8_mask_2-4,eax
        mov    d ds:g2pt_if8_mask_3-4,eax
        mov    d ds:g2pt_if8_mask_4-4,eax
        mov    d ds:g2pt_if8_mask_5-4,eax
        mov    d ds:g2pt_if8_mask_6-4,eax
        mov    d ds:g2pt_if8_mask_7-4,eax
        mov    d ds:g2pt_if8_mask_8-4,eax

g2pt_pf8_setup_done:
        pop    ebx
        pop    eax

        ret



;
;  PALETTE LIT 8-RUN FILTER
;

  _FUNCDEF_ASM g2pt_pallit_8
        mov    edi,_g2pt_buffer
        mov    ecx,_g2pt_lit_buffer

        add    edi,ecx
        mov    ecx,_g2pt_lit_buffer+4

        mov    _g2pt_buffer,edi
        mov    edi,_g2pt_buffer+4

        add    edi,ecx

        mov    _g2pt_buffer+4,edi
        jmp    [_g2pt_pallit_8_chain]

  _FUNCDEF_ASM g2pt_pallit_store_8
        mov    edi,_g2pt_dest
        mov    ecx,_g2pt_buffer

        mov    edx,_g2pt_lit_buffer
        mov    ebp,_g2pt_buffer+4

        add    ecx,edx
        mov    edx,_g2pt_lit_buffer+4

        mov    [edi],ecx
        add    edx,ebp

        mov    [edi+4],edx
        add    edi,8

        ret

; we use the following chain for clutted surfaces:
;   g2pt_unlit_8 -> g2pt_pallit_8 -> g2pt_clut_8 -> g2pt_generic_8;
;   so here is the last three in one step

  _FUNCDEF_ASM g2pt_pallit_clut_store_8
        push   edx
        push   eax

        push   ebx
        push   esi

        push   ebp
        mov    esi,_g2pt_clut

        mov    ecx,_g2pt_buffer
        mov    ebp,_g2pt_lit_buffer

;   edi = final dest
;   esi = clut

        add    ecx,ebp
        xor    eax,eax

        mov    al,cl
        xor    ebx,ebx

        mov    edi,_g2pt_dest
        mov    bl,ch

        shr    ecx,16
        mov    dl,[esi+eax]

        mov    al,cl
        mov    dh,[esi+ebx]

        shl    edx,16
        mov    bl,ch

        mov    dl,[esi+eax]
        mov    ecx,_g2pt_buffer+4

        mov    dh,[esi+ebx]
        mov    ebp,_g2pt_lit_buffer+4

        ror    edx,16
        add    ecx,ebp

        mov    [edi],edx
        mov    al,cl

        mov    bl,ch
        add    edi,8

        shr    ecx,16
        mov    dl,[esi+eax]

        mov    al,cl
        mov    dh,[esi+ebx]

        shl    edx,16
        mov    bl,ch

        mov    dl,[esi+eax]
        pop    ebp

        mov    dh,[esi+ebx]
        pop    esi

        ror    edx,16
        pop    ebx

        mov    [edi-4],edx
        pop    eax

        pop    edx
        ret

  _FUNCDEF_ASM g2pt_dest_pallit_8
        mov    edi,_g2pt_dest
        mov    al,8

        add    edi,8

        mov    _g2pt_dest,edi
        sub    edi,8

        jmp    g2pt_dest_pallit_n_

; UNLIT N-RUN, non-power of two, non-wrapping
;
;  EAX  =  v_lo << 16
;  ESI  =  u_lo << 16
;  EBP  = dv_lo << 16
;  EDX  = du_lo << 16
;  EBX  = source + u_hi + v_hi * source_row

  _FUNCDEF_ASM g2pt_unlit_arb_8
        mov    dl,[ebx]
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer,dl
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer+1,al
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer+2,al
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer+3,al
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer+4,al
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer+5,al
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    b _g2pt_buffer+6,al
        mov    al,[ebx]

        mov    edi,_g2pt_step_table[edi*4+4]
        mov    b _g2pt_buffer+7,al

        adc    ebx,edi
        jmp    [_g2pt_unlit_8_chain]


; massively unrolled pallit core

  _FUNCDEF_ASM g2pt_pallit_perspective_core
        mov    al,cl       ; number of 8 pixels blocks to do
        nop

g2pt_pallit_perspective_loop:
        nop
        nop

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

    ; update lighting

        mov    edx,_g2pt_light
        mov    ebp,_g2pt_dlight_8

        add    edx,ebp
        mov    cl,b _g2pt_light+2      ; compute previous light level

        mov    _g2pt_light,edx

        shr    edx,16

        cmp    cl,dl
        jne    g2pt_pallit_core_do_lit_run

        mov    b _g2pt_lit_buffer,cl
        mov    edx,d _g2pt_float_buf

        ror    edx,16
        mov    ecx,d _g2pt_float_buf2

        shr    ecx,16
        mov    ebp,d _g2pt_float_buf2
        
        shl    ebp,16
        mov    dh,cl

        mov    ecx,ebp
        mov    edi,_g2pt_tmap_ptr

        mov    ch,b _g2pt_lit_buffer

        add    eax,ecx
        mov    ebp,ecx

        mov    ah,[edi+ebx]

        adc    bh,dh
        add    esi,edx

        adc    bl,dl

        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_1:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_2:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+1,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_3:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+2,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_4:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+3,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_5:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+4,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_6:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+5,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
g2pt_if8_mask_7:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    b _g2pt_buffer+6,ah

        and    ebx,0deadbeefh
g2pt_if8_mask_8:
        mov    ah,[edi+ecx]

        mov    [esp-128],eax
        add    eax,ebp

        and    ebp,0ffff0000h
        mov    b _g2pt_buffer+7,ah

        mov    edi,_g2pt_dest
        mov    ecx,_g2pt_buffer

        mov    eax,[esp-128]
        mov    edx,_g2pt_buffer+4

        mov    [edi],ecx
        mov    4[edi],edx

        add    edi,8
        dec    al

        mov    _g2pt_dest,edi
        jnz    g2pt_pallit_perspective_loop

        ret

g2pt_pallit_core_do_lit_run:

        mov    b count_save,al
        mov    edx,_g2pt_light

        mov    ebp,_g2pt_dlight
        sub    edx,ebp
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+7,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+6,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+5,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+4,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+3,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+2,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    b _g2pt_lit_buffer+1,cl
        mov    ecx,edx
        shr    ecx,16
        mov    b _g2pt_lit_buffer,cl

        mov    ebp,d _g2pt_float_buf2
        mov    edx,d _g2pt_float_buf

        ror    edx,16
        mov    ecx,ebp

        ror    ecx,16
        
        shl    ebp,16
        mov    dh,cl

;        call   g2pt_unlit_8_

        mov    _g2pt_dest,edi
        mov    al,b count_save

        dec    al
        jnz    g2pt_pallit_perspective_loop

        ret


; takes:
; cl = count
; esi = u
; eax = v

  _FUNCDEF_ASM g2pt_unlit_perspective_unrolled
ptup_top:
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

        mov     edx, d _g2pt_float_buf
        mov     ebp, d _g2pt_float_buf2

        mov     b count_save,cl
g2pt_un8_common:
        mov     _g2pt_dest, edi

        mov     ebx, esi
        mov     edi, eax

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_0:

        shr     edi, 0ach
g2pt_un8_shift_v_0:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_0:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_0:
        mov     edi, eax

        mov     b _g2pt_buffer, cl
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_1:

        shr     edi, 0ach
g2pt_un8_shift_v_1:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_1:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_1:
        mov     edi, eax

        mov     b _g2pt_buffer+1, ch
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_2:

        shr     edi, 0ach
g2pt_un8_shift_v_2:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_2:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_2:
        mov     edi, eax

        mov     b _g2pt_buffer+2, cl
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_3:

        shr     edi, 0ach
g2pt_un8_shift_v_3:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_3:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_3:
        mov     edi, eax

        mov     b _g2pt_buffer+3, ch
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_4:

        shr     edi, 0ach
g2pt_un8_shift_v_4:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_4:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_4:
        mov     edi, eax

        nop
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_5:

        shr     edi, 0ach
g2pt_un8_shift_v_5:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_5:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_5:
        mov     edi, eax

        shl     ecx, 16
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_6:

        shr     edi, 0ach
g2pt_un8_shift_v_6:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_6:

        add     esi, edx
        add     eax, ebp

        mov     cl, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_6:
        mov     edi, eax

        nop
        mov     ebx, esi

        shr     ebx, 16
        and     edi, 0deadbeefh
g2pt_un8_mask_v_7:

        shr     edi, 0ach
g2pt_un8_shift_v_7:
        and     ebx, 0deadbeefh
g2pt_un8_mask_u_7:

        add     esi, edx
        add     eax, ebp

        mov     ch, 0deadbeefh[edi + ebx]
g2pt_un8_p_src_7:
        mov     edi, _g2pt_dest

        ror     ecx, 16
        mov     edx, _g2pt_buffer

        mov     [edi+4], ecx
        mov     [edi], edx

        mov     cl, b count_save
        add     edi, 8

        dec     cl
        jnz     ptup_top

        ret

; setup
;
; takes pointer to bitmap in eax

  _FUNCDEF g2pt_unlit_8_setup,1
        push    esi
        push    edi

        push    ebx
        push    edx

        push    ecx
        mov     ebx, GRS_BITMAP_BITS[eax]

        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

;eax = mask_u, edx = mask_v, ebx = p_src, cl = shift_v
        mov     esi, u8_last_mask_u
        mov     edi, u8_last_mask_v

        cmp     esi, eax
        jz      g2pt16_u8_skip_mask_u

        mov     d ds:[g2pt_un8_mask_u_0-4], eax
        mov     d ds:[g2pt_un8_mask_u_1-4], eax
        mov     d ds:[g2pt_un8_mask_u_2-4], eax
        mov     d ds:[g2pt_un8_mask_u_3-4], eax
        mov     d ds:[g2pt_un8_mask_u_4-4], eax
        mov     d ds:[g2pt_un8_mask_u_5-4], eax
        mov     d ds:[g2pt_un8_mask_u_6-4], eax
        mov     d ds:[g2pt_un8_mask_u_7-4], eax
        mov     u8_last_mask_u, eax
g2pt_u8_skip_mask_u:
        cmp     edi, edx
        jz      g2pt_u8_skip_mask_v

        mov     d ds:[g2pt_un8_mask_v_0-4], edx
        mov     d ds:[g2pt_un8_mask_v_1-4], edx
        mov     d ds:[g2pt_un8_mask_v_2-4], edx
        mov     d ds:[g2pt_un8_mask_v_3-4], edx
        mov     d ds:[g2pt_un8_mask_v_4-4], edx
        mov     d ds:[g2pt_un8_mask_v_5-4], edx
        mov     d ds:[g2pt_un8_mask_v_6-4], edx
        mov     d ds:[g2pt_un8_mask_v_7-4], edx
        mov     u8_last_mask_v, edx
g2pt_u8_skip_mask_v:

        mov     eax, u8_last_p_src
        mov     dl, u8_last_shift_v

        cmp     eax, ebx
        jz      g2pt_u8_skip_p_src

        mov     d ds:[g2pt_un8_p_src_0-4], ebx
        mov     d ds:[g2pt_un8_p_src_1-4], ebx
        mov     d ds:[g2pt_un8_p_src_2-4], ebx
        mov     d ds:[g2pt_un8_p_src_3-4], ebx
        mov     d ds:[g2pt_un8_p_src_4-4], ebx
        mov     d ds:[g2pt_un8_p_src_5-4], ebx
        mov     d ds:[g2pt_un8_p_src_6-4], ebx
        mov     d ds:[g2pt_un8_p_src_7-4], ebx
        mov     u8_last_p_src, ebx
g2pt_u8_skip_p_src:
        cmp     dl, cl
        jz      g2pt_u8_skip_shift_v

        mov     b ds:[g2pt_un8_shift_v_0-1], cl
        mov     b ds:[g2pt_un8_shift_v_1-1], cl
        mov     b ds:[g2pt_un8_shift_v_2-1], cl
        mov     b ds:[g2pt_un8_shift_v_3-1], cl
        mov     b ds:[g2pt_un8_shift_v_4-1], cl
        mov     b ds:[g2pt_un8_shift_v_5-1], cl
        mov     b ds:[g2pt_un8_shift_v_6-1], cl
        mov     b ds:[g2pt_un8_shift_v_7-1], cl
        mov     u8_last_shift_v, cl
g2pt_u8_skip_shift_v:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret
endif

_TEXT   ends

END
