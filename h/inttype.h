/*
 *  sean.h
 *
 *  miscellaneous definitions I always use
 *
 *  Basically these make the code portable.  Of course, we may
 *  not really care about the code being portable, but it doesn't
 *  cost me anything to use these (except slightly slower compiles 
 *  from extra macro expansion?), and I'm used to doing it anyway,
 *  and it may be good if it ends up being ported.
 */


#ifndef __INTTYPES_H
#define __INTTYPES_H

////////////////////////////////////////////////////////////////
//
//                                      debugging stuff
//


#ifdef DBG_ON
  #define CHECK_ASSERT
#endif

  // temporary assertion system until something better comes along--
  // really this shouldn't be conditional on DBG_ON, so we can test
  // assertions in optimized executables, etc.

  // We use the SCAM_ASSERT function.  This allows us to embed long
  // assertion output strings in the source which aren't compiled into
  // the executable, but are instead looked up at run-time.  Convenient,
  // straightforward, no space overhead, time overhead only when we're
  // about to exit due to an assertion failure.  But totally wacky.

#ifdef CHECK_ASSERT

  #define eAssert(x)    \
    if (!(x))           \
      SCAM_ASSERT();    \
    else

#else

  #define eAssert(x)     \
    do { } while(0)

#endif



///////////////////////////////////////////////////////////////
//
//                                     portability stuff
//
//


//////////////////////////////////
//
//       types
//
// these definitions all work for both 32-bit and 16-bit
//   PC compilers, all 68k products I've ever worked with,
//   and all 32-bit workstation compilers I've ever used
//
// but since they're typedefs they'll be easy to change if
//   there's some other platform they don't work for
                      
typedef unsigned char uint8;
typedef   signed char sint8;
typedef   signed char  int8;

typedef unsigned short uint16;
typedef   signed short sint16;
typedef   signed short  int16;

typedef unsigned long uint32;
typedef   signed long sint32;
typedef   signed long  int32;

//typedef unsigned int    uint;
typedef   signed int    sint;

// note the use of longs instead of ints for 32 bit numbers.
// this keeps compatibility with 16-bit compilers.


  // In some environments we may not actually be able to get
  // 32 bit integers, but rather something longer.  If that
  // happens, unset the following flag.
#define INT32_IS_EXACTLY_32
  
  // For even more portable, we could have flags indicating
  // whether chars are 8 bits, and whether arithmetic uses
  // 2's-complement math.  Instead, we choose to not be portable
  // to such platforms.  Sorry.



// also note that I never use bools.  As a general rule, one
// should only use explicit length types (as above) if they're
// longer than ints (e.g. 32 bits required) or if you need to
// pack them tightly (in which case you use 8 or 16 bit types).
//
// If the exact length isn't important, I always use ints or uints.
// By definition these are the most efficient types to operate on
// (since shorter types are supposed to be expanded to ints before
// arithmetic operations are performed on them.) 
//
// In general, then, if I have structures or arrays, I'll never use
// ints, but rather I'll use one of the explicitly-lengthed quantities.
// local variables and global variables which aren't in arrays will
// generally be ints if they're "short" or int32s if they're not.
// Or "fix", of course.
//
// If I'm manipulating boolean quantities, I want them in ints (as above);
// if I'm sticking them in structures or arrays, I want to use int8s.  Thus
// using one particular bool type won't work.  I could make "fast_bool" and
// "small_bool", but really, what's the point?



// unions for storing integers overlapping pointers; use sizeof
// these unions rather than assuming particular sizes.

typedef union {
  int i;
  void *p;
} int_or_ptr;

typedef union {
  int32 i;
  void *p;
} int32_or_ptr;

typedef union {
  int32 i;
  void *p;
  void (*fp)(void);
} int32_or_fptr;


// if they've set the DEFINE_SIZEOFS flag, then create
// #defines for all the standard sizeof()s.
//
// This fills the namespace a lot, so the idea is to
// only define the flag in the files that actually use this,
// before the include of this.

#ifdef DEFINE_SIZEOFS

  #define SINT    sizeof(int)
  #define SINT32  sizeof(int32)  // this better be 4 if INT32_IS_EXACTLY_32

  #define SPTR    sizeof(void *)
  #define SFPTR   sizeof(void (*)(void))

  #define SIP     sizeof(int_or_ptr)
  #define SI32P   sizeof(int32_or_ptr)
  #define SI32FP  sizeof(int32_or_fptr)

#endif


//////////////////////////////////////////////////
//
//      platform/compiler specific stuff
//

  // really we need a matrix of platforms and compilers,
  // and each element of the matrix is a possible configuration
  // for which we have inline asm

  // preferably we would move all asm to special files that
  // are specific to each compiler, and then just have one
  // compiler flag (USE_INLINE_ASM), and then make sure the
  // makefile selects the appropriate files.
  //   Oh well.

#define USE_WATCOM_INLINE_ASM




  // define the possible endianness values

#define BIG_ENDIAN         1       // 3210
#define LITTLE_ENDIAN      2       // 0123
#define ENDIAN_WACKY_1     3       // 1032
#define ENDIAN_WACKY_2     4       // 2301
#define ENDIAN_UNDEFINED   5       // varies at runtime, or other

  // the following is true on all known PC compilers
  //   note that the value of this is technically compiler/platform
  //   dependent, not platform dependent, although I don't know of
  //   any compilers that do non-native endianness support.
  //   
  // Not clear if it's worth trying to support having a single
  //   executable which can run in either endianness mode, for
  //   multi-endianness architechtures like MIPS, Power PC, etc.

#define ENDIAN  LITTLE_ENDIAN


  // define endian-independent byte accessing mechanisms;
  // these take a pointer and return an lvalue, unless
  // the endianness type is special, in which case we get rvalues
  //
  // if they're lvalues, we can assign to them; thus if you
  // ever assign to one of these macros, the code is not portable
  // to any architectures other than compile-time selected BIG_ENDIAN
  // or LITTLE_ENDIAN (although it can be made so if you add a new
  // case to this code).
  //
  // we return unsigned values, because generally such
  // subfields are supposed to be unsigned (they would be
  // if you masked and shifted, for instance)

#if ENDIAN==LITTLE_ENDIAN

  #define low_byte_32(x)        (*((uint8 *) (x)))
  #define midlow_byte_32(x)     (*((uint8 *) (x)+1))
  #define midhigh_byte_32(x)    (*((uint8 *) (x)+2))
  #define high_byte_32(x)       (*((uint8 *) (x)+3))

  #define low_word_32(x)        (*((uint16 *) (x)))
  #define high_word_32(x)       (*((uint16 *) (x)+1))

  #define low_byte_16(x)        (*((uint8 *) (x)))
  #define high_byte_16(x)       (*((uint8 *) (x)+1))

#else
#if ENDIAN==BIG_ENDIAN
  #define low_byte_32(x)        (*((uint8 *) (x)+3))
  #define midlow_byte_32(x)     (*((uint8 *) (x)+2))
  #define midhigh_byte_32(x)    (*((uint8 *) (x)+1))
  #define high_byte_32(x)       (*((uint8 *) (x)))

  #define low_word_32(x)        (*((uint16 *) (x)+1))
  #define high_word_32(x)       (*((uint16 *) (x)))

  #define low_byte_16(x)        (*((uint8 *) (x)+1))
  #define high_byte_16(x)       (*((uint8 *) (x)))

#else

  // wow, an endianness other than big or little
  // we'll just use shifts and masks, which makes the value
  // an rvalue instead of an lvalue
  //
  // this may be slower, but it's probably faster than testing
  // a run time endianness flag and branching to the appropriate
  // pointer arithmetic code

  #define low_byte_32(x)        (((*(uint32 *) (x))      ) & 0xff)
  #define midlow_byte_32(x)     (((*(uint32 *) (x)) >>  8) & 0xff)
  #define midhigh_byte_32(x)    (((*(uint32 *) (x)) >> 16) & 0xff)
  #define high_byte_32(x)       (((*(uint32 *) (x)) >> 24) & 0xff)

  #define low_word_32(x)        (((*(uint32 *) (x))      ) & 0xffff)
  #define high_word_32(x)       (((*(uint32 *) (x)) >> 16) & 0xffff)

  #define low_byte_16(x)        (((*(uint16 *) (x))      ) & 0xff)
  #define high_byte_16(x)       (((*(uint16 *) (x)) >>  8) & 0xff)

#endif  // BIG_ENDIAN test
#endif  // LITTLE_ENDIAN test

#endif