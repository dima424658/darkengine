/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/motion/mvrflags.cpp,v 1.7 1999/08/05 17:08:08 Justin Exp $
//

#include <lg.h>

#include <hashset.h>
#include <hshsttem.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <medmenu.h>
#include <mvrflags.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

static const char *frame_flag_names[] = 
{
   "Standing",
   "Left Footfall",
   "Right Footfall",
   "Left Foot Up",
   "Right Foot Up", 
   "Fire Release",
   "Can Interrupt",
   "Start Motion Here",
   "End Motion Here",
   "",
   "",
   "",
   "Trigger 1",
   "Trigger 2",
   "Trigger 3",
   "Trigger 4",
   "Trigger 5",
   "Trigger 6",
   "Trigger 7",
   "Trigger 8",
};

static sFieldDesc frame_flag_fields[] =
{
   { "Flag Value", kFieldTypeBits, FieldLocation(sFlagValue, value), FullFieldNames(frame_flag_names) },
};

sStructDesc frame_flag_desc = StructDescBuild(sFlagValue, kStructFlagNone, frame_flag_fields);

////////////////////////////////////////////////////////////////////////////////

class cMotionFlagTable : public cHashSet <sFlagListen *, const ObjID *, cHashFunctions>
{
   virtual tHashSetKey GetKey(tHashSetNode p) const
   {
      return (tHashSetKey)&((sFlagListen *)p)->objID;
   }
};

cMotionFlagTable motionFlagTable;

////////////////////////////////////////////////////////////////////////////////

void MotionFlagsInit()
{
   motionFlagTable.DestroyAll();
}

////////////////////////////////////////

void MotionFlagsTerm()
{
   motionFlagTable.DestroyAll();
}

////////////////////////////////////////////////////////////////////////////////

void ClearMotionFlagListeners(ObjID objID)
{
   sFlagListen *pFlagListen;

   if ((pFlagListen = motionFlagTable.Search(&objID)) != NULL)
      delete motionFlagTable.Remove(pFlagListen);
}

////////////////////////////////////////

void AddMotionFlagListener(ObjID objID, ulong flags, tFlagListenFunc ListenerFunc)
{
   sFlagListen *pFlagListen;
   BOOL new_entry = FALSE;
   int i;

   if ((pFlagListen = motionFlagTable.Search(&objID)) == NULL)
   {
      new_entry = TRUE;
      pFlagListen = new sFlagListen;

      pFlagListen->objID = objID;
      pFlagListen->interestedFlags = flags;
      pFlagListen->triggeredFlags = 0;

      for (i=0; i<MF_MAX_BITS; i++)
         pFlagListen->fnFlagListeners[i] = NULL;
   }
   else
      pFlagListen->interestedFlags |= flags;

   for (i=0; i<MF_MAX_BITS; i++)
   {
      if (flags & (1 << i))
         pFlagListen->fnFlagListeners[i] = ListenerFunc;
   }

   if (new_entry)
      motionFlagTable.Insert(pFlagListen);
}

////////////////////////////////////////

void RemoveMotionFlagListener(ObjID objID, ulong flags)
{
   sFlagListen *pFlagListen;

   if ((pFlagListen = motionFlagTable.Search(&objID)) == NULL)
      return;

   BOOL any_flags = FALSE;

   for (int i=0; i<MF_MAX_BITS; i++)
   {
      if (flags & (1 << i))
      {
         pFlagListen->interestedFlags &= ~(1 << i);
         pFlagListen->fnFlagListeners[i] = NULL;
      }

      if (pFlagListen->interestedFlags & (1 << i))
         any_flags = TRUE;
   }

   if (!any_flags)
      delete motionFlagTable.Remove(pFlagListen);
}

////////////////////////////////////////

void NotifyMotionFlags(ObjID objID, ulong flags, IMotor *pMotor, cMotionCoordinator *pCoord)
{
   sFlagListen *pFlagListen;

   if ((pFlagListen = motionFlagTable.Search(&objID)) == NULL)
      return;

   for (int i=0; i<MF_MAX_BITS; i++)
   {
      if ((flags & (1 << i)) && (pFlagListen->interestedFlags & (1 << i)))
      {
         // Either callback or set triggered flags
         if (pFlagListen->fnFlagListeners[i] != NULL)
            pFlagListen->fnFlagListeners[i](objID, pMotor, pCoord, (1 << i));
         else
            pFlagListen->triggeredFlags |= (1 << i);
      }
   }
}

////////////////////////////////////////

ulong PollMotionFlags(ObjID objID, ulong flags)
{
   sFlagListen *pFlagListen;

   if ((pFlagListen = motionFlagTable.Search(&objID)) == NULL)
   {
      Warning(("Polling motion flags of object without motion flag listener.\n"));
      return 0;
   }

   ulong triggered = 0;

   // Build triggered flags
   for (int i=0; i<MF_MAX_BITS; i++)
   {
      if (flags & (1 << i))
         triggered |= (pFlagListen->triggeredFlags & (1 << i));
   }

   // Clear triggered flags we are informing about
   pFlagListen->triggeredFlags &= ~triggered;

   return triggered;
}

////////////////////////////////////////////////////////////////////////////////





