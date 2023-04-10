/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/skyobj.h,v 1.3 2000/01/31 09:53:21 adurant Exp $
#pragma once

#ifndef _SKYOBJ_H_
#define _SKYOBJ_H_


// We are assuming only one implementation of this - the implementation file contains statics!

#include <skyint.h>

class cSky : public ISkyObject
{
public:
   virtual void AppInit();
   virtual void Init();
   virtual void Term();
   virtual void Render();

   // public accessor. Return TRUE if new sky rendering is enabled for this mission.
   BOOL Enabled();
   float GetHorizonDip();
   float GetSkyIntensity(mxs_vector *pDir);
   float GetSkyDist(mxs_vector *pDir);
   float GetClipLat();
};


#endif
