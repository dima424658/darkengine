/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>

#include <r3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <conio.h>
#include <mprintf.h>
#include <port.h>
#include <pt.h>
#include <string.h>
#include <refsys.h>
#define I_AM_PART_OF_EOS
#include <objects.h>

#include <fault.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

ObjRefInfo ObjRefInfos[1];
ObjInfo ObjInfosArray[1];
ObjInfo *ObjInfos = ObjInfosArray;

grs_canvas *off_canv;

void blit(void)
{
   gr_set_canvas(grd_visible_canvas);
   gr_bitmap(&(off_canv->bm),0,0);
   gr_set_canvas(off_canv);
}

mxs_vector anchor_point;

mxs_vector cam_vec;
mxs_vector cam_vel;
mxs_angvec cam_ang;
struct
{
   fix x,y,z;
} cam_angvel;

Location camera_loc;

mxs_real cam_zoom = 1.0;
mxs_real lt_zoom = 1.0;

void start_3d(void)
{
   r3_start_frame();
   r3_set_space(R3_CLIPPING_SPACE);
   r3_set_view_angles(&cam_vec,&cam_ang,R3_DEFANG);
   r3_set_zoom(cam_zoom);

   r3_set_2d_clip(FALSE);
   r3_set_clipmode(R3_CLIP);
   r3_set_polygon_context(R3_PL_POLYGON | R3_PL_CHECK_FACING);
   r3_set_near_plane(0.001); // 1.0
}

void end_3d(void)
{
   r3_end_frame();
}

#define ANG_INC (MX_ANG_PI/128)
#define MOV_INC (0.1)

int add_light, spotlight;
bool pick;
extern bool raycast_light;

void print_info(int vol)
{
   char *p;
   while ((p = portal_scene_info(vol)) != 0)
      mprintf("%s\n", p);
}
   
extern bool span_clip, linear_map;
extern bool project_space;
extern bool cache_feedback;
bool direct;

// return TRUE if exiting
bool parse_keys(void)
{
   mxs_vector tmp;
   while (kbhit())
   {
      int k=getch();
      switch (k)
      {
         case 'z': cam_zoom *= 1.41; break;
         case 'Z': cam_zoom /= 1.41; break;
         case '[': lt_zoom *= 1.41; break;
         case ']': lt_zoom /= 1.41; break;

         case 'c': mx_zero_vec(&cam_vec); cam_zoom = 1.0;
            cam_ang.tx = cam_ang.ty = cam_ang.tz = 0; break;
         case 'l': cam_angvel.z -= ANG_INC; break;
         case 'j': cam_angvel.z += ANG_INC; break;
         case 'i': cam_angvel.y -= ANG_INC; break;
         case 'k': cam_angvel.y += ANG_INC; break;
         case 'u': cam_angvel.x -= ANG_INC; break;
         case 'o': cam_angvel.x += ANG_INC; break;

         case 'w': cam_vel.x += MOV_INC; break;
         case 's': cam_vel.x -= MOV_INC; break;

         case 'a': cam_vel.y += MOV_INC; break;
         case 'd': cam_vel.y -= MOV_INC; break;

         case 'e': cam_vel.z += MOV_INC; break;
         case 'q': cam_vel.z -= MOV_INC; break;

         case 'm': mprintf("xxx\n"); break;
         case '!': render_backward = !render_backward; break;
         case 'b': add_light = 1; break;
         case 'B': add_light = 2; break;
         case '$': raycast_light = !raycast_light; break;
         case 'S': linear_map = !linear_map; break; // project_space = !project_space;
         case '/': direct = !direct; break;

         case '\\': span_clip = !span_clip; break;
#if 0
         case '\t': mprintf("cache_feedback=%d\n", (cache_feedback = !cache_feedback)); break;
#else
         case '\t': spotlight = !spotlight; break;
#endif

         case 'R': anchor_point = cam_vec; break;
         case 'p': pick = !pick; break;

         case '?': print_info(30); break;
         case '<': print_info(5); break;
         case '>': print_info(50); break;

         case ' ':
            cam_vel.x = cam_vel.y = cam_vel.z = 0;
            cam_angvel.x = cam_angvel.y = cam_angvel.z = 0;
            break;

         case 'Q':
         case 27: return TRUE;
      }
   }
   cam_ang.tx += cam_angvel.x;
   cam_ang.ty += cam_angvel.y;
   cam_ang.tz += cam_angvel.z;

   tmp = *r3_get_forward_slew();
   mx_scaleeq_vec(&tmp, cam_vel.x);
   mx_addeq_vec(&cam_vec, &tmp);

   tmp = *r3_get_left_slew();
   mx_scaleeq_vec(&tmp, cam_vel.y);
   mx_addeq_vec(&cam_vec, &tmp);

   tmp = *r3_get_up_slew();
   mx_scaleeq_vec(&tmp, cam_vel.z);
   mx_addeq_vec(&cam_vec, &tmp);

   return FALSE;
}

uchar shade_table[64][256];

void setup_pal(void)
{
   uchar pal[768];
   int i,j,k,r,g,b,dr,dg,db,m;
   k = 0;
   j = cam_zoom;
   for (i=0; i < 32; ++i) {
      dr = rand() & 127;
      dg = rand() & 127;
      db = rand() & 127;
      dr += 128;
      dg += 128;
      db += 128;
#if 0
      r = dr*2;
      g = dg*2;
      b = db*2;
      dr = dr * 3/4;
      dg = dg * 3/4;
      db = db * 3/4;
#else
      r = g = b = 0;
#endif

      for (j=0; j < 8; ++j) {
         r += dr;
         g += dg;
         b += db;
         pal[k++] = r >> 3;
         pal[k++] = g >> 3;
         pal[k++] = b >> 3;
         for (m=0; m < 32; ++m)
            shade_table[m][i*8+j] = i*8 + ((7-j) * m / 32);
      }
   }
   gr_set_pal(0, 256, pal);
   pt_light_table = shade_table[0];
}

r3s_context con;
extern void WorldRepLoadGodFile(char *);
extern void WorldRepLoadWrFile(char *);

#pragma off(unreferenced)
void main(int argc,char **argv)
{
   grs_screen *screen;
   grs_bitmap *bm;
   mxs_vector obj;
   mxs_plane pln;
   float j;
   int acc;
   mxs_real racc;
   mxs_ang ang;

   if (argc > 1 && !strcmp(argv[1], "+nofault")) {
      ex_startup(EXM_DIVIDE_ERR);
      --argc;
      ++argv;
   } else
      ex_startup(EXM_ALL);

   init_portal_renderer(0,7);
   span_clip = FALSE;
   obj.x = 12.0;
   obj.y = 0;
   obj.z = 0;

   // init the mode, make a screen
   gr_init();
   atexit(gr_close);
   gr_set_mode(GRM_320x200x8,TRUE);

   screen = gr_alloc_screen(grd_mode_info[grd_mode].w,grd_mode_info[grd_mode].h);
   gr_set_screen(screen);

   setup_pal();

   r3_init_defaults(-1,-1,-1,-1,-1);

   if (argc > 1)
      WorldRepLoadWrFile(argv[1]);
   else
      WorldRepLoadGodFile("world.god");

   mx_mk_vec(&cam_vec,0,0,0);
   MakeLocationFromVector(&camera_loc, &cam_vec);
   
   off_canv = gr_alloc_canvas(BMT_FLAT8,grd_bm.w, grd_bm.h);

   for(;;) {
      Position pos;
      gr_set_canvas(off_canv);
      //gr_clear(rand());
      MakeHintedLocationFromVector(&camera_loc, &cam_vec, &camera_loc);
      pos.loc = camera_loc;
      pos.fac = cam_ang;
      reset_dynamic_lights();
      if (spotlight) 
#if 1
         portal_add_omni_light(128, 0, &camera_loc, LIGHT_DYNAMIC);
#else
         portal_add_omni_light(2.0, cam_vec.x, cam_vec.y, cam_vec.z, cam_ang.tz, cam_ang.ty, lt_zoom,TRUE);
#endif

      if (pick) {
         int val;
         gr_set_canvas(off_canv);
         val = PortalRenderPick(&pos, grd_bm.w/2, grd_bm.h/2, 1.0);
#if 0
         if (val >= 0) {
            portal_light_poly(val >> 8, val & 255);
         }
#endif
      }
      if (direct)
         gr_set_canvas(grd_visible_canvas);
      else
         gr_set_canvas(off_canv);
      start_3d();
      if (parse_keys()) break;
      end_3d();
      portal_render_scene(&pos, 1.0);
      if (!direct)
         blit();
      if (add_light) {
         Location loc;
         MakeLocationFromVector(&loc, &cam_vec);
         portal_add_omni_light(128, 0, &loc, LIGHT_STATIC);
         add_light = 0;
      }
#if 0
      if (add_light) {
         if (add_light == 1)
            //portal_add_omni_light(1.0, cam_vec.x, cam_vec.y, cam_vec.z, FALSE);
            portal_add_omni_light_perfect(cam_vec.x, cam_vec.y, cam_vec.z);
         else
            portal_add_spotlight(2.0, cam_vec.x, cam_vec.y, cam_vec.z, cam_ang.tz, cam_ang.ty, lt_zoom, FALSE);
         add_light = 0;
      }
#endif
   }
}
#pragma on(unreferenced)
