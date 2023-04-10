// $Header: x:/prj/tech/libsrc/lgd3d/RCS/tdrv.h 1.8 1998/03/06 13:02:31 KEVIN Exp $
#ifndef __TDRV_H
#define __TDRV_H

typedef struct grs_bitmap grs_bitmap;

typedef struct tdrv_texture_info {
   grs_bitmap *bm;
   int id;
   int flags;
   int scale_w;
   int scale_h;
   int w, h;
   int size;
   uchar *bits;
   ulong cookie;
} tdrv_texture_info;


typedef struct texture_driver {
   int (*load_texture)(tdrv_texture_info *info); 
   void (*release_texture)(int n);
   void (*set_texture_id)(int n);
   void (*unload_texture)(int n);      // disconnect texture from bitmap
   void (*synchronize)(void);
   void (*start_frame)(int n);
   void (*end_frame)(void);
   void (*reload_texture)(tdrv_texture_info *info);
   void (*cook_info)(tdrv_texture_info *info);
} texture_driver;

#define TDRV_FAILURE -1
#define TDRV_SUCCESS 0

#define TDRV_ID_SOLID     -1
#define TDRV_ID_CALLBACK  -2
#define TDRV_ID_INVALID   -3

// texture flags
#define TF_ALPHA 1
#define TF_RGB 2
#define TF_TRANS 4

#endif
