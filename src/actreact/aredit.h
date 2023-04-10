/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/aredit.h,v 1.4 1998/06/30 21:13:15 mahk Exp $
#pragma once  
#ifndef __AREDIT_H
#define __AREDIT_H
#include <comtools.h>

#include <stimtype.h>
#include <senstype.h>
#include <ssrctype.h>
#include <pg8rtype.h>


enum eARDlgFlags_
{
   kARDlgNonModal = 0,
   kARDlgModal = (1 << 0),
   kARDlgReadOnly = (1 << 1),
};

typedef ulong eARDlgFlags;


////////////////////////////////////////////////////////////
// ACT/REACT EDITORS
//
// This interface is the liason between the editor DLL and the act/react editing
// commands.  It is not an aggregate member yet.  Dialogs return S_OK for the 
// OK button, S_FALSE for cancel
//

#undef INTERFACE
#define INTERFACE IActReactEditors
DECLARE_INTERFACE_(IActReactEditors,IUnknown)
{
   DECLARE_UNKNOWN_PURE();

   //
   // Source Dialog
   //
   STDMETHOD(SourceDialog)(THIS_ eARDlgFlags flags, sObjStimPair* pair, sStimSourceDesc* desc) PURE;

   //
   // Receptron Dialog
   STDMETHOD(ReceptronDialog)(THIS_ eARDlgFlags flags, sObjStimPair* pair, sReceptron* tron) PURE; 

   //
   // Receptron List, either elem of pair can be LINKOBJ_WILDCARD
   STDMETHOD(ReceptronList)(THIS_ eARDlgFlags flags, const sObjStimPair* pair) PURE; 

   //
   // Source List, either elem of pair can be LINKOBJ_WILDCARD
   STDMETHOD(SourceList)(THIS_ eARDlgFlags flags, const sObjStimPair* pair) PURE; 



};

#undef INTERFACE



#endif // __AREDIT_H


