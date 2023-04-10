// $Header: x:/prj/tech/libsrc/matrix/RCS/mxsym.h 1.3 1997/08/11 18:40:51 dc Exp $

// symmetric octant based sin/cos
// ie. they relativize the fixang into a 0-0x2000 first quadrant value
//   then flip sin/cos and/on pos/neg
// this guarantees that cos45==cos315 and other symmetries

#ifdef __cplusplus
extern "C"
{
#endif

mxs_real mx_sym_sin(mxs_ang ang);
mxs_real mx_sym_cos(mxs_ang ang);
void mx_sym_sincos(mxs_ang ang,mxs_real *s, mxs_real *c);

// these are a little slower, so they are there own calls
// however, you can #define USE_SYMMETRIC_TRIG before including mxsym.h
// and the usual functions will be #defined to these
// make sure you include this file (mxsym) after matrix itself

#ifdef USE_SYMMETRIC_TRIG
#define mx_sin     mx_sym_sin
#define mx_cos     mx_sym_cos
#define mx_sincos  mx_sym_sincos
#endif

#ifdef __cplusplus
};
#endif

