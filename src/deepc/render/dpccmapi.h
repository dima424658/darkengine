/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCMAPI_H
#define __DPCCMAPI_H

#ifndef __COMTOOLS_H
#include <comtools.h>
#endif // !__COMTOOLS_H

#ifndef __MATRIXS_H
#include <matrixs.h>
#endif // !__MATRIXS_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

//------------------------------------
// Camera controls
//

typedef int eCameraEditType;

enum eCameraEditType_ {kCameraNormal, kCameraPlayback, kCameraRecord, kCameraEdit,};

F_DECLARE_INTERFACE(IDPCCamera); 

#undef INTERFACE
#define INTERFACE IDPCCamera

DECLARE_INTERFACE_(IDPCCamera,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD(SetLocation)(THIS_ const mxs_vector *pPos, const mxs_angvec *pAng) PURE;
   STDMETHOD_(BOOL, Attach)(THIS_ ObjID objID) PURE;
   STDMETHOD_(BOOL, Attach)(THIS_ const Label *pName) PURE;
   STDMETHOD_(BOOL, Attach)(THIS_ const char *pName) PURE;
   STDMETHOD(Detach)(THIS_) PURE;
   STDMETHOD_(ObjID, GetAttachObject)(THIS_) PURE;

// state is what the user defines the camera to do when we start a cut-scene
// mode is what the camera is actually doing right now
   STDMETHOD(SetEditState)(THIS_ eCameraEditType state) PURE;
   // note: you should always set the scene name BEFORE you set the mode
   STDMETHOD(SetScene)(THIS_ const char *pSceneName) PURE;
   STDMETHOD(SetEditMode)(THIS_ eCameraEditType mode) PURE;
   STDMETHOD_(eCameraEditType, GetEditState)(THIS) PURE;
   STDMETHOD_(eCameraEditType, GetEditMode)(THIS) PURE;

   STDMETHOD(SwitchCamera)(int cameraNum) PURE;
   STDMETHOD_(int, GetNumCameras)(THIS) PURE;

   STDMETHOD(SetCameraSpeed)(float speed) PURE;
   STDMETHOD_(float, GetCameraSpeed)(void) PURE;

   STDMETHOD(SaveScene)(void) PURE;
   STDMETHOD(MakeScenes)(void) PURE;

   STDMETHOD(Frame)(THIS) PURE;
};


#undef INTERFACE

void DPCCameraCreate();

#endif  // __DPCCMAPI_H