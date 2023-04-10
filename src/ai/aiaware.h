/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiaware.h,v 1.9 2000/01/04 19:28:50 BFarquha Exp $
//
//
//

#ifndef __AIAWARE_H
#define __AIAWARE_H

#include <aitype.h>
#include <aiutils.h>
#include <linktype.h>

F_DECLARE_INTERFACE(IRelation);

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

void AIInitAwareness();
void AITermAwareness();

///////////////////////////////////////
//
// Awareness links. It is *strongly* recommended code use the functions on
// IInternalAI and IAISenses to access these.
//

extern IRelation * g_pAIAwarenessLinks;

///////////////////////////////////////////////////////////////////////////////
//
// Enums and constants
//

enum eAIAwarenessFlags
{
   // The object can be sensed
   kAIAF_Seen         =   0x01,
   kAIAF_Heard        =   0x02,
   kAIAF_Sensed       =   (kAIAF_Seen | kAIAF_Heard),

   // Could have LOS if had 360 vision
   kAIAF_CanRaycast   =   0x04,

   // Have line of sight
   kAIAF_HaveLOS      =   0x08,

   // Blind to the object
   kAIAF_Blind        =   0x10,

   // Deaf to the object
   kAIAF_Deaf         =   0x20,

   kAIAF_Highest      =   0x40,

   kAIAF_FirstHand    =   0x80,

   kAIAF_Freshened    = 0x0100
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIAwareCapacitor
//
// Describes how long it takes, in the absense of stimulation, for an awareness
// link to "cool down"
//

struct sAIAwareCapacitor
{
   union
   {
      struct
      {
         // Time to discharge from 1 to 0
         int low;

         // Time to discharge from 2 to 1
         int medium;

         // Time to discharge from 3 to 2
         int high;
      };

      int dischargeTimes[3];
   };

   // For future use
   int reserved;
};

///////////////////////////////////////

extern sAIAwareCapacitor g_AIDefAwareCap;

///////////////////////////////////////////////////////////////////////////////
//
// Reaction times
//

struct sAIAwareDelay
{
   unsigned toTwo;
   unsigned toThree;

   unsigned twoReuse;
   unsigned threeReuse;

   unsigned ignoreRange;
};

///////////////////////////////////////////////////////////////////////////////
//
// STRUCT: sAIAwareness
//
// Describes how aware of an object an AI is
//

struct sAIAwareness
{
   sAIAwareness()
   {
      memset(this, 0, sizeof(*this));
   }

   eAIAwareLevel DecLev()
   {
      return (eAIAwareLevel)(--(*((int *)&level)));
   }

   eAIAwareLevel IncLev()
   {
      return (eAIAwareLevel)(++(*((int *)&level)));
   }

   BOOL ValidLastPos() const
   {
      return !(lastPos.x == FLT_MAX && lastPos.y == FLT_MAX && lastPos.z == FLT_MAX);
   }

   ulong TimeSinceContact() const
   {
      return AIGetTime() - lastContact;
   }

   // Of what the awareness pertains
   ObjID         object;

   // Flags
   unsigned      flags;

   // Current awareness
   eAIAwareLevel level;

   // Highest awareness of object
   eAIAwareLevel peak;

   // Time entered current level
   int           time;

   // Time of last contact
   int           lastContact;

   // Location of last contact
   mxs_vector    lastPos;

   // Level of last pulse
   eAIAwareLevel lastPulse;

   // Vision cone, if any, that is the source of the awareness
   int           sourceCone;

   // Vision management data
   ulong         updateTime;
   ulong         LOSTime;

   LinkID        linkID;

   // True last contact (ignoring forced freshness)
   int           trueLastContact;
   int           freshness;

   // Set first time going to high alert.
   BOOL          bDidHighAlert;
};

///////////////////////////////////////

EXTERN sAIAwareness g_AINullAwarenessScratch;

inline const sAIAwareness * AINullAware(ObjID object)
{
   g_AINullAwarenessScratch.object = object;
   g_AINullAwarenessScratch.lastPos = kInvalidLoc;
   return &g_AINullAwarenessScratch;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAWARE_H */
