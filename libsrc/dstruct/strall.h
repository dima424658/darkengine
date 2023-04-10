//		Strall.H		String allocator
//		Rex E. Bradford (REX)
//
//		Provides routine for allocation & deallocation of strings
//		which uses the homogenous heap system to provide much less
//		Malloc() and Free() traffic, reducing heap fragmentation.

/*
* $Header: r:/prj/lib/src/dstruct/rcs/strall.h 1.1 1993/04/19 09:29:00 rex Exp $
* $Log: strall.h $
 * Revision 1.1  1993/04/19  09:29:00  rex
 * Initial revision
 * 
*/

#ifndef STRALL_H
#define STRALL_H

//	StringAlloc() allocates memory and copies the supplied string
//	into that storage.

char *StringAlloc(char *s);

//	StringFree() frees up a string allocated with StringAlloc().

void StringFree(char *s);

#endif