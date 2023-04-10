// $Header: x:/prj/tech/libsrc/lgd3d/RCS/tmgr.h 1.16 1998/07/02 11:48:38 PATMAC Exp $
#ifndef __TMGR_H
#define __TMGR_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct grs_bitmap grs_bitmap;

#define TMGR_SUCCESS 0
#define TMGR_FAILURE -1
#define BMF_LOADED 0x40

#define TMGRF_SPEW 1

typedef struct texture_manager {
   int (*init)(grs_bitmap *bm, int max_textures, int flags);
   void (*shutdown)(void);
   void (*start_frame)(int frame);
   void (*end_frame)(void);
   void (*load_texture)(grs_bitmap *bm);
   void (*unload_texture)(grs_bitmap *bm);
   void (*set_texture)(grs_bitmap *bm);
   void (*set_texture_callback)(void);
   void (*stats)(void);                      // spew stats to the mono screen
   uint (*bytes_loaded)(void);               // bytes downloaded this frame
   BOOL (*get_utilization)(float *utilization);
   uchar *(*set_clut)(uchar *clut);
   void (*restore_bits)(grs_bitmap *bm);
   void (*reload_texture)(grs_bitmap *bm);
} texture_manager;

typedef struct texture_driver texture_driver;

extern texture_manager *get_block_texture_manager(texture_driver *driver);
extern texture_manager *get_dopey_texture_manager(texture_driver *driver);

extern texture_manager        *g_tmgr;       // this is kinda silly, but it'll do for now...

#ifdef __cplusplus
}
#endif
#endif
