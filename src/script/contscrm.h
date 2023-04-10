/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/contscrm.h,v 1.4 1998/09/09 02:28:13 mahk Exp $
#pragma once  
#ifndef __CONTSCRM_H
#define __CONTSCRM_H
#include <scrptmsg.h>

////////////////////////////////////////////////////////////
// Container script message 
// 



struct sContainerScrMsg: public sScrMsg
{
   sContainerScrMsg(ObjID towhom, int ev, int targ)
      : event(ev),  
        containee(targ),
        sScrMsg(towhom,"Container")
   {
   };

   // contains event, see contain.h 
   // Only Add, Remove, and Combine events are sent. 
   // For a combine event, the containee is a member of the container that 
   // is the result of the combination 
   int event; 
   int containee; // contanee object.  self is container
}; 

struct sContainedScrMsg: public sScrMsg
{
   sContainedScrMsg(ObjID towhom, int ev, int targ)
      : event(ev),  
        container(targ),
        sScrMsg(towhom,"Contained")
   {
   };

   // contains event, see contain.h 
   // Only Add, Remove, and Combine events are sent. 
   // For a combine event, the containee is a member of the container that 
   // is the result of the combination 
   int event; 
   int container; // contaner object.  self is containee
}; 


struct sCombineScrMsg: public sScrMsg
{
   sCombineScrMsg(ObjID towhom, int targ)
      : combiner(targ),
        sScrMsg(towhom,"Combine")
   {
   };

   int combiner; // the other object that combines with us, and is about to die
}; 


#ifdef SCRIPT
#define OnContainer() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Container,sContainerScrMsg)
#define DefaultOnContainer() SCRIPT_CALL_BASE(Container)

#define OnContained() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Contained,sContainedScrMsg)
#define DefaultOnContained() SCRIPT_CALL_BASE(Contained)


#define OnCombine() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(Combine,sCombineScrMsg)
#define DefaultOnCombine() SCRIPT_CALL_BASE(Combine)


#define CONTAINER_MESSAGE_DEFAULTS() \
        SCRIPT_DEFINE_MESSAGE_DEFAULT_(Combine,sCombineScrMsg) \
        SCRIPT_DEFINE_MESSAGE_DEFAULT_(Container,sContainerScrMsg) \
        SCRIPT_DEFINE_MESSAGE_DEFAULT_(Contained,sContainedScrMsg)


#define CONTAINER_MESSAGE_MAP_ENTRIES() \
           SCRIPT_MSG_MAP_ENTRY(Combine) \
           SCRIPT_MSG_MAP_ENTRY(Container) \
           SCRIPT_MSG_MAP_ENTRY(Contained) 


#endif // SCRIPT

#endif // __CONTSCRM_H
