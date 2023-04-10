#include <fcntl.h>
#include <io.h>
#include <string.h>

#include <region.h>
#include <gadgets.h>
#include <butarray.h>
#include <tngbarry.h>
#include <event.h>
#include <mouse.h>
#include <kbcook.h>
#include <2dres.h>

Gadget *gad_buttonarray_create(Gadget *parent, Point coord, int z, int msize_x, int msize_y, int window_x, int window_y,
   int bsize_x, int bsize_y, int num_sel, ushort options, TNGStyle *sty, char *name)
{
   Gadget *retgad;
   Rect dim;
   TNG *temp_tng;
   Point basize;
   Point msize, bsize, wsize;

   temp_tng = (TNG *)Malloc(sizeof(TNG));

   msize.x = msize_x;   msize.y = msize_y;
   wsize.x = window_x;  wsize.y = window_y;
   bsize.x = bsize_x;   bsize.y = bsize_y;
   tng_buttonarray_init(NULL, temp_tng, sty, options, msize, wsize, bsize, num_sel);

   dim.ul = coord;
   tng_buttonarray_size(temp_tng, &basize);
   dim.lr.x = dim.ul.x + basize.x;
   dim.lr.y = dim.ul.y + basize.y;

   gadget_create_setup(&retgad, parent, CLASS_BUTTONARRAY, &dim, z, name);
   Free(retgad->tng_data);
   temp_tng->ui_data = retgad;
   retgad->tng_data = temp_tng;

   // Let the TNG get another crack at things....
   tng_buttonarray_init2(retgad->tng_data);

   return (retgad);
}

errtype gad_buttonarray_addbutton_at(Gadget *g, int type, void *disp_data, int coord_x, int coord_y)
{
   return(tng_buttonarray_addbutton_at(g->tng_data, type, disp_data, coord_x, coord_y));
}

errtype gad_buttonarray_addbutton(Gadget *g, int type, void *disp_data)
{
   return(tng_buttonarray_addbutton(g->tng_data, type, disp_data));
}

errtype gad_buttonarray_setoffset(Gadget *g, int offset_x, int offset_y)
{
   return(tng_buttonarray_setoffset(g->tng_data, offset_x, offset_y));
}
