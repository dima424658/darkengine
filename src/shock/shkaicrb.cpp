/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaicrb.cpp,v 1.1 1999/04/11 11:08:52 JON Exp $
//
// Shock AI Combat - ranged backup
//

#include <shkaicrb.h>
#include <shkaiutl.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

int cAIShockRangedBackup::SuggestApplicability(void)
{
   if (AIInDoorTripwire(GetID()))
      return kAIRC_AppNone;
   return cAIRangedBackup::SuggestApplicability();
}

