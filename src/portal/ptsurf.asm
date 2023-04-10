; $Header: r:/t2repos/thief2/src/portal/ptsurf.asm,v 1.15 1998/04/06 12:43:14 MAT Exp $
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
        include thunks.inc

	.list

	assume	cs:_TEXT, ds:_DATA

        extd _grd_light_table

_DATA	segment	dword public USE32 'DATA'

	align	4

texture_row dd 0
output_row  dd 0
light_row   dd 0
didy        dd 0
didxdy      dd 0
temp_light1 dd 0
temp_light2 dd 0
tempy       dd 0


; import
externdef __portal_surface_texture_row: dword
externdef __portal_surface_output_row: dword
externdef __portal_surface_lightmap_row: dword
externdef __portal_surface_texture: dword
externdef __portal_surface_output: dword
externdef __portal_surface_lightmap: dword
                                   
_DATA	ends


_TEXT	segment para public USE32 'CODE'


; eax i
; ebx light table
; ecx bytes being processed
; edx bytes being processed
; esi source
; edi dest
; ebp didx
pt_buildspan_8:
        mov  ch,ah
        add  eax,ebp

        mov  cl,[esi]
        mov  dh,ah

        mov  dl,[esi+1]
        add  eax,ebp

        mov  cl,[ebx+ecx]
        nop

        mov  dl,[ebx+edx]
        mov  [edi],cl

        mov  ch,ah
        add  eax,ebp

        mov  cl,[esi+2]
        mov  dh,ah    

        mov  [edi+1],dl
        mov  dl,[esi+3]

        mov  cl,[ebx+ecx]
        add  eax,ebp

        mov  [edi+2],cl
        mov  ch,ah

        add  eax,ebp
        mov  dl,[ebx+edx]

        mov  cl,[esi+4]
        mov  dh,ah

        mov  [edi+3],dl
        mov  dl,[esi+5]

        mov  cl,[ebx+ecx]
        add  eax,ebp

        mov  dl,[ebx+edx]
        mov  [edi+4],cl

        mov  ch,ah
        add  eax,ebp

        mov  cl,[esi+6]
        mov  dh,ah    

        mov  [edi+5],dl
        mov  dl,[esi+7]

        mov  cl,[ebx+ecx]
        add  eax,ebp

        mov  [edi+6],cl
        mov  dl,[ebx+edx]

        mov  [edi+7],dl
        ret




pt_buildspan_8_x:
        mov  ecx,eax
        add  eax,ebp
        shr  ecx,8

        mov  cl,[esi]
        mov  edx,eax
        shr  edx,8

        mov  dl,[esi+1]
        add  eax,ebp

        mov  cl,[ebx+ecx]
        nop

        mov  dl,[ebx+edx]
        mov  [edi],cl

        mov  ecx,eax
        add  eax,ebp
        shr  ecx,8

        mov  cl,[esi+2]
        mov  edx,eax
        shr  edx,8

        mov  [edi+1],dl
        mov  dl,[esi+3]

        mov  cl,[ebx+ecx]
        add  eax,ebp

        mov  [edi+2],cl
        mov  ecx,eax
        shr  ecx,8

        add  eax,ebp
        mov  dl,[ebx+edx]

        mov  cl,[esi+4]
        mov  edx,eax
        shr  edx,8

        mov  [edi+3],dl
        mov  dl,[esi+5]

        mov  cl,[ebx+ecx]
        add  eax,ebp

        mov  dl,[ebx+edx]
        mov  [edi+4],cl

        mov  ecx,eax
        shr  ecx,8
        add  eax,ebp

        mov  cl,[esi+6]
        mov  edx,eax
        shr  edx,8

        mov  [edi+5],dl
        mov  dl,[esi+7]

        mov  cl,[ebx+ecx]
        add  eax,ebp

        mov  [edi+6],cl
        mov  dl,[ebx+edx]

        mov  [edi+7],dl
        ret

  ;   ESI   =  texture source
  ;   EDI   =  output location
  ;   EDX   =  lightmap source

  ;   EAX   =  texture row
  ;   EBX   =  output row
  ;   ECX   =  lightmap row
_FUNCDEF_ASM pt_surfbuild_16
        push  ebp
        push  esi

        push  eax
        push  ebx

        push  ecx
        push  edx

        push  edi
        mov   eax, [__portal_surface_texture_row]

        mov   ebx, [__portal_surface_output_row]
        mov   ebp, [__portal_surface_lightmap_row]

        mov   edx, [__portal_surface_lightmap]
        sub   eax, 8

        mov   esi, [__portal_surface_texture]
        sub   ebx,8

        mov   texture_row, eax
        mov   output_row, ebx

        mov   bl,[edx]
        mov   cl,[edx + ebp + 1]

        mov   bh, [edx + 1] 
        mov   ch, [edx + ebp]

        and   ebx, 0f0f0h       ; mask off all but upper 4 bits of light levels
        and   ecx, 0f0f0h

        mov   edi, [__portal_surface_output]
        cmp   ebx, ecx

        jne   pt_surfbuild_gouraud_16

        cmp   cl, ch

        je    pt_surfbuild_uniform_16

pt_surfbuild_gouraud_16:
        xor   eax,eax
        xor   ebx,ebx

        mov   al, [edx]
        xor   ecx, ecx

        mov   bl, [edx+1]
        mov   cl, [edx+ebp]

        sub   ebx,eax       ; ebx == di(0)dx * 16
        mov   dl,[edx+ebp+1]

        shl   eax,4         ; eax == i(0)*16
        and   edx,0ffh

        sub   edx,ecx       ; edx == di(16)dx * 16

        shl   ecx,4         ; ecx == i(16)*16
        sub   edx,ebx       ; edx == di(0)dxdy * 256

        shl   ebx,4         ; ebx == di(0)dx * 64 
        sub   ecx,eax       ; ecx == di(0)dy * 256

        shl   eax,4         ; eax == i(0)*256
        mov   didy,ecx

        xor   ecx,ecx
        mov   didxdy,edx

        mov   ebp,ebx
        mov   ebx,_grd_light_table

        xor   edx,edx
        mov   ecx, 16

        ; eax =  0..15 * 256*16
        ; eax derived from number from 0..255, so
        ; eax = 0..255 * 256

pt_surf16_looptop:
        push  eax
        push  ecx

        shr   eax, 4
        mov   edx, ebp

        shr   edx, 28           ; round negative didx towards 0
        push  ebp

        mov   cl,[edi]          ; cache prefetch
        add   ebp, edx

        sar   ebp, 4
        call  pt_buildspan_8

        add   esi, 8
        add   edi, 8

        mov   cl,[edi]          ; cache prefetch
        call  pt_buildspan_8
        
        add   esi,texture_row
        add   edi,output_row

        ; wait for 2-cycle instructions

        pop   ebp
        pop   ecx
        pop   eax

        add   eax,didy
        add   ebp,didxdy

        ; wait for 2-cycle instructions

        dec   ecx
        jnz   pt_surf16_looptop

        jmp   pt_surfbuild_done_16

pt_surfbuild_uniform_16:
        shr   ebx, 4
        mov   ebp, _grd_light_table

        mov   eax, ebx
        mov   ecx, ebx

        mov   edx, ebx
        mov   texture_row, 16

pt_surfbuild_uniform_16_loop:
        mov   al, [edi] ; prefetch
        nop

        ; first set of 4 texels
        mov al, [esi]
        mov bl, [esi + 4]

        mov cl, [esi + 8]
        mov dl, [esi + 12]

        mov al, [eax + ebp]
        mov bl, [ebx + ebp]
        
        mov cl, [ecx + ebp]
        mov dl, [edx + ebp]

        mov [edi], al
        mov [edi + 4], bl
        
        mov [edi + 8], cl
        mov [edi + 12], dl

        ; second set of 4 texels
        mov al, [esi + 1]
        mov bl, [esi + 5]

        mov cl, [esi + 9]
        mov dl, [esi + 13]

        mov al, [eax + ebp]
        mov bl, [ebx + ebp]
        
        mov cl, [ecx + ebp]
        mov dl, [edx + ebp]

        mov [edi + 1], al
        mov [edi + 5], bl
        
        mov [edi + 9], cl
        mov [edi + 13], dl

        ; third set of 4 texels
        mov al, [esi + 2]
        mov bl, [esi + 6]

        mov cl, [esi + 10]
        mov dl, [esi + 14]

        mov al, [eax + ebp]
        mov bl, [ebx + ebp]
        
        mov cl, [ecx + ebp]
        mov dl, [edx + ebp]

        mov [edi + 2], al
        mov [edi + 6], bl
        
        mov [edi + 10], cl
        mov [edi + 14], dl

        ; fourth set of 4 texels
        mov al, [esi + 3]
        mov bl, [esi + 7]

        mov cl, [esi + 11]
        mov dl, [esi + 15]

        mov al, [eax + ebp]
        mov bl, [ebx + ebp]

        mov cl, [ecx + ebp]
        mov dl, [edx + ebp]

        mov [edi + 3], al
        mov [edi + 7], bl
        
        mov [edi + 11], cl
        mov [edi + 15], dl

        add esi, [__portal_surface_texture_row]
        add edi, __portal_surface_output_row

        dec texture_row

        jnz pt_surfbuild_uniform_16_loop

pt_surfbuild_done_16:
        pop   edi
        pop   edx
        pop   ecx
        pop   ebx
        pop   eax
        pop   esi
        pop   ebp
        ret        




_FUNCDEF_ASM pt_surfbuild_8
        push  ebp
        push  esi

        push  eax
        push  ebx

        push  ecx
        push  edx
        
        push  edi
        mov   eax, [__portal_surface_texture_row]

        mov   ebx, [__portal_surface_output_row]
        mov   ebp, [__portal_surface_lightmap_row]

        mov   edx, [__portal_surface_lightmap]
        mov   texture_row,eax

        mov   esi, [__portal_surface_texture]
        mov   output_row, ebx

        mov   bl, [edx]
        mov   cl, [edx + ebp + 1]

        mov   bh, [edx + ebp]
        mov   ch, [edx + 1]

        and   ebx, 0f0f0h
        and   ecx, 0f0f0h

        mov   edi, [__portal_surface_output]
        cmp   ebx, ecx

        jne   pt_surfbuild_gouraud_8

        cmp   cl, ch

        je    pt_surfbuild_uniform_8

pt_surfbuild_gouraud_8:
        xor   eax,eax
        xor   ebx,ebx

        mov   al, [edx]
        xor   ecx, ecx

        mov   bl, [edx+1]
        mov   cl, [edx+ebp]

        sub   ebx, eax      ; ebx == di(0)dx * 16
        mov   dl, [edx+ebp+1]

        shl   eax, 3        ; eax == i(0)*8
        and   edx, 0ffh

        sub   edx,ecx       ; edx == di(16)dx * 16

        shl   ecx,3         ; ecx == i(16)*16
        sub   edx,ebx       ; edx == di(0)dxdy * 256

        shl   ebx,3         ; ebx == di(0)dx * 64 
        sub   ecx,eax       ; ecx == di(0)dy * 256

        shl   eax,3         ; eax == i(0)*256
        mov   didy,ecx

        xor   ecx,ecx
        mov   didxdy,edx

        mov   ebp,ebx
        mov   ebx,_grd_light_table

        xor   edx,edx
        mov   ecx,8

pt_surf8_looptop:
        push  eax
        push  ecx

        shr   eax, 2
        mov   edx, ebp

        shr   edx, 30           ; round negative didx towards 0
        push  ebp

        mov   cl,[edi]          ; cache prefetch
        add   ebp, edx

        sar   ebp, 2
        call  pt_buildspan_8

        add   esi,texture_row
        add   edi,output_row

        ; wait for 2-cycle instructions

        pop   ebp
        pop   ecx
        pop   eax

        add   eax,didy
        add   ebp,didxdy

        ; wait for 2-cycle instructions

        dec   ecx
        jnz   pt_surf8_looptop

        jmp   pt_surfbuild_done_8

pt_surfbuild_uniform_8:
        shr   ebx, 4
        mov   ebp, [_grd_light_table]

        mov   eax, ebx
        mov   ecx, ebx

        mov   edx, ebx
        mov   texture_row, 8

pt_surfbuild_uniform_8_loop:
        mov   al, [edi] ; prefetch
        nop

        ; first set of 4 texels
        mov al, [esi]
        mov bl, [esi + 4]

        mov cl, [esi + 2]
        mov dl, [esi + 6]

        mov al, [eax + ebp]
        mov bl, [ebx + ebp]

        mov cl, [ecx + ebp]
        mov dl, [edx + ebp]

        mov [edi], al
        mov [edi + 4], bl

        mov [edi + 2], cl
        mov [edi + 6], dl

        ; second set of 4 texels
        mov al, [esi + 1]
        mov bl, [esi + 5]

        mov cl, [esi + 3]
        mov dl, [esi + 7]

        mov al, [eax + ebp]
        mov bl, [ebx + ebp]

        mov cl, [ecx + ebp]
        mov dl, [edx + ebp]

        mov [edi + 1], al
        mov [edi + 5], bl

        mov [edi + 3], cl
        mov [edi + 7], dl

        add esi, [__portal_surface_texture_row]
        add edi, [__portal_surface_output_row]

        dec texture_row
        jnz pt_surfbuild_uniform_8_loop

pt_surfbuild_done_8:                        
        pop   edi
        pop   edx
        pop   ecx
        pop   ebx
        pop   eax
        pop   esi
        pop   ebp
        ret   


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; pt_surfbuild_4
;
_FUNCDEF_ASM pt_surfbuild_4
        push  edx
        push  edi

        push  eax
        push  esi

        push  ecx
        push  ebx

        push  ebp
        mov   edi, [__portal_surface_output]

        mov   ebp, [__portal_surface_lightmap_row]
        mov   al, [edi]         ;  prefetch first row

        mov   esi, [__portal_surface_texture]
        mov   edx, [__portal_surface_lightmap]

; setup has four light levels--            a---b
; we want our results x16                  |   |
; for our light table lookups              c---d

        xor   eax, eax
        xor   ecx, ecx

        mov   al, [edx]
        mov   cl, [edx + ebp]

        mov   bl, [edx + 1]
        mov   dl, [edx + ebp + 1]

        and   ebx, 0ffh
        and   edx, 0ffh

        sub   edx, ecx
        mov   ebp, ebx

        sub   ecx, eax
        sub   ebp, eax

        shl   ebp, 2            ; initial didx = (-4a + 4b) / 16
        add   edx, eax

        shl   ecx, 2            ; didy = (-4a + 4c) / 16
        sub   edx, ebx          ; didxdy = (a - b - c + d) / 16

        shl   eax, 4            ; i0 = (16a) / 16
        mov   [didy], ecx

        mov   [didxdy], edx
        mov   dl, [edi + 0999999h];  prefetch second row
row_4_self_mod_1:

; eax   left edge intensity
; ebx   first point being processed
; ecx   second point being processed
; edx   output bytes
; esi   source texture
; edi   surface being built + misc.
; ebp   didx

        lea   ebx, [eax + 2 * ebp]
        lea   ecx, [eax + 2 * ebp]

        mov   bl, [esi + 2]     ; We process the right-hand texels first
        add   ecx, ebp          ; so we can shift them into upper 16 of edx.

        mov   cl, [esi + 3]
        mov   edi, [didxdy]

        mov   dl, [ebx + 0999999h]; self-modifying reference to light table
gouraud_4_self_mod_1:
        mov   ebx, eax

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_2:
        lea   ecx, [eax + ebp]

        shl   edx, 16
        mov   bl, [esi]

        mov   cl, [esi + 1]
        add   ebp, edi          ; didx += didxdy

        mov   dl, [ebx + 0999999h]
gouraud_4_self_mod_3:
        mov   edi, [__portal_surface_output]

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_4:
        mov   ebx, [didy]

        mov   ecx, [__portal_surface_texture_row]
        add   eax, ebx          ; left edge i += didy

        mov   [edi], edx
        add   esi, ecx          ; texture pointer += texture row

        mov   dl, [edi + 0999999h]   ;  prefetch third row
row_4_self_mod_2:
        nop

; second row
        lea   ebx, [eax + 2 * ebp]
        lea   ecx, [eax + 2 * ebp]

        mov   bl, [esi + 2]     ; We process the second pair of texels first
        add   ecx, ebp          ; so we can shift them into upper 16 of edx.

        mov   cl, [esi + 3]
        mov   edi, [didxdy]

        mov   dl, [ebx + 0999999h]; self-modifying reference to light table
gouraud_4_self_mod_5:
        mov   ebx, eax

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_6:
        lea   ecx, [eax + ebp]

        shl   edx, 16
        mov   bl, [esi]

        mov   cl, [esi + 1]
        add   ebp, edi          ; didx += didxdy

        mov   dl, [ebx + 0999999h]
gouraud_4_self_mod_7:
        mov   edi, [__portal_surface_output]

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_8:
        mov   ebx, [didy]

        mov   ecx, [__portal_surface_texture_row]
        add   eax, ebx          ; left edge i += didy

        mov   [edi + 0999999h], edx
row_4_self_mod_3:
        add   esi, ecx          ; texture pointer += texture row

        mov   dl, [edi + 0999999h];  prefetch fourth row
row_4_self_mod_4:
        nop

; third row
        lea   ebx, [eax + 2 * ebp]
        lea   ecx, [eax + 2 * ebp]

        mov   bl, [esi + 2]     ; We process the second pair of texels first
        add   ecx, ebp          ; so we can shift them into upper 16 of edx.

        mov   cl, [esi + 3]
        mov   edi, [didxdy]

        mov   dl, [ebx + 0999999h]; self-modifying reference to light table
gouraud_4_self_mod_9:
        mov   ebx, eax

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_10:
        lea   ecx, [eax + ebp]

        shl   edx, 16
        mov   bl, [esi]

        mov   cl, [esi + 1]
        add   ebp, edi          ; didx += didxdy
        
        mov   dl, [ebx + 0999999h]
gouraud_4_self_mod_11:
        mov   edi, [__portal_surface_output]

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_12:
        mov   ebx, [didy]

        mov   ecx, [__portal_surface_texture_row]
        add   eax, ebx          ; left edge i += didy

        mov   [edi + 0999999h], edx
row_4_self_mod_5:
        add   esi, ecx          ; texture pointer += texture row

; fourth row
        lea   ebx, [eax + 2 * ebp]
        lea   ecx, [eax + 2 * ebp]

        mov   bl, [esi + 2]     ; We process the second pair of texels first
        add   ecx, ebp          ; so we can shift them into upper 16 of edx.

        mov   cl, [esi + 3]
        nop

        mov   dl, [ebx + 0999999h]; self-modifying reference to light table
gouraud_4_self_mod_13:
        mov   ebx, eax

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_14:
        lea   ecx, [eax + ebp]

        shl   edx, 16
        mov   bl, [esi]

        mov   cl, [esi + 1]
        pop   ebp

        mov   dl, [ebx + 0999999h]
gouraud_4_self_mod_15:
        pop   ebx

        mov   dh, [ecx + 0999999h]
gouraud_4_self_mod_16:
        pop   ecx

        pop   esi
        pop   eax

        mov   [edi + 0999999h], edx
row_4_self_mod_6:

        pop   edi
        pop   edx
        ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; pt_surfbuild_2
;
_FUNCDEF_ASM pt_surfbuild_2

        push  eax
        push  ebx

        push  ecx
        push  edx

        push  esi
        push  edi

        xor   ecx, ecx
        xor   edx, edx

        ; We'll build one pixel/light combo in ecx and one in edx.
        ; While the light for the upper-left pixel is raw data, the
        ; light for the upper-right is the average of two values.
        mov   edi, [_grd_light_table]
        mov   esi, [__portal_surface_lightmap]
        
        mov   ebx, [__portal_surface_texture]
        mov   al, [edi]          ; prefetch

        mov   cl, [esi]
        mov   dl, [esi + 1]

        add   edx, ecx
        mov [temp_light1], ecx  ; We'll be averaging these light values

        ; We move our light bytes into ch and dh with bit shifts to
        ; get them down to 4 bits each.
        shl   ecx, 4
        mov [temp_light2], edx  ; with others for the lower pixels.

        shl   edx, 3            ; average of two lightmap points
        mov   cl, [ebx]
        
        mov   dl, [ebx + 1]
        mov   cl, [edi + ecx]   ; clut upper-left
        
        mov   eax, [__portal_surface_output]
        mov   dl, [edi + edx]   ; clut upper-right
        
        mov   [eax], cl
        mov   [eax + 1], dl

        add   esi, [__portal_surface_lightmap_row]

        ; The light for the lower two pixels is a little more involved
        ; since they are both averages.
        xor   ecx, ecx
        xor   edx, edx

        mov   cl, [esi]
        mov   dl, [esi + 1]

        add   edx, [temp_light2]; average of two right-hand lights
        add   ebx, [__portal_surface_texture_row]

        add   edx, ecx          ; lower-right light * 4
        add   eax, [__portal_surface_output_row]

        add   ecx, [temp_light1]; lower-left light * 2

        shl   edx, 2            ; average of four lightmap points

        shl   ecx, 3            ; average of two lightmap points

        mov   cl, [ebx]         ; get texture points
        mov   dl, [ebx + 1]

        mov   cl, [edi + ecx]   ; clut upper-left
        mov   dl, [edi + edx]   ; clut upper-right

        mov   [eax], cl
        mov   [eax + 1], dl

        pop   edi
        pop   esi
        pop   edx
        pop   ecx
        pop   ebx
        pop   eax

        ret   


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; pt_surfbuild_1
;
; We prefetch here since this is called on seqential bytes.
;
_FUNCDEF_ASM pt_surfbuild_1
        push  eax
        push  edx

        push  ebx
        push  ecx
        
        mov   eax, [__portal_surface_lightmap]
        mov   ebx, [__portal_surface_output]

        xor   edx, edx
        mov   ecx, [__portal_surface_texture]

        mov   dh, [eax]         ; high byte is light level
        mov   al, [ebx]         ; prefetch

        shr   dh, 4             ; 16 levels of shading
        mov   eax, [_grd_light_table]

        mov   dl, [ecx]         ; low byte is texture data
        
        mov   al, [eax + edx]   ; translate light
        pop   ecx

        mov   [ebx], al
        pop   ebx

        pop   edx
        pop   eax

        ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; voif pt_surfbuild_setup (void)
; 
; These set up some self-modifying offsets.  Actually, only one of
; them does.

_FUNCDEF_ASM pt_surfbuild_setup_1
        ret

_FUNCDEF_ASM pt_surfbuild_setup_2
        ret
        
_FUNCDEF_ASM pt_surfbuild_setup_4
        push   eax
        push   edx

        mov    eax, [_grd_light_table]
        mov    edx, dword ptr ds:[gouraud_4_self_mod_1 - 4]

        cmp    eax, edx
        je     pt_surfbuild_setup_4_light_done

        mov    dword ptr ds:gouraud_4_self_mod_1  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_2  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_3  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_4  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_5  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_6  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_7  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_8  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_9  - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_10 - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_11 - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_12 - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_13 - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_14 - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_15 - 4, eax
        mov    dword ptr ds:gouraud_4_self_mod_16 - 4, eax

pt_surfbuild_setup_4_light_done:
        mov   eax, __portal_surface_output_row
        mov   edx, dword ptr ds:[gouraud_4_self_mod_1 - 4]

        cmp    eax, edx
        je     pt_surfbuild_setup_4_row_done

        mov    dword ptr ds:row_4_self_mod_1 - 4, eax
        mov    dword ptr ds:row_4_self_mod_3 - 4, eax

        lea    edx, [eax + eax * 2]     ; row * 3
        add    eax, eax                 ; row * 2

        mov    dword ptr ds:row_4_self_mod_4 - 4, edx
        mov    dword ptr ds:row_4_self_mod_6 - 4, edx

        mov    dword ptr ds:row_4_self_mod_2 - 4, eax
        mov    dword ptr ds:row_4_self_mod_5 - 4, eax

pt_surfbuild_setup_4_row_done:
        pop   edx
        pop   eax

        ret


_FUNCDEF_ASM pt_surfbuild_setup_8
        ret               

_FUNCDEF_ASM pt_surfbuild_setup_16
        ret

        
        
;pt_buildspan_8_pal:
;        mov  cl,ah
;        add  eax,ebp
;
;        mov  ch,ah
;        add  eax,ebp
;
;        mov  dl,ah
;        add  eax,ebp
;
;        mov  dh,ah
;        add  eax,ebp
;
;        shl  edx,16
;        and  ecx,0ffffh
;
;        or   edx,ecx
;        mov  ecx,[esi]
;
;        add  edx,ecx
;        mov  cl,ah
;
;        mov  [edi],edx
;        add  eax,ebp
;
;        mov  ch,ah
;        add  eax,ebp
;
;        mov  dl,ah
;        add  eax,ebp
;
;        mov  dh,ah
;        add  eax,ebp
;
;        shl  edx,16
;        and  ecx,0ffffh
;
;        or   edx,ecx
;        mov  ecx,[esi]
;
;        add  edx,ecx
;
;        mov  [edi+4],edx
;        ret
;
;  ;   ESI   =  texture source
;  ;   EDI   =  output location
;  ;   EDX   =  lightmap source
;
;  ;   EAX   =  texture row
;  ;   EBX   =  output row
;  ;   ECX   =  lightmap row
;pt_surfbuild_16_pal:
;        push  ebp
;        sub   eax,8
;
;        mov   texture_row,eax
;        sub   ebx,8
;
;        mov   output_row,ebx
;        mov   ebp,ecx
;
;        xor   eax,eax
;        xor   ebx,ebx
;
;        mov   al,[edx]
;        xor   ecx,ecx
;
;        mov   bl,[edx+1]
;        mov   cl,[edx+ebp]
;
;        sub   ebx,eax       ; ebx == di(0)dx * 16
;        mov   dl,[edx+ebp+1]
;
;        shl   eax,4         ; eax == i(0)*16
;        and   edx,0ffh
;
;        sub   edx,ecx       ; edx == di(16)dx * 16
;
;        shl   ecx,4         ; ecx == i(16)*16
;        sub   edx,ebx       ; edx == di(0)dxdy * 256
;
;        shl   ebx,4         ; ebx == di(0)dx * 64 
;        sub   ecx,eax       ; ecx == di(0)dy * 256
;
;        shl   eax,4         ; eax == i(0)*256
;        mov   didy,ecx
;
;        shr   edx,4
;        xor   ecx,ecx
;
;        shr   ebx,4
;        mov   didxdy,edx
;
;        mov   ebp,ebx
;        mov   ebx,_grd_light_table
;
;        xor   edx,edx
;        mov   ecx,16
;
;pt_surf16_looptop_pal:
;        push  eax
;        push  ecx
;
;        shr   eax,5
;        mov   cl,[edi]
;        shr   ebp,1
;        call  pt_buildspan_8_pal
;
;        add   esi,8
;        add   edi,8
;
;        mov   cl,[edi]
;        call  pt_buildspan_8_pal
;        
;        add   esi,texture_row
;        add   edi,output_row
;
;        ; wait for 2-cycle instructions
;
;        pop   ecx
;        pop   eax
;
;        add   eax,didy
;        add   ebp,didxdy
;        
;        ; wait for 2-cycle instructions
;
;        dec   ecx
;        jnz   pt_surf16_looptop_pal
;
;        pop   ebp
;        ret        
;
;pt_surfbuild_8_pal:
;        push  ebp
;        mov   texture_row,eax
;
;        mov   output_row,ebx
;        mov   ebp,ecx
;
;        xor   eax,eax
;        xor   ebx,ebx
;
;        mov   al,[edx]
;        xor   ecx,ecx
;
;        mov   bl,[edx+1]
;        mov   cl,[edx+ebp]
;
;        sub   ebx,eax       ; ebx == di(0)dx * 16
;        mov   dl,[edx+ebp+1]
;
;        shl   eax,3         ; eax == i(0)*16
;        and   edx,0ffh
;
;        sub   edx,ecx       ; edx == di(16)dx * 16
;
;        shl   ecx,3         ; ecx == i(16)*16
;        sub   edx,ebx       ; edx == di(0)dxdy * 256
;
;        shl   ebx,3         ; ebx == di(0)dx * 64 
;        sub   ecx,eax       ; ecx == di(0)dy * 256
;
;        shl   eax,3         ; eax == i(0)*256
;        mov   didy,ecx
;
;        shr   edx,2
;        xor   ecx,ecx
;
;        shr   ebx,2
;        mov   didxdy,edx
;
;        mov   ebp,ebx
;        mov   ebx,_grd_light_table
;
;        xor   edx,edx
;        mov   ecx,8
;
;pt_surf8_looptop_pal:
;        push  eax
;        push  ecx
;
;        shr   eax,3
;        mov   cl,[edi]
;        shr   ebp,1
;        call  pt_buildspan_8_pal
;
;        add   esi,texture_row
;        add   edi,output_row
;
;        ; wait for 2-cycle instructions
;
;        pop   ecx
;        pop   eax
;
;        add   eax,didy
;        add   ebp,didxdy
;        
;        ; wait for 2-cycle instructions
;
;        dec   ecx
;        jnz   pt_surf8_looptop_pal
;
;        pop   ebp
;        ret        

_TEXT   ends

END
