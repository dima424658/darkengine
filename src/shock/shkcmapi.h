/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcmapi.h,v 1.3 2000/01/29 13:40:37 adurant Exp $
#pragma once

#ifndef __SHKCMAPI_H
#define __SHKCMAPI_H

#include <comtools.h>
#include <matrixs.h>
#include <objtype.h>

//------------------------------------
// Camera controls
//

typedef int eCameraEditType;

enum eCameraEditType_ {kCameraNormal, kCameraPlayback, kCameraRecord, kCameraEdit,};

F_DECLARE_INTERFACE(IShockCamera); 

#undef INTERFACE
#define INTERFACE IShockCamera

DECLARE_INTERFACE_(IShockCamera,IUnknown)
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

void ShockCameraCreate();

#endif