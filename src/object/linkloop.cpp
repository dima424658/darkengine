/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkloop.cpp,v 1.7 1998/04/24 14:12:38 TOML Exp $

#include <lg.h>
#include <loopapi.h>

#include <loopmsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <dbasemsg.h>
#include <linksave.h>

//
//
#include <linkloop.h>

// Must be last header
#include <dbmem.h>

/////////////////////////////////////////////////////////////
// SAMPLE LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
//
//
#define MY_FACTORY LinkLoopFactory
#define MY_GUID   LOOPID_LinkMan

//
// Context data
typedef void Context;

//
// State record
typedef struct _StateRecord
{
   Context* context;
} StateRecord;


////////////////////////////////////////////////////////////
// Database message handler
//

static void db_message(DispatchData* msg)
{
   msgDatabaseData data; 
   data.raw = msg->data;

#ifdef JUNK
   LinkMan* linkman = LinkMan::GetCurrent();
#endif 
   switch (msg->subtype)
   {
      case kDatabaseReset:
#ifdef TOO_COOL
      {
        LinkMan* next = new LinkMan();
        LinkMan::SetCurrent(next);
        if (linkman != NULL)
           delete linkman;

      }
#endif 
        break;
      case kDatabaseLoad:
         //         LoadAllLinks((IUnknown*)data.load);
         break;
      case kDatabaseSave:
         //         SaveAllLinks((IUnknown*)data.save);
         break;
   }
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata; 
   // 
   //


   switch(msg)
   {
      case kMsgDatabase:
         db_message(info.dispatch);
         break;
      case kMsgEnd:
         Free(state);
         break;   
   }
   return result;
}

////////////////////////////////////////////////////////////
// 
// Loop client factory function. 
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,&LinkLoopClientDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 


sLoopClientDesc LinkLoopClientDesc =
{
   &MY_GUID,
   "Link Manager",              
   kPriorityNormal,              
   kMsgEnd|kMsgDatabase, // interests 

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      {kNullConstraint}
   }
};

