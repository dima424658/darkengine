/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapiact.h,v 1.9 1999/12/01 16:05:26 BFarquha Exp $
//
//
//

#ifndef __AIAPIACT_H
#define __AIAPIACT_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <dynarray.h>

#include <ctagset.h>

#include <aiapicmp.h>

#pragma once
#pragma pack(4)

class cAnsiStr;
typedef cAnsiStr cStr;

F_DECLARE_INTERFACE(IAIActor);
F_DECLARE_INTERFACE(IAIAction);
F_DECLARE_INTERFACE(IAIPath);

typedef unsigned tAIActionType;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIActions
//

class cAIActions : public cDynArray<IAIAction *>
{
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIAction
//

// No percentage known
#define kAIA_PctUnknown (-1)

enum eAIActionFlags
{
   kAIAF_UsesMoveEnactor  = 0x01,
   kAIAF_UsesSoundEnactor = 0x02,

   kAIAF_ApiFirstAvail    = 0x04,

   kAIAF_ApiIntMax = 0xffffffff
};

///////////////////////////////////////

struct sAIAction
{
   // Kind of action
   tAIActionType  type;

   // Tag set
   cTagSet        tags;

   // Flags
   unsigned       flags;

   // Time the action was created
   ulong          time;

   // Representation of progress, if possible
   int            pctComplete;

   // The result of the action
   eAIResult      result;

   // Action suggestor
   IAIActor *   pOwner;

   #define kAIA_OwnerDataSetSize 2
   union
   {
      DWORD       ownerData;
      DWORD       ownerDataSet[kAIA_OwnerDataSetSize];
   };
};

///////////////////////////////////////

const char * AIGetActTypeName(tAIActionType);

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIAction
//

DECLARE_INTERFACE_(IAIAction, IUnknown)
{
   //
   // Describe the action, in detailed human terms
   //
   STDMETHOD_(void, Describe)(cStr * pStr) PURE;

   //
   // Access the internals of the action
   //
   STDMETHOD_(BOOL,              InProgress)() PURE;
   STDMETHOD_(tAIActionType,     GetType)() PURE;
   STDMETHOD_(eAIResult,         GetResult)() PURE;
   STDMETHOD_(DWORD,             GetData)(unsigned index = 0) PURE;
   STDMETHOD_(const sAIAction *, Access)() PURE;

   //
   // Proxy support
   //
   // Note a proxy action need not necessarily be kAIAT_Proxy -- kAIAT_Proxy
   // is just one possible implementation
   //

   STDMETHOD_(BOOL,        IsProxy)() PURE;
   STDMETHOD_(IAIAction *, GetInnerAction)() PURE;
   STDMETHOD_(IAIAction *, GetTrueAction)() PURE;

   // Get the owner, or true owner if the action is some sort of adaptor or proxy
   STDMETHOD_(IAIActor *,  GetTrueOwner)() PURE;

   //
   // Update the action
   //
   STDMETHOD_(eAIResult, Update)() PURE;

   //
   // Do the action
   //
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime) PURE;

   //
   // Terminate the action
   //
   STDMETHOD_(eAIResult, End)() PURE;

   //
   // Save/Load already created action.  Load returns false if the action should be deleted.
   //
   STDMETHOD_(BOOL, Save)(ITagFile * pTagFile) PURE;
   STDMETHOD_(BOOL, Load)(ITagFile * pTagFile) PURE;

   //
   // Retrieve current path, if any.
   //
   STDMETHOD_(IAIPath *, GetPath)() PURE;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()
#endif /* !__AIAPIACT_H */
