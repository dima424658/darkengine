// $Header: x:/prj/tech/libsrc/dev2d/RCS/init.c 1.7 1998/03/12 10:42:15 KEVIN Exp $

#include <dbg.h>
#include <grd.h>
#include <grdev.h>
#include <detect.h>
#include <init.h>
#include <state.h>
#include <memall.h>
#include <tmpalloc.h>

/* start up dev2d system.  try to detect what kind of video hardware is
   present, call device-dependent initialization, and save state.
   returns same as gr_detect() 0 if all is well, or error code. */

int gd_init(void)
{
   int err;
   MemStack *tmp;

#ifdef _WIN32
   gd_use_com();
#endif

   if (gdd_active != 0) {
      Warning (("gd_init(): dev2d already active.\n"));
      return 0;
   }
   if (gdd_detect_func == NULL)
      Error (1,"gd_init(): drivers to install were not specified.\n");

   tmp=temp_mem_get_stack();
   if (tmp==NULL)
      if ((err=temp_mem_init(NULL))!=0)
         return err;

   err = gdd_detect_func(&grd_info);
   if (err != 0)
		return err;

   gr_push_video_state();
   gdd_active = 1;

   return 0;
}

int gd_close(void)
{
   if (gdd_active == 0)
      return 0;
   gr_pop_video_state();
   gr_close_device (&grd_info);
   gdd_active = 0;

   temp_mem_uninit();

#ifdef _WIN32
   gd_release_com();
#endif

   return 0;
}
