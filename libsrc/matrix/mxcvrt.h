// $Header: x:/prj/tech/libsrc/matrix/RCS/mxcvrt.h 1.1 1997/08/12 06:42:19 dc Exp $

#ifndef __MXCVRT_H
#define __MXCVRT_H

// Vector conversion

#define mx_vec_cvrt(dst,src,cast_type) \
   do { \
      (dst)->el[0]=(cast_type)(src)->el[0]; \
      (dst)->el[1]=(cast_type)(src)->el[1]; \
      (dst)->el[2]=(cast_type)(src)->el[2]; \
   } while (0)

#define mx_vec_d_from_s(dst,src) mx_vec_cvrt(dst,src,double)
#define mx_vec_s_from_d(dst,src) mx_vec_cvrt(dst,src,float)

// Matrix conversion

#define mx_mat_cvrt(dst,src,cast_type) \
   do { \
      (dst)->el[0]=(cast_type)(src)->el[0]; \
      (dst)->el[1]=(cast_type)(src)->el[1]; \
      (dst)->el[2]=(cast_type)(src)->el[2]; \
      (dst)->el[3]=(cast_type)(src)->el[3]; \
      (dst)->el[4]=(cast_type)(src)->el[4]; \
      (dst)->el[5]=(cast_type)(src)->el[5]; \
      (dst)->el[6]=(cast_type)(src)->el[6]; \
      (dst)->el[7]=(cast_type)(src)->el[7]; \
      (dst)->el[8]=(cast_type)(src)->el[8]; \
   } while (0)

#define mx_mat_d_from_s(dst,src) mx_mat_cvrt(dst,src,double)
#define mx_mat_s_from_d(dst,src) mx_mat_cvrt(dst,src,float)

#endif  // __MXCVRT_H
