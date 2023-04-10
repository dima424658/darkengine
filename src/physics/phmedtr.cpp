/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phmedtr.cpp,v 1.2 2000/02/19 12:32:17 toml Exp $

#include <phmedtr.h>
#include <dynarray.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// PHYSICS MEDIA TRANSITION CALLBACK CHAIN
//

struct sMediaCB
{
   tPhysMediaHandler func;
   void* data; 
}; 

//
// The static list of all such handlers
//

typedef cDynArray<sMediaCB> cMediaCBList; 

static cMediaCBList gMediaCBList; 

////////////////////////////////////////

void PhysMediaTransitionsInit(void)
{
   
}

void PhysMediaTransitionsTerm(void)
{
   
}

////////////////////////////////////////


void PhysListenToMediaEvents(tPhysMediaHandler func, void* data)
{
   if (func != NULL)
   {
      sMediaCB cb = { func, data}; 
      gMediaCBList.Append(cb); 
   }
}

////////////////////////////////////////

void PhysSendMediaEvent(const sPhysMediaEvent* event)
{
   for (int i = 0; i < gMediaCBList.Size(); i++)
      gMediaCBList[i].func(event,gMediaCBList[i].data); 
}





