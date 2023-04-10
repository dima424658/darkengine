; $Header: r:/t2repos/thief2/src/portal/ptmap_n.asm,v 1.6 1997/02/07 15:30:26 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptmap_n.asm
;
; Variable length n-pixel loops
; (at most 16 pixels due to buffer constraints)

.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc
        include thunks.inc

extd    _pt_tmap_ptr,_pt_tmap_mask
extd    _pt_clut, _pt_tluc_table, _pt_light_table
extd    _pt_buffer, _pt_lit_buffer

extd    _pt_clut_n_chain
extd    _pt_transp_n_chain
extd    _pt_tluc_n_chain

extd    _pt_step_table

	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'

	align	4

temp_ebp dd 0
du_frac  dd 0
dv_frac  dd 0
dv       dd 0

_DATA	ends


_TEXT	segment para public USE32 'CODE'

;extn    pt_unlit_n,pt_unlit_n_setup
;extn    pt_pallit_n,pt_pallit_n_setup
;extn    pt_dest_pallit_n,pt_dest_pallit_n_setup
;extn    pt_palflat_n,pt_palflat_n_setup
;extn    pt_lit_n,pt_lit_n_setup
;extn    pt_clut_n
;extn    pt_transp_n
;extn    pt_tluc_n
;extn    pt_unlit_256_n, pt_unlit_256_n_setup
;extn    pt_unlit_arb_n


;  first we have all of the texture mappers themselves
;
;  Basically, all of these mappers may have extra state
;  in self-modified things or in global variables, but
;  since they're called there can't be any other state
;  preserved.  They are also free to trash the other
;  registers not listed:

;    U is stored as  8.16  in  BL and ESI
;    V is stored as  8.16  in  BH and EAX
;   DU is stored as  8.16  in  DL and EDX
;   DV is stored as  8.16  in  DH and EBP

;  The number of bytes to write is found in AL, and the
;  destination is in EDI.  The tmapper must preserve
;  EDI and return with EDI = EDI + AL; it can trash AL & AH.

;  Lighting is stored starting at ECX.
;  Regardless, ECX can be trashed on return.

;  VARIABLE LENGTH MAPPERS

;
;  UNLIT N-RUN
;

  _FUNCDEF_ASM pt_unlit_n
pt_unlit_n_top:
        mov    cl,0babebeach[ebx]   ; sample pixel
pt_un_texture:
        add    esi,edx              ; u_frac += du_frac

        adc    bl,dl                ; u_int  += du_int
        add    eax,ebp              ; v_frac += dv_frac

        adc    bh,dh                ; v_int  += dv_int
        mov    [edi],cl             ; write out pixel

        and    ebx,0babebeach       ; mask for wrapping
pt_un_mask:
        inc    edi                  ; go to next output loc

        dec    al                   ; AL is # of pixels
        jnz    pt_unlit_n_top

        ret


; setup
;
;  this self-modifies the various constants in the loop
;  the code is reused by other routines so takes 2 pointers

  _FUNCDEF pt_unlit_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx

        lea    esi,dword ptr ds:pt_un_texture-4
        lea    edi,dword ptr ds:pt_un_mask-4

pt_setup_core:
        mov    eax,_pt_tmap_ptr
        mov    ebx,_pt_tmap_mask
        mov    ecx,[esi]
        mov    edx,[edi]
        cmp    eax,ecx
        je     pt_un_domask
        mov    [esi],eax
pt_un_domask:
        cmp    ebx,edx
        je     pt_un_done
        mov    [edi],ebx
pt_un_done:
        pop    edx
        pop    ecx
        pop    ebx
        pop    eax
        pop    edi
        pop    esi
        ret


;  special case w=h=256

  _FUNCDEF_ASM pt_unlit_256_n
        xor    ecx,ecx

        mov    cl,al

        add    edi,ecx
        neg    ecx

pt_unlit_256_n_top:
        mov    al,0babebeach[ebx]   ; sample pixel
pt_un_256_texture:
        add    esi,edx              ; u_frac += du_frac

        adc    bl,dl                ; u_int  += du_int
        add    eax,ebp              ; v_frac += dv_frac

        adc    bh,dh                ; v_int  += dv_int
        mov    [edi+ecx],al         ; write out pixel

        inc    ecx                  ; go to next output loc
        jnz    pt_unlit_256_n_top

        ret

  _FUNCDEF pt_unlit_256_n_setup,0
        push   eax
        push   ecx
        push   esi

        lea    esi,dword ptr ds:pt_un_256_texture-4
        mov    eax,_pt_tmap_ptr
        mov    ecx,[esi]
        cmp    eax,ecx
        je     pt_un_256_done
        mov    [esi],eax
pt_un_256_done:
        pop    esi
        pop    ecx
        pop    eax

        ret

;
;  PALETTE LIT N-RUN
;

  _FUNCDEF_ASM pt_pallit_n
        mov    temp_ebp,ebp
        mov    ebp,ecx

        mov    ecx,temp_ebp
        xor    cl,cl

pt_pn_loop:
        mov    ah,0babebeach[ebx]   ; add in pixel color
pt_pn_texture:
        mov    ch,[ebp]         ; fetch lighting

        add    esi,edx
        inc    ebp

        adc    bl,dl
        add    eax,ecx

        adc    bh,dh
        mov    [edi],ah

        and    ebx,0deadbeefh
pt_pn_mask:
        inc    edi

        dec    al
        jnz    pt_pn_loop

        mov    ebp,temp_ebp
        ret

; setup
  _FUNCDEF pt_pallit_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        lea    esi,dword ptr ds:pt_pn_texture-4
        lea    edi,dword ptr ds:pt_pn_mask-4
        jmp    pt_setup_core


  _FUNCDEF_ASM pt_dest_pallit_n
        mov    ecx,ebp
        mov    ebp,eax

        and    ebp,0ffh
        mov    ch,[edi]             ; sample color

        add    edi,ebp
        neg    ebp

pt_dpn_loop:
        mov    ah,0babebeach[ebx]   ; sample pixel
pt_dpn_texture:
        add    esi,edx              ; u_frac += du_frac

        adc    bl,dl                ; u_int  += du_int
        add    eax,ecx              ; v_frac += dv_frac

        adc    bh,dh                ; v_int  += dv_int
        mov    ch,[edi+ebp+1]       ; sample color for next iteration

        and    ebx,0babebeach       ; mask for wrapping
pt_dpn_mask:
        mov    [edi+ebp],ah         ; write out pixel

        inc    ebp                  ; go to next output loc
        jnz    pt_dpn_loop

        ret

; setup
  _FUNCDEF pt_dest_pallit_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        lea    esi,dword ptr ds:pt_dpn_texture-4
        lea    edi,dword ptr ds:pt_dpn_mask-4
        jmp    pt_setup_core


;
;  PALETTE FLAT-LIT N-RUN
; lighting is first byte in _pt_lit_buffer
;
; so we move it into the second byte of ebp so
; we get the lighting for free

  _FUNCDEF_ASM pt_palflat_n
        mov    ecx,ebp

        mov    ch,byte ptr _pt_lit_buffer

pt_pfn_loop:
        mov    ah,0babebeach[ebx]    ; fetch pixel color
pt_pfn_texture:
        add    esi,edx

        adc    bl,dl
        add    eax,ecx

        adc    bh,dh
        mov    [edi],ah

        and    ebx,0babebeach
pt_pfn_mask:
        inc    edi

        dec    al
        jnz    pt_pfn_loop

        and    ebp,0ffff0000h
        ret

; setup
  _FUNCDEF pt_palflat_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        lea    esi,dword ptr ds:pt_pfn_texture-4
        lea    edi,dword ptr ds:pt_pfn_mask-4
        jmp    pt_setup_core

;
;  LIGHTING TABLE LIT N-RUN
;

  _FUNCDEF_ASM pt_lit_n
        sub    ecx,edi
        mov    dword ptr ds:(pt_ln_lighting-4),ecx
        xor    ecx,ecx

pt_ln_loop:
        mov    cl,0babebeach[ebx]
pt_ln_texture:
        add    esi,edx

        mov    ch,0babebeach[edi]         ; ch = sample light here
pt_ln_lighting:
        lea    edi,1[edi]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    cl,0babebeach[ecx]         ; CL = light_table[light][color]
pt_ln_light_table:

        and    ebx,0deadbeefh
pt_ln_mask:
        mov    [edi-1],cl

        dec    al
        jnz    pt_ln_loop

        ret


; setup
  _FUNCDEF pt_lit_n_setup,0
        push   esi
        push   edi
        push   eax
        push   ebx
        push   ecx
        push   edx
        mov    eax,_pt_light_table
        lea    esi,dword ptr ds:pt_ln_texture-4

        cmp    eax,dword ptr ds:pt_ln_light_table-4
        je     pt_ln_setup_skip

        mov    dword ptr ds:pt_ln_light_table-4,eax

pt_ln_setup_skip:
        lea    edi,dword ptr ds:pt_ln_mask-4
        jmp    pt_setup_core     


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


  _FUNCDEF_ASM pt_unlit_arb_n
        add    eax,ebp
        jmp    pt_uan_middle

pt_uan_top:
        mov    ecx,_pt_step_table[ecx*4+4]
        add    esi,edx   ; update u

        adc    ebx,ecx
        add    eax,ebp   ; update v

        mov    [edi],ah
        inc    edi

pt_uan_middle:
        sbb    ecx,ecx   ; save v carry
        mov    ah,[ebx]

        dec    al
        jnz    pt_uan_top

        mov    ecx,_pt_step_table[ecx*4+4]
        add    esi,edx

        adc    ebx,ecx
        mov    [edi],ah

        inc    edi
        ret


; NEXT WE HAVE THE VARIABLE
; LENGTH FILTERS

; these are called with identical
; parameters as above, and have to
; process things without stomping
; registers


  _FUNCDEF_ASM pt_clut_n
        ; save the final destination; we don't want to
        ; write temp data there in case it's the screen
        ; so we redirect to the buffer

        push   edi
        push   eax

        lea    edi,_pt_buffer
        call   [_pt_clut_n_chain]

        pop    ecx            ; restore size
        pop    edi            ; restore dest

        mov    al,cl          ; get size back in al
        xor    ecx,ecx

        push   ebx
        push   esi
 
        mov    esi,_pt_clut
        lea    ebx,_pt_buffer

        mov    cl,[ebx]     ; this code has rotated up out of the loop
        inc    ebx          ; this code has rotated up out of the loop

        sub    edi,ebx
        mov    ah,[esi+ecx] ; this code has rotated up out of the loop

pt_clut_loop:
        mov    [edi+ebx],ah  ; write output pixel
        mov    cl,[ebx]      ; sample pixel from buffer

        inc    ebx           ; increment output/input index
        dec    al            ; count of pixels

        mov    ah,[esi+ecx]  ; sample pixel from clut
        jnz    pt_clut_loop
        
        pop    esi
        add    edi,ebx

        pop    ebx
        ret

;
;   n-pixels of transparency
;  this must come last in chain since it needs to write to real buffer

  _FUNCDEF_ASM pt_transp_n

        push   edi
        push   eax

        lea    edi,_pt_buffer
        call   [_pt_transp_n_chain]

        pop    ecx            ; restore size
        pop    edi            ; restore dest

        mov    al,cl
        lea    ecx,_pt_buffer
 
pt_transp_loop:

        mov    ah,[ecx]
        inc    ecx

        or     ah,ah
        jz     pt_transp_skip

        mov    [edi],ah
pt_transp_skip:
        inc    edi

        dec    al
        jnz    pt_transp_loop

        ret


;   n-pixels of translucency
;  this must come last in chain since it needs to read from real buffer

 _FUNCDEF_ASM pt_tluc_n
        ; save the final destination; we don't want to
        ; write temp data there in case it's the screen
        ; so we redirect to the buffer

        push   edi
        push   eax

        lea    edi,_pt_buffer
        call   [_pt_tluc_n_chain]

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
        lea    ebx,_pt_buffer

        sub    edi,ebp         ; use edi+ebp to access dest
        mov    ch,byte ptr _pt_buffer   ; load up for first iteration

        mov    esi,_pt_tluc_table
        sub    ebx,ebp         ; use ebx+ebp to access source

;
;  this loop doesn't quite work out to 3 cycles because
;  I couldn't get rid of all of the AGIs.  I believe it's
;  impossible to avoid them all without using another register,
;  which would involve another copy and thus push us over 3 cycles
;  I could unroll it once possibly, might be worth exploring, because
;  with only 4 real ops maybe we can end up at 5 cycles/2 instead
;  of 4 cycles/1

pt_tluc_loop:
        mov    ah,[esi+ecx]
        mov    cl,1[edi+ebp]

        mov    [edi+ebp],ah
        mov    ch,1[ebx+ebp]

        inc    ebp
        jnz    pt_tluc_loop
        
        add    edi,ebp
        pop    ebp

        pop    esi
        pop    ebx

        ret

_TEXT   ends

END
