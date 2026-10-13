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

   // NewDark-compatible camera queries and coordinate transforms.  These are
   // appended so the original four-entry service ABI remains intact.
   STDMETHOD_(object, GetCameraParent)() PURE;
   STDMETHOD_(BOOL, IsRemote)() PURE;
   STDMETHOD_(vector, GetPosition)() PURE;
   STDMETHOD_(vector, GetFacing)() PURE;
   STDMETHOD_(vector, CameraToWorld)(const vector &local_pos) PURE;
   STDMETHOD_(vector, WorldToCamera)(const vector &world_pos) PURE;
   STDMETHOD(LockMovement)(BOOL move_allowed) PURE;

};



#endif // __CAMSCRS_H
