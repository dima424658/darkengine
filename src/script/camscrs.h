/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once  
#ifndef __CAMSCRS_H
#define __CAMSCRS_H

#include <objscrt.h>

//get a real number
DECLARE_SCRIPT_SERVICE(Camera,0x140)
{
   //
   // Attach a camera to an object(fixed facing)
   //
   STDMETHOD(StaticAttach)(object attachee) PURE; 

   //
   // Set camera to object's position
   //
   STDMETHOD(DynamicAttach)(object attachee) PURE; 

   //
   // Return camera to player, if attached to attachee
   //

   STDMETHOD(CameraReturn)(object attachee) PURE;

   //
   // Return camera to player forcibly.
   //

   STDMETHOD(ForceCameraReturn)() PURE;

};



#endif // __CAMSCRS_H
