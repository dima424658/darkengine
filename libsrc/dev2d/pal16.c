// $Header: x:/prj/tech/libsrc/dev2d/RCS/pal16.c 1.3 1998/03/12 10:42:42 KEVIN Exp $
// Routines and data for 16 bit palette manipulation.

#include <grmalloc.h>
#include <grd.h>
#include <pal16.h>

// global data
ushort *grd_default_pal16_list[256] = {
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

ushort **grd_pal16_list = grd_default_pal16_list;

ushort *grd_pal16 = NULL;

uchar *grd_ipal16 = NULL;

// Routines for 16 bit palette manipulation
static void set_pal565_from_pal888(int start, int n, ushort *pal16, uchar *pal_data)
{
   int i;
   for (i=0; i<n; i++) {
      int r=pal_data[3*i];
      int g=pal_data[3*i+1];
      int b=pal_data[3*i+2];
      pal16[start+i] = ((r>>3)<<11)|((g>>2)<<5)|(b>>3);
   }
}

static void set_pal555_from_pal888(int start, int n, ushort *pal16, uchar *pal_data)
{
   int i;
   for (i=0; i<n; i++) {
      int r=pal_data[3*i];
      int g=pal_data[3*i+1];
      int b=pal_data[3*i+2];
      pal16[start+i] = ((r>>3)<<10)|((g>>3)<<5)|(b>>3);
   }
}

static void set_pal1555_from_pal888(int start, int n, ushort *pal16, uchar *pal_data)
{
   int i;
   for (i=0; i<n; i++) {
      int r=pal_data[3*i];
      int g=pal_data[3*i+1];
      int b=pal_data[3*i+2];
      pal16[start+i] = 0x8000|((r>>3)<<10)|((g>>3)<<5)|(b>>3);
   }
}



void gr_make_pal565_from_pal888(ushort *pal16, uchar *pal_data)
{
   set_pal565_from_pal888(0, 256, pal16, pal_data);
}

ushort *gr_alloc_pal565_from_pal888(uchar *pal_data)
{
   ushort *pal16 = (ushort *)gr_malloc(512);
   if (pal16!=NULL)
      gr_make_pal565_from_pal888(pal16, pal_data);
   return pal16;
}

void gr_make_pal555_from_pal888(ushort *pal16, uchar *pal_data)
{
   set_pal555_from_pal888(0, 256, pal16, pal_data);
}

ushort *gr_alloc_pal555_from_pal888(uchar *pal_data) {
   ushort *pal16 = (ushort *)gr_malloc(512);
   if (pal16!=NULL)
      gr_make_pal555_from_pal888(pal16, pal_data);
   return pal16;
}

void gr_make_pal1555_from_pal888(ushort *pal16, uchar *pal_data)
{
   set_pal1555_from_pal888(0, 256, pal16, pal_data);
}

ushort *gr_alloc_pal1555_from_pal888(uchar *pal_data) {
   ushort *pal16 = (ushort *)gr_malloc(512);
   if (pal16!=NULL)
      gr_make_pal1555_from_pal888(pal16, pal_data);
   return pal16;
}

// assumes bitmasks form either 565 or 555 rgb or bgr
void gr_make_pal16(int start, int n, ushort *pal16, uchar *pal_data, grs_rgb_bitmask *bitmask)
{
   uchar swap_pal[768];
   uchar *pal;

   if (bitmask->blue == 0x1f)
   {  // we're OK
      pal = pal_data;
   }
   else
   {  // need to swap red and blue
      int i;
      for (i=0; i<256; i++) {
         swap_pal[3*i] = pal_data[3*i+2];
         swap_pal[3*i+1] = pal_data[3*i+1];
         swap_pal[3*i+2] = pal_data[3*i];
      }
      pal = swap_pal;
   }

   if (bitmask->green == 0x3e0)
   {  // 15 bit
      set_pal555_from_pal888(start, n, pal16, pal);
   }
   else
   {  // 16 bit
      set_pal565_from_pal888(start, n, pal16, pal);
   }
}

uchar pal_bits[3*256];
uchar *grd_top_pal16=NULL;
uchar *grd_bot_pal16=NULL;

void gr_set_pal16(ushort *pal, int n) {
   int i;

   if ((n<256)&&(n>=0))
      grd_pal16_list[n]=pal;
   if ((n!=GRC_OPTIMIZED_PAL16)||(pal == NULL))
      return;

   if (grd_top_pal16 == NULL) {
      grd_bot_pal16 = (uchar *)((((ulong )pal_bits)+256)&(~255));
      grd_top_pal16 = grd_bot_pal16+256;
   }
   for (i=0; i<256; i++) {
      ushort entry=pal[i];
      grd_bot_pal16[i] = entry;
      grd_top_pal16[i] = entry>>8;
   }
}

#define rmask (0x1f)
#define gmask (0x1f<<5)
#define bmask (0x1f<<10)
void gr_make_ipal1555(uchar *ipal)
{
   int i;
   for (i=0; i<65536; i++) {
      int c = ((i&rmask)<<10)|(i&gmask)|((i&bmask)>>10);
      ipal[i]=grd_ipal[c];
   }
}

uchar *gr_alloc_ipal1555()
{
   uchar *ipal=(uchar *)gr_malloc(65536);
   if (ipal!=NULL)
      gr_make_ipal1555(ipal);
   return ipal;
}






