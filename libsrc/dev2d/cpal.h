
// $Header: x:/prj/tech/libsrc/dev2d/RCS/cpal.h 1.2 1998/02/13 10:24:57 JAEMZ Exp $
// Prototypes and macros for Gamma correction

#ifndef __CPAL_H
#define __CPAL_H

#ifdef __cplusplus
extern "C" {
#endif

// Sets the the gamma auto correct state, toggles if negative
// else you should set TRUE or FALSE
extern void gr_set_contrast_auto(int correct);

// Sets the contrast value, but won't do anything if not on
// About is what value to scale about, scale is the value.
extern void gr_set_contrast_value(float about,float scale);

// Returns whether contrast is on, and stuffs gamma if non-null
extern bool gr_get_contrast_state(float *about,float *scale);

// Set a range of indices to protect or not
// start to end inclusive
// protect==TRUE mean that index will not be contrast adjusted
extern void gr_contrast_protect_range(int start,int end,bool protect);

// Set a list of indices to protect or not
extern void gr_contrast_protect_list(uchar *list,int num,bool protect);


#ifdef __cplusplus
};
#endif
#endif /* !__CPAL_H */



