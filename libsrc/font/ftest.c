#include <conio.h>

#include <grtb.h>
#include <font.h>
#include <mprintf.h>

typedef struct {
   short x,y;
} point;

point pos={0,0};
point size={20,20};
int fill_type=0;
int do_box=0;
int do_font=0;
int do_line=1;
#define FONT_HEIGHT 8
#define FONT_WIDTH 8
#define FONT_ROW 16
#define FONT_RANGE 2
#define FONT_START 17
#define FONT_STRUCT_SIZE 86
uchar font_bits[FONT_HEIGHT * FONT_ROW] = {
 9, 0, 0, 0, 0, 0, 0, 9,   0, 0, 0, 12, 12, 0, 0, 0,
 0, 9, 0, 0, 0, 0, 9, 0,   0, 0, 0, 24, 24, 0, 0, 0,
 0, 0, 9, 0, 0, 9, 0, 0,   0, 0, 0, 24, 24, 0, 0, 0,
 0, 0, 0, 12, 12, 0, 0, 0,   0, 0, 0, 9, 9, 0, 0, 0,
 0, 0, 0, 12, 12, 0, 0, 0,   0, 0, 0, 9, 9, 0, 0, 0,
 0, 0, 9, 0, 0, 9, 0, 0,   0, 0, 0, 24, 24, 0, 0, 0,
 0, 9, 0, 0, 0, 0, 9, 0,   0, 0, 0, 24, 24, 0, 0, 0,
 9, 0, 0, 0, 0, 0, 0, 9,   0, 0, 0, 24, 24, 0, 0, 0
};
uchar clut[256];
char s[4]={FONT_START+1, FONT_START, FONT_START+1, 0};
uchar font_data[FONT_HEIGHT*FONT_ROW + FONT_STRUCT_SIZE + 2*FONT_RANGE];
grs_font *fontp;
void font_init()
{
   int i;
   uchar *bits_ptr = font_data + FONT_STRUCT_SIZE + 2*FONT_RANGE;
   ushort *off_tab_ptr = (ushort *)(font_data + FONT_STRUCT_SIZE - 2);

   for (i=0; i<FONT_HEIGHT*FONT_ROW; i++)
      bits_ptr[i] = font_bits[i];
   for (i=0; i<=FONT_RANGE; i++)
      off_tab_ptr[i] = i*FONT_WIDTH;
   fontp = (grs_font *)font_data;
   fontp->id=0xcccc;
   fontp->min=FONT_START;
   fontp->max=FONT_START+FONT_RANGE-1;
   fontp->buf=FONT_STRUCT_SIZE+2*FONT_RANGE;
   fontp->w=FONT_ROW;
   fontp->h=FONT_HEIGHT;
}

int get_input(void (*cb)())
{
   uchar c=getch();
   if (cb!=NULL)
      cb();
   switch (c) {
      case '2': size.y+=1; break;
      case '8': size.y-=1; break;
      case '4': size.x-=1; break;
      case '6': size.x+=1; break;
      case 'w': pos.y-=5; break;
      case 'x': pos.y+=5; break;
      case 'a': pos.x-=5; break;
      case 'd': pos.x+=5; break;
      case 'b': do_box^=1; break;
      case 'm': do_font^=1; break;
      case 'f': fill_type++;
                if (fill_type==FILL_TYPES) fill_type=0;
                break;
      case 'q': return 0;
   }
   return 1;
}

void clear()
{
   gr_clear(0);
}

void do_gfx()
{
   grd_gc.fill_type=fill_type;
   if (fill_type==FILL_CLUT)
      grd_gc.fill_parm = (ulong )clut;
   if (fill_type==FILL_SOLID)
      grd_gc.fill_parm = grd_gc.fcolor;
   grd_gc.fcolor=0xffff;
   if (do_line)
      gr_int_line(pos.x, pos.y, pos.x+size.x, pos.y+size.y);
   if (do_box)
      gr_box(pos.x, pos.y, pos.x+size.x, pos.y+size.y);
   grd_gc.fcolor=0xf800;
   if (do_font)
//      gr_font_scale_string(fontp, s, pos.x, pos.y, size.x, size.y);
      gr_font_string(fontp, s, pos.x, pos.y);
   grd_gc.fill_type=FILL_NORM;
}

void init()
{
   int i;
   font_init();
   for (i=0; i<256; i++) {
      clut[i]=255-i;
      tluc8tab[i] = clut;
   }

   gr_set_pal16(gr_alloc_pal565_from_pal888(grd_pal),0);
   grd_light_table_size = 16;
}

main ()
{
   tb_uvbe_init();
   tb_default_mode = GRM_640x480x16;
   tb_init();

   init();
   tb_start_frame();
   grd_gc.fcolor=0xffff;
   grd_gc.bcolor=0;
   do {
      do_gfx();
   } while (get_input(clear));
}


