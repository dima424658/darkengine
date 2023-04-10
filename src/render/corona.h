/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/corona.h,v 1.1 2000/01/18 15:17:33 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   corona.h

   Supposing object Gus has the corona property.  When Gus is rendered
   we raycast from the camera to Gus; if the cast succeeds we render
   a texture in front of the object.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifndef _CORONA_H_
#pragma once
#define _CORONA_H_


   //////========----------

///////////////////////////////////////////////
// overall app flow
EXTERN void CoronaInit(void);
EXTERN void CoronaTerm(void);
EXTERN void CoronaFrame(const Position *pPos);

///////////////////////////////////////////////

// per-object call--checks for presence of property & does the work if it find
EXTERN void CoronaCheckObj(ObjID Obj);

#endif // ~_CORONA_H_
