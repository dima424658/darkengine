/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/engscrpt.h,v 1.29 1998/08/11 14:12:29 mahk Exp $
//
//

#pragma once

#ifndef __ENGSCRPT_H
#define __ENGSCRPT_H

#include <scrptapi.h>
#include <scrptsrv.h>
#include <objscrt.h>

#include <objscrm.h>
#include <objscrpt.h>
#include <linkscpt.h>
#include <propscpt.h>
#include <pupscrpt.h>
#include <arscrp.h>
#include <arscrs.h>
#include <contscrs.h>
#include <contscrm.h>
#include <simscrm.h>
#include <antxscpt.h>
#include <datascrs.h>


///////////////////////////////////////////////////////////////////////////////
//
// UTILITIES
//

///////////////////////////////////////////////////////////////////////////////
//
// SERVICES
//

///////////////////////////////////////
//
// Debugging and other non-ship functions
//

DECLARE_SCRIPT_SERVICE(Debug, 0xd7)
{
   STDMETHOD (MPrint)(const string ref,
                      const string ref = NULL_STRING,
                      const string ref = NULL_STRING,
                      const string ref = NULL_STRING,
                      const string ref = NULL_STRING,
                      const string ref = NULL_STRING,
                      const string ref = NULL_STRING,
                      const string ref = NULL_STRING) PURE;

   STDMETHOD (Command)(const string ref,
                       const string ref = NULL_STRING,
                       const string ref = NULL_STRING,
                       const string ref = NULL_STRING,
                       const string ref = NULL_STRING,
                       const string ref = NULL_STRING,
                       const string ref = NULL_STRING,
                       const string ref = NULL_STRING) PURE;

   STDMETHOD (Break)() PURE;
};

///////////////////////////////////////////////////////////////////////////////
//
// MESSAGES
//

///////////////////////////////////////////////////////////////////////////////
//
// Base scripts
//

/////////
// here is where we put headers for script stuff which will be part of the engine
// root script message map, but who have header/script stuff in their own header
#include <twqscrpt.h>  // IMPLEMENTS TweqComplete
#include <lockscpt.h>  // implements NowLocked, NowUnlocked
#include <sndscrpt.h>  // SoundDone and SchemaDone
#include <frobscrp.h>  // frob message maps
#include <keyscpt.h>   // implements the key service, no messages, though
#include <litscrpt.h>  // animated light control
#include <bodscrpt.h>
#include <quesscpt.h>  // quest data change messages

#ifdef SCRIPT

///////////////////////////////////////
//
// SCRIPT: EngineRootScript
//

////////////////////////////////////////
//
// Services
//

BEGIN_ROOT_SCRIPT(EngineRootScript)

   MESSAGE_DEFS:

      SCRIPT_DEFINE_MESSAGE_DEFAULT_(TweqComplete,sTweqMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT(NowLocked)
      SCRIPT_DEFINE_MESSAGE_DEFAULT(NowUnlocked)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(SoundDone, sSoundDoneMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(SchemaDone, sSchemaDoneMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(QuestChange, sQuestMsg)
      SCRIPT_DEFINE_MESSAGE_DEFAULT_(Sim,sSimMsg)
      
      BODY_MESSAGE_DEFAULTS()
      OBJSYS_MESSAGE_DEFAULTS()
      FROB_MESSAGE_DEFINES()      
      CONTAINER_MESSAGE_DEFAULTS()

      SCRIPT_BEGIN_MESSAGE_MAP()
         SCRIPT_MSG_MAP_ENTRY(TweqComplete)
         SCRIPT_MSG_MAP_ENTRY(NowLocked)
         SCRIPT_MSG_MAP_ENTRY(NowUnlocked)
         SCRIPT_MSG_MAP_ENTRY(SoundDone)
         SCRIPT_MSG_MAP_ENTRY(SchemaDone)
         SCRIPT_MSG_MAP_ENTRY(QuestChange)
         SCRIPT_MSG_MAP_ENTRY(Sim)
  
         CONTAINER_MESSAGE_MAP_ENTRIES() 
         BODY_MESSAGE_MAP_ENTRIES()
         OBJSYS_MESSAGE_MAP_ENTRIES()
         FROB_MSG_MAP()
      SCRIPT_END_MESSAGE_MAP()

END_SCRIPT(EngineRootScript)

#endif

///////
// i think we want to have scripts which derive from EngRoot include here
// room script messages and message map - derives from the EngineRootScript
 

///////////////////////////////////////////////////////////////////////////////

#endif // __ENGSCRPT_H
