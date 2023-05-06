
;this file was translated from 2d.h with h2inc, which choked, then
;fixed up by hand. eventually we'll need to figure out how to make
;a real 2d.inc

option expr32

IFNDEF	_2d_inc
_2d_inc	EQU		1

	include	grs.inc

CLIP_NONE    = 0
CLIP_LEFT    = 1
CLIP_TOP     = 2
CLIP_RIGHT   = 4
CLIP_BOT     = 8
CLIP_ALL     = 16

TMF_PER                 equ     1
TMF_CLUT                equ     2
TMF_FLOOR               equ     4
TMF_WALL                equ     4

FILL_NORM               equ     0
FILL_CLUT               equ     1
FILL_XOR                equ     2
FILL_BLEND              equ     3
FILL_SOLID              equ     4

GR_UNPACK_RSD8_OK       equ     0

GRS_SCREEN_LTAB         equ     32

LINE		= 0
ILINE		= 1
HLINE		= 2
VLINE		= 3
SLINE		= 4
CLINE		= 5
GR_WIRE_POLY_LINE       = 6
GR_WIRE_POLY_SLINE      = 7
GR_WIRE_POLY_CLINE      = 8
GRD_LINE_TYPES	= 9
 
; Begin of file 2d.h
	INCLUDE	fix.inc

grs_rgb		TYPEDEF		ulong

;;@@vertex		STRUCT ;2t
;;@@_x		REAL4		?
;;@@_y		REAL4		?
;;@@_z		REAL4		?
;;@@_xs		REAL4		?
;;@@_ys		REAL4		?
;;@@_u		REAL4		?
;;@@_v		REAL4		?
;;@@_l		REAL4		?
;;@@_w		REAL4		?
;;@@x		REAL4		?
;;@@y		REAL4		?
;;@@z		REAL4		?
;;@@w		REAL4		?
;;@@xs		REAL4		?
;;@@ys		REAL4		?
;;@@u		REAL4		?
;;@@v		REAL4		?
;;@@clip_flags		uchar		?
;;@@vertex		ENDS
;;@@
;;@@grs_tmap_vertex		TYPEDEF		vertex

@tag__2d0		STRUCT ;2t
_x		REAL8		?
_y		REAL8		?
_z		REAL8		?
@tag__2d0		ENDS

grs_vector		TYPEDEF		@tag__2d0

@t__2d1		TYPEDEF		PTR uchar
@tag__2d1		STRUCT ;2t
bits		@t__2d1		?
bmtype		uchar		?
bmflags		ushort		?
_align		uchar		?
w		SWORD		?
h		SWORD		?
row		ushort		?
@tag__2d1		ENDS

;grs_bitmap		TYPEDEF		@tag__2d1

grs_bitmap		STRUCT ;2t
bits		@t__2d1		?
bmtype		uchar		?
flags		ushort		?
bm_align		uchar		?
bm_w		SWORD		?
bm_h		SWORD		?
row		ushort		?
pad		ushort	?
grs_bitmap		ENDS


@t__2d2		TYPEDEF		PTR uchar
@t__2d3		TYPEDEF		PTR _lrpp
_lrpp		STRUCT ;2t
l		SWORD		?
r		SWORD		?
pp		@t__2d2		?
n		@t__2d3		?
_lrpp		ENDS

grs_span		TYPEDEF		_lrpp

@t__2d4		TYPEDEF		PTR _sten
_sten		STRUCT ;2t
l		SWORD		?
r		SWORD		?
n		@t__2d4		?
_sten		ENDS

grs_sten_elem		TYPEDEF		_sten

@t__2d5		TYPEDEF		PTR grs_sten_elem
@tag__2d2		STRUCT ;2t
elem		@t__2d5		?
flags		SDWORD		?
@tag__2d2		ENDS

grs_stencil		TYPEDEF		@tag__2d2

@t__2d6		TYPEDEF		PTR grs_stencil
@tag__2d4		STRUCT ;2t
sten		@t__2d6		?
left		fix		?
top		fix		?
right		fix		?
bot		fix		?
@tag__2d4		ENDS

@t__2d7		TYPEDEF		PTR grs_stencil
@tag__2d5		STRUCT ;2t
sten		@t__2d7		?
pad0		SWORD		?
left		SWORD		?
pad1		SWORD		?
top		SWORD		?
pad2		SWORD		?
right		SWORD		?
pad3		SWORD		?
bot		SWORD		?
@tag__2d5		ENDS

@tag__2d3		UNION
f		@tag__2d4		<>
i		@tag__2d5		<>
@tag__2d3		ENDS

grs_clip		TYPEDEF		@tag__2d3

@tag__2d6		STRUCT ;2t
fcolor		SDWORD		?
bcolor		SDWORD		?
font		SDWORD		?
text_attr		SDWORD		?
fill_type		SDWORD		?
fill_parm		SDWORD		?
clip		grs_clip		<>
@tag__2d6		ENDS

;grs_context		TYPEDEF		@tag__2d6

grs_context		STRUCT ;2t
fcolor		SDWORD		?
bcolor		SDWORD		?
font		SDWORD		?
text_attr		SDWORD		?
fill_type		SDWORD		?
fill_parm		SDWORD		?
clip		grs_clip		<>
grs_context		ends


@t__2d8		TYPEDEF		PTR PTR uchar
@tag__2d7		STRUCT ;2t
bm		grs_bitmap		<>
gc		grs_context		<>
ytab		@t__2d8		?
@tag__2d7		ENDS

;grs_canvas		TYPEDEF		@tag__2d7

grs_canvas		STRUCT ;2t
canv_bm		grs_bitmap		<>
canv_gc		grs_context		<>
ytab		@t__2d8		?
grs_canvas		ENDS

@t__2d9		TYPEDEF		PTR grs_canvas
@t__2d10		TYPEDEF		PTR uchar
@t__2d11		TYPEDEF		PTR grs_rgb
@t__2d12		TYPEDEF		PTR uchar
@tag__2d8		STRUCT ;2t
bm		grs_bitmap		<>
canv		@t__2d9		?
pal		@t__2d10		?
bpal		@t__2d11		?
ipal		@t__2d12		?
_x		SWORD		?
_y		SWORD		?
@tag__2d8		ENDS

grs_screen		TYPEDEF		@tag__2d8

@t__2d13		TYPEDEF		PTR uchar
@tag__2d9		STRUCT ;2t
rec@@tag__2d9_0		RECORD	@_2d2@@tag__2d9:12,
			speed@@tag__2d9:2,
			vblank@@tag__2d9:1,
			cursor@@tag__2d9:1
@bit_0		rec@@tag__2d9_0	<>
aspect		SDWORD		?
w		SWORD		?
h		SWORD		?
vbase		@t__2d13		?
@tag__2d9		ENDS

grs_drvcap		TYPEDEF		@tag__2d9

grs_tmap_info           STRUCT
ti_tmap_type     ushort          ?
ti_flags         ushort          ?
ti_clut          @t__2d1         ?       ;(uchar *), that is        
grs_tmap_info           ENDS

;;	INCLUDE	stdarg.inc

	INCLUDE	icanvas.inc
	INCLUDE	idevice.inc
        INCLUDE ifcn.inc
        INCLUDE bitmap.inc

EXTERNDEF		C grd_id:uchar

EXTERNDEF		C grd_screen:PTR grs_screen

EXTERNDEF               C grd_pal:PTR uchar

EXTERNDEF               C grd_bpal:PTR uchar

EXTERNDEF               C grd_ipal:PTR uchar

EXTERNDEF		C grd_scr_canv:PTR grs_canvas

EXTERNDEF		C grd_vis_canv:PTR grs_canvas

EXTERNDEF		C grd_canvas:PTR grs_canvas

EXTERNDEF		C grd_bank:uchar

EXTERNDEF		C grd_cap:PTR grs_drvcap

EXTERNDEF		C grd_device_func:PTR PTR 

EXTERNDEF		C grd_span_func:PTR PTR 

EXTERNDEF		C grd_driver_list:PTR PTR 

EXTERNDEF		C grd_driver_span_list:PTR PTR 

EXTERNDEF		C grd_pixel_table:PTR PTR 
EXTERNDEF		C grd_device_table:PTR PTR 
EXTERNDEF		C grd_canvas_table:PTR PTR 
EXTERNDEF               C grd_function_fill_table:PTR PTR
EXTERNDEF               C grd_function_table:PTR PTR
EXTERNDEF               C grd_uline_fill_table:PTR PTR
EXTERNDEF               C grd_uline_fill_vector:PTR PTR
EXTERNDEF 		C grd_line_clip_fill_vector:PTR PTR

EXTERNDEF               C gr_clut_lit_tol:fix
EXTERNDEF               C flat8_per_ltol:ubyte
EXTERNDEF               C grd_unpack_buf:fix

        extn    gr_rsd8_convert_
        extn    h_umap_
        extn    v_umap_
        extn    per_umap_
        extn    h_map_
        extn    v_map_
        extn    per_map_

;calls a clipped 2d function. the first parameter, findex, is the only 
;one required.  The second, scratch, tells the macro what register
;to use for the function table ptr.  The third, functable, tells the
;macro that the ptr is already in the specified register.  Note that 
;specifying both scratch and functable is invalid.

gr_call	macro	findex,scratch,functable
	local	ftable

	ifnb	<functable>
	 ifnb	<scratch>
	  echo	Error: gr_call macro: invalid to specify both 'scratch' and 'functable'
	  .err
	 endif
	 ftable equ functable

	else

	 ifnb	<scratch>
	  ftable equ scratch
	 else
	  ftable equ esi
	 endif

	 mov	ftable,grd_canvas_table

	endif

	call	dword ptr [ftable+findex*4]

	endm

;sets the color of the current canvas. scratch is optional; esi is default
gr_setcol	macro	color,scratch:=<esi>
	local	canv

	mov	scratch,grd_canvas
	mov	[scratch].canv_gc.fcolor,color

	endm

gr_getcol macro reg,scratch:=<esi>
        mov     scratch,grd_canvas
        mov     reg,[scratch].canv_gc.fcolor
        endm

gr_getfp macro reg,scratch:=<esi>
        mov     scratch,grd_canvas
        mov     reg,[scratch].canv_gc.fill_parm
        endm

gr_setfp macro reg,scratch:=<esi>
        mov     scratch,grd_canvas
        mov     [scratch].canv_gc.fill_parm,reg
        endm

gr_getft macro reg,scratch:=<esi>
        mov     scratch,grd_canvas
        mov     reg,[scratch].canv_gc.fill_type
        endm

; destroys eax,edx,
gr_setft macro ftype
        mov     edx,ftype
        push    edx
        mov     eax,grd_canvas
        mov     [eax].canv_gc.fill_type,edx
        mov     eax,4*GRD_FUNCS*REAL_BMT_TYPES
        mul     edx
        add     eax,[grd_function_fill_table]
        mov     grd_function_table,eax
        pop     edx
        mov     eax,4*GRD_LINE_TYPES
        mul     edx
        add     eax,[grd_uline_fill_table]
        mov     grd_uline_fill_vector,eax

        endm

; destroys eax
gr_setft_clut macro
        mov     eax,grd_canvas
        mov     [eax].canv_gc.fill_type,1
        mov     eax,4*GRD_FUNCS*REAL_BMT_TYPES
        add     eax,[grd_function_fill_table]
        mov     grd_function_table,eax
        mov     eax,4*GRD_LINE_TYPES
        add     eax,[grd_uline_fill_table]
        mov     grd_uline_fill_vector,eax
        endm

; destroys eax 
gr_setft_norm macro 
        mov     eax,grd_canvas
        mov     [eax].canv_gc.fill_type,0
        mov     eax,[grd_function_fill_table]
        mov     grd_function_table,eax
        mov     eax,[grd_uline_fill_table]
        mov     grd_uline_fill_vector,eax

        endm



; split rgb value (in reg) and push onto stack, 
; scratch is optional; esi is default
gr_splitrgb macro rgb,scratch:=<esi>
	mov	scratch,rgb
	shr	scratch,24
	and	scratch,00ffh
	push	scratch
	mov	scratch,rgb
	shr	scratch,13
	and	scratch,00ffh
	push	scratch
	mov	scratch,rgb
	shr	scratch,2
	and	scratch,00ffh
	push	scratch
	endm
gr_bind_brgb macro rgb,index,scratch:=<esi>
        mov     index,rgb
        shr     index,5
        and     index,01fh
        mov     scratch,rgb
        shr     scratch,11
        and     scratch,03e0h
        or      index,scratch
        mov     scratch,rgb
        shr     scratch,17
        and     scratch,07c00h
        or      index,scratch
        endm
ENDIF

