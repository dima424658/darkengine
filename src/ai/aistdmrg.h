/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aistdmrg.h,v 1.5 1999/12/01 16:07:07 BFarquha Exp $
//
// The standard movement regulators
//
// @Note (toml 03-31-98):
// Most of the ones from the previous AI have been removed or reexpressed. Do
// do: possibly reintegrate cliff prediction, object avoidance, maybe holy/esp
// related game specific forces
//

#ifndef __AISTDMRG_H
#define __AISTDMRG_H

#include <aibasmrg.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWallsCliffsMovReg
//
// Responsible for making sure the AI doesn't run into walls
//

class cAIWallsCliffsMovReg : public cAIMoveRegulator
{
public:
   STDMETHOD_(const char *, GetName)();

   //
   STDMETHOD_(BOOL, NewRegulations)();

   //
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &);

};

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIObjectsMovReg
//
// Responsible for making sure the AI doesn't run into objects
//

class cAIObjectsMovReg : public cAIMoveRegulator
{
public:
   STDMETHOD_(const char *, GetName)();

   //
   STDMETHOD_(BOOL, NewRegulations)();

   //
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &);
};


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILVLMovReg
//
// Responsible for making sure the AI doesn't run into unused LVL cells
//

class cAILVLMovReg : public cAIMoveRegulator
{
public:
   STDMETHOD_(const char *, GetName)();

   //
   STDMETHOD_(BOOL, NewRegulations)();

   //
   STDMETHOD (SuggestRegulations)(cAIMoveSuggestions &);

};


///////////////////////////////////////////////////////////////////////////////


#pragma pack()

#endif /* !__AISTDMRG_H */
