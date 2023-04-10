; $Header: r:/t2repos/thief2/src/portal/ptmap_8.asm,v 1.10 1997/02/07 15:30:23 KEVIN Exp $
;
; PORTAL Texture Mappers
;
; ptmap_8.asm
;
; 8-pixel loops which write to _pt_buffer;
; filters transparent, translucent,
; and generic all write to edi

.486
	.nolist

	include	macros.inc
	include	lg.inc
        include fix.inc
        include thunks.inc

extd    _pt_tmap_ptr,_pt_tmap_mask
extd    _pt_clut, _pt_tluc_table, _pt_light_table
extd    _pt_buffer, _pt_lit_buffer
extd    _pt_light, _pt_dlight, dlight_8
extd    _pt_step_table

extd    _pt_unlit_8_chain
extd    _pt_pallit_8_chain
extd    _pt_palflat_8_chain
extd    _pt_lit_8_chain
extd    _pt_clut_8_chain
extd    _pt_transp_8_chain
extd    _pt_tluc_8_chain
extd    _pt_generic_8_chain
extd    _pt_generic_8_2_chain

extd    pt_dest

	.list

	assume	cs:_TEXT, ds:_DATA

_DATA	segment	dword public USE32 'DATA'

	align	4

pt_dummy_buffer    dd 0,0,0,0

count_save dd 0
dest_save  dd 0

externdef float_buf:qword
externdef float_buf2:qword
externdef _gdc:qword
externdef _u_step_data:qword
externdef _v_step_data:qword
externdef _one:qword

_DATA	ends


_TEXT	segment para public USE32 'CODE'

; import
extn    tmap_float_uv_iterate
extn    pt_dest_pallit_n_
extn    pt_unlit_arb_n_

; export
;extn    pt_unlit_8,pt_unlit_8_setup
;extn    pt_palflat_8,pt_palflat_8_setup,pt_palflat_direct_8
;extn    pt_lit_8,pt_lit_8_setup
;extn    pt_pallit_8,pt_pallit_store_8
;extn    pt_clut_8
;extn    pt_pallit_clut_store_8
;extn    pt_transp_8
;extn    pt_tluc_8
;extn    pt_generic_8, pt_generic_8_2
;extn    pt_generic_preload_8, pt_generic_preload_8_2
;extn    pt_dest_pallit_8
;extn    pt_unlit_arb_8
;extn    pt_pallit_perspective_core
;extn    pt_unlit_perspective_unrolled


;  first we have all of the texture mappers themselves
;
;  Basically, all of these mappers may have extra state
;  in self-modified things or in global variables, but
;  since they're called there can't be any other state
;  preserved.  They are also free to trash the other
;  registers not listed:

;    U is stored as  8.24  in  BL and ESI
;    V is stored as  8.24  in  BH and EAX
;   DU is stored as  8.24  in  DL and EDX
;   DV is stored as  8.24  in  DH and EBP

;  If the tmapper must read the source (or not write
;  some of it), then the source/destination is pt_dest.  We only
;  have this for the outer filters: transparent, translucent,
;  and generic (which just is a wrapper that redirects to
;  pt_dest).

;  Lighting is stored in a static buffer.
;  These routines are not C callable and therefore
;  don't preserve EBP.


;
;  UNLIT 8-RUN
;

 _FUNCDEF_ASM pt_unlit_8
        mov    edi,_pt_tmap_ptr
        mov    ecx,ebx

        add    esi,edx
        ; V pipe stall

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer,al
        and    ebx,0babebeach
pt_u8_mask_1:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+1,al
        and    ebx,0babebeach
pt_u8_mask_2:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+2,al
        and    ebx,0babebeach
pt_u8_mask_3:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+3,al
        and    ebx,0babebeach
pt_u8_mask_4:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+4,al
        and    ebx,0babebeach
pt_u8_mask_5:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+5,al
        and    ebx,0babebeach
pt_u8_mask_6:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+6,al
        and    ebx,0babebeach
pt_u8_mask_7:

        add    esi,edx
        mov    ecx,ebx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    al,[edi+ecx]

        mov    byte ptr _pt_buffer+7,al
        and    ebx,0babebeach
pt_u8_mask_8:

        jmp    [_pt_unlit_8_chain]


; setup
 _FUNCDEF pt_unlit_8_setup,0

        push   eax
        push   ebx

        mov    eax,_pt_tmap_mask
        mov    ebx,dword ptr ds:pt_u8_mask_1-4

        cmp    eax,ebx
        je     pt_u8_setup_done

        mov    dword ptr ds:pt_u8_mask_1-4,eax
        mov    dword ptr ds:pt_u8_mask_2-4,eax
        mov    dword ptr ds:pt_u8_mask_3-4,eax
        mov    dword ptr ds:pt_u8_mask_4-4,eax
        mov    dword ptr ds:pt_u8_mask_5-4,eax
        mov    dword ptr ds:pt_u8_mask_6-4,eax
        mov    dword ptr ds:pt_u8_mask_7-4,eax
        mov    dword ptr ds:pt_u8_mask_8-4,eax

pt_u8_setup_done:
        pop    ebx
        pop    eax

        ret

;
;  PALETTE FLATLIT 8-RUN
;
;     cycle 0:   write bl   add  ebp
;     cycle 1:   write bh                                    write al
;     cycle 2:   copy ebx->ecx           al = [ecx]
;     cycle 3:   mask ecx                add  eax

  _FUNCDEF_ASM pt_palflat_8
        mov    ecx,ebp
        mov    edi,_pt_tmap_ptr

        mov    ch,byte ptr _pt_lit_buffer

        add    eax,ecx
        mov    ebp,ecx

        mov    ah,[edi+ebx]

        adc    bh,dh
        add    esi,edx

        adc    bl,dl

        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_1:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_2:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+1,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_3:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+2,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_4:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+3,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_5:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+4,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_6:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+5,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_pf8_mask_7:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+6,ah

        and    ebx,0deadbeefh
pt_pf8_mask_8:
        mov    ah,[edi+ecx]

        push   eax
        add    eax,ebp

        and    ebp,0ffff0000h
        mov    byte ptr _pt_buffer+7,ah

        pop    eax
        jmp    [_pt_palflat_8_chain]


; setup
  _FUNCDEF pt_palflat_8_setup,0
        push   eax
        push   ebx

        mov    eax,_pt_tmap_mask
        mov    ebx,dword ptr ds:pt_pf8_mask_1-4

        cmp    eax,ebx
        je     pt_pf8_setup_done

        mov    dword ptr ds:pt_pf8_mask_1-4,eax
        mov    dword ptr ds:pt_pf8_mask_2-4,eax
        mov    dword ptr ds:pt_pf8_mask_3-4,eax
        mov    dword ptr ds:pt_pf8_mask_4-4,eax
        mov    dword ptr ds:pt_pf8_mask_5-4,eax
        mov    dword ptr ds:pt_pf8_mask_6-4,eax
        mov    dword ptr ds:pt_pf8_mask_7-4,eax
        mov    dword ptr ds:pt_pf8_mask_8-4,eax

        mov    dword ptr ds:pt_if8_mask_1-4,eax
        mov    dword ptr ds:pt_if8_mask_2-4,eax
        mov    dword ptr ds:pt_if8_mask_3-4,eax
        mov    dword ptr ds:pt_if8_mask_4-4,eax
        mov    dword ptr ds:pt_if8_mask_5-4,eax
        mov    dword ptr ds:pt_if8_mask_6-4,eax
        mov    dword ptr ds:pt_if8_mask_7-4,eax
        mov    dword ptr ds:pt_if8_mask_8-4,eax

pt_pf8_setup_done:
        pop    ebx
        pop    eax

        ret


;
;  LIGHTING TABLE LIT 8-RUN
;

  _FUNCDEF_ASM pt_lit_8
        mov    edi,_pt_tmap_ptr
        xor    ecx,ecx

        add    esi,edx
        mov    ch,byte ptr _pt_lit_buffer

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_1:

        and    ebx,0deadbeefh
pt_l8_mask_1:
        mov    byte ptr _pt_buffer,ch

        mov    ch,byte ptr _pt_lit_buffer+1
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_2:

        and    ebx,0deadbeefh
pt_l8_mask_2:
        mov    byte ptr _pt_buffer+1,ch

        mov    ch,byte ptr _pt_lit_buffer+2
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_3:

        and    ebx,0deadbeefh
pt_l8_mask_3:
        mov    byte ptr _pt_buffer+2,ch

        mov    ch,byte ptr _pt_lit_buffer+3
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_4:

        and    ebx,0deadbeefh
pt_l8_mask_4:
        mov    byte ptr _pt_buffer+3,ch

        mov    ch,byte ptr _pt_lit_buffer+4
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_5:

        and    ebx,0deadbeefh
pt_l8_mask_5:
        mov    byte ptr _pt_buffer+4,ch

        mov    ch,byte ptr _pt_lit_buffer+5
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_6:

        and    ebx,0deadbeefh
pt_l8_mask_6:
        mov    byte ptr _pt_buffer+5,ch

        mov    ch,byte ptr _pt_lit_buffer+6
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_7:

        and    ebx,0deadbeefh
pt_l8_mask_7:
        mov    byte ptr _pt_buffer+6,ch

        mov    ch,byte ptr _pt_lit_buffer+7
        add    esi,edx

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        mov    ch,0babebeach[ecx]
pt_l8_light_table_8:

        and    ebx,0deadbeefh
pt_l8_mask_8:
        mov    byte ptr _pt_buffer+7,ch

        jmp    [_pt_lit_8_chain]


; setup
  _FUNCDEF pt_lit_8_setup,0
        push   eax
        push   ebx

        mov    eax,_pt_tmap_mask
        mov    ebx,dword ptr ds:pt_l8_mask_1-4

        cmp    eax,ebx
        je     pt_l8_dolight

        mov    dword ptr ds:pt_l8_mask_1-4,eax
        mov    dword ptr ds:pt_l8_mask_2-4,eax
        mov    dword ptr ds:pt_l8_mask_3-4,eax
        mov    dword ptr ds:pt_l8_mask_4-4,eax
        mov    dword ptr ds:pt_l8_mask_5-4,eax
        mov    dword ptr ds:pt_l8_mask_6-4,eax
        mov    dword ptr ds:pt_l8_mask_7-4,eax
        mov    dword ptr ds:pt_l8_mask_8-4,eax

pt_l8_dolight:
        mov    eax,_pt_light_table
        mov    ebx,dword ptr ds:pt_l8_light_table_1-4

        cmp    eax,ebx
        je     pt_l8_setup_done

        mov    dword ptr ds:pt_l8_light_table_1-4,eax
        mov    dword ptr ds:pt_l8_light_table_2-4,eax
        mov    dword ptr ds:pt_l8_light_table_3-4,eax
        mov    dword ptr ds:pt_l8_light_table_4-4,eax
        mov    dword ptr ds:pt_l8_light_table_5-4,eax
        mov    dword ptr ds:pt_l8_light_table_6-4,eax
        mov    dword ptr ds:pt_l8_light_table_7-4,eax
        mov    dword ptr ds:pt_l8_light_table_8-4,eax

pt_l8_setup_done:
        pop    ebx
        pop    eax

        ret


; NEXT WE HAVE THE 8-BYTE FILTERS

; these are called with identical
; parameters as above, and have to
; process things without stomping
; registers

;
;  PALETTE LIT 8-RUN FILTER
;

  _FUNCDEF_ASM pt_pallit_8
        mov    edi,_pt_buffer
        mov    ecx,_pt_lit_buffer

        add    edi,ecx
        mov    ecx,_pt_lit_buffer+4

        mov    _pt_buffer,edi
        mov    edi,_pt_buffer+4

        add    edi,ecx

        mov    _pt_buffer+4,edi
        jmp    [_pt_pallit_8_chain]

  _FUNCDEF_ASM pt_pallit_store_8
        mov    edi,pt_dest
        mov    ecx,_pt_buffer

        mov    edx,_pt_lit_buffer
        mov    ebp,_pt_buffer+4

        add    ecx,edx
        mov    edx,_pt_lit_buffer+4

        mov    [edi],ecx
        add    edx,ebp

        mov    [edi+4],edx
        add    edi,8

        ret

  _FUNCDEF_ASM pt_clut_8
        push   esi
        push   edx

        push   ebx
        push   eax

        mov    esi,_pt_clut
        nop

        xor    ecx,ecx
        xor    eax,eax

        xor    ebx,ebx
        xor    edx,edx

        mov    al,byte ptr _pt_buffer
        mov    bl,byte ptr _pt_buffer+1

        mov    cl,byte ptr _pt_buffer+2
        mov    dl,byte ptr _pt_buffer+3

        mov    al,[esi+eax]
        mov    bl,[esi+ebx]

        mov    byte ptr _pt_buffer,al
        mov    cl,[esi+ecx]

        mov    byte ptr _pt_buffer+1,bl
        mov    dl,[esi+edx]

        mov    byte ptr _pt_buffer+2,cl
        mov    al,byte ptr _pt_buffer+4

        mov    byte ptr _pt_buffer+3,dl
        mov    bl,byte ptr _pt_buffer+5

        mov    cl,byte ptr _pt_buffer+6
        mov    dl,byte ptr _pt_buffer+7

        mov    al,[esi+eax]
        mov    bl,[esi+ebx]

        mov    byte ptr _pt_buffer+4,al
        mov    cl,[esi+ecx]

        mov    byte ptr _pt_buffer+5,bl
        mov    dl,[esi+edx]

        mov    byte ptr _pt_buffer+6,cl
        mov    byte ptr _pt_buffer+7,dl

        pop    eax
        pop    ebx

        pop    edx
        pop    esi

        jmp    [_pt_clut_8_chain]

; we use the following chain for clutted surfaces:
;   pt_unlit_8 -> pt_pallit_8 -> pt_clut_8 -> pt_generic_8;
;   so here is the last three in one step

  _FUNCDEF_ASM pt_pallit_clut_store_8
        push   edx
        push   eax

        push   ebx
        push   esi

        push   ebp
        mov    esi,_pt_clut

        mov    ecx,_pt_buffer
        mov    ebp,_pt_lit_buffer

;   edi = final dest
;   esi = clut

        add    ecx,ebp
        xor    eax,eax

        mov    al,cl
        xor    ebx,ebx

        mov    edi,pt_dest
        mov    bl,ch

        shr    ecx,16
        mov    dl,[esi+eax]

        mov    al,cl
        mov    dh,[esi+ebx]

        shl    edx,16
        mov    bl,ch

        mov    dl,[esi+eax]
        mov    ecx,_pt_buffer+4

        mov    dh,[esi+ebx]
        mov    ebp,_pt_lit_buffer+4

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


;
;   8 pixels of transparency
;  edi is the destination

  _FUNCDEF_ASM pt_transp_8
        mov    edi,pt_dest
        push   ebx

        push   eax
        mov    eax,_pt_buffer

; if all four pixels are transparent, don't do anything more

        test   eax,eax
        jz     pt_t8_long_1

; check for all four non-zero
        mov    ecx,eax
        and    eax,07f7f7f7fh

        sub    eax,01010101h

        and    eax,080808080h
        jnz    pt_t8_byte_test_1

        mov    [edi],ecx
        jmp    pt_t8_long_1

pt_t8_byte_test_1:
        cmp    cl,0
        je     pt_t8_byte_1

        mov    [edi],cl
        nop

pt_t8_byte_1:
        cmp    ch,0
        je     pt_t8_byte_2

        mov    1[edi],ch
        nop

pt_t8_byte_2:
        shr    ecx,16

        cmp    cl,0
        je     pt_t8_byte_3

        mov    2[edi],cl
        nop

pt_t8_byte_3:
        cmp    ch,0
        je     pt_t8_byte_4

        mov    3[edi],ch
        nop

pt_t8_byte_4:
pt_t8_long_1:

        mov    eax,_pt_buffer+4
        nop

; if all four pixels are transparent, don't do anything more

        test   eax,eax
        jz     pt_t8_long_2

        mov    ecx,eax
        and    eax,07f7f7f7fh

        sub    eax,001010101h

        and    eax,080808080h
        jnz    pt_t8_byte_test_2

        mov    [edi+4],ecx
        jmp    pt_t8_long_2

pt_t8_byte_test_2:
        cmp    cl,0
        je     pt_t8_byte_5

        mov    4[edi],cl
        nop

pt_t8_byte_5:
        cmp    ch,0
        je     pt_t8_byte_6

        mov    5[edi],ch
        nop

pt_t8_byte_6:
        shr    ecx,16

        cmp    cl,0
        je     pt_t8_byte_7

        mov    6[edi],cl
        nop

pt_t8_byte_7:
        cmp    ch,0
        je     pt_t8_byte_8

        mov    7[edi],ch
        nop

pt_t8_byte_8:
pt_t8_long_2:

        pop    eax
        pop    ebx

        add    edi,8
        ret


;   8 pixels of translucency
;  this must come last in chain since it needs to read from real buffer

  _FUNCDEF_ASM pt_tluc_8
        ; save the final destination; we don't want to
        ; write temp data there in case it's the screen
        ; so we redirect to the buffer

        push   eax

        push   ebx
        push   esi

        mov    edi,pt_dest
        mov    esi,_pt_tluc_table

        xor    eax,eax
        xor    ebx,ebx

        xor    ecx,ecx
        xor    edx,edx

        mov    al,2[edi]
        mov    bl,3[edi]

        mov    ah,byte ptr _pt_buffer+2
        mov    bh,byte ptr _pt_buffer+3

        mov    cl,0[edi]
        mov    dl,1[edi]

        mov    al,[esi+eax]
        mov    ch,byte ptr _pt_buffer+0

        mov    ah,[esi+ebx]
        mov    dh,byte ptr _pt_buffer+1

        shl    eax,16
        mov    cl,[esi+ecx]

        mov    ch,[esi+edx]
        mov    dl,5[edi]

        or     eax,ecx
        mov    cl,4[edi]

        mov    [edi],eax
        mov    ch,byte ptr _pt_buffer+4

        xor    eax,eax
        mov    dh,byte ptr _pt_buffer+5

        mov    al,6[edi]
        mov    cl,[esi+ecx]

        mov    bl,7[edi]
        mov    ah,byte ptr _pt_buffer+6

        mov    bh,byte ptr _pt_buffer+7
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


  _FUNCDEF_ASM pt_generic_8
        mov    edi,pt_dest
        mov    ecx,_pt_buffer

        mov    edx,_pt_buffer+4

        mov    [edi],ecx
        mov    4[edi],edx

        add    edi,8
        ret


  _FUNCDEF_ASM pt_generic_preload_8
        mov    edi,pt_dest

        mov    ecx,_pt_buffer
        mov    edx,_pt_buffer+4

        mov    [edi],ecx
        mov    4[edi],edx

        mov    ecx,8[edi]
        add    edi,8

        ret

  _FUNCDEF_ASM pt_dest_pallit_8
        mov    edi,pt_dest
        mov    al,8

        add    edi,8

        mov    pt_dest,edi
        sub    edi,8

        jmp    pt_dest_pallit_n_

; UNLIT N-RUN, non-power of two, non-wrapping
;
;  EAX  =  v_lo << 16
;  ESI  =  u_lo << 16
;  EBP  = dv_lo << 16
;  EDX  = du_lo << 16
;  EBX  = source + u_hi + v_hi * source_row

  _FUNCDEF_ASM pt_unlit_arb_8
        mov    dl,[ebx]
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer,dl
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer+1,al
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer+2,al
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer+3,al
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer+4,al
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer+5,al
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]

        adc    ebx,edi
        add    eax,ebp

        sbb    edi,edi   ; save v carry
        add    esi,edx   ; update u

        mov    byte ptr _pt_buffer+6,al
        mov    al,[ebx]

        mov    edi,_pt_step_table[edi*4+4]
        mov    byte ptr _pt_buffer+7,al

        adc    ebx,edi
        jmp    [_pt_unlit_8_chain]


; massively unrolled pallit core

  _FUNCDEF_ASM pt_pallit_perspective_core
        mov    al,cl       ; number of 8 pixels blocks to do
        nop

pt_pallit_perspective_loop:
        nop
        nop

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

    ; update lighting

        mov    edx,_pt_light
        mov    ebp,dlight_8

        add    edx,ebp
        mov    cl,byte ptr _pt_light+2      ; compute previous light level

        mov    _pt_light,edx

        shr    edx,16

        cmp    cl,dl
        jne    pt_pallit_core_do_lit_run

        mov    byte ptr _pt_lit_buffer,cl
        mov    edx,dword ptr float_buf

        ror    edx,16
        mov    ecx,dword ptr float_buf2

        shr    ecx,16
        mov    ebp,dword ptr float_buf2
        
        shl    ebp,16
        mov    dh,cl

        mov    ecx,ebp
        mov    edi,_pt_tmap_ptr

        mov    ch,byte ptr _pt_lit_buffer

        add    eax,ecx
        mov    ebp,ecx

        mov    ah,[edi+ebx]

        adc    bh,dh
        add    esi,edx

        adc    bl,dl

        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_1:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_2:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+1,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_3:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+2,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_4:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+3,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_5:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+4,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_6:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+5,ah

        mov    ah,[edi+ecx]
        mov    ecx,ebx

        and    ecx,0deadbeefh
pt_if8_mask_7:
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    byte ptr _pt_buffer+6,ah

        and    ebx,0deadbeefh
pt_if8_mask_8:
        mov    ah,[edi+ecx]

        mov    [esp-128],eax
        add    eax,ebp

        and    ebp,0ffff0000h
        mov    byte ptr _pt_buffer+7,ah

        mov    edi,pt_dest
        mov    ecx,_pt_buffer

        mov    eax,[esp-128]
        mov    edx,_pt_buffer+4

        mov    [edi],ecx
        mov    4[edi],edx

        add    edi,8
        dec    al

        mov    pt_dest,edi
        jnz    pt_pallit_perspective_loop

        ret

pt_pallit_core_do_lit_run:

        mov    byte ptr count_save,al
        mov    edx,_pt_light

        mov    ebp,_pt_dlight
        sub    edx,ebp
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+7,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+6,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+5,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+4,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+3,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+2,cl
        mov    ecx,edx
        shr    ecx,16
        sub    edx,ebp
        mov    byte ptr _pt_lit_buffer+1,cl
        mov    ecx,edx
        shr    ecx,16
        mov    byte ptr _pt_lit_buffer,cl

        mov    ebp,dword ptr float_buf2
        mov    edx,dword ptr float_buf

        ror    edx,16
        mov    ecx,ebp

        ror    ecx,16
        
        shl    ebp,16
        mov    dh,cl

        call   pt_unlit_8_

        mov    pt_dest,edi
        mov    al,byte ptr count_save

        dec    al
        jnz    pt_pallit_perspective_loop

        ret


  _FUNCDEF_ASM pt_unlit_perspective_unrolled
        mov    al,cl
        mov    edi,_pt_tmap_ptr

ptup_top:
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

        mov    edx,dword ptr float_buf
        mov    ebp,dword ptr float_buf2

        ror    edx,16

        shl    ebp,16
        mov    ecx,dword ptr float_buf2

        sar    ecx,16
        add    esi,edx
        
        mov    dh,cl
        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        nop
        nop

        mov    ch,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh

        shl    ecx,16

        mov    cl,[edi+ebx]
        add    esi,edx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        nop

        nop
        nop

        mov    ch,[edi+ebx]

        ror    ecx,16

        mov    _pt_buffer,ecx
        add    esi,edx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        nop
        nop

        mov    cl,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        add    esi,edx

        nop
        nop

        mov    ch,[edi+ebx]

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh

        shl    ecx,16

        mov    cl,[edi+ebx]
        add    esi,edx

        adc    bl,dl
        add    eax,ebp

        adc    bh,dh
        add    eax,ebp

        mov    ebp,pt_dest
        nop

        mov    ch,[edi+ebx]

        adc    bh,dh
        add    esi,edx

        adc    bl,dl
        mov    edx,_pt_buffer

        mov    [ebp],edx
        add    ebp,8

        ror    ecx,16

        mov    [ebp-4],ecx
        mov    pt_dest,ebp

        dec    al
        jnz    ptup_top

        mov    edi,pt_dest
        ret


_TEXT   ends

END
