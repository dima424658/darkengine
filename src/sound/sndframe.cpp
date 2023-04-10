/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndframe.cpp,v 1.2 2000/02/28 17:28:27 toml Exp $

#include <sndframe.h>

#include <dynarray.h>
#include <dbmem.h>

// Simple implementation:
// Because number of sounds started in each frame is small, we
// have a dynarray of all objid's which started sounds in a frame.
// When testing for existence of one ObjID, we simply linear search.

// Because there's no one time when a sound is initiated 
static cDynArray<ObjID> sgObjs[2];
static int  sg_iCurrentTracking = 0;

void SNDFrameInit()
{
   sg_iCurrentTracking = 0;
   sgObjs[0].SetSize(0);
   sgObjs[1].SetSize(0);
}

void SNDFrameTerm()
{
   sgObjs[0].SetSize(0);
   sgObjs[1].SetSize(0);
}

void SNDFrameNextFrame()
{
   sg_iCurrentTracking = 1 - sg_iCurrentTracking;
   sgObjs[sg_iCurrentTracking].SetSize(0);
}

void SNDFrameAddObj( ObjID objID )
{
   sgObjs[sg_iCurrentTracking].Append( objID );
}

BOOL SNDFramePlayedObj( ObjID objID )
{
   cDynArray<ObjID> & arr = sgObjs[1 - sg_iCurrentTracking];
   int arrSize = arr.Size();

   for( int iObj = 0; iObj < arrSize; iObj++ )
   {
      if( arr[ iObj ] == objID )
         return TRUE;
   }

   return FALSE;
}
