;
; $Source: x:/prj/tech/libsrc/3d/RCS/3d.inc $
; $Revision: 1.55 $
; $Author: JAEMZ $
; $Date: 1996/08/22 13:20:38 $
;
; 3d Header file, with structures and functions
;
 

include macros.inc
include fix.inc
include 2d.inc

;types

sfix    TYPEDEF SWORD

;constants


;these are for rotation order when generating a matrix from angles
;bit 2 means y before z if 0, 1 means x before z if 0, 0 means x before y if 0

ORDER_XYZ = 0   ;000
ORDER_YXZ = 1   ;001
ORDER_BAD = 2   ;010 impossible
ORDER_YZX = 3   ;011
ORDER_XZY = 4   ;100
ORDER_BAD = 5   ;101 impossible
ORDER_ZXY = 6   ;110
ORDER_ZYX = 7   ;111

;To specify user's coordinate system: use one of these for each user_x_axis,
;user_y_axis,& user_z_axis in g3_init to tell the 3d what your x,y,& z mean.

AXIS_RIGHT =  1
AXIS_UP    =  2
AXIS_IN    =  3
AXIS_LEFT  = -AXIS_RIGHT
AXIS_DOWN  = -AXIS_UP
AXIS_OUT   = -AXIS_IN

;structures

;vector structure

g3s_vector      STRUCT 2t
x       fix     ?
y       fix     ?
z       fix     ?
g3s_vector      ENDS

;angle vector structure

g3s_angvec      STRUCT 2t
tx      fixang  ?       ;theta x -> rotation around x axis
ty      fixang  ?
tz      fixang  ?
g3s_angvec      ENDS

;matrix

g3s_matrix      STRUCT 2t
m1      fix     ?
m2      fix     ?
m3      fix     ?
m4      fix     ?
m5      fix     ?
m6      fix     ?
m7      fix     ?
m8      fix     ?
m9      fix     ?
g3s_matrix      ENDS


;point structure

g3s_point       STRUCT 2t
	UNION
vec     g3s_vector <>
	STRUCT
x       fix     ?
y       fix     ?
z       fix     ?
	ENDS
nextp   dword   ?
	ENDS
sx      fix     ?
sy      fix     ?
codes   byte    ?
p3_flags        ubyte   ?
	UNION
	STRUCT
u       sfix    ?
v       sfix    ?
	ENDS
rgb     grs_rgb ?
	ENDS
i       sfix    ?
pad     db      (32 - $) dup (?)
g3s_point       ENDS

	ife     size g3s_point EQ 32
	 echo   g3s_point structure must be 32 bytes long!
	 .err
	endif

g3s_modinfo     STRUCT 2t
mname      db   8 dup (?)
bmax       g3s_vector  <>
bmin       g3s_vector  <>
cen        g3s_vector  <>
pcen       g3s_vector  <>
rad        fix  ?
prad       fix  ?
pgons      dw   ?
verts      dw   ?
parms      dw   ?
vcalls     db   ?
vhots      db   ?
lit        db   ?
mats       db   ?
vh_off     dd   ?
hsize      dd   ?
res1       g3s_vector <>
g3s_modinfo     ENDS
; 94 long


;flags

;point flags
PF_U            EQU             1t
PF_V            EQU             1t
; use this from now on
PF_UV           EQU             1t
PF_CODED        EQU             2t
PF_I            EQU             4t
PF_PROJECTED    EQU             8t
PF_RGB          EQU             16t
PF_CLIPPNT      equ             32t

;clip codes
CC_OFF_LEFT             EQU             1t
CC_OFF_RIGHT            EQU             2t
CC_OFF_BOT              EQU             4t
CC_OFF_TOP              EQU             8t
CC_BEHIND               EQU             128t
CC_CLIP_OVERFLOW        equ             16t
;above gets set when projection overflows. will force a 3d-clip

;lighting codes
LT_NONE                 equ             0t
LT_SOLID                equ             1t
LT_DIFF                 equ             2t
LT_SPEC                 equ             4t
LT_GOUR                 equ             128t
LT_NEAR_LIGHT           equ             16t
LT_NEAR_VIEW            equ             8t
LT_LOC_LIGHT            equ             32t

LT_TABSIZE              equ             24t
LT_BASELIT              equ             15t   

FT_LIGHT                equ     0t
FT_FLAT_LIGHT           equ     1t
FT_NO_LIGHT             equ     2t
FT_SHADOW               equ     3t
FT_SHADOWED             equ     4t
FT_WIRE                 equ     5t
FT_SOLID                equ     6t
FT_CALLBACK             equ     7t

;global vars - from globvars.asm

	extd    first_free,point_list
	extw    n_points

	extd    _scrw,_scrh,window_width,window_height,up_axis,ww2,wh2
	extd    axis_x,axis_y,axis_z,axis_x_ofs,axis_y_ofs,axis_z_ofs
	extb    axis_swap_flag,axis_neg_flag

        extb    _g3d_clipflags

	extfix  _view_zoom,view_heading,view_pitch,view_bank,_biasx,_biasy,pixel_ratio

	extdef  g3s_vector,_view_position,_matrix_scale,horizon_vector
        extdef  g3s_matrix, unscaled_matrix,_view_matrix

	extdef  sfix, gouraud_base

	extb    shift_table

        extdef g3s_vector,g3d_zero_vec

	; lighting
	extb    _g3d_light_type
	extfix  _g3d_amb_light,_g3d_diff_light,_g3d_spec_light,_g3d_flash
	extfix  _g3d_ldotv,_g3d_sdotl,_g3d_sdotv;
	extd    _g3d_light_tab
        extb    _g3d_ltab_shift
        extd    _g3d_ltab_max
        extd    _g3d_flat_mask  ; for flat shaded polys
        extb    _g3d_flat_shift


        extfix  _g3d_w_scale

	extdef g3s_vector,_g3d_light_src,_g3d_light_trans
        extdef g3s_vector,_g3d_view_vec,_g3d_light_vec
        extdef g3s_vector,_g3d_o2w_pos
        extdef g3s_matrix,_g3d_o2w_mat

	;ifdef   stereo_on
	extfix  _g3d_eyesep_raw, _g3d_eyesep,_g3d_stereo_offset
    	extd    _g3d_stereo_base
        extd    _g3d_stereo_list
	extb    _g3d_stereo
	extd    _g3d_rt_canv
	extd    _g3d_rt_canv_bits
	extd    _g3d_lt_canv_bits
	extd    _g3d_stereo_tmp
	;endif

;global vars - from detail.c
        extd    _g3_tmap_func
        extd    _g3_lit_tmap_func


;for statistics gathering
	ifdef   dbg_on
	 extw   n_polys,n_polys_drawn,n_polys_triv_acc,n_polys_triv_rej,n_polys_clip_2d,n_polys_clip_3d
	endif

vm1 equ _view_matrix.m1
vm2 equ _view_matrix.m2
vm3 equ _view_matrix.m3
vm4 equ _view_matrix.m4
vm5 equ _view_matrix.m5
vm6 equ _view_matrix.m6
vm7 equ _view_matrix.m7
vm8 equ _view_matrix.m8
vm9 equ _view_matrix.m9

uvm1 equ unscaled_matrix.m1
uvm2 equ unscaled_matrix.m2
uvm3 equ unscaled_matrix.m3
uvm4 equ unscaled_matrix.m4
uvm5 equ unscaled_matrix.m5
uvm6 equ unscaled_matrix.m6
uvm7 equ unscaled_matrix.m7
uvm8 equ unscaled_matrix.m8                
uvm9 equ unscaled_matrix.m9

;3d macros

;gets the next available pnt in reg.  trashes ebx or specified reg
getpnt  macro   reg,scratch:=<ebx>
	local   no_point
	if      cmpstr(reg,&scratch)
%        echo   dest (reg) and temp (scratch) cannot be same in getpnt
	 .err
	endif
	mov     reg,first_free
	or      reg,reg
	break_if        z,'No free points!'
	jz      no_point
	mov     scratch,[reg].nextp
	mov     first_free,scratch
no_point:
	endm

;frees the point in the specified register. uses ebx as scratch
freepnt macro   reg
	mov     ebx,first_free
	mov     [reg].nextp,ebx
	mov     first_free,reg
	endm

;increment statistics variable

statinc macro   var
	ifdef   dbg_on
	dbg     DSRC_3D_Statistics
	 pushf
	 inc    var
	 popf
	%dbgend
	endif
	endm

;vector spew macro

spewv   macro   flag,str,v
	spew    flag,@CatStr(@SubStr(&str,1,@SizeStr(&str)-1),< !%lx !%lx !%lx\n!">),dword ptr v,dword ptr v+4,dword ptr v+8
	endm

;macro to generate a spew macro
spewgen macro   id,flag

id&spew macro   str,parms:vararg
	spew    flag,str,parms
	endm

	endm

;macro to generate a spewv macro
spewvgen        macro   id,flag

id&spewv        macro   str,parms:vararg
	spewv   flag,str,parms
	endm

	endm

;dbg macro fool
;endm
;
;dbgs macro fool
;endm
;
;enddbg macro fool
;endm
;
;dbgsend macro fool
;endm
;
;dbgend macro fol
;endm

;now generate a spew macro for each flag

	spewgen x,DSRC_3D_Xforms        ;xforms
	spewgen p,DSRC_3D_Project       ;projection
	spewgen m,DSRC_3D_Matrix        ;matrix
	spewgen a,DSRC_3D_Pnt_Alloc     ;point allocation
	spewgen s,DSRC_3D_Setup ;setup
	spewgen pg,DSRC_3D_Polygons     ;polygons
	spewgen c,DSRC_3D_Clipping      ;clipping
	spewgen t,DSRC_3D_TextureMaps   ;texture maps
	spewgen st,DSRC_3D_Statistics

	spewvgen        t,DSRC_3D_TextureMaps


dump_matrix     macro   flag,m
	spew    flag,"%8lx %8lx %8lx\n",dword ptr m,    dword ptr m+4,  dword ptr m+8
	spew    flag,"%8lx %8lx %8lx\n",dword ptr m+12, dword ptr m+16, dword ptr m+20
	spew    flag,"%8lx %8lx %8lx\n",dword ptr m+24, dword ptr m+28, dword ptr m+32
	endm

;public functions

ifdef stereo_on
;from stereo.asm
	extn    g3_init_stereo
        extn    g3_set_eyesep,g3_set_stoffset
	extn    g3_start_stereo_frame
endif

;from light.asm
	extn    g3_light_diff,g3_light_spec,g3_light_dands
	extn    g3_light,g3_light_list
	extn    g3_set_light_src
	extn    g3_eval_vec_light,g3_trans_loc_light
        extn    g3_eval_loc_light
	extn    g3_eval_ldotv,g3_eval_view
	extn    g3_eval_light_obj_cen
        extn    g3_eval_view_ahead

;from alloc.asm
	extn    g3_init,g3_shutdown,g3_alloc_point
	extn    g3_free_point,g3_start_frame,g3_start_fake_frame,g3_end_frame
	extn    g3_free_list,g3_dup_point,g3_count_free_points
	extn    g3_copy_point,g3_alloc_list

;from vector.asm
	extn    g3_vec_dotprod,g3_vec_mag,g3_vec_normalize
	extn    g3_compute_normal_quick,g3_compute_normal
	extn    g3_vec_add,g3_vec_sub,g3_vec_scale

;from matrix.asm
	extn    g3_set_view_angles,g3_set_view_matrix
	extn    g3_transpose,g3_copy_transpose
	extn    g3_matrix_x_matrix_,g3_vec_rotate
	extn    g3_get_view_pyramid

;from points.asm
	extn    g3_transform_point,g3_transform_list
        extn    g3_rotate_o2w
        extn    g3_rotate_w2o
	extn    g3_rotate_point,g3_rotate_list
	extn    g3_rotate_grad,g3_rotate_norm
	extn    g3_project_point,g3_project_list
        extn    g3_rotate_light_norm

	extn    g3_rotate_delta_v,g3_add_delta_v,g3_copy_add_delta_v
	extn    g3_rotate_delta_x,g3_rotate_delta_y,g3_rotate_delta_z
	extn    g3_rotate_delta_xz,g3_rotate_delta_xy,g3_rotate_delta_yz
	extn    g3_rotate_delta_xyz

	extn    g3_add_delta_x,g3_add_delta_y,g3_add_delta_z
	extn    g3_add_delta_xz,g3_add_delta_xy,g3_add_delta_yz
	extn    g3_add_delta_xyz

	extn    g3_copy_add_delta_x,g3_copy_add_delta_y,g3_copy_add_delta_z
	extn    g3_copy_add_delta_xz,g3_copy_add_delta_xy,g3_copy_add_delta_yz
	extn    g3_copy_add_delta_xyz

	extn    g3_replace_add_delta_x,g3_replace_add_delta_y,g3_replace_add_delta_z

;from polygon.asm
	extn    g3_draw_line,g3_draw_point
	extn    g3_draw_sline, g3_draw_cline
	extn    g3_draw_tluc_poly,g3_draw_tluc_spoly
	extn    g3_draw_poly
	extn    g3_draw_spoly,g3_draw_cpoly
	extn    g3_check_normal_facing,g3_check_poly_facing
	extn    g3_check_and_draw_poly,g3_check_and_draw_spoly
	extn    g3_check_and_draw_cpoly
	extn    g3_check_and_draw_tluc_poly
	extn    g3_check_and_draw_tluc_spoly
	extn    g3_check_codes

;from horizon.asm
	extn    g3_draw_horizon

;from instance.asm
	extn    g3_start_o2w_,g3_start_object_angles_v,g3_start_object_matrix
	extn    g3_end_object,g3_end_o2w_,g3_start_object_angles_xyz
	extn    g3_start_object_angles_y,g3_start_object_angles_x
	extn    g3_start_object,g3_start_object_angles_z
	extn    g3_start_object_angles_xy
	extn    g3_start_object_angles_xz
	extn    g3_start_object_angles_yz
	extn    g3_scale_object
        extn    save_context

;from tmap.asm
	extn    g3_draw_tmap_quad_tile,g3_check_and_draw_tmap_quad_tile
	extn    g3_light_tmap_quad_tile,g3_check_and_light_tmap_quad_tile
	extn    g3_draw_tmap_tile,g3_light_tmap_tile
	extn    g3_draw_lmap_quad_tile,g3_check_and_draw_lmap_quad_tile
	extn    g3_light_lmap_quad_tile,g3_check_and_light_lmap_quad_tile
	extn    g3_draw_lmap_tile,g3_light_lmap_tile
	extn    g3_draw_lmap_,g3_light_lmap_
	extn    g3_draw_tmap_,g3_light_tmap_
	extn    g3_draw_floor_map_,g3_light_floor_map_
	extn    g3_draw_wall_map_,g3_light_wall_map_

;from fov.asm
	extn    g3_get_FOV,g3_get_FOV_,g3_get_zoom

;from interp.asm
	extn    g3_interpret_object
        extn    g3_interpret_vhot

;from slew.asm
	extn    g3_get_slew_step

;from bitmap.asm
	extn    g3_bitmap,g3_anchor_bitmap
	extn    g3_light_bitmap,g3_light_anchor_bitmap
	extn    g3_full_light_bitmap,g3_full_light_anchor_bitmap
	extn    g3_set_bitmap_scale

;from fill.asm
	extn    g3_push_fill,g3_pop_fill

;internal functions
        extn    rotate_vector
	extn    angles_2_matrix,code_point,g3_
	extn    g3_clip_polygon_,g3_clip_line_
        extn    g3_fix_uv_to_sfix_uv_
        extn    g3_sfix_uv_to_fix_uv_
	extn    gen_fix_line_,gen_fix_sline_,gen_fix_cline_

