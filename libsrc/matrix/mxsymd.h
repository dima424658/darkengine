// $Header: x:/prj/tech/libsrc/matrix/RCS/mxsymd.h 1.3 1997/08/11 18:40:51 dc Exp $

// symmetric octant based sin/cos
// ie. they relativize the fixang into a 0-0x2000 first quadrant value
//   then flip sin/cos and/on pos/neg
// this guarantees that cos45==cos315 and other symmetries

#ifdef __cplusplus
extern "C"
{
#endif

double mxd_sym_sin(mxs_ang ang);
double mxd_sym_cos(mxs_ang ang);
void mxd_sym_sincos(mxs_ang ang,double *s, double *c);

// these are a little slower, so they are there own calls
// however, you can #define USE_SYMMETRIC_TRIG before including mxsymd.h
// and the usual functions will be #defined to these
// make sure you include this file (mxsym) after matrix itself

#ifdef USE_SYMMETRIC_TRIG
#define mxd_sin     mxd_sym_sin
#define mxd_cos     mxd_sym_cos
#define mxd_sincos  mxd_sym_sincos
#endif

#ifdef __cplusplus
};
#endif

