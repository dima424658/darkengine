/*
 * $Source: x:/prj/tech/libsrc/matrix/RCS/dmat.h $
 * $Revision: 1.2 $
 * $Author: TOML $
 * $Date: 1997/08/05 12:22:22 $
 *
 * Double Matrix library structures and constructors
 * Okay, I'm the first to admit this should be a c++
 * class.  When the great switch over happens...
 */

#ifndef __DMAT_H
#define __DMAT_H

#include <stdio.h>
#include <dmats.h>

#ifdef __cplusplus
extern "C"
{
#endif

dms_matrix *dm_alloc(int col,int row);

// Creates a matrix from float array, defined column wise
dms_matrix *dm_float_create(int col,int row,float *);

// Creates a matrix from double array, defined column wise
dms_matrix *dm_double_create(int col,int row,double *);

// Creates a zero matrix
dms_matrix *dm_zero(int col,int row);

// Creates an identity matrix
dms_matrix *dm_identity(int col,int row);

// Frees a matrix
void dm_free(dms_matrix *m);

// m^t
dms_matrix *dm_transpose(dms_matrix *m);

// d = m1 * m2;
dms_matrix *dm_multiply(dms_matrix *m1,dms_matrix *m2);

// swap rows
void dm_swaprow(dms_matrix *m,int r1,int r2);

// d = m^-1
dms_matrix *dm_inverse(dms_matrix *m1);

// Outputs a formatted array
void dm_output(FILE *,dms_matrix *m1);

// standard out
#define dm_print(m1) dm_output(stdout,m1)

#ifdef __cplusplus
};
#endif

#endif
