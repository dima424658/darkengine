/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/lgd3d/RCS/tmgr.h 1.18 1998/09/18 16:13:05 KEVIN Exp $
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

#define TF_ALPHA 1
#define TF_RGB 2
#define TF_TRANS 4


typedef struct texture_manager {
   int (*init)(grs_bitmap *bm, int max_textures, int *texture_size_list, int num_texture_sizes, int flags);
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

typedef struct tmap_chain
{
	int head;
} tmap_chain;

typedef struct tmgr_texture_info
{
	grs_bitmap* bitmap;
	uchar* bits;
	int frame;
	uchar* clut;
	int next;
	int size_index;
	uint cookie;
} tmgr_texture_info;


//zb
extern texture_manager *get_dopey_texture_manager( void* driver );


extern texture_manager        *g_tmgr;       // this is kinda silly, but it'll do for now...

#ifdef __cplusplus
}
#endif
#endif
