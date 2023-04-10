#include <lg.h>
#include <error.h>
#include <mouse.h>
#include <mprintf.h>

void (*ui_mouse_convert)(short *px, short *py, bool down) = NULL;
void (*ui_mouse_convert_round)(short *px, short *py, bool down) = NULL;

errtype ui_mouse_do_conversion(short *pmx, short *pmy, bool down)
{
   if (ui_mouse_convert != NULL)
      ui_mouse_convert(pmx,pmy,down);
   return(OK);
}

errtype ui_mouse_get_xy(short *pmx, short *pmy)
{
   errtype retval;
   retval = mouse_get_xy(pmx,pmy);
   ui_mouse_do_conversion(pmx,pmy,TRUE);
   return(retval);
}

errtype ui_mouse_put_xy(short pmx, short pmy)
{
   errtype retval;
   ui_mouse_do_conversion(&pmx,&pmy,FALSE);
   retval = mouse_put_xy(pmx,pmy);
   return(retval);
}

errtype ui_mouse_constrain_xy(short xl, short yl, short xh, short yh)
{
   if (ui_mouse_convert == NULL)
      return(mouse_constrain_xy(xl,yl,xh,yh));
   else
   {
      short uxl,uyl,uxh,uyh;
      uxl=xl;
      uyl=yl;
      uxh=xh;
      uyh=yh;
      ui_mouse_convert_round(&uxl,&uyl,FALSE);
      ui_mouse_convert_round(&uxh,&uyh,FALSE);
      return(mouse_constrain_xy(uxl,uyl,uxh,uyh));
   }
}
