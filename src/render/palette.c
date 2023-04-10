/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/palette.c,v 1.15 2000/02/19 12:35:31 toml Exp $
// stupid little file to concentrate palette gnosis

#include <string.h>

#include <2d.h>

#include <palette.h>

#include <scrnman.h>
#include <status.h>
#include <dispatch.h>
#include <dispbase.h>
#include <vismsg.h>

#include <lgassert.h>
#include <dbasemsg.h>
#include <memall.h>
#include <string.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

int pal_bind_color(int r, int g, int b)
{
   uchar const *ipal = ScrnGetInvPalette();
   return ipal[gr_index_brgb(gr_bind_rgb(r,g,b))];
}

void pal_build_clut(ubyte (*pal)[3], ubyte* clut)
{
   int i; 
   for (i = 0; i < 256; i++)
   {
      clut[i] = pal_bind_color(pal[i][0],pal[i][1],pal[i][2]);
   }
}


void pal_send_msg()
{
   DispatchData msg = { kPaletteChange,};
   msg.data = grd_pal;
   DispatchMsg(kMsgVisual,&msg);
}


void pal_update(void)
{

   // if the palette is actually changing 
   if (grd_bpp == 8)
   {
      const uchar* newpal = ScrnGetPalette(); 
      // Blacken the screen, so it doesn't look stupid 
      if (memcmp(grd_pal,newpal,768) != 0)
      {
         char pal[768]; 
         // copy the palette before ScrnBlacken Nukes it
         memcpy(pal,newpal,sizeof(pal)); 
         ScrnBlacken(); 
         ScrnLoadPalette(pal,0,256); 
      }
   }

   ScrnUsePalette(); 
   pal_send_msg(); 

}




