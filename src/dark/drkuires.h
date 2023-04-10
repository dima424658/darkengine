/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkuires.h,v 1.3 1998/09/18 19:50:50 mahk Exp $
#pragma once  
#ifndef __DRKUIRES_H
#define __DRKUIRES_H
#include <str.h>
#include <comtools.h>
#include <rect.h>
#include <dynarray.h>

#define INTERFACE_PATH "intrface" 

//
// RESOURCE ACCESS LAYER FOR DARK UI
// Basically, an abstraction around the resource system whose API 
// is in transition as I type. 
//

// 
// Fetch an image
//

F_DECLARE_INTERFACE(IDataSource);
extern IDataSource* FetchUIImage(const char* name, const char* relpath = INTERFACE_PATH); 

//
// Fetch a string
//
extern cStr FetchUIString(const char* table, const char* name, const char* relpath = INTERFACE_PATH); 

//
// Fetch an array of rectangles 
//
typedef cDynArray<Rect> cUIRectArray; 

extern BOOL FetchUIRects(const char* name, cUIRectArray& array, const char* relpath = INTERFACE_PATH); 


#endif // __DRKUIRES_H


