// Source code for the Text Gadget

// THINGS STILL NEEDED:
// Cursors!!

// Gadgety stuff
#include <string.h>
#include <gadgets.h>
#include <textgadg.h>
#include <tngtextg.h>

// fonts
#include <fakefont.h>

Gadget *gad_text_create(Gadget *parent, Rect *dim, int z, ulong options, TNGStyle *sty, char *name)
{
   Gadget *retgad;
   TNG *temp_tng;
   Point dsize, dloc;

   temp_tng = (TNG *)Malloc(sizeof(TNG));

   dsize.x = RectWidth(dim);
   dsize.y = RectHeight(dim);
   dloc.x = parent->rep->abs_x + dim->ul.x;
   dloc.y = parent->rep->abs_y + dim->ul.y;
   tng_textgadget_init(NULL, temp_tng, sty, options, dsize, dloc); 

   gadget_create_setup(&retgad, parent, CLASS_TEXT, dim, z, name);
   Free(retgad->tng_data);
   temp_tng->ui_data = retgad;
   retgad->tng_data = temp_tng;

   // Let the TNG get another shot...
   tng_textgadget_init2(temp_tng);

   return (retgad);
}

Gadget *gad_textgadget_create_from_tng(void *ui_data, Point loc, TNG **pptng, TNGStyle *sty, ulong options, Point size)
{
   Rect newrect;
   Gadget *rgad, *g;
   char new_name[128];

   g = (Gadget *)ui_data;   
   newrect.ul = loc;
   newrect.lr.x = newrect.ul.x + size.x;
   newrect.lr.y = newrect.ul.y + size.y;
   strcpy(new_name, "textgadg-sub-");
   strcat(new_name, GD_NAME(g->rep));
   rgad = gad_text_create(g, &newrect, 0, options, sty, new_name);
   *pptng = rgad->tng_data;
   return(rgad);
}
