/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/icanvas.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:55:10 $
 *
 * Canvas function table indexes.
 *
 * This file is part of the dev2d library.
 */

#ifndef __ICANVAS_H
#define __ICANVAS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
   GDC_UPIX,
   GDC_UPIX_EXPOSE,

   GDC_UPIX8,
   GDC_UPIX8_EXPOSE,

   GDC_UPIX16,
   GDC_UPIX16_EXPOSE,

   GDC_UPIX24,
   GDC_UPIX24_EXPOSE,

   GDC_LPIX,
   GDC_LPIX_EXPOSE,

   GDC_LPIX8,
   GDC_LPIX8_EXPOSE,

   GDC_LPIX16,
   GDC_LPIX16_EXPOSE,

   GDC_LPIX24,
   GDC_LPIX24_EXPOSE,

   GDC_UGPIX8,
   GDC_UGPIX8_EXPOSE,

   GDC_UGPIX16,
   GDC_UGPIX16_EXPOSE,

   GDC_UGPIX24,
   GDC_UGPIX24_EXPOSE,

   GDC_UBITMAP,
   GDC_UBITMAP_EXPOSE,

   GDC_UHLINE,
   GDC_UHLINE_EXPOSE,
   GDC_UVLINE,
   GDC_UVLINE_EXPOSE,

   GDC_CANVAS_FUNCS
};

#ifdef __cplusplus
};
#endif
#endif
