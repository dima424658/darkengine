// $Header: x:/prj/tech/libsrc/lgd3d/RCS/d3dtest.c 1.20 1998/05/11 10:56:28 KEVIN Exp $
#include <ctype.h>   // for toupper
#include <stdlib.h>  // for atexit

#include <appagg.h>
#include <dbg.h>
#include <dispapi.h>
#include <grtb.h>    // gr testbed library headers
#include <g2.h>      // g2 library headers
#include <mprintf.h>
#include <r3ds.h>
#include <lgd3d.h>

typedef struct {
   short x,y;
} point;

static double hazetest = 200.0;
static double ztest = 1.0;
static grs_canvas *zcanvas;
static fixang theta=0;

static uchar bm_bits[256*256*2];
static grs_bitmap bm={
   bm_bits, BMT_FLAT16, 0, BMF_POW2, 128, 16, 256, 7, 4
};
static uchar bm2_bits[64];
static grs_bitmap bm2={
   bm2_bits, BMT_FLAT8, 0, BMF_POW2, 8, 8, 8, 3, 3
};
static point pos={40,40};
static point size={256,256};

void break_here(void )
{
#ifdef WIN32
   extern void __stdcall DebugBreak();
   DebugBreak();
#endif
};

static char input_char=0;
static int do_switch(void ) {
   switch (input_char) {
      case '2': size.y+=1; break;
      case '8': size.y-=1; break;
      case '4': size.x-=1; break;
      case '6': size.x+=1; break;
      case 'w': pos.y-=5; break;
      case 'x': pos.y+=5; break;
      case 'a': pos.x-=5; break;
      case 'd': pos.x+=5; break;
      case 'W': pos.y-=1; break;
      case 'X': pos.y+=1; break;
      case 'A': pos.x-=1; break;
      case 'D': pos.x+=1; break;
      case 'r': theta += 0x100; break;
      case 'R': theta += 0x1000; break;
      case 'v': theta -= 0x100; break;
      case 'V': theta -= 0x1000; break;
      case 'z': ztest += 0.1; mprintf("ztest: %g\n", ztest); break;
      case 'Z': ztest -= 0.1; mprintf("ztest: %g\n", ztest); break;
      case 'h': hazetest *= 1.5; mprintf("hazetest: %g\n", hazetest); break;
      case 'H': hazetest /= 1.5; mprintf("hazetest: %g\n", hazetest); break;
      case 'q': return 0;
   }
   return 1;
}

static int auto_repeat=0;
extern bool AppGetKey(char *c);
static int get_input(void)
{
   if (AppGetKey(&input_char))
      return do_switch();

   if (auto_repeat)
      return do_switch();

   return 1;
}

static r3s_point pl[4];
static r3s_point *ppl[4];
static void make_ppl(void )
{
   fix x0, y0;
   fix dx, dy;
   fix sint, cost;

   sint = fix_sin(theta);
   cost = fix_cos(theta);
   ppl[0]=pl, ppl[1]=pl+1, ppl[2]=pl+2;
   x0 = fix_make(pos.x,0), y0 = fix_make(pos.y,0);
   dx = fix_make(size.x,0xffff), dy = fix_make(size.y,0xffff);
   pl[0].grp.sx = x0; pl[0].grp.sy = y0;
   pl[1].grp.sx = x0 + fix_mul(dx, cost);
   pl[1].grp.sy = y0 + fix_mul(dy, sint);
   pl[2].grp.sx = x0 - fix_mul(dx, sint);
   pl[2].grp.sy = y0 + fix_mul(dy, cost);

   pl[0].grp.u = pl[0].grp.v = -1.0;
   pl[1].grp.u = 6.5, pl[1].grp.v=-1.0;
   pl[2].grp.v = 1.5, pl[2].grp.u=-1.0;
   pl[0].grp.i = 0.5, pl[1].grp.i = 0.1, pl[2].grp.i = 0.9;
   pl[0].grp.w = pl[1].grp.w = pl[2].grp.w = 0.1;

   pl[3].grp.sx = x0 + ((fix_mul(dx, cost) - fix_mul(dx, sint))>>2);
   pl[3].grp.sy = y0 + ((fix_mul(dy, cost) + fix_mul(dy, sint))>>2);
   pl[3].grp.w = 0.05;
   pl[3].p.z = 20.0;
}

static void do_gfx(void )
{
   static int frame=0;
   int count;
   IDisplayDevice *pDDev = AppGetObj(IDisplayDevice);

   tb_start_frame();
   gr_clear(0);

   lgd3d_set_offsets(0,0);
   make_ppl();

   count=pDDev->lpVtbl->BreakLock(pDDev);

   lgd3d_start_frame(frame++);

   lgd3d_set_fog_color(0,0,0);
   lgd3d_set_fog_density(1.0/hazetest);
   lgd3d_set_fog_enable(TRUE);

   ppl[0]->p.z = 5.0;
   ppl[1]->p.z = 5.0;
   ppl[2]->p.z = 5.0;
   ppl[0]->grp.w = 0.2;
   ppl[1]->grp.w = 0.2;
   ppl[2]->grp.w = 0.2;

   g_tmgr->unload_texture(&bm);

//   lgd3d_set_zcompare(FALSE);
   g_tmgr->set_texture(&bm);
//   lgd3d_trifan(3, ppl);

   ppl[0]->p.z = 0.1;
   ppl[1]->p.z = ztest;
   ppl[2]->p.z = 0.1;
   ppl[0]->grp.w = 10.0;
   ppl[1]->grp.w = 1.0/ztest;
   ppl[2]->grp.w = 10.0;

//   lgd3d_set_zcompare(TRUE);
   g_tmgr->set_texture(&bm2);
   lgd3d_trifan(3, ppl);

   lgd3d_hack_light(ppl[0],200.0);

   gr_set_fcolor(255);
   lgd3d_draw_point(&pl[3]);

   g_tmgr->unload_texture(&bm2);

   lgd3d_end_frame();
//   g_tmgr->reload_texture(&bm);

   pDDev->lpVtbl->RestoreLock(pDDev, count);

//   gr_bitmap(&bm, pos.x, pos.y);
//   g_tmgr->stats();
   tb_end_frame();
}

static lgd3ds_device_info *device_info;

static void init(void )
{
   int i;
   ushort *bits = (ushort *)bm.bits;

   for (i=0; i<bm2.h*bm2.row; i++)
      bm2.bits[i]=((i&1) ^ ((i>>bm2.wlog)&1))?55:0;
   for (i=0; i<bm.h*bm.w; i++)
      bits[i]=i;

   gr_set_pal16(gr_alloc_pal565_from_pal888(grd_pal),0);
   zcanvas = gr_alloc_canvas(BMT_FLAT16, grd_cap->w, grd_cap->h);

   lgd3d_init(device_info);
   atexit(lgd3d_shutdown);
#if 0
{
   extern BOOL lgd3d_z_normal;
   extern void lgd3d_set_zwrite(BOOL val);
   extern void lgd3d_set_znearfar(double zn, double zf);

   lgd3d_z_normal = FALSE;
   lgd3d_set_zwrite(TRUE);
   lgd3d_set_znearfar(1.0, 20.0);
}
#endif
}


GUID *d3dtestParseArgs(int argc, const char **argv)
{
   int i, device=-1, num_devices;

   tb_default_mode = GRM_640x480x16;

   for (i=1; i<argc; i++) {
      if (*argv[i] == '-' || *argv[i] == '/') {
         char c = toupper(*(argv[i]+1));
         switch (c) {
         case '0':
         case '1':
         case '2':
            device = c - '0';
            break;
         case 'A':
            auto_repeat = 1;
            break;
         case 'M':
            mono_logon("ml.txt", MONO_LOG_NEW, MONO_LOG_ALLWIN);
            break;
         }
      }
   }

   mono_clear();
   num_devices = lgd3d_enumerate_devices();
   if (num_devices < 0)
      Error(1, "Direct X 5 or higher required.\n");

   if (num_devices == 0)
      Error(1, "No suitable Direct3D device available!\n");

   if ((device<0)||(device>=num_devices))
      device = num_devices-1;

   device_info = lgd3d_get_device_info(device);
//   device_info->flags |= LGD3DF_ZBUFFER|LGD3DF_SPEW;

   mprintf("Using %s\n", device_info->p_ddraw_desc);

   return device_info->p_ddraw_guid;
}

bool d3dtestLoop(void )
{
   do_gfx();
   return get_input();
}

void d3dtestInit(void )
{
   tb_init();
   init();
}

// this is the last atexit() called before the AppAgg gets destroyed
void d3dtestExit(void ) {
}

