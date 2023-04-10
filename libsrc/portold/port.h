/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/prj/cam/src/portal/RCS/port.h 1.45 1998/12/22 10:34:19 JON Exp $
//   Interface to portal

#ifndef _PORT_H_
#define _PORT_H_

#include <r3ds.h>
#include <wrlimit.h>
#include <wrtype.h>
#include <wrfunc.h>
#include <wrdb.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef int ObjID;  // can't include objtype.h 

//////                                 Initialization


  // Initialize, and set range of palette-based lighting; e.g.
  // 0,7 means add 0 for darkest, add 7 for lightest; we use 11,0.
extern void init_portal_renderer(int dark_color, int light_color);

extern void portal_setup_water_hack(
   int num_textures, r3s_texture *tex_list, float *alpha_list, int *rgb_list);

extern void portal_cleanup_water_hack(void);


//////                                 Render


  // Render scene from this position&orientation
extern void portal_render_scene(Position *pos, float zoom);

  // function maps texture_id to mipmapped r3s_texture
extern r3s_texture (*portal_get_texture)(int d);

  // table maps clut_id to clut for portals
extern uchar *pt_clut_list[256];

  // tables map media to cluts; entry & exit are simulated
  // translucent surfaces; haze is a watery haze which uses
  // uses cluts haze_clut..haze_clut+15
extern unsigned char pt_medium_entry_clut[256];
extern unsigned char pt_medium_exit_clut[256];
extern unsigned char pt_medium_haze_clut[256];

// Like medium-haze clut, except per motion index.  Overrides 
// medium haze if non-zero. 
extern unsigned char pt_motion_haze_clut[256]; 

   // Both of these expect results in milliseconds.
extern long (*portal_get_time)(void);
extern long (*portal_get_frame_time)(void);

  // tell portal where the near/far clipping planes are
extern void portal_set_znearfar(double z_near, double z_far);

  // which texture id for the sky?
extern void portal_setup_star_hack(int tex_id);

   // This controls MIP levels.  It should be 0-1 and defaults to .9.
extern mxs_real portal_detail_level;

  // render your object through the passed-in clut;
  // callback is in appropriate r3s state, outside of a block
extern void (*portal_render_object)(ObjID o, uchar *clut, ulong fragment);
extern BOOL (*portal_object_visible)(ObjID);
extern BOOL (*portal_object_blocks)(ObjID blocker, ObjID blockee);
extern void (*portal_pre_draw_callback)(void);
extern Position* (*portal_object_pos)(ObjID obj);

// kinds of object fragments--OTHER means the object has been split
// and this fragment is not the first
#define OBJ_NO_SPLIT      0
#define OBJ_SPLIT_FIRST   1
#define OBJ_SPLIT_OTHER   2

  // tools for rendering objects

// push the r3s clip planes for the bounds of this cell;
// don't bother pushing ones which don't clip the passed
// in model
extern void portal_push_clip_planes(
     mxs_vector *bbox_min, mxs_vector *bbox_max,
     mxs_vector *sphere_center, float radius);
extern void portal_pop_clip_planes(void);


// bitfield telling us which cells have been in the visible set in
// portal_render_scene since the last call to portal_unmark_visible_cells
extern uchar portal_cell_rendered[];

// was cell ThisCell in the visible set last frame?
#define portal_cell_visible(ThisCell) \
   ((portal_cell_rendered[(ThisCell) >> 3] & ((1 << ((ThisCell) & 7)))) ? \
    TRUE : FALSE)


// clear portal_cell_visible
void portal_unmark_visible_cells();

//////                                 Lighting

// These flags apply to lights when we determine how they affect the
// world--when lighting a level, or in the dynamic case during gameplay.

// for moving lights--not part of world rep
#define LIGHT_DYNAMIC           1

// separate lightmap, added in with a weighting
#define LIGHT_ANIMATED          2

// not raycast lit, for prototype level lighting
#define LIGHT_QUICK             4

// oversampled raycast
#define LIGHT_QUAD              8

// raycast w/objects 
#define LIGHT_OBJCAST          16

extern int portal_add_omni_light(float br, float ambient,
      Location *loc, uchar dynamic, float radius);
extern int portal_add_spotlight(float br,
      Position *pos, float zoom, uchar dynamic);
extern void portal_add_simple_dynamic_light(float br, float ambient,
                                            Location *loc, float radius);
EXTERN void portal_add_simple_dynamic_dark(float br, float ambient,
                                           Location *loc, float radius);


  // call this once a frame (or however often you recompute dynamic lights)
extern void reset_dynamic_lights(void);

  // animated lights are controlled outside Portal--we call this once/frame
  // to update animated_light_intensity.  num_animated_lights is static
  // over a given level.
extern void (*portal_anim_light_callback)(long time_change_millisec);
extern uchar *portal_anim_light_intensity;
extern int num_anim_lights;

  // callbacks used when we light a level
extern void (*failed_light_callback)(Location *hit, Location *dest);
extern void (*lightmap_point_callback)(mxs_vector *loc, bool lit);
extern void (*lightmap_callback)(PortalLightMap *lightmap);

extern void clear_surface_cache(void);

  // clear all texture cache entries for a given texture id
extern void clear_surfaces_for_texture(uchar texture_id);

  // This oversamples static lighting.  Dynamic lighting is not affected.
extern bool portal_quadruple_lighting;

  // Sometimes, you just want to see in shadows.
extern bool portal_render_from_texture;

  // This should be set for each level, 0-255.
extern int portal_ambient_light_level;

EXTERN void portal_set_normalized_color(int rm, int gm, int bm);
EXTERN void portal_convert_hsb_to_rgb(int *rp, int *gp, int *bp, float hue, float saturation);
EXTERN void portal_shine_omni_light(int light_index, Location *loc, 
                                        uchar lighting_type);


//////                                 World Rep


  // hit_loc is returned; end_loc's cell is updated if reached
  // Pass in something nonzero for the last parameter to cast sans
  // epsilon.
extern void PortalRaycastVector (Location *start_loc, mxs_vector *vec,
                                 Location *hit_loc, int use_zero_epsilon);
extern bool PortalRaycast(Location *start_loc, Location *end_loc,
                          Location *hit_loc, int use_zero_epsilon);

  // collects extra refs
extern bool PortalRaycastRefs(Location *start_loc, Location *end_loc,
                              Location *hit_loc, int use_zero_epsilon);

  // This finds the polygon reached by the most recent raycast.
  // It returns -1 if the most recent cast returned TRUE.
extern int PortalRaycastFindPolygon(void);

  // This function uses so much of the raycaster's internals that it's
  // basically a wrapper for them.
extern bool PortalPointInPolygon(mxs_vector *point, PortalCell *cell,
                                 PortalPolygonCore *polygon,
                                 int vertex_offset, bool set_hull_test);

  // These are set by PortalRaycast() and PortalRaycastRefs().
extern bool PortalRaycastResult;        // return value of most recent raycast
extern int PortalRaycastPlane;          // -1 if we hit nothing
extern int PortalRaycastCell;           // end cell of cast if we didn't hit
extern float PortalRaycastTime;         // along total cast: 0-1
extern mxs_vector PortalRaycastHit;     // undefined if we hit nothing

#define RAYCAST_MAX_REFS 128
  // These are set by PortalRaycastRefs().
extern int PortalRaycastRefCount;
extern int PortalRaycastRef[RAYCAST_MAX_REFS];

  // returns cell*256+poly
extern int PortalRenderPick(Position *pos, int x, int y, float zoom);

extern int PortalCellsInArea(int *cell_list, int list_max,
   mxs_vector *mn, mxs_vector *mx, Location *seed);

extern void PortalComputeBoundingSphere(PortalCell *cell);


//////                                 Memory management

  // texture memory manager (allocate rectangles with row==ptmem_row)

extern int     ptmem_row;
extern uchar   ptmem_wlog;
extern void portal_set_mem_rect_row(int row);
extern void portal_free_all_mem_rects(void);
extern unsigned char *portal_allocate_mem_rect(int x, int y);
extern void portal_free_mem_rect(unsigned char *p, int x, int y);

  // lightmap aggregation stuff
extern void porthw_init(void);
extern void porthw_shutdown(void);
extern void porthw_start_frame(int frame);
extern void porthw_end_frame(void);


//////                         Moving Surfaces at Medium Boundaries
// The number of cell motion structures is fixed.  This constant
// appears here to make code using it more legible.
#define MAX_CELL_MOTION 256
extern PortalCellMotion portal_cell_motion[];
extern void (*portal_anim_medium_callback)(long time_change_millisec);


// This will move completely into the editor later, and not be here.
extern int medium_motion_index;

//////                                 Debugging tools


  // get informative strings about last scene rendered;
  // "vol" from 0..100 determines how much feedback;
  // call over and over until you get null string
extern char *portal_scene_info(int vol);

extern bool render_backward;      // lets you see how far away it renders


//////                                 Debugging flags
// These are things which are be useful in the editor,
// but would be kind of silly to include in the game.

// These are in portdraw.c, also referenced in portal.c.
#ifndef SHIP
   extern bool draw_solid_by_MIP_level;
   extern bool draw_solid_by_cell;
   extern bool draw_solid_wireframe;
   extern bool draw_solid_by_poly_flags;
   extern bool draw_solid_by_cell_flags;
   extern bool draw_wireframe_around_tmap;
   extern bool draw_wireframe_around_poly;
#endif // SHIP


/*
Local Variables:
typedefs:("Location" "ObjID" "Position" "uchar")
End:
*/

#ifdef __cplusplus
};
#endif

#endif

extern void (*portal_sfx_callback)(int cell);