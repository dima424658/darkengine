/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/dataopst.h,v 1.1 1998/03/26 13:51:36 mahk Exp $
#pragma once  
#ifndef __DATAOPST_H
#define __DATAOPST_H

#include <comtools.h>

F_DECLARE_INTERFACE(IDataOps);
F_DECLARE_INTERFACE(IDataOpsFile); 

#ifndef CONSTFUNC
#ifdef __cplusplus
#define CONSTFUNC const
#else
#define CONSTFUNC
#endif 
#endif 

//
// A generic 32-bit value, that might be a pointer, or might not.
//

struct sDatum
{
   void* value; 

   operator void*() { return value; }; 
   sDatum(void* v = NULL) : value(v) {}; 
}; 


#endif // __DATAOPST_H
