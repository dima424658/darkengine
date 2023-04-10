/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkpsapi.h,v 1.8 2000/01/31 09:58:45 adurant Exp $
//
// Psionics system API
//
#pragma once

#ifndef __SHKPSAPI_H
#define __SHKPSAPI_H

#include <comtools.h>
#include <objtype.h>
#include <simtime.h>
#include <shkpsity.h>
#include <shkpltyp.h>

F_DECLARE_INTERFACE(IPlayerPsi); 
F_DECLARE_INTERFACE(IPsiActiveIter); 

//////////////////////////////////////////
// Player psi powers
//

#undef INTERFACE
#define INTERFACE IPlayerPsi

enum ePsiActivateStatus_ {kPsiOK, kPsiInvalidPower, kPsiInsufficientPoints};
typedef int ePsiActivateStatus;

DECLARE_INTERFACE_(IPlayerPsi,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD(Frame)(THIS_ tSimTime timeDelta) PURE;

   // Power selection
   STDMETHOD(Select)(ePsiPowers power) PURE;
   STDMETHOD_(ePsiPowers, GetSelectedPower)(void) PURE;

   // Power activation
   STDMETHOD_(ePsiActivateStatus, CanActivate)(void) PURE;
   STDMETHOD(Activate)(void) PURE;
   STDMETHOD(Deactivate)(ePsiPowers power) PURE;
   STDMETHOD(OnDeactivate)(ePsiPowers power) PURE;
   STDMETHOD_(BOOL, IsActive)(ePsiPowers power) PURE;

   // Psi points
   STDMETHOD_(int, GetPoints)(void) PURE;
   STDMETHOD_(int, GetMaxPoints)(void) PURE;
   STDMETHOD(SetMaxPoints)(int maxPoints) PURE;
   STDMETHOD(SetPoints)(int points) PURE;

   // Get current sustained power (kPsiNone if none)
   STDMETHOD_(ePsiPowers, GetSustainedPower)(void) PURE;

   STDMETHOD_(void, OnSchemaEnd)(int hSchema) PURE;

   // Iterator over active powers
   STDMETHOD_(IPsiActiveIter*, Iter)(void) PURE;

   STDMETHOD(BeginOverload)(void) PURE;
   STDMETHOD_(BOOL,EndOverload)(void) PURE;

   STDMETHOD(PsiTarget)(ObjID what) PURE;
};

#undef INTERFACE

//////////////////////////////////////////
// Iterator 
//

#define INTERFACE IPsiActiveIter

DECLARE_INTERFACE_(IPsiActiveIter,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   STDMETHOD_(BOOL, Done)(THIS) PURE;
   STDMETHOD(Next)(THIS) PURE;
   STDMETHOD_(const char*, GetName)(THIS) PURE;
   STDMETHOD_(ePsiPowers, GetPower)(THIS) PURE;
};
#undef INTERFACE

//////////////////////////////////////////

EXTERN void ShockPlayerPsiCreate(void);
EXTERN void PsiInit(void);
EXTERN void PsiTerm(void);

//////////////////////////////////////////
// Utility

EXTERN BOOL IsCharmed(ObjID objID);

#endif