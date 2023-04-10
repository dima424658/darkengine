// $Header: x:/prj/tech/libsrc/lgd3d/RCS/lgd3d.h 1.35 1998/07/06 15:32:28 buzzard Exp $

#ifndef __LGD3D_H
#define __LGD3D_H

#include <r3ds.h>
#include <g2.h>
#include <tmgr.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _D3DDeviceDesc DevDesc;

typedef struct lgd3ds_device_info {
   GUID     device_guid;
   GUID *   p_ddraw_guid;
   DevDesc *device_desc;
   short *  supported_modes; // -1 terminated list 
   char *   p_ddraw_desc;
   ulong    flags;
} lgd3ds_device_info;

#define LGD3DF_ZBUFFER  1
#define LGD3DF_SPEW     2

// pre-initiazation calls
extern void lgd3d_set_RGB(void);
extern void lgd3d_set_hardware(void);
extern void lgd3d_set_software(void);
extern void lgd3d_texture_set_RGB(bool is_RGB);

// state query functions (won't change between init and shutdown)
extern BOOL lgd3d_is_RGB(void);
extern BOOL lgd3d_is_hardware(void);

// initialization and shutdown
extern void lgd3d_init(lgd3ds_device_info *device_info);
extern void lgd3d_shutdown(void);

extern void lgd3d_start_frame(int frame);  // frame count passed to texture manager
extern void lgd3d_end_frame(void);

// r3d interface
extern int (*lgd3d_draw_poly_func)(int n, r3s_phandle *pl);
extern void lgd3d_tmap_setup(grs_bitmap *bm);
extern void lgd3d_lit_tmap_setup(grs_bitmap *bm);
extern void lgd3d_rgblit_tmap_setup(grs_bitmap *bm);
extern void lgd3d_poly_setup(grs_bitmap *bm);
extern void lgd3d_spoly_setup(grs_bitmap *bm);
extern void lgd3d_rgb_poly_setup(grs_bitmap *bm);
extern int lgd3d_draw_point(r3s_point *p);

// application direct interface
extern void lgd3d_set_zwrite(BOOL zwrite);
extern void lgd3d_set_zcompare(BOOL zwrite);
extern void lgd3d_zclear(void);


#define lgd3d_set_texture(bm) \
do if (g_tmgr) g_tmgr->set_texture(bm); while (0)
#define lgd3d_load_texture(bm) \
do if (g_tmgr) g_tmgr->load_texture(bm); while (0)
#define lgd3d_unload_texture(bm) \
do if (g_tmgr) g_tmgr->unload_texture(bm); while (0)
extern void lgd3d_set_pal(uint start, uint n, uchar *pal);
extern void lgd3d_set_pal_slot(uint start, uint n, uchar *pal, int slot);
extern void lgd3d_get_texture_bitmask(grs_rgb_bitmask *bitmask);
extern void lgd3d_set_texture_clut(uchar *clut);
extern uchar *lgd3d_set_clut(uchar *clut);
extern void lgd3d_set_fog_level(float fog_level);     // 0.0 <= fog_level <= 1.0
extern void lgd3d_set_fog_color(int r, int g, int b); // 0..255
extern void lgd3d_set_fog_enable(BOOL enable);
extern void lgd3d_set_fog_density(float density);
extern void lgd3d_set_alpha(float alpha);             // 0.0 <= alpha <= 1.0
extern void lgd3d_set_blend(BOOL do_blend);
extern void lgd3d_blend_multiply(int blend_mode);
enum {
   BLEND_DEST_ZERO=0,
   BLEND_SRC_ZERO=0,
   BLEND_SRC_ONE=1,
   BLEND_SRC_SRC=2,
   BLEND_SRC_DEST=3,
   BLEND_DEST_ONE=4,
   BLEND_DEST_SRC=8,
   BLEND_DEST_DEST=12
};
extern void lgd3d_blend_normal(void);
extern void lgd3d_set_offsets(int x, int y);
extern void lgd3d_set_z(float z);                     // z coord for flat (2d) polys
extern void lgd3d_disable_palette(void);
extern void lgd3d_enable_palette(void);
extern void lgd3d_hack_light(r3s_point *p, float r);
extern void lgd3d_hack_light_extra(r3s_point *p, float r, grs_bitmap *bm);
extern void lgd3d_draw_line(r3s_point *p0, r3s_point *p1);
extern int lgd3d_trifan(int n, r3s_point **vpl);
extern int lgd3d_lit_trifan(int n, r3s_point **vpl);
extern int lgd3d_poly(int n, r3s_point **vpl);
extern int lgd3d_spoly(int n, r3s_point **vpl);
extern int lgd3d_g2upoly(int n, g2s_point **vpl);
extern int lgd3d_g2poly(int n, g2s_point **vpl);
extern int lgd3d_g2utrifan(int n, g2s_point **vpl);
extern int lgd3d_g2trifan(int n, g2s_point **vpl);

// push all points closer to camera by zbias.
// returns old zbias.
extern double lgd3d_set_zbias(double zbias);

extern int lgd3d_enumerate_devices(void);
extern lgd3ds_device_info *lgd3d_get_device_info(int device);

// miscellaneous

// color range is 0..255
extern void lgd3d_set_chromakey(int red, int green, int blue);
extern void lgd3d_set_alpha_pal(ushort *pal);
extern void lgd3d_blit();
extern void lgd3d_clear(int color_index);

#ifdef __cplusplus
}
#endif

#endif



