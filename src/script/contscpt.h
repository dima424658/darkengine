/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/contscpt.h,v 1.2 2000/01/29 13:39:52 adurant Exp $
#pragma once

#ifndef __CONTSCPT_H
#define __CONTSCPT_H

#include <scrptmsg.h>
#include <contains.h>

// Container script service, for handling script issues with containment and combination

DECLARE_SCRIPT_SERVICE(Container, 0x146)
{
   // Does container contain containee? 
   STDMETHOD(Contains)(const object ref container, const object ref findobj) PURE; 
   // How many containtype-combineable objects are in container?
   STDMETHOD_(int, CombineCount)(const object ref container, const object ref containtype) PURE;
   // Put object into container
   // deals with getting it out of the world as well (hm, can do this game-independantly?)
   STDMETHOD(ContainerAdd)(const object ref container, const object ref obj);
   STDMETHOD
}

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//

#define kContainMsgVer 1

struct sContainMsg : public sScrMsg
{
   ObjID container;
   ObjID containee;

   sContainMsg()
   {
   }

   // real constructor
   sContainMsg(ObjID obj_to, ObjID container, ObjID containee, eContainsEvent evtype)
   {
      static char *msg_names[4] = { "ContainSimActivate", "ContainAdd", "ContainRemove", "ContainCombine" };

      // setup base scrMsg fields
      to = obj_to;
      free((void *)message);
      message = strdup(msg_names[evtype]);
   }
         
   DECLARE_SCRMSG_PERSISTENT();

};

#ifdef SCRIPT
#define OnContainSimActivate()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ContainSimActivate, sContainMsg)
#define DefaultOnContainSimActivate()    SCRIPT_CALL_BASE(ContainSimActivate)
#define OnContainAdd()            SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ContainAdd, sContainMsg)
#define DefaultOnContainAdd()     SCRIPT_CALL_BASE(ContainAdd)
#define OnContainRemove()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ContainRemove, sContainMsg)
#define DefaultOnContainRemove()    SCRIPT_CALL_BASE(ContainRemove)
#define OnContainCombine()           SCRIPT_MESSAGE_HANDLER_SIGNATURE_(ContainCombine, sContainMsg)
#define DefaultOnContainCombine()    SCRIPT_CALL_BASE(ContainCombine)
#endif

#endif