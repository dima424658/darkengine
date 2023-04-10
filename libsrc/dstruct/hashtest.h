#ifndef _HASHTEST_H
#define _HASHTEST_H

/*
 * $Source: x:/prj/tech/libsrc/dstruct/RCS/hashtest.h $
 * $Revision: 1.1 $
 * $Author: mahk $
 * $Date: 1993/03/25 19:22:39 $
 *
 * $Log: hashtest.h $
 * Revision 1.1  1993/03/25  19:22:39  mahk
 * Initial revision
 * 
 *
 */



#define HASHELEMSIZE 8

typedef struct _hashelem
{
   char key[HASHELEMSIZE];
   int val;
} HashElem;





#endif // _HASHTEST_H
