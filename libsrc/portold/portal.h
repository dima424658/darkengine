/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/portal.h,v 1.15 1997/01/29 00:02:38 buzzard Exp $
//   Interface to portal

#include <r3ds.h>
#include <wr.h>

extern void init_portal_renderer(int dark_color, int light_color);
extern void portal_render_scene(Location *loc);
extern bool PortalRaycast(Location *, Location *, int);
extern int PortalRenderPick(Location *loc, int x, int y);
        // returns cell*256+poly

   // table maps clut_id to clut for portals
extern uchar *pt_clut_list[256];

   // function maps texture_id to mipmapped r3s_texture
extern r3s_texture (*portal_get_texture)(int d);

   // texture memory manager (allocate rectangles with row==256)
extern void portal_free_all_mem_rects(void);
extern unsigned char *portal_allocate_mem_rect(int x, int y);
extern void portal_free_mem_rect(unsigned char *p, int x, int y);

   // lighting functions
extern void reset_dynamic_lights(void);
extern void portal_add_omni_light(float br, float, float, float, bool);
extern void portal_add_spotlight(float br, float x, float y, float z, fixang a, fixang b, float zoom, bool);

extern void clear_surface_cache(void);

   // debugging tools
extern bool render_backward;      // lets you see how far away it renders

   // get informative strings about last scene rendered;
   // "vol" from 0..100 determines how much feedback;
   // call over and over until you get null string
extern char *portal_scene_info(int vol);
