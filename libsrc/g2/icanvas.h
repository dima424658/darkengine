/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/icanvas.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:03:08 $
 *
 * Symbolic constants for function table references.
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __ICANVAS_H
#define __ICANVAS_H
/* here are the indices for all the indirected driver functions. */
enum {
   G2C_ULINE,
   G2C_ULINE_EXPOSE,
   G2C_USLINE,
   G2C_USLINE_EXPOSE,
   G2C_UCLINE,
   G2C_UCLINE_EXPOSE,

   G2C_USCALE,
   G2C_USCALE_EXPOSE,
   G2C_SCALE,
   G2C_SCALE_EXPOSE,

   G2C_UPOLY,
   G2C_UPOLY_EXPOSE,
   G2C_USPOLY,
   G2C_USPOLY_EXPOSE,
   G2C_UCPOLY,
   G2C_UCPOLY_EXPOSE,

   G2C_ULMAP,
   G2C_ULMAP_EXPOSE,
   G2C_UWMAP,
   G2C_UWMAP_EXPOSE,
   G2C_UFMAP,
   G2C_UFMAP_EXPOSE,
   G2C_UPMAP,
   G2C_UPMAP_EXPOSE,

   G2C_LIT_ULMAP,
   G2C_LIT_ULMAP_EXPOSE,
   G2C_LIT_UWMAP,
   G2C_LIT_UWMAP_EXPOSE,
   G2C_LIT_UFMAP,
   G2C_LIT_UFMAP_EXPOSE,
   G2C_LIT_UPMAP,
   G2C_LIT_UPMAP_EXPOSE,

   G2C_TYPES
};
#endif /* !__ICANVAS_H */
