/*
 * $Source: x:/prj/tech/libsrc/dump/RCS/bmpdump.c $
 * $Revision: 1.3 $
 * $Author: JAEMZ $
 * $Date: 1996/12/19 12:15:32 $
 *
 * BMP dumper
 * 
*/

#include <io.h>
#include <lg.h>
#include <dev2d.h>
#include <dump.h>

int dmp_bmp_file(char *buff,char *prefix)
{
   int fd = dmp_find_free_file(buff,prefix,"bmp");

   if (fd <= 0) return -1;

   dmp_bmp_dump_screen(fd);

   return 0;
}

typedef struct {
   char bfType[2];
   ulong bfSize;
   ushort bfReserved1;
   ushort bfReserved2;
   ulong bfOffBits;
}  dps_file_head;


typedef struct {
   ulong biSize;
   ulong biWidth;
   ulong biHeight;
   ushort biPlanes;
   ushort biBitCount;
   ulong biCompression;
   ulong biSizeImage;
   ulong biXPelsPerMeter;
   ulong biYPelsPerMeter;
   ulong biClrUsed;
   ulong biClrImportant;
} dps_bitmap_head;

// Dumps an 8 or 16 or 15 bit image
int  dmp_bmp_dump_screen(int fp)
{
   dps_file_head bmp;
   dps_bitmap_head bhd;
   int i;

   // determine size and offbits after we know pixel depth
   bmp.bfType[0] = 'B';
   bmp.bfType[1] = 'M';
   bmp.bfReserved1 = 0;
   bmp.bfReserved2 = 0;

   bhd.biSize = sizeof(dps_bitmap_head);
   bhd.biWidth = grd_bm.w;
   bhd.biHeight = grd_bm.h;
   bhd.biPlanes = 1;
   bhd.biCompression = 0;
   bhd.biXPelsPerMeter = 0;
   bhd.biYPelsPerMeter = 0;
   bhd.biClrUsed = 0;
   bhd.biClrImportant = 0;

   if (grd_mode_info[grd_mode].bitDepth==8) {
      uchar *pal;
      grs_bitmap *buff;
      int rowsize;

      rowsize = ((grd_bm.w+3)/4)*4;

      bmp.bfSize = sizeof(bmp)+sizeof(bhd)+256*4+rowsize*grd_bm.h;
      bmp.bfOffBits = sizeof(bmp)+sizeof(bhd)+256*4;

      bhd.biSizeImage = rowsize*grd_bm.h;
      bhd.biBitCount = 8;

      // write the headers
      write(fp,&bmp,sizeof(bmp));
      write(fp,&bhd,sizeof(bhd));

      // Write the palette out
      pal = Malloc(256*4);
      for (i=0;i<256;++i) {
         pal[i*4] = grd_pal[i*3+2];
         pal[i*4+1] = grd_pal[i*3+1];
         pal[i*4+2] = grd_pal[i*3];
         pal[i*4+3] = 0;
      }
      write(fp,pal,4*256);
      Free(pal);

      // Make a flat 8 bitmap to transfer potentially non flat8 to
      buff = gr_alloc_bitmap(BMT_FLAT8, 0,rowsize,1);

      // works with non flat buffers
      for (i=grd_bm.h-1;i>=0;--i) {
         gr_get_bitmap(buff,0,i);
         write(fp,buff->bits,rowsize);
      }

      gr_free(buff);

   } else {
      int j;
      ushort col,*src;
      ulong gmask,bshift;
      uchar *dst,*buff;
      int rowsize;
      grs_bitmap *bmbuff;

      switch(grd_mode_info[grd_mode].bitDepth)
      {
         case 16:
         {
            gmask = 0x3F;
            bshift = 11;
            break;
         }
         case 15:
         {
            gmask = 0x1F;
            bshift = 10;
            break;
         }
         default:
         {
            return -1;
         }
      }

      rowsize = ((grd_bm.w*3+3)/4)*4;


      bmp.bfSize = sizeof(bmp)+sizeof(bhd)+rowsize*grd_bm.h;
      bmp.bfOffBits = sizeof(bmp)+sizeof(bhd);

      bhd.biSizeImage = rowsize*grd_bm.h;
      bhd.biBitCount = 24;

      // write the headers
      write(fp,&bmp,sizeof(bmp));
      write(fp,&bhd,sizeof(bhd));

      // Make a flat 16 bitmap to transfer potentially non flat data to
      buff = Malloc(rowsize);
      bmbuff = gr_alloc_bitmap(BMT_FLAT16, 0,rowsize,1);

      for(j=grd_bm.h-1;j>=0;--j) {
         gr_get_bitmap(bmbuff,0,j);
         src = (ushort *)(bmbuff->bits);
         dst = buff;
         for(i=0;i<grd_bm.w;++i) {
            col = *src;
            // could table-ize these for speed increase
            // write b,g,r
            *dst = ((ulong)(col&0x1F) * (ulong)0xFF)/(ulong)0x1F;
            dst++;
            *dst = ((ulong)((col>>5)&gmask) * (ulong)0xFF)/gmask;
            dst++;           
            *dst = ((ulong)((col>>bshift)&0x1F) * (ulong)0xFF)/(ulong)0x1F;
            dst++;
            src++;
         }
         // This is WAAAAY faster than individual writes
         write(fp,buff,rowsize);
      }

      gr_free(bmbuff);
      Free(buff);
   }

   close(fp);

   return 0;
}

