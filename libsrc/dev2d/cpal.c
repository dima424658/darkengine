/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/cpal.c $
 * $Revision: 1.4 $
 * $Author: JAEMZ $
 * $Date: 1998/02/13 10:25:32 $
 *
 * Contrast palette functions
 *
 */

#include <math.h>
#include <cpal.h>
#include <pal_.h>

static uchar _table[256];
static float _about;
static float _scale;
static bool _protect[256];

// Gamma correct the range
static void _correct(uchar *dst,uchar *src,int start,int n)
{
   int i;

   // Rescale to index space
   start /= 3;

   for (i=start;i<start+n;++i) {
      if (!_protect[i]) {
         *dst++ = _table[*src++];
         *dst++ = _table[*src++];
         *dst++ = _table[*src++];
      } else {
         *dst++ = *src++;
         *dst++ = *src++;
         *dst++ = *src++;
      }
   }
}

// Sets the the gamma auto correct state, toggles if negative
// else you should set TRUE or FALSE
extern void gr_set_contrast_auto(int correct)
{
   bool val = (grd_pal_callbacks[PAL_CONTRAST]!=NULL);
   if (correct<0) val = !val;
   else val = correct;

   grd_pal_callbacks[PAL_CONTRAST]=(val?_correct:NULL);
}


// Given a source value, the average to contrast around, and 
// the contrast amount, return a new one.
// Contrast goes from (0..1..)
// Cribbed from Contrast library
static int _ContrastValue(float val)
{
   float del;

   if (val>_about) {
      float k = 255.5 - _about;
      // Transform, scale, then transform back
      del = (val-_about)*(k/(255.5-val));
      del *= _scale;
      val = ((255.5*del)+(k*_about))/(k+del);
   } else {
      float k = -.5 - _about;
      // Transform, scale, then transform back
      del = (val-_about)*(k/(-.5-val));
      del *= _scale;
      val = ((-.5*del)+(k*_about))/(k+del);
   }

   val = floor(val + .5);
   return val;
}


// Sets the contrast value, but won't do anything if not on
// About is what value to scale about, scale is the value.
extern void gr_set_contrast_value(float about,float scale)
{
   int i;

   if (about!=_about || scale!=_scale) {
      _about=about;
      _scale=scale;
      for (i=0;i<256;i++) {
         _table[i]=(uchar)_ContrastValue(i);
      }
   }
}

// Returns whether contrast is on, and stuffs gamma if non-null
extern bool gr_get_contrast_state(float *about,float *scale)
{
   if (about) *about = _about;
   if (scale) *scale = _scale;
   return (grd_pal_callbacks[PAL_CONTRAST]!=NULL);
}

// Set a range of indices to protect or not
// start to end inclusive
// protect==TRUE mean that index will not be contrast adjusted
extern void gr_contrast_protect_range(int start,int end,bool protect)
{
   while (start<=end) 
      _protect[start++]=protect;
}

// Set a list of indices to protect or not
extern void gr_contrast_protect_list(uchar *list,int num,bool protect)
{
   while (num--) 
      _protect[num]=protect;
}






