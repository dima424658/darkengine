/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propinv.h,v 1.3 2000/01/29 13:24:31 adurant Exp $
#pragma once
#ifndef __PROPINV_H
#define __PROPINV_H
#include <property.h>
#include <propdef.h>

////////////////////////////////////////////////////////////
// INVERTIBLE PROPERTY INTERFACES
// 
// Invertible properties are properties that can also be mapped backwards 
// from values to objects.
//

#define DECLARE_INVERSE_PROPERTY_ACCESSORS(TYPE) \
   STDMETHOD_ (BOOL, GetObj)(THIS_ TYPE val, ObjID* objp) CONSTFUNC PURE; 



#endif // __PROPINV_H



