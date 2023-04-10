/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnovls.cpp,v 1.9 1998/10/05 17:27:49 mahk Exp $

//
// Bitmap screen overlay system (health bars, chat windows, etc.)
//

#include <string.h>

#include <appagg.h>

#include <2d.h>
#include <rect.h>
#include <config.h>

#include <scrnovls.h>

// hack for lit hand art
#include <objlight.h>
#include <playrobj.h>
#include <mprintf.h>

#include <resapi.h>
#include <imgrstyp.h>


// Must be last header 
#include <dbmem.h>


#define INTERFACE_PATH "intrface\\"

static IRes *ovls[SCRNOVLS_MAX_ELEM];
static Point ovls_pos[SCRNOVLS_MAX_ELEM];
static int ovls_on[SCRNOVLS_MAX_ELEM];

static float light_min=0.3, light_mul=2.0;

void ScreenOverlaysInit(void)
{
   char fname[40], cfg_name[40], cfg_strs[2][40];
   int i, cnt, cfg_vals[6];

   AutoAppIPtr(ResMan);

   for (i=0; i<SCRNOVLS_MAX_ELEM; i++)
      ovls_on[i] = 0;
   cnt = SCRNOVLS_MAX_ELEM;
   config_get_value("interface_on", CONFIG_INT_TYPE, (config_valptr)ovls_on, &cnt);

   for (i=0; i<SCRNOVLS_MAX_ELEM; i++)
   {  // lets just get this out of the way now, in case things dont work out
      ovls[i] = NULL;
      
      strcpy(cfg_strs[0], "NONE");
      strcpy(cfg_strs[1], "NONE");
      cfg_vals[0] = cfg_vals[1] = cfg_vals[2] = cfg_vals[3] = cfg_vals[4] = cfg_vals[5] = 0;

      sprintf(cfg_name, "interface_%d", i);
      if (config_get_raw(cfg_name, fname, 39))
      {
         sscanf(fname, "%s %s", cfg_strs[0], cfg_strs[1]);

         sprintf(cfg_name, "interface_pos_%d", i);
         cnt = 6;
         config_get_value(cfg_name, CONFIG_INT_TYPE, (config_valptr)cfg_vals, &cnt);

         if (grd_canvas->bm.w >= 640)
         {
            strcpy(fname, cfg_strs[0]);
            ovls_pos[i].x = (short)cfg_vals[0];
            ovls_pos[i].y = (short)cfg_vals[1];
         }
         else if (grd_canvas->bm.w == 512)
         {
            strcpy(fname, cfg_strs[1]);
            ovls_pos[i].x = (short)cfg_vals[2];
            ovls_pos[i].y = (short)cfg_vals[3];
         }
         else
         {
            strcpy(fname, cfg_strs[1]);
            ovls_pos[i].x = (short)cfg_vals[4];
            ovls_pos[i].y = (short)cfg_vals[5];
         }

         if (strcmp(fname, "NONE"))
         {
            strcat(fname, ".pcx");
            ovls[i] = pResMan->Bind(fname,
                                    RESTYPE_IMAGE,
                                    NULL,
                                    INTERFACE_PATH);
         }
      }
   }

   if (config_get_int("light_min",&i))
      light_min=i/100.0;
   if (config_get_int("light_mul",&i))
      light_mul=i;
}

// free the resources!
void ScreenOverlaysFree(void)
{
   for (int i=0; i<SCRNOVLS_MAX_ELEM; i++)
      if (ovls[i] != NULL)
         SafeRelease(ovls[i]);
}

// draw the screen overlays (could also do animation or whatever)
void ScreenOverlaysUpdate(void)
{
   for (int i=0; i<SCRNOVLS_MAX_ELEM; i++)
      if (ovls_on[i])
         if (ovls[i] != NULL)
         {
            grs_bitmap *bm = (grs_bitmap *) ovls[i]->Lock();
            if (!bm) {
               Warning(("ScreenOverlaysUpdate: overlay with no data!\n"));
               continue;
            }
            gr_set_bitmap_format(bm, (ushort)BMF_TRANS);
            if (ovls_on[i]==9) // secret lit hand art code
            {
               float l_level=compute_object_lighting(PlayerObject());
               uchar *use_clut=gr_get_light_tab();
               int cl_lvl;

               l_level*=light_mul; 
               if (l_level<light_min) l_level=light_min; else if (l_level>1.0) l_level=1.0;
               cl_lvl=l_level*16; if (cl_lvl>=16) cl_lvl=15;
               gr_clut_bitmap(bm, ovls_pos[i].x, ovls_pos[i].y, use_clut+(cl_lvl*256));
            }
            else
               gr_bitmap(bm, ovls_pos[i].x, ovls_pos[i].y);
            ovls[i]->Unlock();
         }
}

// turn individual overlays on or off, or toggle them
void ScreenOverlaysChange(int which, int mode)
{
   if (which >= 0 && which < SCRNOVLS_MAX_ELEM)
   {
      if (mode == kScreenOverlaysToggle)
         ovls_on[which] = !ovls_on[which];
      else
         ovls_on[which] = mode;
   }
}

BOOL ScreenOverlayCurrentlyUp(int which)
{
   return ovls_on[which];
}
