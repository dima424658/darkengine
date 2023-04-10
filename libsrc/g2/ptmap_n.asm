; $Header: x:/prj/tech/libsrc/g2/RCS/ptmap_n.asm 1.9 1998/04/06 15:42:10 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptmap_n.asm
;
; Variable length n-pixel loops
; (at most 16 pixels due to buffer constraints)

.486
	.nolist

        include type.inc
        include thunks.inc
	include	macros.inc
        include grs.inc

extd    _g2pt_tmap_ptr,_g2pt_tmap_mask
extd    _g2pt_clut, _g2pt_tluc_table, _g2pt_light_table
extd    _g2pt_buffer, _g2pt_lit_buffer

extd    _g2pt_clut_n_chain
extd    _g2pt_transp_n_chain
extd    _g2pt_tluc_n_chain

extd    _g2pt_step_table

extd    _grd_light_table, _grd_ltab816
extd    _pixpal

	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'

	align	4

temp_ebp dd 0
du_frac  dd 0
dv_frac  dd 0
dv       dd 0
dux      dd 0
dvx      dd 0
un_last_mask_u        dd 0deadbeefh
un_last_mask_v        dd 0deadbeefh
un_last_p_src         dd 0deadbeefh
ln_last_mask_u        dd 0deadbeefh
ln_last_mask_v        dd 0deadbeefh
ln_last_p_src         dd 0deadbeefh
ln_last_light_table   dd 0deadbeefh
un16_last_mask_u      dd 0deadbeefh
un16_last_mask_v      dd 0deadbeefh
un16_last_p_src       dd 0deadbeefh
un816_last_mask_u     dd 0deadbeefh
un816_last_mask_v     dd 0deadbeefh
un816_last_p_src      dd 0deadbeefh
un816_last_pal16      dd 0deadbeefh
ln16_last_mask_u      dd 0deadbeefh
ln16_last_mask_v      dd 0deadbeefh
ln16_last_p_src       dd 0deadbeefh
ln16_last_light_table dd 0deadbeefh
un_last_shift_v       db 0ach
ln_last_shift_v       db 0ach
un16_last_shift_v     db 0ach
un816_last_shift_v    db 0ach
ln16_last_shift_v     db 0ach

_DATA	ends


_TEXT	segment para public USE32 'CODE'

;extn    g2pt_unlit_n,g2pt_unlit_n_setup
;extn    g2pt_pallit_n,g2pt_pallit_n_setup
;extn    g2pt_dest_pallit_n,g2pt_dest_pallit_n_setup
;extn    g2pt_palflat_n,g2pt_palflat_n_setup
;extn    g2pt_lit_n,g2pt_lit_n_setup
;extn    g2pt_clut_n
;extn    g2pt_transp_n
;extn    g2pt_tluc_n
;extn    g2pt_unlit_256_n, g2pt_unlit_256_n_setup
;extn    g2pt_unlit_arb_n


;  first we have all of the texture mappers themselves
;
;  Basically, all of these mappers may have extra state
;  in self-modified things or in global variables, but
;  since they're called there can't be any other state
;  preserved.  They are also free to trash the other
;  registers not listed:

;  The number of bytes to write is found in ECX, and the
;  destination is in EDI.  The tmapper must preserve
;  EDI and return with EDI = EDI + ECX; it can trash ECX.
;  VARIABLE LENGTH MAPPERS

;
;  UNLIT N-RUN
;

; u esi
; v eax
; du edx
; dv ebp
; u_temp ebx
; v_temp edx
; count ecx

  _FUNCDEF_ASM g2pt_unlit_n
        add     edi, ecx
        neg     ecx

        mov     dux, edx                ; store du
        mov     dvx, ebp                ; store dv

        mov     ebx, esi                ; u->u_temp
        mov     edx, eax                ; v->v_temp
                
g2pt_unlit_n_top:
        shr     ebx, 16                 ; shift u_temp
        and     edx, 0deadbeefh         ; mask v_temp
g2pt_un_mask_v:

        shr     edx, 0ach               ; shift v_temp
g2pt_un_shift_v:
        and     ebx, 0deadbeefh         ; mask u_temp
g2pt_un_mask_u:

        add     esi, dux           ; inc u
        add     eax, dvx           ; inc v

        mov     bl, 0deadbeefh[edx+ebx] ; read pixel
g2pt_un_p_src:
        mov     edx, eax                ; v->v_temp

        mov     [edi+ecx], bl           ; write pixel
        mov     ebx, esi                ; u->u_temp

        inc     ecx                     ; done?
        jnz     g2pt_unlit_n_top

        ret


; setup
;
; takes pointer to bitmap in eax

  _FUNCDEF g2pt_unlit_n_setup,1
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

        sub     cl, al                  ;v_shift = (16-bm->wlog)
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

;eax = mask_u, edx = mask_v, ebx = p_src, cl = shift_v
        mov     esi, un_last_mask_u
        mov     edi, un_last_mask_v

        cmp     esi, eax
        jz      g2pt_un_skip_mask_u

        mov     d ds:[g2pt_un_mask_u-4], eax
        mov     un_last_mask_u, eax
g2pt_un_skip_mask_u:
        cmp     edi, edx
        jz      g2pt_un_skip_mask_v

        mov     d ds:[g2pt_un_mask_v-4], edx
        mov     un_last_mask_v, edx
g2pt_un_skip_mask_v:

        mov     eax, un_last_p_src
        mov     dl, un_last_shift_v

        cmp     eax, ebx
        jz      g2pt_un_skip_p_src

        mov     d ds:[g2pt_un_p_src-4], ebx
        mov     un_last_p_src, ebx
g2pt_un_skip_p_src:
        cmp     dl, cl
        jz      g2pt_un_skip_shift_v

        mov     b ds:[g2pt_un_shift_v-1], cl
        mov     un_last_shift_v, cl
g2pt_un_skip_shift_v:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret

;
;  UNLIT 16 to 16 bit N-RUN
;

; u esi
; v eax
; du edx
; dv ebp
; u_temp ebx
; v_temp edx
; count ecx

  _FUNCDEF_ASM g2pt16_unlit_n
        mov     dux, edx                ; store du
        mov     dvx, ebp                ; store dv

        lea     edi, [edi + 2*ecx]
        neg     ecx

        mov     ebx, esi                ; u->u_temp
        mov     edx, eax                ; v->v_temp
                
g2pt16_unlit_n_top:
        shr     ebx, 16                 ; shift u_temp
        and     edx, 0deadbeefh         ; mask v_temp
g2pt16_un_mask_v:

        shr     edx, 0ach               ; shift v_temp
g2pt16_un_shift_v:
        and     ebx, 0deadbeefh         ; mask u_temp
g2pt16_un_mask_u:

        add     esi, dux                ; inc u
        add     eax, dvx                ; inc v

; woo woo! 2 instructions; 4 cycles!

        mov     bx, 0deadbeefh[edx+2*ebx] ; read pixel
g2pt16_un_p_src:

        mov     [edi+2*ecx], bx         ; write pixel

        mov     ebx, esi                ; u->u_temp
        mov     edx, eax                ; v->v_temp

        inc     ecx                     ; done?
        jnz     g2pt16_unlit_n_top

        ret


; setup
;
; takes pointer to bitmap in eax

  _FUNCDEF g2pt16_unlit_n_setup,1
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
        mov     esi, un16_last_mask_u
        mov     edi, un16_last_mask_v

        cmp     esi, eax
        jz      g2pt16_un_skip_mask_u

        mov     d ds:[g2pt16_un_mask_u-4], eax
        mov     un16_last_mask_u, eax
g2pt16_un_skip_mask_u:
        cmp     edi, edx
        jz      g2pt16_un_skip_mask_v

        mov     d ds:[g2pt16_un_mask_v-4], edx
        mov     un16_last_mask_v, edx
g2pt16_un_skip_mask_v:

        mov     eax, un16_last_p_src
        mov     dl, un16_last_shift_v

        cmp     eax, ebx
        jz      g2pt16_un_skip_p_src

        mov     d ds:[g2pt16_un_p_src-4], ebx
        mov     un16_last_p_src, ebx
g2pt16_un_skip_p_src:
        cmp     dl, cl
        jz      g2pt16_un_skip_shift_v

        mov     b ds:[g2pt16_un_shift_v-1], cl
        mov     un16_last_shift_v, cl
g2pt16_un_skip_shift_v:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret


;
;  UNLIT 8 to 16 bit N-RUN
;

; u esi
; v eax
; du edx
; dv ebp
; u_temp ebx
; v_temp edx
; count ecx

  _FUNCDEF_ASM g2pt816_unlit_n
        mov     dux, edx                ; store du
        mov     dvx, ebp                ; store dv

        lea     edi, [edi + 2*ecx]
        neg     ecx

        mov     ebx, esi                ; u->u_temp
        mov     edx, eax                ; v->v_temp
                
g2pt816_unlit_n_top:
        shr     ebx, 16                 ; shift u_temp
        and     edx, 0deadbeefh         ; mask v_temp
g2pt816_un_mask_v:

        shr     edx, 0ach               ; shift v_temp
g2pt816_un_shift_v:
        and     ebx, 0deadbeefh         ; mask u_temp
g2pt816_un_mask_u:

        add     esi, dux                ; inc u
        add     eax, dvx                ; inc v

        mov     bl, 0deadbeefh[edx+ebx] ; read pixel
g2pt816_un_p_src:
        mov     edx, eax                ; v->v_temp

        and     ebx, 0ffh
        inc     ecx                     ; done?

        mov     bx, 0deadbeefh[2*ebx]
g2pt816_un_pal16:

        mov     [edi+2*ecx-2], bx       ; write pixel

        mov     ebx, esi                ; u->u_temp
        jnz     g2pt816_unlit_n_top

        ret


; setup
;
; takes pointer to bitmap in eax

  _FUNCDEF g2pt816_unlit_n_setup,1
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
        mov     esi, un816_last_mask_u
        mov     edi, un816_last_mask_v

        cmp     esi, eax
        jz      g2pt816_un_skip_mask_u

        mov     d ds:[g2pt816_un_mask_u-4], eax
        mov     un816_last_mask_u, eax
g2pt816_un_skip_mask_u:
        cmp     edi, edx
        jz      g2pt816_un_skip_mask_v

        mov     d ds:[g2pt816_un_mask_v-4], edx
        mov     un816_last_mask_v, edx
g2pt816_un_skip_mask_v:

        mov     eax, un816_last_p_src
        mov     dl, un816_last_shift_v

        cmp     eax, ebx
        jz      g2pt816_un_skip_p_src

        mov     d ds:[g2pt816_un_p_src-4], ebx
        mov     un816_last_p_src, ebx
g2pt816_un_skip_p_src:
        cmp     dl, cl
        jz      g2pt816_un_skip_shift_v

        mov     b ds:[g2pt816_un_shift_v-1], cl
        mov     un816_last_shift_v, cl
g2pt816_un_skip_shift_v:

        mov     edx, _pixpal
        mov     ecx, un816_last_pal16

        cmp     ecx, edx
        je      g2pt816_un_skip_pal16

        mov     d ds:[g2pt816_un_pal16 - 4], edx
        mov     un816_last_pal16, edx

g2pt816_un_skip_pal16:
        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret


;
;  LIGHTING TABLE LIT N-RUN
;

  _FUNCDEF_ASM g2pt_lit_n
        mov     dux, edx                ; store du
        mov     dvx, ebp                ; store dv

        mov     ebp, esi
        lea     edi, [edi+ecx-1]

        lea     esi, _g2pt_lit_buffer[ecx]
        neg     ecx

        mov     ebx, ebp                ; u->u_temp
        mov     edx, eax                ; v->v_temp
                
g2pt_lit_n_top:
        shr     ebx, 16
        and     edx, 0deadbeefh         ; mask v_temp
g2pt_ln_mask_v:

        shr     edx, 0ach               ; shift v_temp
g2pt_ln_shift_v:
        and     ebx, 0deadbeefh
g2pt_ln_mask_u:

        add     ebp, dux                ; inc u
        add     eax, dvx                ; inc v

        mov     dl, 0deadbeefh[edx+ebx] ; read pixel
g2pt_ln_p_src:
        mov     ebx, ebp                ; u->u_temp

        mov     dh, [esi+ecx]
        nop

        inc     ecx                     ; done?
        nop

        mov     bl, 0deadbeefh[edx]
g2pt_ln_light_table:
        mov     edx, eax                ; v->v_temp

        mov     [edi+ecx], bl           ; write pixel
        jnz     g2pt_lit_n_top

        inc     edi                     ; fixup p_dest
        mov     esi, ebp                ; restore u
        ret


; setup
  _FUNCDEF g2pt_lit_n_setup,1
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
        mov     esi, ln_last_mask_u
        mov     edi, ln_last_mask_v

        cmp     esi, eax
        jz      g2pt_ln_skip_mask_u

        mov     d ds:[g2pt_ln_mask_u-4], eax
        mov     ln_last_mask_u, eax
g2pt_ln_skip_mask_u:
        cmp     edi, edx
        jz      g2pt_ln_skip_mask_v

        mov     d ds:[g2pt_ln_mask_v-4], edx
        mov     ln_last_mask_v, edx
g2pt_ln_skip_mask_v:

        mov     eax, ln_last_p_src
        mov     dl, ln_last_shift_v

        cmp     eax, ebx
        jz      g2pt_ln_skip_p_src

        mov     d ds:[g2pt_ln_p_src-4], ebx
        mov     ln_last_p_src, ebx
g2pt_ln_skip_p_src:
        cmp     dl, cl
        jz      g2pt_ln_skip_shift_v

        mov     b ds:[g2pt_ln_shift_v-1], cl
        mov     ln_last_shift_v, cl
g2pt_ln_skip_shift_v:

        mov     ebx, ln_last_light_table
        mov     edx, _grd_light_table

        cmp     ebx, edx
        jz      g2pt_ln_skip_light_table

        mov     d ds:[g2pt_ln_light_table-4], edx
        mov     ln_last_light_table, edx
g2pt_ln_skip_light_table:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret

;
;  LIGHTING TABLE LIT N-RUN
;

  _FUNCDEF_ASM g2pt16_lit_n
        mov     dux, edx                ; store du
        mov     dvx, ebp                ; store dv

        mov     ebp, esi
        lea     edi, [edi+2*ecx]

        lea     esi, _g2pt_lit_buffer[ecx]
        neg     ecx

        mov     ebx, ebp                ; u->u_temp
        mov     edx, eax                ; v->v_temp
                
g2pt16_lit_n_top:
        shr     ebx, 16
        and     edx, 0deadbeefh         ; mask v_temp
g2pt16_ln_mask_v:

        shr     edx, 0ach               ; shift v_temp
g2pt16_ln_shift_v:
        and     ebx, 0deadbeefh
g2pt16_ln_mask_u:

        add     ebp, dux                ; inc u
        add     eax, dvx                ; inc v

        mov     dl, 0deadbeefh[edx+ebx] ; read pixel
g2pt16_ln_p_src:
        mov     ebx, ebp                ; u->u_temp

        mov     dh, [esi+ecx]
        inc     ecx                     ; done?

        mov     bx, 0deadbeefh[2*edx]
g2pt16_ln_light_table:

        mov     edx, eax                ; v->v_temp

        mov     [edi+2*ecx-2], bx       ; write pixel
        jnz     g2pt16_lit_n_top

        mov     esi, ebp                ; restore u
        ret


; setup
  _FUNCDEF g2pt16_lit_n_setup,1
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
        mov     esi, ln16_last_mask_u
        mov     edi, ln16_last_mask_v

        cmp     esi, eax
        jz      g2pt16_ln_skip_mask_u

        mov     d ds:[g2pt16_ln_mask_u-4], eax
        mov     ln16_last_mask_u, eax
g2pt16_ln_skip_mask_u:
        cmp     edi, edx
        jz      g2pt16_ln_skip_mask_v

        mov     d ds:[g2pt16_ln_mask_v-4], edx
        mov     ln16_last_mask_v, edx
g2pt16_ln_skip_mask_v:

        mov     eax, ln16_last_p_src
        mov     dl, ln16_last_shift_v

        cmp     eax, ebx
        jz      g2pt16_ln_skip_p_src

        mov     d ds:[g2pt16_ln_p_src-4], ebx
        mov     ln16_last_p_src, ebx
g2pt16_ln_skip_p_src:
        cmp     dl, cl
        jz      g2pt16_ln_skip_shift_v

        mov     b ds:[g2pt16_ln_shift_v-1], cl
        mov     ln16_last_shift_v, cl
g2pt16_ln_skip_shift_v:

        mov     ebx, ln16_last_light_table
        mov     edx, _grd_ltab816

        cmp     ebx, edx
        jz      g2pt16_ln_skip_light_table

        mov     d ds:[g2pt16_ln_light_table-4], edx
        mov     ln16_last_light_table, edx
g2pt16_ln_skip_light_table:

        pop     ecx
        pop     edx
        pop     ebx
        pop     edi
        pop     esi

        ret

; NEXT WE HAVE THE VARIABLE
; LENGTH FILTERS

; these are called with identical
; parameters as above, and have to
; process things without stomping
; registers


  _FUNCDEF_ASM g2pt_clut_n
        ; save the final destination; we don't want to
        ; write temp data there in case it's the screen
        ; so we redirect to the buffer

        push   edi
        push   ecx

        lea    edi,_g2pt_buffer
        call   [_g2pt_clut_n_chain]

        pop    ecx            ; restore size
        pop    edi            ; restore dest

        push   eax
        push   esi
 
        mov    al,cl          ; get size back in al
        xor    ecx,ecx

        mov    esi,_g2pt_clut
        lea    ebx,_g2pt_buffer

        mov    cl,[ebx]     ; this code has rotated up out of the loop
        inc    ebx          ; this code has rotated up out of the loop

        sub    edi,ebx
        mov    ah,[esi+ecx] ; this code has rotated up out of the loop

g2pt_clut_loop:
        mov    [edi+ebx],ah  ; write output pixel
        mov    cl,[ebx]      ; sample pixel from buffer

        inc    ebx           ; increment output/input index
        dec    al            ; count of pixels

        mov    ah,[esi+ecx]  ; sample pixel from clut
        jnz    g2pt_clut_loop
        
        pop    esi
        add    edi,ebx

        pop    eax
        ret

;
;   n-pixels of transparency
;  this must come last in chain since it needs to write to real buffer

  _FUNCDEF_ASM g2pt_transp_n

        push   edi
        push   ecx

        lea    edi,_g2pt_buffer
        call   [_g2pt_transp_n_chain]

        lea    ebx,_g2pt_buffer
        nop
 
        pop    ecx            ; restore size
        pop    edi            ; restore dest

g2pt_transp_loop:

        mov    dh,[ebx]
        inc    ebx

        or     dh,dh
        jz     g2pt_transp_skip

        mov    [edi],dh
g2pt_transp_skip:
        inc    edi

        dec    ecx
        jnz    g2pt_transp_loop

        ret


  _FUNCDEF_ASM g2pt816_transp_n

        push   ecx
        push   edi

        lea    edi,_g2pt_buffer
        call   [_g2pt_transp_n_chain]

        pop    edi            ; restore dest
        pop    ecx            ; restore size

        xor    edx, edx
        mov    ebp, _pixpal

        lea    ebx, _g2pt_buffer

        lea    edi, [edi + 2*ecx]
        neg    ecx

g2pt816_transp_loop:
        mov    dl,[ebx]
        inc    ebx

        or     dl,dl
        jz     g2pt816_transp_skip

        mov    dx, [ebp + 2*edx]

        mov    [edi + 2*ecx], dx

        xor    edx, edx

g2pt816_transp_skip:
        inc    ecx
        jnz    g2pt816_transp_loop

        ret


;   n-pixels of translucency
;  this must come last in chain since it needs to read from real buffer

 _FUNCDEF_ASM g2pt_tluc_n
        ; save the final destination; we don't want to
        ; write temp data there in case it's the screen
        ; so we redirect to the buffer

        push   edi
        push   eax

        lea    edi,_g2pt_buffer
        call   [_g2pt_tluc_n_chain]

        pop    ecx            ; restore size
        pop    edi            ; restore dest

        push   ebx

        push   esi
        push   ebp

        mov    ebp,ecx
        xor    ecx,ecx
        mov    cl,[edi]        ; load up for first iteration

        and    ebp,0ffh

        neg    ebp             ; increment ebp to step through buffer
        lea    ebx,_g2pt_buffer

        sub    edi,ebp         ; use edi+ebp to access dest
        mov    ch,byte ptr _g2pt_buffer   ; load up for first iteration

        mov    esi,_g2pt_tluc_table
        sub    ebx,ebp         ; use ebx+ebp to access source

;
;  this loop doesn't quite work out to 3 cycles because
;  I couldn't get rid of all of the AGIs.  I believe it's
;  impossible to avoid them all without using another register,
;  which would involve another copy and thus push us over 3 cycles
;  I could unroll it once possibly, might be worth exploring, because
;  with only 4 real ops maybe we can end up at 5 cycles/2 instead
;  of 4 cycles/1

g2pt_tluc_loop:
        mov    ah,[esi+ecx]
        mov    cl,1[edi+ebp]

        mov    [edi+ebp],ah
        mov    ch,1[ebx+ebp]

        inc    ebp
        jnz    g2pt_tluc_loop
        
        add    edi,ebp
        pop    ebp

        pop    esi
        pop    ebx

        ret

; The remaining code is no longer supported and is preserved only for reference
if 0
; UNLIT N-RUN, non-power of two, non-wrapping
;
;  EAX  =  v_lo << 16
;  ESI  =  u_lo << 16
;  EBP  = dv_lo << 16
;  EDX  = du_lo << 16
;  EBX  = source + u_hi + v_hi * source_row
;
;   AL  = run length
;
; these registers are arranged so we have to save
; the exact same registers as the normal mapper


  _FUNCDEF_ASM g2pt_unlit_arb_n
        add    eax,ebp
        jmp    g2pt_uan_middle

g2pt_uan_top:
        mov    ecx,_g2pt_step_table[ecx*4+4]
        add    esi,edx   ; update u

        adc    ebx,ecx
        add    eax,ebp   ; update v

        mov    [edi],ah
        inc    edi

g2pt_uan_middle:
        sbb    ecx,ecx   ; save v carry
        mov    ah,[ebx]

        dec    al
        jnz    g2pt_uan_top

        mov    ecx,_g2pt_step_table[ecx*4+4]
        add    esi,edx

        adc    ebx,ecx
        mov    [edi],ah

        inc    edi
        ret

;
;  PALETTE LIT N-RUN
;

  _FUNCDEF_ASM g2pt_pallit_n
        mov    temp_ebp,ebp
        mov    ebp,ecx

        mov    ecx,temp_ebp
        xor    cl,cl

g2pt_pn_loop:
        mov    ah,0babebeach[ebx]   ; add in pixel color
g2pt_pn_texture:
        mov    ch,[ebp]         ; fetch lighting

        add    esi,edx
        inc    ebp

        adc    bl,dl
        add    eax,ecx

        adc    bh,dh
        mov    [edi],ah

        and    ebx,0deadbeefh
g2pt_pn_mask:
        inc    edi

        dec    al
        jnz    g2pt_pn_loop

        mov    ebp,temp_ebp
        ret

; setup
  _FUNCDEF g2pt_pallit_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        lea    esi,dword ptr ds:g2pt_pn_texture-4
        lea    edi,dword ptr ds:g2pt_pn_mask-4
g2pt_setup_core:
        mov    eax,_g2pt_tmap_ptr
        mov    ebx,_g2pt_tmap_mask
        mov    ecx,[esi]
        mov    edx,[edi]
        cmp    eax,ecx
        je     g2pt_un_domask
        mov    [esi],eax
g2pt_un_domask:
        cmp    ebx,edx
        je     g2pt_un_done
        mov    [edi],ebx
g2pt_un_done:
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        pop    edi
        pop    esi
        ret


  _FUNCDEF_ASM g2pt_dest_pallit_n
        mov    ecx,ebp
        mov    ebp,eax

        and    ebp,0ffh
        mov    ch,[edi]             ; sample color

        add    edi,ebp
        neg    ebp

g2pt_dpn_loop:
        mov    ah,0babebeach[ebx]   ; sample pixel
g2pt_dpn_texture:
        add    esi,edx              ; u_frac += du_frac

        adc    bl,dl                ; u_int  += du_int
        add    eax,ecx              ; v_frac += dv_frac

        adc    bh,dh                ; v_int  += dv_int
        mov    ch,[edi+ebp+1]       ; sample color for next iteration

        and    ebx,0babebeach       ; mask for wrapping
g2pt_dpn_mask:
        mov    [edi+ebp],ah         ; write out pixel

        inc    ebp                  ; go to next output loc
        jnz    g2pt_dpn_loop

        ret

; setup
  _FUNCDEF g2pt_dest_pallit_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        lea    esi,dword ptr ds:g2pt_dpn_texture-4
        lea    edi,dword ptr ds:g2pt_dpn_mask-4
        jmp    g2pt_setup_core


;
;  PALETTE FLAT-LIT N-RUN
; lighting is first byte in _g2pt_lit_buffer
;
; so we move it into the second byte of ebp so
; we get the lighting for free

  _FUNCDEF_ASM g2pt_palflat_n
        mov    ecx,ebp

        mov    ch,byte ptr _g2pt_lit_buffer

g2pt_pfn_loop:
        mov    ah,0babebeach[ebx]    ; fetch pixel color
g2pt_pfn_texture:
        add    esi,edx

        adc    bl,dl
        add    eax,ecx

        adc    bh,dh
        mov    [edi],ah

        and    ebx,0babebeach
g2pt_pfn_mask:
        inc    edi

        dec    al
        jnz    g2pt_pfn_loop

        and    ebp,0ffff0000h
        ret

; setup
  _FUNCDEF g2pt_palflat_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        lea    esi,dword ptr ds:g2pt_pfn_texture-4
        lea    edi,dword ptr ds:g2pt_pfn_mask-4
        jmp    g2pt_setup_core
endif
_TEXT   ends

END
