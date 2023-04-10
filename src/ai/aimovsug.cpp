/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimovsug.cpp,v 1.2 1998/05/27 08:16:39 TOML Exp $
//
//
//

#include <aimovsug.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// AIComputeWeightedBias
//

struct sBiasWeighting
{
   int minWeight;    // at client bias level = 0%
   int maxWeight;    // at client bias level = +-100%
};

///////////////////////////////////////

static sBiasWeighting g_BiasWeightings[] =
{
   // Suggestion is an idle action (kAIMS_Idle)
   { 10,   15 },

   // Suggestion is a normal locomotion (kAIMS_Loco)
   { 10,   50 },

   // Suggestion is a combat maneuver (kAIMS_Combat)
   { 40,   75 },

   // Suggestion is an object avoidance/attraction (kAIMS_Object)
   { 10,   50 },

   // Suggestion is an danger avoidance (kAIMS_Danger)
   { 5,   80 },
};

///////////////////////////////////////

int AIComputeWeightedBias(eAIMoveSuggKind kind, int baseBias)
{
   int wieghtedBias;
   int sign;
   
   sign = (kind != kAIMS_Danger) ? 1 : -1;

   AssertMsg(baseBias >= 0 && baseBias <= 100, "Invalid move suggestion bias");

   wieghtedBias = g_BiasWeightings[kind].minWeight +
                  ((g_BiasWeightings[kind].maxWeight -
                    g_BiasWeightings[kind].minWeight) * baseBias) / 100;

   wieghtedBias *= sign;

   AssertMsg((kind != kAIMS_Danger) || (baseBias >= 0), "Expected wieghtedBias to be non-positive");
   
   return wieghtedBias;
}

///////////////////////////////////////////////////////////////////////////////
