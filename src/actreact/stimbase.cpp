/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/stimbase.cpp,v 1.2 1998/10/05 17:28:15 mahk Exp $

#include <objtype.h>
#include <stimtype.h>
#include <stimbase.h>
#include <chevkind.h>


// Must be last header 
#include <dbmem.h>


static sStimEventData def_data = { 0 }; 

sStimEvent::sStimEvent(sStimEventData* dat,sChainedEvent* ev)
   : sChainedEvent(kEventKindStim,sizeof(sStimEvent),ev),
     sStimEventData((dat) ? *dat : def_data)
{
} 




