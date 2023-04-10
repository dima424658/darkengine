/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/meshtex.h,v 1.1 2000/01/14 10:39:41 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   meshtex.h

   This is a simple system for customizing what textures are used
   where on mesh models.  It's rather nasty.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifndef _MESHTEX_H_
#pragma once
#define _MESHTEX_H_

   //////========----------

///////////////////////////////////////////////
// global functions

// flow control
EXTERN void MeshTexInit();
EXTERN void MeshTexTerm();
EXTERN void MeshTexReset();

EXTERN void MeshTexPrerender(ObjID Obj, mms_model *pModel);
EXTERN void MeshTexPostrender();

// exposed for use in face.cpp
EXTERN grs_bitmap *MeshTexGetBitmap(char *pszName);

#endif // ~_MESHTEX_H_
