// $Header: x:/prj/tech/libsrc/matrix/RCS/_test.h 1.1 1996/09/23 10:33:24 JAEMZ Exp $

#ifndef ___TEST_H
#define ___TEST_H

#include <lg.h>

#ifdef DBG_ON 
   #define TEST_SRCDST(fname,dst,src)   \
   do { \
      if ((src)==(dst)) { \
         Warning(("mx: %s: src == dst, won't work.\n",fname)); \
         return; \
      } \
   } while(0)
#else 
   #define TEST_SRCDST(fname,dst,src)
#endif

#endif
   

