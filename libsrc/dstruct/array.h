#ifndef __ARRAY_H
#define __ARRAY_H

/*
 * $Source: x:/prj/tech/libsrc/dstruct/RCS/array.h $
 * $Revision: 1.3 $
 * $Author: mahk $
 * $Date: 1996/07/16 00:14:52 $
 *
 * $Log: array.h $
 * Revision 1.3  1996/07/16  00:14:52  mahk
 * Added array iterator
 * 
 * Revision 1.2  1996/01/22  15:38:31  DAVET
 * Added cplusplus stuff
 * 
 * Revision 1.1  1993/04/16  22:09:58  mahk
 * Initial revision
 * 
 * Revision 1.2  1993/03/22  15:23:41  mahk
 * Added prototype for array_destroy.
 * 
 * Revision 1.1  1993/03/22  15:21:34  mahk
 * Initial revision
 * 
 *
 */

// Includes
#include "lg.h"

// C Library Includes

// System Library Includes
#include "error.h" 


#ifdef __cplusplus
extern "C"  {
#endif


// Master Game Includes

// Game Library Includes

// Game Object Includes

// ======================
//  ARRAY TYPE 
// ======================
// Here is an implementation of a dynamically-growing array.  
// It is intended for used in places where superfluous calls to 
// Malloc are not desirable.  

// Defines

typedef struct _array
{
   int elemsize;  // How big is each array element
   int vecsize;   // How many elements in the vector
   int fullness;  // How many elements are used.
   int freehead;  // index to head of the free list.  
   int *freevec;  // free list
   char *vec;     // the actual vector;
} Array;


// Prototypes


// Initialize an array.  Fill in the structure, allocate the vector and free list.  
errtype array_init(Array* toinit, int elemsize, int vecsize);

// Find a place for a new element of the array, extending the array if necessary. 
// returns the new index in *index
errtype array_newelem(Array* a, int* index);

// Mark an element as unused and eligible for recycling by a subsequent
// array_newelem call. 
errtype array_dropelem(Array* a, int index);

// Destroy an array, deallocating its vec and freevec
errtype array_destroy(Array* a);


//--------------------
// Iterating across all used indices

typedef struct _array_iter
{
   Array* a;
   int next;
} array_iter;

extern void array_iter_init(Array *a, array_iter *iter); // initialize iterator
extern int array_iter_curr(array_iter *iter); // current index
extern int array_iter_next(array_iter *iter); // go to next index
extern bool array_iter_done(array_iter *iter); // am I done iterating? 



// Globals

#ifdef __cplusplus
}
#endif


#endif //__ARRAY_H
