/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkrutil.h,v 1.1 1998/05/22 19:35:32 dc Exp $

#pragma once
#ifndef __DRKRUTIL_H
#define __DRKRUTIL_H

#include <osystype.h>

// get the attached obj stuff ready
EXTERN void _darkru_attached_obj_init(void);
EXTERN void _darkru_attached_obj_term(void);

// setup any attached objects, return final one of them
EXTERN ObjID _darkru_setup_attached_objs(ObjID o);

// remove any object we had just attached
EXTERN void _darkru_remove_attached_objs(ObjID o);

#endif  // __DRKRUTIL_H
