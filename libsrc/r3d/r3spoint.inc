; // The big guy himself.  This may change over time
; typedef struct _r3s_point {
;    mxs_vector p;
;    ulong ccodes;
;    fix sx, sy;
;    mxs_real w,i,u,v;
; } r3s_point;

R3S_POINT_X       equ 0
R3S_POINT_Y       equ 4
R3S_POINT_Z       equ 8
R3S_POINT_CCODES  equ 12
R3S_POINT_SX      equ 16
R3S_POINT_SY      equ 20
R3S_POINT_W       equ 24
R3S_POINT_I       equ 28
R3S_POINT_U       equ 32
R3S_POINT_V       equ 36
R3S_POINT_SIZEOF  equ 40

; values in CCODES

R3C_OFF_LEFT    equ 1
R3C_OFF_RIGHT   equ 2
R3C_OFF_TOP     equ 4
R3C_OFF_BOTTOM  equ 8
R3C_BEHIND      equ 16
