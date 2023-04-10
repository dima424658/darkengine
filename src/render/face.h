/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/face.h,v 1.1 2000/01/14 10:38:21 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   face.h

   This is a subsystem of meshtex.  It controls facial expressions.

   This subsystem has its own property for specifying what graphics
   files a given object uses for each expression.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifndef _FACE_H_
#pragma once
#define _FACE_H_


enum eFaceVisage
{
   kFaceNeutral,        // passive, espressionless, like watching TV
   kFaceSmile,
   kFaceWince,
   kFaceSurprise,
   kFaceStunned,

   kFaceVisageCount
};


// Only death faces should use maximum priority.  That way they
// override whatever else the face could try to show.
const int kFacePriorityMin = 1;
const int kFacePriorityMax = 10;
const int kFaceDurationInfinite = 0x7fffffff;

///////////////////////////////////////////////

// If the given priority is higher than the one the object is using we
// switch to it and return TRUE.  Otherwise we keep the visage we've
// got and return FALSE.
extern BOOL FaceSetVisage(ObjID Obj, eFaceVisage Visage, uint nPriority,
                          int iDuration);

extern void FaceInit();
extern void FaceTerm();
extern void FaceReset();

#ifdef EDITOR
extern void BuildFacePosFile();
#endif // EDITOR

extern void FacePrerender(ObjID Obj, mms_model *pModel);
extern void FacePostrender();

#endif // ~_FACE_H_
