#include <gadgets.h>
#include <plain.h>
#include <tngplain.h>
#include <_ui.h>
#include <dbg.h>

Gadget *gad_plain_create(Gadget *parent, Rect *dim, int z, char *name)
{
   Gadget *retgad;
   Point size;

   gadget_create_setup(&retgad, parent, CLASS_PLAIN, dim, z, name);

   // Fill in class-specific data
   size.x = RectWidth(dim);  size.y = RectHeight(dim);
   tng_plain_init(retgad, retgad->tng_data, size);

   return (retgad);
}

Gadget *gad_plain_create_from_tng(void *ui_data, Point loc, TNG **pptng, Point size)
{
   Rect newrect;
   Gadget *rgad;

   newrect.ul = loc;
   newrect.lr.x = newrect.ul.x + size.x;
   newrect.lr.y = newrect.ul.y + size.y;
   rgad = gad_plain_create((Gadget *)ui_data, &newrect, 0, GD_NAME(((Gadget *)ui_data)->rep));

   *pptng = rgad->tng_data;
   return(rgad);
}
