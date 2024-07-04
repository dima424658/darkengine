/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/lgd3d/RCS/lgd3d.h 1.59 1970/01/01 00:00:00 Zarko Exp $

#ifndef __LGD3D_H
#define __LGD3D_H


#include <g2.h>
#include <tmgr.h>

#include <r3ds.h>



#ifdef __cplusplus
extern "C" {
#endif

typedef struct ILGSurface ILGSurface;

typedef struct _D3DDeviceDesc DevDesc;

typedef struct lgd3ds_device_info {
   GUID         device_guid;
   GUID*        p_ddraw_guid;
   DevDesc*     device_desc;
   short*       supported_modes; // -1 terminated list 
   char*        p_ddraw_desc;
   ulong        flags;
} lgd3ds_device_info;


extern int lgd3d_enumerate_devices(void);
extern lgd3ds_device_info *lgd3d_get_device_info(int device);


//////////////NEW in a 50% bigger packaging!!!////////////////////////////

/// lgd3ds_device_info flags passed to the libray
/// or returned by the enumeration.  

extern int lgd3d_enumerate_devices_capable_of( ulong flags );

#define LGRT_SINGLE_TEXTURE_SINGLE_PASS     0L
#define LGRT_MULTI_TEXTURE_SINGLE_PASS      1L

//NOTE: multipass multitexturing is not supported!!!

// Capabilities that can be requested:

#define LGD3DF_SPEW                    0x00000002L

//depth buffer:
#define LGD3DF_ZBUFFER                 0x00000001L // Z-buffer 
#define LGD3DF_WBUFFER                 0x00000004L // W-buffer 
#define LGD3DF_DEPTH_BUFFER_REQUIRED   0x00000008L // the 
// selected (Z_ or W-) depth buffer REQUIRED!
//fog:
#define LGD3DF_TABLE_FOG               0x00000010L // table based pixel fog
#define LGD3DF_VERTEX_FOG              0x00000020L 

//dithering
#define LGD3DF_DITHER                  0x00000040L // use dithering 

//antialiasing
#define LGD3DF_ANTIALIAS				   0x00000080L // use (sort independent) antialiasing 

#define LGD3DF_MULTI_TEXTURING         0x00000100L // we support ONLY two sets of textures and
// and texture coordinates

#define LGD3DF_MODULATEALPHA			   0x00000200L // we want to use modulate alpha for single texture mode
#define LGD3DF_BLENDDIFFUSE				0x00000400L // we want to use blend diffuse for single texture mode

#define LGD3DF_MULTITEXTURE_COLOR		0x00000800L // we want to use color light maps for single or multi texture mode
#define LGD3DF_MULTITEXTURE_ALPHA		0x00001000L // we want to use alpha light maps for single or multi texture mode

#define LGD3DF_DO_WINDOWED             0x00002000L

#define LGD3DF_MT_BLENDDIFFUSE         0x00004000L // we want to use alpha light maps for single or multi texture mode

// Supported capabilities: (returned by the enumeration)

//depth buffer
#define LGD3DF_CAN_DO_ZBUFFER				0x00010000L
#define LGD3DF_CAN_DO_WBUFFER				0x00040000L // W-buffer 
//fog:
#define LGD3DF_CAN_DO_TABLE_FOG			0x00080000L // table based pixel fog
#define LGD3DF_CAN_DO_VERTEX_FOG			0x00100000L // vertex fog
//dithering
#define LGD3DF_CAN_DO_DITHER				0x00200000L // can dither
#define LGD3DF_CAN_DO_ANTIALIAS			0x00400000L // can use (sort independent) antialiasing 

#define LGD3DF_CAN_DO_SINGLE_PASS_MT	0x02000000L // we can do single pass double texturing
#define LGD3DF_CAN_DO_WINDOWED         0x04000000L // we can play the game in lil' window

#define LGD3DF_CAN_DO_ITERATE_ALPHA    0x08000000L // can do Gouraud interpolation between vetices alpha color


///////////////////////////////////////////////////////////////////////////




// Constants passed to lgd3d_set_pal_slot_flags()
#define LGD3DPALF_OPAQUE 1
#define LGD3DPALF_TRANS  2

// pre-initiazation calls
extern void lgd3d_set_RGB(void);
extern void lgd3d_set_hardware(void);
extern void lgd3d_set_software(void);
extern void lgd3d_texture_set_RGB(bool is_RGB);

// state query functions (won't change between init and shutdown)
extern BOOL lgd3d_is_RGB(void);
extern BOOL lgd3d_is_hardware(void);

// initialization and shutdown

//@TODO change this to return HRESULT error codes and support the message strings for them

extern BOOL lgd3d_init( lgd3ds_device_info *device_info );
// N.B. If the flag LGD3DF_DEPTH_BUFFER_REQUIRED was set in device_info->flags,
// and a depth buffer could not be created the call would fail.  To test for this 
// condition check whether the flags LGD3DF_ZBUFFER and LGD3DF_WBUFFER  
// was UNSET in device_info->flags.



extern BOOL lgd3d_attach_to_lgsurface( ILGSurface* pILGSurface );
extern void lgd3d_clean_render_surface( BOOL bDepthBuffToo );

extern void lgd3d_shutdown(void);

extern void lgd3d_start_frame(int frame);  // frame count passed to texture manager
extern void lgd3d_end_frame(void);

extern BOOL lgd3d_overlays_master_switch( BOOL bOverlaysOn );



extern void lgd3d_blit();
extern void lgd3d_clear(int color_index);

// application direct interface: death to COM!
extern void lgd3d_set_zwrite(BOOL zwrite);
extern void lgd3d_set_zcompare(BOOL zwrite);
extern void lgd3d_zclear(void);
extern void lgd3d_set_znearfar(double znear, double zfar);
extern void lgd3d_get_znearfar( double* pdZNear, double* pdZFar );

extern BOOL lgd3d_z_normal;

//z-stuff
extern void lgd3d_clear_z_rect(int x0, int y0, int x1, int y1);
extern void lgd3d_set_z(float z);    // z coord for flat (2d) polys

extern int /* D3DZBUFFERTYPE? */ lgd3d_get_depth_buffer_state( void );
extern BOOL lgd3d_is_zwrite_on( void );
extern BOOL lgd3d_is_zcompare_on( void );

// push all points closer to camera by zbias.
// returns old zbias.
extern double lgd3d_set_zbias(double zbias);

//  Z-Bias stack
#define LGD3D_ZBIAS_STACK_DEPTH   8

// 0 <= nZbias <= 16
extern void lgd3d_push_zbias_i( int nZBias );
extern void lgd3d_pop_zbias( void );

// States


//shading
// "smooth"(i.e., linera or bilinear) or "nearest point"
extern BOOL lgd3d_set_shading( BOOL bSmoothShading ); //NEW
extern BOOL lgd3d_is_smooth_shading_on( void );



//alpha blending
extern BOOL lgd3d_is_alpha_blending_on( void );

extern void lgd3d_set_alpha(float alpha);             // 0.0 <= alpha <= 1.0
extern void lgd3d_set_blend(BOOL do_blend);
extern void lgd3d_blend_normal(void);

extern void lgd3d_blend_multiply(int blend_mode); //from below
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


//efects:
// color range is 0..255
extern void lgd3d_set_chromakey(int red, int green, int blue);

extern void lgd3d_set_dithering( int bOn );
extern int lgd3d_is_dithering_on( void );
	
extern void lgd3d_set_antialiasing( int bOn );
extern int lgd3d_is_antialiasing_on( void );

extern BOOL lgd3d_enable_specular( BOOL bUseIt );

//fog
extern void lgd3d_set_fog_level(float fog_level);     // 0.0 <= fog_level <= 1.0
extern void lgd3d_set_fog_color(int r, int g, int b); // 0..255
extern void lgd3d_set_fog_enable(BOOL enable);
extern void lgd3d_set_fog_density(float density);
extern BOOL lgd3d_is_fog_on( void );

//  linear table fog
 
// returns the previous setting
extern int lgd3d_use_linear_table_fog( BOOL bUseIt );

// fogginess:     (end)____
//               /
//  _____(start)/

// start and end are floats in "W-coordiantes", that is, in the [ z_near , z_far ].
extern void lgd3d_set_fog_start_end( float fStart, float fEnd );   //please, check that  z_near <= fStart < fEnd <= z_far

// set the distance at which we should have full fog (results vary for different video cards)
extern void lgd3d_set_linear_fog_distance( float fDistance );  //please, check that  fStart <= fDistance <= fEnd 

//Textures:

extern void lgd3d_set_texture_level( int n );
extern void lgd3d_get_texblending_modes( ulong* pulLevel0Mode, ulong* pulLevel1Mode );

//texture manager
#define lgd3d_set_texture(bm) \
do if (g_tmgr) g_tmgr->set_texture(bm); while (0)

#define lgd3d_load_texture(bm) \
do if (g_tmgr) g_tmgr->load_texture(bm); while (0)

#define lgd3d_unload_texture(bm) \
do if (g_tmgr) g_tmgr->unload_texture(bm); while (0)

// procedural textures
extern void lgd3d_hack_light(r3s_point *p, float r);
extern void lgd3d_hack_light_extra(r3s_point *p, float r, grs_bitmap *bm);

// texture wrapping
//  TRUE == wrap the texture (texture is a torus), 
//  FALSE == clamp the texture (texture is a closed rectangle)
extern BOOL lgd3d_get_texture_wrapping( DWORD dwLevel );
extern BOOL lgd3d_set_texture_wrapping( DWORD dwLevel, BOOL bSetSmooth );


// palettes
extern void lgd3d_set_pal(uint start, uint n, uchar *pal);
extern void lgd3d_set_pal_slot(uint start, uint n, uchar *pal, int slot);
extern void lgd3d_set_pal_slot_flags(uint start, uint n, uchar *pal, int slot, int flags);

extern void lgd3d_get_trans_texture_bitmask(grs_rgb_bitmask *bitmask);
extern void lgd3d_get_opaque_texture_bitmask(grs_rgb_bitmask *bitmask);
extern void lgd3d_get_alpha_texture_bitmask(grs_rgb_bitmask *bitmask);

extern void lgd3d_set_texture_clut(uchar *clut);
extern uchar *lgd3d_set_clut(uchar *clut);

extern void lgd3d_disable_palette(void);
extern void lgd3d_enable_palette(void);

extern void lgd3d_set_alpha_pal(ushort *pal);


extern void lgd3d_set_offsets(int x, int y);
extern void lgd3d_get_offsets(int *x, int *y);



// drawing:

extern int lgd3d_draw_point(r3s_point *p);
extern void lgd3d_draw_line(r3s_point *p0, r3s_point *p1);


//Poligon Drowing states
//NOTE: for now it does *NOT* affect indexed and multytextured polies

typedef enum ePolyMode{
    kLgd3dPolyModeFillWTexture  = 0x00000001L,
    kLgd3dPolyModeFillWColor    = 0x00000002L,   
    kLgd3dPolyModeDrawEdges     = 0x00000004L,  // use color
    
    kLgd3dPolyModeDefault       = 0x00000001L, // normal, == kLgd3dPolyModeFillTexture
    kLgd3dPolyModeWireframe     = 0x00000004L, // == kLgd3dPolyModeDrawEdges
    kLgd3dPolyModeOutlineTex    = 0x00000005L, // == kLgd3dPolyModeFillWTexture + kLgd3dPolyModeDrawEdges
    kLgd3dPolyModeOutlineColor  = 0x00000006L, // == kLgd3dPolyModeFillWColor + kLgd3dPolyModeDrawEdges

    kLgd3dILLEGALPolyMode       = 0x00000000L
} ePolyMode;

extern BOOL lgd3d_set_poly_mode( ePolyMode eNewMode );
extern ePolyMode lgd3d_get_poly_mode();


extern int lgd3d_trifan(int n, r3s_point **vpl);
extern int lgd3d_lit_trifan(int n, r3s_point **vpl);
extern int lgd3d_poly(int n, r3s_point **vpl);
extern int lgd3d_spoly(int n, r3s_point **vpl);
extern int lgd3d_g2upoly(int n, g2s_point **vpl);
extern int lgd3d_g2poly(int n, g2s_point **vpl);
extern int lgd3d_g2utrifan(int n, g2s_point **vpl);
extern int lgd3d_g2trifan(int n, g2s_point **vpl);


// r3d interface
typedef int (*tLgd3dDrawPolyFunc)(int n, r3s_phandle *pl);

extern tLgd3dDrawPolyFunc           lgd3d_draw_poly_func;

extern void lgd3d_tmap_setup(grs_bitmap *bm);
extern void lgd3d_lit_tmap_setup(grs_bitmap *bm);
extern void lgd3d_rgblit_tmap_setup(grs_bitmap *bm);
extern void lgd3d_rgbalit_tmap_setup(grs_bitmap *bm);
extern void lgd3d_rgbafoglit_tmap_setup(grs_bitmap *bm); // rgba + fog 
extern void lgd3d_diffspecular_tmap_setup(grs_bitmap *bm); // rgba + fog 
extern void lgd3d_poly_setup(grs_bitmap *bm);
extern void lgd3d_spoly_setup(grs_bitmap *bm);
extern void lgd3d_rgb_poly_setup(grs_bitmap *bm);
extern void lgd3d_rgba_poly_setup(grs_bitmap *bm);


// indexed primitives:

typedef int (*tLgd3dDrawPolyIndexedFunc)( int n, r3s_phandle *pl, r3ixs_info *info );
    
extern tLgd3dDrawPolyIndexedFunc    lgd3d_draw_poly_indexed_func;

typedef void (*fp_release_IP)( void );

extern fp_release_IP                lgd3d_release_ip_func;

extern int lgd3d_indexed_poly(int n, r3s_point **vpl, r3ixs_info *info);
extern int lgd3d_indexed_spoly(int n, r3s_point **vpl, r3ixs_info *info);
extern void lgd3d_rgblit_tmap_setup(grs_bitmap *bm);
extern int lgd3d_lit_indexed_trifan(int n, r3s_point **vpl, r3ixs_info *info);

extern int lgd3d_release_indexed_primitives( void );



///////////////////////////////////

// Multi texturing

//used for single level texturing
#define LGD3DTB_MODULATE            0L    //default        
#define LGD3DTB_MODULATEALPHA       1L
#define LGD3DTB_BLENDDIFFUSE        2L

#define LGD3DTB_NO_STATES           3L

extern void lgd3d_set_texture_map_method( ulong flag );

//2 levels texturing:
#define LGD3D_MULTITEXTURE_COLOR             0L  //default
#define LGD3D_MULTITEXTURE_ALPHA             1L  
#define LGD3D_MULTITEXTURE_BLEND_TEX_ALPHA   2L  

#define LGD3D_MULTITEXTURE_NO_STATES         3L

extern void lgd3d_set_light_map_method( ulong flag ); //from the above


// additional sets of texture coordinates are added
typedef struct {
    float   u, v;
} LGD3D_tex_coord;




extern int lgd3d_TrifanMTD( int n, r3s_point **ppl, LGD3D_tex_coord **pptc );
extern int lgd3d_LitTrifanMTD( int n, r3s_point **ppl, LGD3D_tex_coord **pptc );
extern int lgd3d_RGBlitTrifanMTD( int n, r3s_point **ppl, LGD3D_tex_coord **pptc );
extern int lgd3d_RGBAlitTrifanMTD( int n, r3s_point **ppl, LGD3D_tex_coord **pptc );
extern int lgd3d_RGBAFoglitTrifanMTD( int n, r3s_point **ppl, LGD3D_tex_coord **pptc );
extern int lgd3d_DiffuseSpecularMTD( int n, r3s_point **ppl, LGD3D_tex_coord **pptc );

extern int lgd3d_g2UTrifanMTD( int n, g2s_point **vpl, LGD3D_tex_coord **vptc );
extern int lgd3d_g2TrifanMTD( int n, g2s_point **vpl, LGD3D_tex_coord **vptc );



// error codes:( the first argument of "lgd3d_get_error" )
#define LGD3D_EC_OK                             0L
#define LGD3D_EC_DD_KAPUT                       1L
#define LGD3D_EC_RESTORE_ALL_SURFS              2L
#define LGD3D_EC_QUERY_D3D                      3L
#define LGD3D_EC_GET_DD_CAPS                    4L
#define LGD3D_EC_ZBUFF_ENUMERATION              5L
#define LGD3D_EC_CREATE_3D_DEVICE               6L
#define LGD3D_EC_CREATE_VIEWPORT                7L
#define LGD3D_EC_ADD_VIEWPORT                   8L
#define LGD3D_EC_SET_VIEWPORT                   9L
#define LGD3D_EC_SET_CURR_VP                    10L
#define LGD3D_EC_CREATE_BK_MATERIAL             11L
#define LGD3D_EC_SET_BK_MATERIAL                12L
#define LGD3D_EC_GET_BK_MAT_HANDLE              13L
#define LGD3D_EC_GET_SURF_DESC                  14L
#define LGD3D_EC_GET_3D_CAPS                    15L
#define LGD3D_EC_VD_MPASS_MT                    16L
#define LGD3D_EC_VD_S_DEFAULT                   17L
#define LGD3D_EC_VD_SPASS_MT                    18L
#define LGD3D_EC_VD_M_DEFAULT                   19L
#define LGD3D_EC_VD_SPASS_BLENDDIFFUSE          20L
#define LGD3D_EC_VD_MPASS_BLENDDIFFUSE          21L



BOOL lgd3d_get_error(  DWORD* pdwCode, DWORD* phResult );

extern uchar* texture_clut, *lgd3d_clut;
extern BOOL lgd3d_punt_d3d;
extern BOOL g_bWFog;
extern double z_near, z_far;

#ifdef __cplusplus
}
#endif

#endif



