/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/ctrast.c,v 1.12 2000/03/19 15:18:43 patmac Exp $

#include <string.h>
#include <stdio.h>
#include <stdlib.h>  // atof is here in watcom...
#include <math.h>

#include <timer.h>
#include <matrix.h>
#include <config.h>
#include <dev2d.h>
#include <mprintf.h>

#include <scrnman.h>
#include <command.h>
#include <ctrast.h>
#include <simtime.h>

#include <gamma.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Whether or not to automatically check for contrast about value
static bool _auto_about=TRUE;


// For blinding flashes of light
// old scale value before we slam it
static bool in_flash=FALSE;
static float time_scale=1;
static int frame_spacing=1;
static float flash_scale;        // currently set scale, used by flash

// Get image average rgb, given the number of samples
// on one half line of the screen (so 4*samples*samples) total
// and pointy (fraction of the screen to look at, centered
// so (1 is whole screen, and .5 is middle quarter of screen)
static float get_image_av(int samples,float pointy)
{
   int x,y;
   mxs_vector av;
   uchar *col;
   float weight=0;
   int xstart,ystart;
   int xend,yend;
   int space;
   int pix;
   int hw,hh;

   mx_zero_vec(&av);

   hw = grd_visible_canvas->bm.w/2;
   space = hw/samples;
   xstart = hw - pointy*space*(hw/space);
   xend = hw*(1.0+pointy);

   hh = grd_visible_canvas->bm.h/2;
   ystart = hh - pointy*space*(hh/space);
   yend = hh*(1.0+pointy);

   for (y=ystart;y<yend;y+=space) {
      for (x=xstart;x<xend;x+=space) {
         pix = gr_get_pixel(x,y);
         col = grd_pal+pix*3;
         av.x += col[0];
         av.y += col[1];
         av.z += col[2];
         weight += 1;
      }
   }
   return (av.x+av.y+av.z)/(3.0*weight);
}

// Update the contrast.  Do this right before the blit.
void ContrastUpdate()
{
   static long last_time=0;
   long new_time;
   float dt;

   float current = get_image_av(8,.5);
   static int blarg=8;
   float scale,about;
   
   // This mostly fails, so is at the top
   if (--blarg > 0) return;

   if (!_auto_about) return;
   if (!IsSimTimePassing()) return;

   if (!gr_get_contrast_state(&about,&scale)) return;

   // Use averaging in linear space
   // and this dt should really be time since last check
   // not fixed

   new_time = tm_get_millisec();
   dt = time_scale*(new_time-last_time)/3000.0;
   last_time = new_time;

   if (in_flash) {      
      // slow recovery
      float ftime=dt*.3;
      scale = (scale+flash_scale*ftime)/(ftime+1.0);
      if (fabs(scale-flash_scale) < .5) {
         scale = flash_scale;
         in_flash=FALSE;
      }
   }
   
   about = (about+current*dt)/(dt+1);

   //   mprintf("about %g, scale %g\n",about,scale);
   
   gr_set_contrast_value(about,scale);
   gr_set_pal(0,256,grd_pal);
   blarg=8;
   config_get_int("contrast_frame",&blarg);
}




// Call this after you do the flash effect.
// Makes it look like you've been blinded by something
// super bright.
void ContrastFlash(void)
{
   in_flash=TRUE;
   gr_set_contrast_value(1,.001);
}


// Set the contrast time scale
static void contrast_time_scale(char *val)
{
   time_scale=atof(val);
   //   mprintf("time_scale is %g\n",time_scale);
}



// Set the contrast and refresh
static void contrast(char* val)
{
   float about,scale;
   int outval;

   outval = sscanf(val,"%f %f",&about,&scale);

   if (strnicmp(val,"off",3)==0) {
      gr_set_contrast_auto(FALSE);
      in_flash=FALSE;
   } else if (strnicmp(val,"on",2)==0) {
      gr_set_contrast_auto(TRUE);
      // Set contrast to non-flash values, and turn off flash
      gr_get_contrast_state(&scale,&about);
      gr_set_contrast_value(flash_scale,about);
      in_flash=FALSE;
   } else {

      _auto_about = (strnicmp(val,"auto",4)==0)?TRUE:FALSE;
      if (_auto_about) {
         about = 25;      // good starting value
         scale = atof(val+5);
         outval=2;
      } 
      
      // Only do anything if we can parse it
      if (outval==2) {
         flash_scale = scale;
         gr_set_contrast_value(about,scale);
         gr_set_contrast_auto(TRUE);
      }
   }

   // Set the palette to see the change if it's safe
   if (ScrnGetRes()!=SCR_NOMODE) {
      gr_set_pal(0,256,grd_pal);
   }
}

EXTERN void set_hardware_gamma_level(float level);
float g_gamma = DEFAULT_GAMMA;

// force the gamma value to get propogated through
void gamma_display(void)
{
   // handle 8-bit case
   if (g_gamma == 1.0) {
      gr_set_gamma_auto(FALSE);
   } else {
      gr_set_gamma_value(g_gamma);
      gr_set_gamma_auto(TRUE);
   }

   // Set the palette to see the change if it's safe
   if ((ScrnGetRes()!=SCR_NOMODE) && (grd_bpp==8)) {
      gr_set_pal(0,256,grd_pal);
   }

   // handle hardware
   set_hardware_gamma_level(g_gamma);
}

// call if g_gamma has gotten updated
void gamma_update(void)
{
   config_set_float_from_var("gamma", g_gamma);
   gamma_display();
}

// load an initial gamma value
void gamma_load(void)
{
   config_get_float("gamma", &g_gamma);
   gamma_display();
}

// Set the contrast and refresh
static void gamma(char* val)
{
   if (!strcmp(val, "off"))
      g_gamma = 1.0;
   else
      sscanf(val,"%f",&g_gamma);
   gamma_update();
}

static void gamma_delta(float val)
{
   g_gamma += val;
   if (g_gamma < MIN_GAMMA) g_gamma = MIN_GAMMA;
   if (g_gamma > MAX_GAMMA) g_gamma = MAX_GAMMA;
   gamma_update();
}

#define GAMMA_STEP 0.025

static void
gamma_up_cmd( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      if ( dir > 0 ) {
         gamma_delta( -GAMMA_STEP );
      } else {
         gamma_delta( GAMMA_STEP );
      }
   }
}

static void
gamma_down_cmd( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      if ( dir < 0 ) {
         gamma_delta( -GAMMA_STEP );
      } else {
         gamma_delta( GAMMA_STEP );
      }
   }
}


static Command commands[] = 
{
   { "contrast",FUNC_STRING, contrast, "Set the contrast",HK_ALL },
   { "gamma",FUNC_STRING, gamma, "Set the gamma",HK_ALL },
   { "gamma_delta", FUNC_FLOAT, gamma_delta, "Add float to gamma",HK_ALL },
   { "gamma_up", FUNC_INT, gamma_up_cmd, "increase gamma", HK_ALL },
   { "gamma_down", FUNC_INT, gamma_down_cmd, "decrease gamma", HK_ALL },
   { "flash",FUNC_VOID, ContrastFlash, "Make a blinding flash",HK_ALL},
   { "contrast_time_scale",FUNC_STRING, contrast_time_scale, "contrast time scale",HK_ALL}
};

// If the contrast config variable is set, use that,
// else use the defaults we like.  For shipping, this
// should be done by wiring to the options panel.
// HACK HACK HACK
void ContrastInit(void)
{
   char buffer[80];
   if (config_is_defined("contrast")) {
      config_get_raw("contrast",buffer,79);
      contrast(buffer);
   } else {
      contrast("auto 2");
   }

   // Set at init time
   config_get_float("contrast_time_scale",&time_scale);

   gamma_load();

   COMMANDS(commands,HK_EDITOR);
}

// Terminate the Contrast System
void ContrastTerm(void)
{
}
