; $Header: x:/prj/tech/libsrc/g2/RCS/plyparam.inc 1.3 1998/04/28 14:31:57 KEVIN Exp $
;
; mirror plyparam.h

ifndef plyparam_inc
plyparam_inc equ 1

G2S_POLY_PARAMS_RASTER_LIST     equ      0t
G2S_POLY_PARAMS_RASTER_ROW      equ      4t
G2S_POLY_PARAMS_P               equ      8t
G2S_POLY_PARAMS_CANVAS_ROW      equ     12t
G2S_POLY_PARAMS_FLAGS           equ     16t
G2S_POLY_PARAMS_BM              equ     20t
G2S_POLY_PARAMS_PIX_FUNC        equ     24t
G2S_POLY_PARAMS_LPIX_FUNC       equ     28t

G2S_POLY_PARAMS_DIX             equ     32t
G2S_POLY_PARAMS_DUX             equ     36t
G2S_POLY_PARAMS_DVX             equ     40t
G2S_POLY_PARAMS_DHX             equ     44t
G2S_POLY_PARAMS_DDX             equ     48t

G2S_POLY_PARAMS_DIY             equ     64t
G2S_POLY_PARAMS_DUY             equ     68t
G2S_POLY_PARAMS_DVY             equ     72t
G2S_POLY_PARAMS_DHY             equ     76t
G2S_POLY_PARAMS_DDY             equ     80t

G2S_POLY_PARAMS_I_SCALE         equ     96t
G2S_POLY_PARAMS_U_SCALE         equ     100t   
G2S_POLY_PARAMS_V_SCALE         equ     104t
G2S_POLY_PARAMS_H_SCALE         equ     108t
G2S_POLY_PARAMS_D_SCALE         equ     112t

G2S_POLY_PARAMS_LEFT_X          equ     128t
G2S_POLY_PARAMS_LEFT_DX         equ     132t
G2S_POLY_PARAMS_LEFT_X_FRAC     equ     136t
G2S_POLY_PARAMS_LEFT_DX_FRAC    equ     140t

G2S_POLY_PARAMS_LEFT_I          equ     144t
G2S_POLY_PARAMS_LEFT_U          equ     148t
G2S_POLY_PARAMS_LEFT_V          equ     152t
G2S_POLY_PARAMS_LEFT_H          equ     156t
G2S_POLY_PARAMS_LEFT_D          equ     160t

G2S_POLY_PARAMS_LEFT_DI         equ     176t
G2S_POLY_PARAMS_LEFT_DU         equ     180t
G2S_POLY_PARAMS_LEFT_DV         equ     184t
G2S_POLY_PARAMS_LEFT_DH         equ     188t
G2S_POLY_PARAMS_LEFT_DD         equ     192t

G2S_POLY_PARAMS_RIGHT_X         equ     208t
G2S_POLY_PARAMS_RIGHT_DX        equ     212t

G2S_POLY_PARAMS_INNER_LOOP      equ     216t
G2S_POLY_PARAMS_RASTER_FUNC     equ     220t
G2S_POLY_PARAMS_LE_FUNC         equ     224t
G2S_POLY_PARAMS_RE_FUNC         equ     228t
G2S_POLY_PARAMS_POLY_FUNC       equ     232t
G2S_POLY_PARAMS_GRAD_FUNC       equ     236t
G2S_POLY_PARAMS_CLIP_TYPE       equ     240t
G2S_POLY_PARAMS_YTOP            equ     244t
G2S_POLY_PARAMS_YRIGHT          equ     248t
G2S_POLY_PARAMS_YLEFT           equ     252t
G2S_POLY_PARAMS_SIZE            equ     256t
endif
