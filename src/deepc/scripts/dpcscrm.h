/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once  
#ifndef __DPCSCRM_H
#define __DPCSCRM_H

#include <scrptmsg.h>

///////////////////////////////////////////////////////////////////////////////
//
// DEEP COVER SCRIPT MESSAGES
//

// Yes or No Dialog
#define kYorNMsgVer 1

struct sYorNMsg : public sScrMsg
{
   bool yes;

   sYorNMsg()
   {
   }

   // real constructor
   sYorNMsg(ObjID obj_to, bool decision)
   {
      // setup base scrMsg fields
      to = obj_to;
      free((void *)message);
      message = strdup("YorNDone");

      // then our fields
      yes = decision;
   }
         
   DECLARE_SCRMSG_PERSISTENT();

};

#ifdef SCRIPT
#define OnYorNDone()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(YorNDone, sYorNMsg)
#define DefaultOnYorNDone()  SCRIPT_CALL_BASE(YorNDone)
#endif

// Keypad messages
#define kKeypadMsgVer 1

struct sKeypadMsg : public sScrMsg
{
   int code;

   sKeypadMsg()
   {
   }

   // real constructor
   sKeypadMsg(ObjID obj_to, int codeval)
   {
      // setup base scrMsg fields
      to = obj_to;
      free((void *)message);
      message = strdup("KeypadDone");

      // then our fields
      code = codeval;
   }
         
   DECLARE_SCRMSG_PERSISTENT();

};

#ifdef SCRIPT
#define OnKeypadDone()         SCRIPT_MESSAGE_HANDLER_SIGNATURE_(KeypadDone, sKeypadMsg)
#define DefaultOnKeypadDone()  SCRIPT_CALL_BASE(KeypadDone)
#endif



#endif // __DPCSCRM_H
