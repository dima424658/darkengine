#include <conio.h>

#include <dev2d.h>
#include <mprintf.h>

typedef struct {
   short x,y;
} point;

static uchar bm_bits[32*32*2];
static grs_bitmap bm={
   bm_bits, BMT_FLAT8, 0, 0, 32, 32, 32, 5, 5
};

static uchar clut[256];
static uchar pal[768];

point pos={0,0};
point size={0,0};
int fill_type=0;
int do_box=TRUE;
int do_bm=FALSE;

int get_input(void (*cb)())
{
   uchar c=getch();
   if (cb!=NULL)
      cb();
   switch (c) {
      case '2': size.y++; break;
      case '8': size.y--; break;
      case '4': size.x--; break;
      case '6': size.x++; break;
      case 'w': pos.y-=5; break;
      case 'x': pos.y+=5; break;
      case 'a': pos.x-=5; break;
      case 'd': pos.x+=5; break;
      case 'b': do_box^=1; break;
      case 'm': do_bm^=1; break;
      case 'f': fill_type++;
                if (fill_type==FILL_TYPES) fill_type=0;
                break;
      case 't': bm.flags^=BMF_TRANS; break;
      case 'q': return 0;
   }
   return 1;
}

void clear()
{
   int fc_save=grd_gc.fcolor;
   grd_gc.fcolor = 0;
   gr_rect(0, 0, grd_bm.w, grd_bm.h);
   grd_gc.fcolor = fc_save;
}

void do_gfx()
{
   grd_gc.fill_type=fill_type;
   if (fill_type==FILL_CLUT)
      grd_gc.fill_parm = (ulong )clut;
   if (fill_type==FILL_SOLID)
      grd_gc.fill_parm = grd_gc.fcolor;
   if (do_bm)
      gr_bitmap(&bm, pos.x, pos.y);
   if (do_box)
      gr_box(pos.x, pos.y, pos.x+size.x, pos.y+size.y);
   grd_gc.fill_type=FILL_NORM;
}

void init()
{
   int i;
   for (i=0; i<bm.row*bm.h; i++)
      bm.bits[i]=i;
   for (i=0; i<256; i++) {
      clut[i]=255-i;
      tluc8tab[i] = clut;
   }
   for (i=0; i<768; i++)
      pal[i] = i;
   gr_set_default_pal(pal);
}

main ()
{
   grs_screen *s;

   gd_svga_init();
   init();
   gr_set_mode(GRM_320x400x8, TRUE);
   s = gr_alloc_screen(grd_cap->w, grd_cap->h);
   gr_set_screen(s);
   grd_gc.fcolor=7;
   grd_gc.bcolor=15;
   do {
      do_gfx();
   } while (get_input(clear));
   mprintf("hi there.\n");
   mprintf("bye.\n");
   gd_close();
}


