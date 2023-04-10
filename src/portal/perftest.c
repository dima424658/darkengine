/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <dos.h>

#include <lg.h>
#include <2d.h>
#include <mprintf.h>

#include "perfcore.h"
#include "char.h"
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////////////////////////////////////////////////////////
//
//     interface to 2d
//
//  note wacky linear framebuffer requests; is
//  this still how you do stuff?
//

extern unsigned char *scr;
extern int row;

uchar pal[768];
grs_screen *screen;
grs_canvas draw_canv;
grs_bitmap off;

void clear_off(void)
{
   memset(off.bits, 0, off.w*off.h);
}

uchar pal[768];
void init_offscreen(void)
{
   short w,h;
   w = grd_cap->w;
   h = grd_cap->h;
   if (off.bits)
      Free(off.bits);
   off.bits = (uchar *) Malloc(w*h);
   off.type = BMT_FLAT8;
   off.flags = 0;
   off.align = 0;
   off.w = w;
   off.h = h;
   off.row = w;

   gr_make_canvas(&off, &draw_canv);
   clear_off();

   scr = off.bits;
   row = w;
}

bool try_direct;
bool direct_hack;
bool double_buffer;
bool no_blit;

void set_screen_mode(int mode)
{
   int modeFlags;

   if (screen)
      gr_free_screen(screen);
   if (gr_mode_linear(mode)) {
      modeFlags = CLEAR_LINEAR;
      direct_hack = try_direct;
   } else {
      modeFlags = CLEAR_BANKED;
      direct_hack = 0;
   }
   double_buffer = try_direct && direct_hack;

   gr_set_mode(mode,modeFlags);

   if (try_direct && direct_hack == 0)
      mprintf("Couldn't get a linear %dx%d mode.\n", grd_cap->w, grd_cap->h); 

   screen = gr_alloc_screen( grd_cap->w, grd_cap->h * (1 + double_buffer));
   if (!screen && double_buffer) {
      double_buffer = 0;
      screen = gr_alloc_screen( grd_cap->w, grd_cap->h);
   }
   gr_set_screen(screen);
   gr_clear(0);
   init_offscreen();
}

void setup_draw_canv(void)
{
   if (direct_hack) {
      gr_make_canvas(&grd_bm, &draw_canv);
      draw_canv.bm.h = off.h;
   } else
      gr_make_canvas(&off, &draw_canv);
}

bool cur_screen;
void show_screen(void)
{
   if (!direct_hack) {
      if (!no_blit)
         gr_bitmap(&off, 0, 0);
   }
   if (double_buffer) {
      if (cur_screen == 0) {
         gr_set_focus(0, grd_bm.h);
         draw_canv.bm.bits = grd_bm.bits;
      } else {
         gr_set_focus(0, 0);
         draw_canv.bm.bits = grd_bm.bits + grd_bm.row * draw_canv.bm.h;
      }
      cur_screen = !cur_screen;
   }         
}

void start_graphics(void)
{
   gr_init();
}

#define P0   0
#define P1   (255/3)
#define P2   (255*2/3)
#define P3   255
int pal_data[] =
{
   P3,P3,P3,

   P3,P0,P0,
   P3,P0,P1,
   P3,P0,P2,
   P3,P1,P0,
   P3,P1,P1,
   P3,P1,P2,
   P3,P2,P0,
   P3,P2,P1,
   P3,P2,P2,
   P3,P3,P0,
   P3,P3,P1,
   P3,P3,P2,

   P0,P3,P0,
   P1,P3,P0,
   P2,P3,P0,
   P0,P3,P1,
   P1,P3,P1,
   P2,P3,P1,
// P0,P3,P2,
   P1,P3,P2,
   P2,P3,P2,
// P0,P3,P3,
// P1,P3,P3,
   P2,P3,P3,

   P0,P0,P3,
   P0,P1,P3,
// P0,P2,P3,
   P1,P0,P3,
   P1,P1,P3,
   P1,P2,P3,
   P2,P0,P3,
   P2,P1,P3,
   P2,P2,P3,
// P3,P0,P3,
   P3,P1,P3,
   P3,P2,P3
};
#undef P0
#undef P1
#undef P2
#undef P3

grs_bitmap texture;

void init_graphic_data(void)
{
   int x,y,r,g,b,n;

   for (y=0; y < 32; ++y) {
     // setup y palette banks
     r = pal_data[y*3+0];
     g = pal_data[y*3+1];
     b = pal_data[y*3+2];
     for (x=0; x < 8; ) {
        n = (y * 8 + x) * 3;
        pal[n+0] = r * (x*2+2) / 16;
        pal[n+1] = g * (x*2+2) / 16;
        pal[n+2] = b * (x*2+2) / 16;
        ++x;
     } 
   }

   gr_init_bitmap(&texture, (uchar *) Malloc(256*256), BMT_FLAT8, 0, 256,256);
   for (y=0; y < 256; ++y)
      for (x=0; x < 256; ++x)
         texture.bits[x+256*y] = (x+23)^y;
}

char fps_buffer[64];
extern int tris;
bool updated;
int check_delay = 200;

void process_time(void)
{
   static int checkpoint_time;
   static int checkpoint_frame;
   static int this_frame;

   struct dostime_t time;
   int hund;

   _dos_gettime(&time);
   hund = ((time.hour*60 + time.minute)*60
          + time.second)*100 + time.hsecond;

   if (hund < checkpoint_time) // wrapped!
      this_frame = 0;          // force a restart

   if (!this_frame || (hund > checkpoint_time + check_delay && this_frame - checkpoint_frame > 1)) {
      if (this_frame) {
         // show the FPS
         double fps = (this_frame - checkpoint_frame)
                   / ((hund - checkpoint_time) / 100.0);
         sprintf(fps_buffer, "%.1lf fps  %d polys", fps, tris);
         updated = TRUE;
      }
      checkpoint_time = hund;
      checkpoint_frame = this_frame;
   }
   ++this_frame;
   tris = 0;
}

#define MOV   (0.02/map_scale)
#define ROT   0x10

bool clear=TRUE;
bool swap_uv;
bool wireframe;

extern int grid_size_x, grid_size_y;
extern int scale;

int mode_index;
int modes[] =
{
   GRM_320x200x8,
   GRM_320x400x8,
   GRM_640x400x8,
   GRM_640x480x8,
   GRM_800x600x8
};
int mode_count = sizeof(modes) / sizeof(modes[0]);

void reset_screen(void)
{
   set_screen_mode(modes[mode_index]);
   setup_draw_canv();
   gr_set_pal(0, 256, pal);
}

void mode_switch(int d)
{
   mode_index = (mode_index + d + mode_count) % mode_count;
   reset_screen();
}

void mode_set(int d)
{
   mode_index = d % mode_count;
   reset_screen();
}

FILE *in,*out;
bool show_times = TRUE;

void update_screen(void)
{
   gr_push_canvas(&draw_canv);
   if (clear)
      gr_clear(0);
   draw_scene();
   process_time();
   if (show_times)
      draw_string(scr, row, 2, 160, fps_buffer);
   gr_pop_canvas();
   show_screen();
}

void write_mode_info(void)
{
   fprintf(out, "%c%c%c%c%c%c grid:%2dx%2d screen:%4dx%3d  ",
       "b "[no_blit],
       " c"[clear],
       " u"[swap_uv],
       " d"[direct_hack],
       " w"[wireframe],
       " t"[show_times],
       grid_size_x, grid_size_y,
       grd_bm.w, grd_bm.h);
}

#define GRID   256

bool output_info;
void output_time(void)
{
   int check_old = check_delay;

   // first, we're in the middle of a phase,
   // so clear it out

   updated = FALSE;
   check_delay = 100;
   while (!updated)
      update_screen();

   // now let's process an entire phase
   check_delay = check_old;
   updated = FALSE;
   while (!updated)
      update_screen();

   if (output_info)
      write_mode_info();

   fprintf(out, "%s\n", fps_buffer);
}

extern int multi_grid_poly, multi_grid_split;

void doit(void)
{
   reset_screen();

   for(;;) {
      update_screen();

      while (kbhit() || in) { 
         int c;
         if (kbhit()) {
            c = getch();
               // if non-interactive, don't accept anything except "quit"
            if (in) {
               if (c != 'Q' && c != 27)
                  continue;
            }
         }
         else
            c = feof(in) ? 'Q' : fgetc(in);

         switch(c) {
            case ';':
               if (in) {
                  char buf[256];
                  fgets(buf, 256, in);
               }
               break;

            case '=':
            case '+':
                      if (grid_size_x < GRID) ++grid_size_x;
                      if (grid_size_y < GRID) ++grid_size_y;
                      break;
            case '-':
                      if (grid_size_x > 1) --grid_size_x;
                      if (grid_size_y > 1) --grid_size_y;
                      break;

            case '0': grid_size_x = 1;
                      grid_size_y = 1;
                      break;

              // some default grid sizes; making them multiples of 4 (or 8)
              // lets us use 'h' and 'v' without changing the poly count
            case '9': grid_size_x = 72;
                      grid_size_y = 72;
                      break;

            case '8': grid_size_x = 48;
                      grid_size_y = 48;
                      break;
          
            case '7': grid_size_x = 32;
                      grid_size_y = 32;
                      break;

            case '6': grid_size_x = 24;
                      grid_size_y = 24;
                      break;

            case '5': grid_size_x = 16;
                      grid_size_y = 16;
                      break;

            case 'h': grid_size_x *= 2;
                      grid_size_y /= 2;
                      if (grid_size_x > GRID) grid_size_x = GRID;
                      if (grid_size_y < 1) grid_size_y = 1;
                      break;

            case 'v': grid_size_y *= 2;
                      grid_size_x /= 2;
                      if (grid_size_y > GRID) grid_size_y = GRID;
                      if (grid_size_x < 1) grid_size_x = 1;
                      break;

            case 'x': if (grid_size_x < GRID) ++grid_size_x; break;
            case 'X': if (grid_size_x > 1) --grid_size_x; break;
            case 'y': if (grid_size_y < GRID) ++grid_size_y; break;
            case 'Y': if (grid_size_y > 1) --grid_size_y; break;

            case '[': if (multi_grid_split > 1) --multi_grid_split; break;
            case ']': if (multi_grid_split < 15) ++multi_grid_split; break;

            case '{': if (multi_grid_poly > 1) --multi_grid_poly; break;
            case '}': if (multi_grid_poly < 99) ++multi_grid_poly; break;
            case ')': multi_grid_poly =  0; break;
            case '!': multi_grid_poly =  3; break;
            case '@': multi_grid_poly =  7; break;
            case '#': multi_grid_poly = 12; break;
            case '$': multi_grid_poly = 18; break;
            case '%': multi_grid_poly = 24; break;
            case '^': multi_grid_poly = 30; break;
            case '&': multi_grid_poly = 36; break;
            case '*': multi_grid_poly = 42; break;
            case '(': multi_grid_poly = 48; break;

            case 'm': mode_switch(1); break;
            case 'M': mode_switch(-1); break;

            case '1': mode_set(0); break;
            case '2': mode_set(1); break;
            case '3': mode_set(3); break;

            case 'd': try_direct = !try_direct; reset_screen(); break;
            case 'b': no_blit = !no_blit; break;
            case 'u': swap_uv = !swap_uv; break;
            case 'w': wireframe = !wireframe; break;

            case 't': show_times = !show_times; break;
            case 'T': output_time(); break;
            case 'i': output_info = !output_info; break;

            case '>': if (scale < 8) ++scale; break;
            case '<': if (scale > 0) --scale; break;

            case 'c': clear = !clear; break;

            case 27: case 'Q': return; break;
         }
      }
   }
}

#pragma off(unreferenced)
int main(int argc, char **argv)
{
   int i,j;

   if (argc >= 2) {
      // argv[1] = script
      // argv[2] = output file
      in = fopen(argv[1], "r");
      if (!in) {
         printf("Unable to open script file '%s'.\n", argv[1]);
         return 1;
      }
      if (argc >= 3) {
         out = fopen(argv[2], "w");
         if (!out) {
            printf("Unable to open output file '%s'.\n", argv[2]);
            return 1;
         }
      }
      if (argc >= 4)
         fprintf(out, "%s\n", argv[3]);

      check_delay = 1000;  // run each test for 10 s
      output_info = TRUE;
      show_times = FALSE;
   }

   init_graphic_data();
   start_graphics();

   doit();

   if (in) fclose(in);
   if (out) fclose(out);

   gr_close();
   return 0;
}
#pragma on(unreferenced)
