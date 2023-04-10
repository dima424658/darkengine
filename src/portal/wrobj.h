/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
/* wrobj.h
**
** Object system stuff that needs to know about the World Rep at compile time
**
** $Header: r:/t2repos/thief2/src/portal/wrobj.h,v 1.4 2000/01/29 13:37:45 adurant Exp $
** $Log: wrobj.h,v $
** Revision 1.4  2000/01/29 13:37:45  adurant
** #pragma once
**
** Revision 1.3  1997/11/24 19:32:28  mahk
** Changed to include wrtype.h instead of wr.h
**
 * Revision 1.2  1997/08/05  12:11:37  TOML
 * added extern "C" blocks for C++
 * 
 * Revision 1.1  1996/11/18  13:17:23  buzzard
 * Initial revision
 * 
 * Revision 1.1  1996/07/24  12:39:30  DFAN
 * Initial revision
 * 
*/

#ifndef _WROBJ_H
#define _WROBJ_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <wrtype.h>

// TODO: Hey, these need to set position too!

// Set the location of obj to <x,y,z>
#define ObjSetLocation(obj,objp,x,y,z) \
  (MakeLocation(&((objp)->pos->loc), (x), (y), (z)), \
   ObjUpdateLocs (obj))

// Update the location of obj to <x,y,z>, using obj's old location
// for hinting information
#define ObjUpdateLocation(obj,objp,x,y,z) \
  (MakeHintedLocation(&((objp)->pos->loc), (x), (y), (z), \
                      &((objp)->pos->loc)), \
   ObjUpdateLocs (obj))


// Should this be in wrtype.h?
#define MakeOrientation(dst,a,b,c) \
  ((dst).el[0]=(a), (dst).el[1]=(b), (dst).el[2]=(c))

#define ObjSetOrientation(objp,a,b,c) \
  (MakeOrientation((objp)->pos->fac, (a), (b), (c)))

#define ObjSetPosition(obj,objp,x,y,z,a,b,c) \
  (ObjSetOrientation((objp),(a),(b),(c)), \
   ObjSetLocation((obj),(objp),(x),(y),(z)))

#define ObjUpdatePosition(obj,objp,x,y,z,a,b,c) \
  (ObjSetOrientation((objp),(a),(b),(c)), \
   ObjUpdateLocation((obj),(objp),(x),(y),(z)))

#ifdef __cplusplus
};
#endif

#endif
