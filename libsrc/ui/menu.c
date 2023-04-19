// Source code for Menu Gadget

// Gadgety stuff
#include <string.h>
#include <gadgets.h>
#include <menu.h>
#include <tngmenu.h>
#include <mouse.h>

void gad_menu_up(TNG *ptng)
{
   Gadget *retgad;
   Rect dim;
   GadgetData *gd;
   RectCallback fn;
   menu_struct *ms;

   gd = (GadgetData *)Malloc(sizeof(GadgetData));
   retgad = (Gadget *)ptng->ui_data;
   ms = (menu_struct *)TNG_MN(ptng)->ui_struct;

   // Fill in the gadget data info
   gd->name = (char *)Malloc((strlen(ms->name) + 1) * sizeof(char));
   strcpy(gd->name, ms->name);
   gd->g = retgad;

   switch (retgad->parent->rep->device_type)
   {
      case DISPLAY_VGA:
      case DISPLAY_SVGA:
      case DISPLAY_MODEX:
         fn = &gadget_tng_vga_expose;
         break;
      case DISPLAY_MONO:
         fn = &gadget_tng_mono_expose;
         break;
   }

   // Create that durned rep
   dim.ul = TNG_MN(ptng)->coord;
   dim.lr.x = dim.ul.x + TNG_MN(ptng)->size.x;
   dim.lr.y = dim.ul.y + TNG_MN(ptng)->size.y;
   Spew(DSRC_UI_Menu, ("menu dim = (%d,%d)(%d,%d)\n",RECT_EXPAND_ARGS(&dim)));

   retgad->rep = (Region *)Malloc(sizeof(Region));
   region_create(retgad->parent->rep, retgad->rep, &dim, ms->z, 0,
      REG_USER_CONTROLLED | AUTOMANAGE_FLAG | STENCIL_CLIPPING | OBSCURATION_CHECK,
      fn, NULL, NULL, gd);

   TNG_MN(ptng)->popped_up = TRUE;

   // Install the general pushbutton handler
   uiInstallRegionHandler(retgad->rep, UI_EVENT_MOUSE, &gadget_tng_mouse_handler, retgad, &(retgad->handler_id));
   uiInstallRegionHandler(retgad->rep, UI_EVENT_KBD_COOKED, &gadget_tng_keyboard_handler, retgad, &(retgad->handler_id));

   // Grab focus!
   uiGrabFocus(retgad->rep, UI_EVENT_KBD_COOKED);

   // Display
   gadget_display(retgad,NULL);

//   mouse_put_xy(dim.ul.x + ((dim.lr.x - dim.ul.x) / 2), dim.ul.y + ((dim.lr.y - dim.ul.y) / 2));
   uiShowMouse(NULL);
}

void gad_menu_down(TNG *ptng)
{
   Gadget *g;
   g = (Gadget *)ptng->ui_data;
   Spew(DSRC_UI_Menu, ("About to destroy menu!\n"));
   uiReleaseFocus(g->rep, UI_EVENT_KBD_COOKED);
   region_destroy(g->rep,TRUE);
   g->rep = NULL;
   TNG_MN(ptng)->popped_up = FALSE;
}

Gadget *gad_menu_create(Gadget *parent, Point *coord, int z, TNGStyle *sty, int width, char *name)
{
   Gadget *retgad;
   TNG *mn_tng;
   menu_struct *ms;

   retgad = (Gadget *)Malloc(sizeof(Gadget));
   mn_tng = (TNG *)Malloc(sizeof(TNG));
   ms = (menu_struct *)Malloc(sizeof(menu_struct));

   // Fill out basic gadgety info
   retgad->tng_data = mn_tng;
   retgad->draw_parts = TNG_ALLPARTS;
   retgad->class = CLASS_MENU;
   retgad->parent = parent;
   retgad->device_data = parent->device_data;
   retgad->conversion = parent->conversion;
   retgad->handler_id = -1;
   retgad->destroy_func = NULL;

   ms->z = z;
   ms->name = (char *)Malloc((1 + strlen(name)) * sizeof(char));
   strcpy(ms->name, name);

   tng_menu_init(retgad, retgad->tng_data, sty, *coord, width, &gad_menu_up, &gad_menu_down, ms);

   return (retgad);
}

errtype gad_menu_add_line(Gadget *menu, char *label, hotkey_callback f, short keycode, ulong context, void *user_data, char *help_text)
{
   return(tng_menu_add_line(menu->tng_data, label, f, keycode, context, user_data, help_text));
}

errtype gad_menu_add_submenu(Gadget *menu, char *label, Gadget *sub_menu)
{
   return(tng_menu_add_submenu(menu->tng_data, label, sub_menu->tng_data));
}

errtype gad_menu_popup(Gadget *menu)
{
   return(tng_menu_popup(menu->tng_data));
}

errtype gad_menu_popup_at_mouse(Gadget *menu)
{
   return(tng_menu_popup_at_mouse(menu->tng_data));
}

errtype gad_menu_popdown(Gadget *menu)
{
   TNG_MN(menu->tng_data)->popdown_func(menu->tng_data);
   return(OK);
}
