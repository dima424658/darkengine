/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcmobj.h,v 1.2 2000/01/29 13:40:39 adurant Exp $
#pragma once

#ifndef __SHKCMOBJ_H
#define __SHKCMOBJ_H

#include <comtools.h>
#include <simtime.h>

//------------------------------------
// Camera Objects API
//

F_DECLARE_INTERFACE(ICameraObjects); 

#undef INTERFACE
#define INTERFACE ICameraObjects

DECLARE_INTERFACE_(ICameraObjects,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   // Update
   STDMETHOD(Frame)(THIS_ tSimTime timeDelta) PURE;
   // Fire gun
};

#undef INTERFACE

void ShockCameraObjectsCreate(void);

#endif // __SHKCMOBJ_H