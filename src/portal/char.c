/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/char.c,v 1.2 2000/02/19 12:32:41 toml Exp $

// dumb character renderer
// only does numbers for now

#include <lg.h>
#include "char.h"
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static char *char_data[10] = {
"xxxx\n"
"x  x\n"
"x  x\n"
"x  x\n"
"xxxx\n",

"  x \n"
"  x \n"
"  x \n"
"  x \n"
"  x \n",

"xxxx\n"
"   x\n"
"xxxx\n"
"x   \n"
"xxxx\n",

"xxxx\n"
"   x\n"
" xxx\n"
"   x\n"
"xxxx\n",

"x  x\n"
"x  x\n"
"xxxx\n"
"   x\n"
"   x\n",

"xxxx\n"
"x   \n"
"xxxx\n"
"   x\n"
"xxxx\n",

"xxxx\n"
"x   \n"
"xxxx\n"
"x  x\n"
"xxxx\n",

"xxxx\n"
"   x\n"
"   x\n"
"   x\n"
"   x\n",

"xxxx\n"
"x  x\n"
"xxxx\n"
"x  x\n"
"xxxx\n",

"xxxx\n"
"x  x\n"
"xxxx\n"
"   x\n"
"   x\n"
};

void plot_pixel(uchar *dest, int row, int x, int y, int c)
{
   if (x >= 0 && x < row && y >= 0 && y < 200)
      dest[x+y*row] = c;
}

int draw_char(uchar *dest, int row, int c, int x, int y)
{
   if (c >= '0' && c <= '9') {
      int col = x;
      char *s = char_data[c - '0'];
      while (*s) {
         if (*s == '\n')
            col=x-1,++y;
         else if (*s == 'x')
            plot_pixel(dest,row,col,y,7);
         ++col;
         ++s;
      }
      return 6;
   } else 
      return 6;
}

void draw_string(uchar *dest, int row, int x, int y, char *str)
{
   while (*str)
      x += draw_char(dest, row, *str++, x, y);
}
