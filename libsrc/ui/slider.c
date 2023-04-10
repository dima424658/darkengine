#include <string.h>
#include <gadgets.h>
#include <slider.h>
#include <event.h>
#include <_ui.h>
#include <dbg.h>
#include <tngslidr.h>

Gadget *gad_slider_create(Gadget *parent, Rect *dim, int z, int alignment, int increment,
   int min, int max, TNGStyle *sty, char *name)
{
   return(gad_slider_create_full(parent, dim, z, alignment, increment, min, max, sty, name,
      0, 0, 0, 0, 0));
}

Gadget *gad_slider_create_full(Gadget *parent, Rect *dim, int z, int alignment, int increment,
   int min, int max, TNGStyle *sty, char *name, Ref res_left, Ref res_right, Ref res_up,
   Ref res_down, Ref res_slider)
{
   Gadget *retgad;
//   TNG *sl_tng;
   Point size;

   gadget_create_setup(&retgad, parent, CLASS_SLIDER, dim, z, name);

   size.x = RectWidth(dim);
   size.y = RectHeight(dim);
   tng_slider_full_init(retgad, retgad->tng_data, sty, alignment, min, max,
      min, increment, size, res_left, res_right, res_up, res_down, res_slider);

   return (retgad);
}

char new_name[80];

Gadget *gad_slider_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, int alignment, int min, int max,
   int value, int increment, Point size)
{
   Rect newrect;
   Gadget *rgad;

   newrect.ul = loc;
   newrect.lr.x = newrect.ul.x + size.x;
   newrect.lr.y = newrect.ul.y + size.y;
   strcpy(new_name,"slider-sub-");
   strcat(new_name,GD_NAME(((Gadget *)ui_data)->rep));
   rgad = gad_slider_create((Gadget *)ui_data, &newrect, 0, alignment, increment, min, max, sty, new_name);
//      GD_NAME(((Gadget *)ui_data)->rep));
   TNG_SL(rgad->tng_data)->value = value;
   *pptng = rgad->tng_data;
   return(rgad);
}

