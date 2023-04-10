; $Header: x:/prj/tech/libsrc/g2/RCS/o88il.asm 1.4 1997/11/17 12:32:52 KEVIN Exp $

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc

include grs.inc
include plyrast.inc
include plyparam.inc

assume ds:_DATA
_DATA segment
last_mask_u   dd      0deadbeefh
last_mask_v   dd      0deadbeefh
last_p_src    dd      0deadbeefh
last_shift_v  db      0ffh
_DATA ends

p_dest          equ     G2S_POLY_PARAMS_P
row             equ     G2S_POLY_PARAMS_CANVAS_ROW
dux             equ     G2S_POLY_PARAMS_DUX
dvx             equ     G2S_POLY_PARAMS_DVX
u               equ     G2S_POLY_PARAMS_LEFT_U
v               equ     G2S_POLY_PARAMS_LEFT_V
du              equ     G2S_POLY_PARAMS_LEFT_DU
dv              equ     G2S_POLY_PARAMS_LEFT_DV
x_left          equ     G2S_POLY_PARAMS_LEFT_X
dx_left         equ     G2S_POLY_PARAMS_LEFT_DX
x_frac          equ     G2S_POLY_PARAMS_LEFT_X_FRAC
dx_frac         equ     G2S_POLY_PARAMS_LEFT_DX_FRAC
x_right         equ     G2S_POLY_PARAMS_RIGHT_X
dx_right        equ     G2S_POLY_PARAMS_RIGHT_DX

_TEXT segment

;takes eax = poly params
;      edx = num scanlines
_FUNCDEF opaque_8to8_raster, 2
        push    ebp
        mov     ebp, eax

        push    edi
        push    esi

        push    ecx
        push    ebx

        mov     ebx, x_right[ebp]       ; fixed point
        mov     ecx, x_left[ebp]        ; int

        sar     ebx, 16                 ; convert to int
        mov     edi, p_dest[ebp]        ; beginning of canvas row

        mov     esi, v[ebp]


;ebx = x_right
;ecx = x_left
;edx = num scanlines remaining
;esi = v
;edi = p_dest
;ebp = triangle_params

raster_loop:
        push    edx                     ; number of scanlines remaining

        sub     ecx, ebx                ; ecx = -num pixels in scanline
        jge     skip_pixel_loop

        mov     eax, u[ebp]
        add     edi, ebx                ; point to end of scanline

        mov     ebx, eax                ; two copies of u
        mov     edx, esi                ; two copies of v

;eax = u_temp, al = pixel
;ebx = u
;ecx = - count
;edx = v
;esi = v_temp
;edi = p_dest
;ebp = triangle_params

pixel_loop:
        shr     eax, 16                 ; convert u_temp to int
        and     esi, 0deadbeefh         ; mask v_temp
mask_v:

        shr     esi, 0ffh               ; set up for read from texture
shift_v:
        and     eax, 0deadbeefh         ; mask u_temp
mask_u:

        add     ebx, dux[ebp]           ; increment u
        add     edx, dvx[ebp]           ; increment v  (note: 2 cycles on p5; 1 on p5c)

        mov     al, 0deadbeefh[esi+eax] ; read pixel from texture
p_src:
        mov     esi, edx                ; copy v to v_temp

        mov     [edi+ecx], al           ; write pixel
        mov     eax, ebx                ; copy u to u_temp

        inc     ecx                     ; increment count
        jnz     pixel_loop              ; continue until done

        ; update u, v, x_left (int), x_frac (fraction for x_left), x_right (fix), and p_dest.
        ; maintain sub-pixel precision.

skip_pixel_loop:
        mov     eax, dx_frac[ebp]
        mov     edx, x_frac[ebp]

        add     edx, eax
        mov     eax, dx_left[ebp]

        sbb     ebx, ebx                ; -1 if there was a carry
        mov     ecx, x_left[ebp]

        mov     x_frac[ebp], edx
        add     ecx, eax                ; update x_left
     
        sub     ecx, ebx                ; adjust x_left for carry
        mov     edx, u[ebp]

        mov     x_left[ebp], ecx
        mov     eax, du[ebp]

        add     edx, eax
        mov     eax, dux[ebp]

        and     eax, ebx                ; adjust du for carry
        mov     esi, v[ebp]

        add     edx, eax                ; adjust u for carry
        mov     eax, dv[ebp]

        mov     u[ebp], edx
        add     esi, eax

        mov     eax, dvx[ebp]
        mov     edx, dx_right[ebp]

        and     eax, ebx                ; adjust dv for carry
        mov     ebx, x_right[ebp]

        add     esi, eax                ; adjust v for carry
        add     ebx, edx

        mov     x_right[ebp], ebx
        mov     edi, p_dest[ebp]

        sar     ebx, 16
        mov     eax, row[ebp]           ; canvas row, that is

        add     edi, eax
        mov     v[ebp], esi

        mov     p_dest[ebp], edi
        pop     edx

        dec     edx
        jnz     raster_loop

        pop     ebx
        pop     ecx

        pop     esi
        pop     edi

        pop     ebp
        ret

;takes eax = bitmap
_FUNCDEF opaque_8to8_il_init, 1
        push    esi
        push    edi

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
        mov     esi, last_mask_u
        mov     edi, last_mask_v

        cmp     esi, eax
        jz      skip_mask_u

        mov     d ds:[mask_u-4], eax
        mov     last_mask_u, eax
skip_mask_u:
        cmp     edi, edx
        jz      skip_mask_v

        mov     d ds:[mask_v-4], edx
        mov     last_mask_v, edx
skip_mask_v:

        mov     eax, last_p_src
        mov     dl, last_shift_v

        cmp     eax, ebx
        jz      skip_p_src

        mov     d ds:[p_src-4], ebx
        mov     last_p_src, ebx
skip_p_src:
        cmp     dl, cl
        jz      skip_shift_v

        mov     b ds:[shift_v-1], cl
        mov     last_shift_v, cl
skip_shift_v:
        pop     edi
        pop     esi

        ret

_TEXT ends
end
