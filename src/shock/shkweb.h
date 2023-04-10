/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header:

#ifndef __SHKWEB_H
#define __SHKWEB_H

#pragma once

#include <r3d.h>
#include <mxmats.h>


// This defines a spider web

// A web is initialized with an approximate
// number of spokes.  A radius, an aspect ratio
// and a variability in the radius and a variability in
// the angles in percent.  Radvar goes inward, spokevar goes bigger
// The web is centered at 0,0,0, and extends backward in the x direction

struct sWeb
{
   sWeb(float radius,int spokes,float radVar,float spokeVar,float aspect,float depth);
   ~sWeb();

   // Find spoke between these two spots in screen space, must be in context
   // of render canvas
   // If reject on outer is set, returns -1 if inside outer edge
   int FindSpoke(int x,int y,bool rejectOnOuter);

   // Gives you the outer coords halfway between the spokes as defined by FindSpoke
   void SpokeCenter(sMxVector *pVec,int spoke);

   // Renders the web in a 3d context in a color
   void Render(int color);
   int m_spokes;
   float m_depth;
   float *m_pSpokeAngles;        // the spoke angles in radians (in screen space)
   sMxVector *m_pOuter; // pointer to rim points
   sMxVector *m_pInner; // inner points
   r3s_point *m_pPOuter;
   r3s_point *m_pPInner;
};

#endif  // __SHKWEB_H



