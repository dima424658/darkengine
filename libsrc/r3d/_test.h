// $Header: x:/prj/tech/libsrc/r3d/RCS/_test.h 1.2 1997/05/20 14:43:56 JAEMZ Exp $

#ifndef ___TEST_H
#define ___TEST_H

#include <ctxts.h>
#include <lg.h>

#ifdef DBG_ON 

   #define TEST_STATE(fname)   \
   do { \
      AssertMsg1(!r3d_state.in_block,"r3d: %s: called in a block!\n",fname); \
      AssertMsg1(r3d_state.cur_con,"r3d: %s: called outside of a context!\n",fname); \
   } while(0)


   #define TEST_IN_BLOCK(fname)   \
   do { \
      AssertMsg1(r3d_state.in_block,"r3d: %s: called outside of a block!\n",fname); \
      AssertMsg1(r3d_state.cur_con,"r3d: %s: called outside of a context!\n",fname); \
   } while(0)

   #define TEST_IN_CONTEXT(fname)   \
   do { \
      AssertMsg1(r3d_state.cur_con,"r3d: %s: called outside of a context!\n",fname); \
   } while(0)

#else 
   #define TEST_STATE(fname) 
   #define TEST_STATE_RETVAL(fname,retval) 
   #define TEST_IN_BLOCK(fname) 
   #define TEST_IN_BLOCK_RETVAL(fname,retval)
   #define TEST_IN_CONTEXT(fname) 
   #define TEST_IN_CONTEXT_RETVAL(fname,retval) 
#endif


#endif
