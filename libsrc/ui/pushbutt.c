#include <gadgets.h>
#include <pushbutt.h>
#include <tngpushb.h>
#include <dbg.h>
#include <string.h>

Gadget *gad_pushbutton_create(Gadget *parent, Rect *dim, int z, int type, void *disp_data, TNGStyle *sty, char *name)
{
   Gadget *retgad;
   Point size;

   gadget_create_setup(&retgad, parent, CLASS_PUSHBUTTON, dim, z, name);

   // Fill in class-specific data
   size.x = RectWidth(dim);  size.y = RectHeight(dim);
   tng_pushbutton_init(retgad, retgad->tng_data, sty, type, disp_data, size);

   return (retgad);
}

Gadget *gad_pushbutton_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, int button_type,
   void *display_data, Point size)
{
   Rect newrect;
   Gadget *rgad;
   char new_name[128];

   newrect.ul = loc;
   newrect.lr.x = newrect.ul.x + size.x;
   newrect.lr.y = newrect.ul.y + size.y;
   strcpy(new_name, "pb-sub-");
   strcat(new_name, GD_NAME(((Gadget *)ui_data)->rep));
   rgad = gad_pushbutton_create((Gadget *)ui_data, &newrect, 0, button_type, display_data, sty, new_name);
   *pptng = rgad->tng_data;
   return(rgad);
}

