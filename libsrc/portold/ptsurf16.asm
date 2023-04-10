; $Header: r:/t2repos/thief2/libsrc/portold/ptsurf16.asm,v 1.2 1998/04/17 15:07:23 KEVIN Exp $
; 16 bit surface building

.486
        .nolist

        include type.inc
        include cseg.inc
        include dseg.inc
        include macros.inc
        include thunks.inc

        .list

assume cs:_TEXT
assume ds:_DATA
_DATA   segment
        extd    _grd_ltab816
        extd    __portal_surface_texture
        extd    __portal_surface_output
        extd    __portal_surface_lightmap
        extd    __portal_surface_texture_row
        extd    __portal_surface_output_row
        extd    __portal_surface_lightmap_row

src_row         dd 0
dst_row         dd 0
i_left          dd 0
i_right         dd 0
di_left         dd 0
di_right        dd 0
count           dd 0

last_block1_ltab        dd 0deadbeefh
last_block2_ltab        dd 0deadbeefh
last_block4_ltab        dd 0deadbeefh
last_block8_ltab        dd 0deadbeefh
last_block16_ltab       dd 0deadbeefh
_DATA   ends

_TEXT   segment
_FUNCDEF_ASM pt16_surfbuild_1
        push    edi
        mov     edi, __portal_surface_lightmap

        push    eax
        xor     eax, eax

        mov     al, [edi]
        mov     edi, __portal_surface_texture

        push    edx
        xor     edx, edx

        shl     eax, 5
        mov     dl, [edi]

        and     eax, 01fe00h
        mov     edi, __portal_surface_output

        mov     dx, 0deadbeefh[eax + 2*edx]
block1_ltab:

        mov     [edi], dx

        pop     edx
        pop     eax
        pop     edi
        ret

_FUNCDEF_ASM pt16_surfbuild_setup_1
        push    eax
        push    edx

        mov     eax, _grd_ltab816
        mov     edx, last_block1_ltab

        cmp     eax, edx
        jz      skip_setup_1

        mov     d ds:[block1_ltab-4], eax
        mov     last_block1_ltab, eax
skip_setup_1:
        pop     edx
        pop     eax
        ret




common_setup:
        mov     eax, __portal_surface_texture_row
        mov     edx, __portal_surface_output_row

        mov     src_row, eax
        mov     dst_row, edx

        mov     esi, __portal_surface_lightmap
        mov     edi, __portal_surface_lightmap_row

        xor     eax, eax
        xor     ebx, ebx

        xor     ecx, ecx
        xor     edx, edx

        mov     al, [esi]
        mov     bl, [esi+1]

        mov     cl, [esi+edi]
        mov     dl, [esi+edi+1]

        mov     esi, __portal_surface_texture
        mov     edi, __portal_surface_output

        ret

_FUNCDEF_ASM pt16_surfbuild_2
        push    esi
        push    edi

        push    eax
        push    ebx

        push    ecx
        push    edx

        call    common_setup

        add     ecx, eax
        add     edx, ebx

        add     ebx, eax

        shl     eax, 5

        shl     ebx, 4
        add     edx, ecx

        push    edx
        xor     edx, edx

        mov     dl, [esi]
        and     eax, 01fe00h

        shl     ecx, 4
        and     ebx, 01fe00h

        mov     ax, 0deadbeefh[eax + 2*edx]
block2_ltab0:

        mov     [edi], ax

        xor     edx, edx
        xor     eax, eax

        mov     dl, [esi+1]
        and     ecx, 01fe00h

        mov     bx, 0deadbeefh[ebx + 2*edx]
block2_ltab1:

        mov     [edi + 2], bx

        add     esi, src_row
        add     edi, dst_row

        pop     edx
        xor     ebx, ebx

        shl     edx, 3
        mov     al, [esi]

        and     edx, 01fe00h
        mov     bl, [esi+1]

        mov     ax, 0deadbeefh[ecx + 2*eax]
block2_ltab2:

        mov     [edi], ax

        mov     cx, 0deadbeefh[edx + 2*ebx]
block2_ltab3:

        mov     [edi+2], cx

        pop     edx
        pop     ecx

        pop     ebx
        pop     eax

        pop     edi
        pop     esi

        ret

_FUNCDEF_ASM pt16_surfbuild_setup_2
        push    eax
        push    edx

        mov     eax, _grd_ltab816
        mov     edx, last_block2_ltab

        cmp     eax, edx
        jnz     do_setup_2

        pop     edx
        pop     eax
        ret

do_setup_2:
        mov     d ds:[block2_ltab0-4], eax
        mov     d ds:[block2_ltab1-4], eax
        mov     d ds:[block2_ltab2-4], eax
        mov     d ds:[block2_ltab3-4], eax
        mov     last_block2_ltab, eax
        pop     edx
        pop     eax
        ret





_FUNCDEF_ASM pt16_surfbuild_4
        push    ebp
        push    esi

        push    eax
        push    ebx

        push    ecx
        push    edx

        push    edi

        call    common_setup

        sub     ecx, eax
        mov     ebp, eax

        shl     ecx, 3
        sub     edx, ebx

        shl     edx, 3
        mov     di_left, ecx

        shl     ebp, 5
        mov     di_right, edx

        shl     ebx, 5
        mov     eax, 4

; eax = count
; ebp = i_left
; ebx = i_right
; edi = dest
; esi = src

block4_loop:
        mov     i_left, ebp
        mov     i_right, ebx

        sub     ebx, ebp
        xor     edx, edx

        shr     ebx, 2
        mov     count, eax

        mov     eax, ebp
        add     ebp, ebx

        mov     dl,[esi]
        and     eax, 01fe00h

        mov     ecx, ebp
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block4_ltab0:

        mov     [edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 1[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block4_ltab1:

        mov     2[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 2[esi]
        and     ecx, 01fe00h

        mov     ebp, i_left
        mov     ebx, i_right

        mov     dx, 0deadbeefh[eax+2*edx]
block4_ltab2:

        mov     4[edi], dx

        xor     edx, edx
        mov     eax, count

        mov     dl, 3[esi]
        nop

        add     ebp, di_left
        add     ebx, di_right

        mov     dx, 0deadbeefh[ecx+2*edx]
block4_ltab3:

        mov     6[edi], dx

        add     esi, src_row
        add     edi, dst_row

        dec     eax
        jnz     block4_loop

        pop     edi

        pop     edx
        pop     ecx

        pop     ebx
        pop     eax

        pop     esi
        pop     ebp

_FUNCDEF_ASM pt16_surfbuild_setup_4
        push    eax
        push    edx

        mov     eax, _grd_ltab816
        mov     edx, last_block4_ltab

        cmp     eax, edx
        jnz     do_setup_4

        pop     edx
        pop     eax
        ret

do_setup_4:
        mov     d ds:[block4_ltab0-4], eax
        mov     d ds:[block4_ltab1-4], eax
        mov     d ds:[block4_ltab2-4], eax
        mov     d ds:[block4_ltab3-4], eax
        mov     last_block4_ltab, eax
        pop     edx
        pop     eax
        ret

_FUNCDEF_ASM pt16_surfbuild_8
        push    ebp
        push    esi

        push    eax
        push    ebx

        push    ecx
        push    edx

        push    edi

        call    common_setup

        sub     ecx, eax
        mov     ebp, eax

        shl     ecx, 2
        sub     edx, ebx

        shl     edx, 2
        mov     di_left, ecx

        shl     ebp, 5
        mov     di_right, edx

        shl     ebx, 5
        mov     eax, 8

; eax = count
; ebp = i_left
; ebx = i_right
; edi = dest
; esi = src

block8_loop:
        mov     i_left, ebp
        mov     i_right, ebx

        sub     ebx, ebp
        xor     edx, edx

        shr     ebx, 3
        mov     count, eax

        mov     eax, ebp
        add     ebp, ebx

        mov     dl,[esi]
        and     eax, 01fe00h

        mov     ecx, ebp
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block8_ltab0:

        mov     [edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 1[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block8_ltab1:

        mov     2[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 2[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block8_ltab2:

        mov     4[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 3[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block8_ltab3:

        mov     6[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 4[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block8_ltab4:

        mov     8[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 5[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block8_ltab5:

        mov     10[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 6[esi]
        and     ecx, 01fe00h

        mov     ebp, i_left
        mov     ebx, i_right

        mov     dx, 0deadbeefh[eax+2*edx]
block8_ltab6:

        mov     12[edi], dx

        xor     edx, edx
        mov     eax, count

        mov     dl, 7[esi]
        nop

        add     ebp, di_left
        add     ebx, di_right

        mov     dx, 0deadbeefh[ecx+2*edx]
block8_ltab7:

        mov     14[edi], dx

        add     esi, src_row
        add     edi, dst_row

        dec     eax
        jnz     block8_loop

        pop     edi

        pop     edx
        pop     ecx

        pop     ebx
        pop     eax

        pop     esi
        pop     ebp

_FUNCDEF_ASM pt16_surfbuild_setup_8
        push    eax
        push    edx

        mov     eax, _grd_ltab816
        mov     edx, last_block8_ltab

        cmp     eax, edx
        jnz     do_setup_8

        pop     edx
        pop     eax
        ret

do_setup_8:
        mov     d ds:[block8_ltab0-4], eax
        mov     d ds:[block8_ltab1-4], eax
        mov     d ds:[block8_ltab2-4], eax
        mov     d ds:[block8_ltab3-4], eax
        mov     d ds:[block8_ltab4-4], eax
        mov     d ds:[block8_ltab5-4], eax
        mov     d ds:[block8_ltab6-4], eax
        mov     d ds:[block8_ltab7-4], eax
        mov     last_block8_ltab, eax
        pop     edx
        pop     eax
        ret

_FUNCDEF_ASM pt16_surfbuild_16
        push    ebp
        push    esi

        push    eax
        push    ebx

        push    ecx
        push    edx

        push    edi

        call    common_setup

        sub     ecx, eax
        mov     ebp, eax

        shl     ecx, 1
        sub     edx, ebx

        shl     edx, 1
        mov     di_left, ecx

        shl     ebp, 5
        mov     di_right, edx

        shl     ebx, 5
        mov     eax, 16

; eax = count
; ebp = i_left
; ebx = i_right
; edi = dest
; esi = src

block16_loop:
        mov     i_left, ebp
        mov     i_right, ebx

        sub     ebx, ebp
        xor     edx, edx

        shr     ebx, 4
        mov     count, eax

        mov     eax, ebp
        add     ebp, ebx

        mov     dl,[esi]
        and     eax, 01fe00h

        mov     ecx, ebp
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab0:

        mov     [edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 1[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab1:

        mov     2[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 2[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab2:

        mov     4[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 3[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab3:

        mov     6[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 4[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab4:

        mov     8[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 5[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab5:

        mov     10[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 6[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab6:

        mov     12[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 7[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab7:

        mov     14[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 8[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab8:

        mov     16[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 9[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab9:

        mov     18[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 10[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab10:

        mov     20[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 11[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab11:

        mov     22[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 12[esi]
        add     ebp, ebx

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab12:

        mov     24[edi], dx

        xor     edx, edx
        and     ecx, 01fe00h

        mov     dl, 13[esi]
        mov     eax, ebp

        add     ebp, ebx
        and     eax, 01fe00h

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab13:

        mov     26[edi], dx

        xor     edx, edx
        mov     ecx, ebp

        mov     dl, 14[esi]
        and     ecx, 01fe00h

        mov     ebp, i_left
        mov     ebx, i_right

        mov     dx, 0deadbeefh[eax+2*edx]
block16_ltab14:

        mov     28[edi], dx

        xor     edx, edx
        mov     eax, count

        mov     dl, 15[esi]
        nop

        add     ebp, di_left
        add     ebx, di_right

        mov     dx, 0deadbeefh[ecx+2*edx]
block16_ltab15:

        mov     30[edi], dx

        add     esi, src_row
        add     edi, dst_row

        dec     eax
        jnz     block16_loop

        pop     edi

        pop     edx
        pop     ecx

        pop     ebx
        pop     eax

        pop     esi
        pop     ebp

_FUNCDEF_ASM pt16_surfbuild_setup_16
        push    eax
        push    edx

        mov     eax, _grd_ltab816
        mov     edx, last_block16_ltab

        cmp     eax, edx
        jnz     do_setup_16

        pop     edx
        pop     eax
        ret

do_setup_16:
        mov     d ds:[block16_ltab0-4], eax
        mov     d ds:[block16_ltab1-4], eax
        mov     d ds:[block16_ltab2-4], eax
        mov     d ds:[block16_ltab3-4], eax
        mov     d ds:[block16_ltab4-4], eax
        mov     d ds:[block16_ltab5-4], eax
        mov     d ds:[block16_ltab6-4], eax
        mov     d ds:[block16_ltab7-4], eax
        mov     d ds:[block16_ltab8-4], eax
        mov     d ds:[block16_ltab9-4], eax
        mov     d ds:[block16_ltab10-4], eax
        mov     d ds:[block16_ltab11-4], eax
        mov     d ds:[block16_ltab12-4], eax
        mov     d ds:[block16_ltab13-4], eax
        mov     d ds:[block16_ltab14-4], eax
        mov     d ds:[block16_ltab15-4], eax
        mov     last_block16_ltab, eax
        pop     edx
        pop     eax
        ret

_TEXT   ends

END
