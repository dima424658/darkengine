/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/objedit.h,v 1.4 2000/01/29 13:12:49 adurant Exp $
#pragma once

#ifndef __OBJEDIT_H
#define __OBJEDIT_H

#include <objtype.h>

#ifdef __cplusplus
#include <str.h>
#define ObjWarnName(obj) ((const char *)cStr(ObjEditName(obj)))
#else
#define ObjWarnName ObjEditName
#endif

//
// ObjEditName
//
// Get an object's printable name for editing/debugging
//

EXTERN const char* ObjEditName(ObjID obj);

//
// ObjEditNamed
//
// Gets the object described by a string
//

EXTERN ObjID  EditGetObjNamed(const char* name);

//
// ObjEditInit
//
// Initialize object editing
//

EXTERN void ObjEditInit(void);

#endif // __OBJEDIT_H
