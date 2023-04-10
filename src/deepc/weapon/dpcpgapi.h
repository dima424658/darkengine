/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCPGAPI_H
#define __DPCPGAPI_H

#ifndef __COMTOOLS_H
#include <comtools.h>
#endif // !__COMTOOLS_H
#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // _OBJTYPE_H

#ifndef __SIMTIME_H
#include <simtime.h>
#endif // __SIMTIME_H
//------------------------------------
// Player Gun API
//

F_DECLARE_INTERFACE(IPlayerGun); 

#undef INTERFACE
#define INTERFACE IPlayerGun

DECLARE_INTERFACE_(IPlayerGun,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD(Init)(THIS_) PURE;
   STDMETHOD(GameInit)(THIS_) PURE;
   STDMETHOD(GameTerm)(THIS_) PURE;
   // Set new gun and raise it
   STDMETHOD(Set)(THIS_ ObjID objID) PURE;
   // Set the gun already raised, no activation sound
   STDMETHOD(SetRaised)(THIS_ ObjID objID) PURE;
   // Get current gun
   STDMETHOD_(ObjID, Get)(THIS_) PURE;
   STDMETHOD_(ObjID, GetHand)(THIS_) PURE;
   // Update
   STDMETHOD(Frame)(THIS_ tSimTime timeDelta) PURE;
   // Fire gun
   STDMETHOD(PullTrigger)(THIS_) PURE;
   STDMETHOD(ReleaseTrigger)(THIS_) PURE;
   // Gun state changes
   STDMETHOD(Raise)(THIS_) PURE;
   STDMETHOD(Lower)(THIS_) PURE;
   STDMETHOD(Unequip)(THIS_ ObjID oldGun, ObjID newGun, BOOL spew, BOOL sendToInv) PURE;
   STDMETHOD(Off)(THIS_) PURE;
   STDMETHOD(Load)(void) PURE; 
   // Gun state queries
   STDMETHOD_(BOOL, IsReloading)(THIS_) const PURE;
   STDMETHOD_(BOOL, IsTriggerPulled)(THIS_) const PURE;
   // Queries
   // does this ammo obj match the type last loaded into the gun?
   STDMETHOD_(BOOL, MatchesCurrentAmmo)(THIS_ ObjID ammoID) PURE;
};

#undef INTERFACE

void DPCPlayerGunCreate(void);

#endif // __DPCPGAPI_H
