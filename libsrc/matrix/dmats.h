/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/dmats.h $
 * $Revision: 1.1 $
 * $Author: JAEMZ $
 * $Date: 1997/03/31 00:05:53 $
 *
 * Double Matrix library structures and constructors
 * This is meant to be used non-run time, but who knows
 * Mostly it's here for people doing arbitrary matrix math
 * And so does not distinguish between vectors and matrices
 */


#ifndef __DMATS_H
#define __DMATS_H

#include <types.h>
#include <float.h>


typedef struct dms_matrix
{
   int col;   // columns
   int row;   // rows
   double el[];   // elements
} dms_matrix;

// stored column wise


// Gives you a column pointer.
#define DM_COL(dm_ptr,_col) ( (dm_ptr)->el+(_col*(dm_ptr)->row) )

// Gives a double, which is that element
// can be used left or right
#define DM_EL(dm_ptr,_col,_row) (dm_ptr)->el[_row + (_col*(dm_ptr)->row)]

#endif
