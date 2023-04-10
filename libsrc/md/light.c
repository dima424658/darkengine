#include <mprintf.h>
#include <matrix.h>
#include <r3d.h>
#include <md.h>

// Just set these directly
mxs_vector mdd_sun_vec;
float mdd_lt_amb;    // ambient value
float mdd_lt_diff;   // diffuse value
float mdd_lt_spec;   // specular value

// sun vector in object space
static mxs_vector obj_sun;
static bool light_invalid = TRUE;
static float amb;
static uchar ltype;

void md_light_set_type(uchar type)
{
   ltype = type;
}

// Recomputes constants based on type
void md_light_recompute()
{
   float scale;

   light_invalid = FALSE;

   // transform into object space
   r3_rotate_w2o(&obj_sun,&mdd_sun_vec);

   scale = mdd_lt_diff;
   if (ltype&MD_LT_SPEC) scale += mdd_lt_spec;

   // scale the vector by the diff
   // okay, so sue me, faking specular by saturating
   mx_scaleeq_vec(&obj_sun,scale);

   // set amb
   amb = (ltype&MD_LT_AMB)?mdd_lt_amb:0.0;
}


// starkly light from the side
void md_light_cback(int num,float *ivals,mds_light *lts,mxs_vector *pts,r3s_point *tpts)
{
   int i;
   float l;
   mxs_vector norm;

   // punt if no lighting set.
   if (ltype == MD_LT_NONE) return;

   if (light_invalid)
      md_light_recompute();

   // Diffuse and/or specular
   if (ltype&(MD_LT_DIFF|MD_LT_SPEC)) {
      for (i=0;i<num;++i) {
         norm.x = X_NORM(lts[i].norm);      
         norm.y = Y_NORM(lts[i].norm);      
         norm.z = Z_NORM(lts[i].norm);

         l = mx_dot_vec(&norm,&obj_sun);

         if (l<0)
            l = amb;
         else
            l += amb;

         ivals[i] = l;
      }
   } else {
      // Just ambient
      for (i=0;i<num;++i) {
         ivals[i] = amb;
      }
   }
}

// Recomputes light vals
void md_light_obj(int which)
{
   light_invalid=TRUE;
}  


// Initial lighting conditions
void md_light_init()
{
   // install the lighting routine
   md_set_light_callback(md_light_cback);
   md_set_light_obj_callback(md_light_obj);

   // Make default lighting be a soothing 50/50
   mdd_lt_amb  = .5;
   mdd_lt_diff = .5;
   mdd_lt_spec = 0;

   md_light_set_type(MD_LT_DIFF|MD_LT_AMB);

   // sun straight up
   mx_mk_vec(&mdd_sun_vec,0,0,1);
}
