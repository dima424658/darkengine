/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Deep Cover AI Combat - ranged backup
//

#include <dpcaicrb.h>
#include <dpcaiutl.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

int cAIDPCRangedBackup::SuggestApplicability(void)
{
   if (AIInDoorTripwire(GetID()))
      return kAIRC_AppNone;
   return cAIRangedBackup::SuggestApplicability();
}

