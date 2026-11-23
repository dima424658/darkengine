/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/cmdbutts.c,v 1.5 2000/02/19 12:27:51 toml Exp $

#include <string.h>
#include <stdlib.h>

#include <lgsprntf.h>
#include <config.h>


#include <cmdbutts.h>
#include <command.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


//
// Layout rects
//

void LayoutRectangles(Rect* bounds, Rect* rvec, int n, Point dims, Point sp)
{
   short bw = RectWidth(bounds);
   short bh = RectHeight(bounds);
   Point ul = bounds->ul;
   short w;
   short h;
   int i,j;
   int m;

   if (dims.x <= 0 || dims.y <= 0) 
   {
      Warning(("Tried to lay out rects for a zero-sided matrix\n"));
      return;
   }

   w = (bw - sp.x*(dims.x-1))/dims.x;
   h = (bh - sp.y*(dims.y-1))/dims.y;

   m = 0;
   for (j = 0; j < dims.y; j++)
      for (i = 0; i < dims.x; i++)
      {
         Rect* r = &rvec[m];
         r->ul.x = ul.x;
         if (dims.x > 1)
            r->ul.x += (bw - w)*i/(dims.x-1);
         r->lr.x = r->ul.x + w;
         
         r->ul.y = ul.y;
         if (dims.y > 1)
            r->ul.y += (bh - h)*j/(dims.y-1);
         r->lr.y = r->ul.y + h;

         m++;
         if (m >= n)
            goto out;

      }
out: ;
}

////////////////////////////////////////

