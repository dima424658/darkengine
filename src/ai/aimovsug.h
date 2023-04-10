/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimovsug.h,v 1.4 1998/10/03 10:30:40 TOML Exp $
//
// Movement suggestions: the heart of the locomotion movement suggestion
// system
//

#ifndef __AIMOVSUG_H
#define __AIMOVSUG_H

#include <dynarray.h>
#include <aitype.h>

#pragma once
#pragma pack(4)

struct sAIMoveSuggestion;

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

enum eAIMoveSuggKindEnum
{
   // Suggestion is an idle action
   kAIMS_Idle,

   // Suggestion is a normal locomotion
   kAIMS_Loco,

   // Suggestion is a combat maneuver
   kAIMS_Combat,

   // Suggestion is an object avoidance/attraction
   kAIMS_Object,

   // Suggestion is an danger avoidance
   kAIMS_Danger,

   kAIMS_InMax = 0xffffffff
};

typedef unsigned eAIMoveSuggKind;

///////////////////////////////////////

enum eAIFacing
{
   // Doesn't matter
   kAIF_Any,

   // Face direction of movement
   kAIF_MoveDir,

   // Face the ultimate destination
   kAIF_Dest,

   // Face an alternative position
   kAIF_AltPos,

   // Face a specific direction
   kAIF_SpecificDir,

   kAIF_TypeMax=0xffffffff // force it use an int
};

///////////////////////////////////////

enum eAISuggestionFlags
{
   // Is it a regulation suggestion, as opposed to a positive ability-driven one?
   kAISF_IsRegulation = 0x01,
};

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: AIComputeWeightedBias
//
// For any given suggestion, the creator assigns a 0-100 rating
// indicating confidence about the NEED for the suggestion. This function
// weighs the suggestion relative to other suggestions, based on
// the kind of suggestion it is
//

int AIComputeWeightedBias(eAIMoveSuggKind kind, int baseBias);

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveSuggestions
//
// @Note (toml 03-31-98): Suggestions are always owned and cleaned up by suggestor
//

class cAIMoveSuggestions : public cDynArray_<sAIMoveSuggestion *, 2>
{
public:
   cAIMoveSuggestions();

   void Add(const cAIMoveSuggestions &);
   
   void DestroyAll();
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIFacing
//

struct sAIFacing
{
   eAIFacing     type;
   union
   {
      float      ang; // if kAIF_SpecificDir
      mxs_vector pos; // if kAIF_AltPos
   };
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIMoveGoal
//
// This is the lowest-level instantaneous movement goal of an AI in motion,
// the result of boiling down of numerous movement suggestions
//

struct sAIMoveGoal
{
   sAIMoveGoal();

   // The goal direction, 0 = east
   floatang   dir;

   // The final goal of the movement process, if any
   cMxsVector dest;

   // Facing requirements
   sAIFacing  facing;

   // How fast should we move
   eAISpeed   speed;

   // @TBD (toml 03-26-98): Tags?
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIMoveSuggestion
//

struct sAIMoveSuggestion
{
   sAIMoveSuggestion();
   sAIMoveSuggestion(const sAIMoveSuggestion &);

   // Set the kind and weighted bias
   void SetWeightedBias(eAIMoveSuggKind newKind, int newBias);

   // The kind of the suggestion
   eAIMoveSuggKind kind;

   // Suggestion bias (-100%..+100%)
   int             bias;

   // Flags
   unsigned        flags;

   // The goal direction, 0 = east
   floatarc        dirArc;

   // The final goal of the movement process, if any
   cMxsVector      dest;

   // Facing requirements
   sAIFacing       facing;

   // How fast should we move
   eAISpeed        speed;

   // @TBD (toml 03-26-98): Tags?
};

///////////////////////////////////////////////////////////////////////////////

inline cAIMoveSuggestions::cAIMoveSuggestions()
{
}

///////////////////////////////////////

inline void cAIMoveSuggestions::Add(const cAIMoveSuggestions & from)
{
   if (from.Size())
   {
      const int oldSize = Size();
      Grow(from.Size());
      memcpy(AsPointer() + oldSize, from.AsPointer(), sizeof(sAIMoveSuggestion *) * from.Size());
   }
}

///////////////////////////////////////

inline void cAIMoveSuggestions::DestroyAll()
{
   for (int i = 0; i < Size(); i++)
   {
      delete (AsPointer()[i]);
   }
   SetSize(0);
}

///////////////////////////////////////////////////////////////////////////////

inline sAIMoveGoal::sAIMoveGoal()
{
   memset(this, 0, sizeof(sAIMoveGoal));
}

///////////////////////////////////////////////////////////////////////////////

inline sAIMoveSuggestion::sAIMoveSuggestion()
{
   memset(this, 0, sizeof(sAIMoveSuggestion));
}

///////////////////////////////////////

inline sAIMoveSuggestion::sAIMoveSuggestion(const sAIMoveSuggestion &from)
{
   memcpy(this, &from, sizeof(sAIMoveSuggestion));
}

///////////////////////////////////////

inline void sAIMoveSuggestion::SetWeightedBias(eAIMoveSuggKind newKind, int newBias)
{
   kind = newKind;
   bias = AIComputeWeightedBias(kind, newBias);
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIMOVSUG_H */
