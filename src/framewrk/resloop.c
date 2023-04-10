/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <config.h>
#include <cfgdbg.h>
#include <res.h>

#include <loopapi.h>

#include <resloop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

typedef struct _ResState 
{
   ResLoopContext* context;
   int resfilenums[NUM_RESLOOP_FILES]; 
} ResState;


#pragma off(unreferenced)
eLoopMessageResult LGAPI ResLoopFunc(void* context, eLoopMessage msg, tLoopMessageData hdata)
{
   eLoopMessageResult result = kLoopDispatchContinue; 
   int i;
   ResState* state = (ResState*)context;
   sLoopFrameInfo* info = (msg & kMsgsFrame) ? (sLoopFrameInfo*)hdata : NULL;

   switch(msg)
   {
      case kMsgEnterMode:
         for (i = 0; i < NUM_RESLOOP_FILES; i++)
            if (state->context->resfiles[i] != NULL)
            {
               char* resfilename = state->context->resfiles[i];
               ConfigSpew("resclient",("Loading res file %s\n",resfilename));
               state->resfilenums[i] = 
                  ResOpenFile(resfilename);
               Assrt(state->resfilenums[i] >= 0);
            }
      case kMsgResumeMode: 
         break;
      case kMsgExitMode:
         for (i = 0; i < NUM_RESLOOP_FILES; i++)
            if (state->context->resfiles[i] != NULL)
            {
               ResCloseFile(state->resfilenums[i]);
            }
         break;
      case kMsgSuspendMode:
         break;
      case kMsgEnd:
         Free(state);
         break;
   }
   return result;
}

#pragma off(unreferenced)
ILoopClient* LGAPI CreateResClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   ResLoopContext* context = (ResLoopContext*)data;
   ResState* state;
   state = Malloc(sizeof(ResState));
   state->context = context;
   
   return CreateSimpleLoopClient(ResLoopFunc,state,&ResLoopClientDesc);
}
#pragma on(unreferenced)

sLoopClientDesc ResLoopClientDesc =
{
   &LOOPID_Res, 
   "Resource file loader",
   kPriorityHigh, 
   kMsgsMode|kMsgEnd,
   
   kLCF_Callback,
   CreateResClient,
   
   NO_LC_DATA,

   {
      { kNullConstraint }
   }
};


