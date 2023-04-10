/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCMOBJ_H
#define __DPCCMOBJ_H

#ifndef __COMTOOLS_H
#include <comtools.h>
#endif // !__COMTOOLS_H

#ifndef __SIMTIME_H
#include <simtime.h>
#endif // !__SIMTIME_H

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

void DPCCameraObjectsCreate(void);

#endif // __DPCCMOBJ_H