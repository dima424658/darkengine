/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/gpal.c $
 * $Revision: 1.3 $
 * $Author: JAEMZ $
 * $Date: 1998/02/13 10:25:48 $
 *
 * Gamma palette functions
 *
 */

#include <fix.h>
#include <gpal.h>
#include <pal_.h>

static uchar _table[256];
static float _gamma;
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
extern void gr_set_gamma_auto(int correct)
{
   bool val = (grd_pal_callbacks[PAL_GAMMA]!=NULL);
   if (correct<0) val = !val;
   else val = correct;

   grd_pal_callbacks[PAL_GAMMA]=(val?_correct:NULL);
}

// Sets the gamma value, but won't do anything if not on
void gr_set_gamma_value(float gamma)
{
   int i;
   fix gval= fix_from_float(gamma);

   if (gamma!=_gamma) {
      for (i=0;i<256;i++)
         _table[i]=(uchar )fix_rint(fix_pow(fix_make(i,0)/255,gval)*255);
      _gamma=gamma;
   }
}

// Returns whether gamma correction is on, and stuffs gamma if non-null
extern bool gr_get_gamma_state(float *gamma)
{
   if (gamma) *gamma = _gamma;
   return (grd_pal_callbacks[PAL_GAMMA]!=NULL);
}


// Set a range of indices to protect or not
// start to end inclusive
// protect==TRUE mean that index will not be contrast adjusted
extern void gr_gamma_protect_range(int start,int end,bool protect)
{
   while (start<=end) 
      _protect[start++]=protect;
}

// Set a list of indices to protect or not
extern void gr_gamma_protect_list(uchar *list,int num,bool protect)
{
   while (num) 
      _protect[--num]=protect;
}




