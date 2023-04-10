
// $Header: x:/prj/tech/libsrc/dev2d/RCS/gpal.h 1.2 1998/02/13 10:25:11 JAEMZ Exp $
// Prototypes and macros for Gamma correction

#ifndef __GPAL_H
#define __GPAL_H

#ifdef __cplusplus
extern "C" {
#endif

// Sets the the gamma auto correct state, toggles if negative
// else you should set TRUE or FALSE
extern void gr_set_gamma_auto(int correct);

// Sets the gamma value, but won't do anything if not on
extern void gr_set_gamma_value(float gamma);

// Returns whether gamma correction is on, and stuffs gamma if non-null
extern bool gr_get_gamma_state(float *gamma);

// Set a range of indices to protect or not
// start to end inclusive
// protect==TRUE mean that index will not be contrast adjusted
extern void gr_gamma_protect_range(int start,int end,bool protect);

// Set a list of indices to protect or not
extern void gr_gamma_protect_list(uchar *list,int num,bool protect);



#ifdef __cplusplus
};
#endif
#endif /* !__GPAL_H */



