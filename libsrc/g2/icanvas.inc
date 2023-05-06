;
; $Source: s:/prj/tech/libsrc/g2/RCS/icanvas.inc $
; $Revision: 1.1 $
; $Author: KEVIN $
; $Date: 1996/04/10 17:48:23 $
;
; Symbolic constants for function table references.
;
; This file is part of the g2 library.
;

ifndef icanvas_inc
icanvas_inc equ 1

;here are the indices for all the indirected driver functions. 
;first are the analytic primitive functions. 
G2C_ULINE               equ     0t 
G2C_ULINE_EXPOSE        equ     1t
G2C_USLINE              equ     2t
G2C_USLINE_EXPOSE       equ     3t
G2C_UCLINE              equ     4t 
G2C_UCLINE_EXPOSE       equ     5t
G2C_USCALE              equ     6t
G2C_USCALE_EXPOSE       equ     7t
G2C_SCALE               equ     8t 
G2C_SCALE_EXPOSE        equ     9t
G2C_UPOLY               equ     10t
G2C_UPOLY_EXPOSE        equ     11t
G2C_USPOLY              equ     12t
G2C_USPOLY_EXPOSE       equ     13t
G2C_UCPOLY              equ     14t
G2C_UCPOLY_EXPOSE       equ     15t
G2C_ULMAP               equ     16t
G2C_ULMAP_EXPOSE        equ     17t
G2C_UWMAP               equ     18t
G2C_UWMAP_EXPOSE        equ     19t
G2C_UFMAP               equ     20t
G2C_UFMAP_EXPOSE        equ     21t
G2C_UPMAP               equ     22t
G2C_UPMAP_EXPOSE        equ     23t
G2C_LIT_ULMAP           equ     24t
G2C_LIT_ULMAP_EXPOSE    equ     25t
G2C_LIT_UWMAP           equ     26t
G2C_LIT_UWMAP_EXPOSE    equ     27t
G2C_LIT_UFMAP           equ     28t
G2C_LIT_UFMAP_EXPOSE    equ     29t
G2C_LIT_UPMAP           equ     30t
G2C_LIT_UPMAP_EXPOSE    equ     31t
G2C_TYPES               equ     32t
endif   ;ndef icanvas_inc
                        
                        
                        
                        
