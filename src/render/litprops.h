/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/litprops.h,v 1.7 2000/01/29 13:39:04 adurant Exp $
#pragma once

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   litprops.h

   structures for light properties

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */


#ifndef _LITPROPS_H_
#define _LITPROPS_H_

#include <wr.h>

#include <animlgts.h>

#include <litpropt.h>


// Our structures are a little weird because when we wanted to add
// radii the simplest way was to tack them on to the ends of the structs.
struct sLightPropCore
{
   float brightness;            // same as we use for brush lights
   mxs_vector offset;           // in the space of the host object
};


struct sLightProp : public sLightPropCore
{
   float radius;
   BOOL quad;                   // oversample?
   float inner_radius;
};


typedef int ObjID;

// An animated light knows its magnitude and which cells it reaches.
// For each cell, it also knows its position in the cell's light
// palette and its clamping radius.
struct sAnimLightProp : public sLightPropCore
{
   sAnimLight animation;
   float radius;
   ObjID notify_script;
   BOOL quad;                   // oversample?
   float inner_radius;
};


#endif // ~_LITPROPS_H_
