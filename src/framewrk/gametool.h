/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gametool.h,v 1.2 1999/11/02 15:57:40 BODISAFA Exp $
#pragma once  
#ifndef __GAMETOOL_H
#define __GAMETOOL_H

#include <comtools.h>
#include <objtype.h>
#include <matrixs.h>

////////////////////////////////////////////////////////////
// "Game Tools"  Interface
// 
// Random useful stuff for game systems.
// As this interface gets large, pieces of it may get broken out into new interfaces
// 

#ifdef __cplusplus
#define DEFAULTS_TO(x)  = x
#else
#define DEFAULTS_TO(x)
#endif 

F_DECLARE_INTERFACE(IGameTools); 

#undef INTERFACE
#define INTERFACE IGameTools

typedef BOOL (*fIsToGameGUIDCallback)(REFGUID rguid);

DECLARE_INTERFACE_(IGameTools,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Game-specific mode switching goodness.
   // TRUE:  We're headed into game mode.
   STDMETHOD_(BOOL, IsToGameModeGUID)(REFGUID rguid) PURE;
   
   // Sets up what we're using as the game-specific callback.
   STDMETHOD(SetIsToGameModeGUIDCallback)(fIsToGameGUIDCallback) PURE;

   fIsToGameGUIDCallback m_fIsToGameGUIDCallback;
   //
   // Teleport an object, to a point in the reference frame of another object
   //
   STDMETHOD(TeleportObject)(THIS_ ObjID obj, const mxs_vector* pos, const mxs_angvec* fac, ObjID refframe DEFAULTS_TO(0)) PURE; 
};

#undef INTERFACE

EXTERN void GameToolsCreate(void); 
EXTERN BOOL GameToolIsToGameModeGUID(REFGUID rguid);
#endif // __GAMETOOL_H
