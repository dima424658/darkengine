/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/simtime.cpp,v 1.16 2000/03/17 17:40:22 bfarquha Exp $

#include <lg.h>
#include <loopapi.h>

#include <dispbase.h>
#include <loopmsg.h>

#include <dbasemsg.h>
#include <dbtype.h>
#include <tagfile.h>
#include <vernum.h>

#include <simtloop.h>

#include <simtime.h>
#include <simtbase.h>
#include <simstate.h>
#include <simflags.h>

#include <netman.h>

// not the most appropriate place, but not the least either
#include <simman.h>
#include <appagg.h>
#include <plyrmode.h>

#include <config.h>
#include <command.h>
#include <status.h>

#include <timings.h>

#include <mprintf.h>


// Must be last header
#include <dbmem.h>


////////////////////////////////////////////////////////////
// SIM TIME SYSTEM
//

static struct sSimTime
{
   tSimTime nTicks; // ticks since start of sim
   tSimTime dTicks; // ticks since last frame
} gSimTime;

static tSimTime gFileTicks = 0; // ticks at last save/load

static INetManager *gpNetMan = NULL;

static sSimTimingParams gParams =
{
   DEFAULT_MIN_SIM_TIME,
   DEFAULT_MAX_SIM_TIME,
   1.0,
};

//------------------------------------------------------------

static tSimTime clamp_frame_time(tSimTime ftime)
{
   if (ftime < gParams.minFrameTime)
      ftime = gParams.minFrameTime;
#ifdef NEW_NETWORK_ENABLED
   if (gParams.maxFrameTime != NO_MAX_TIME && !gpNetMan->Networking())
#else
   if (gParams.maxFrameTime != NO_MAX_TIME)
#endif
   {
      if (ftime > gParams.maxFrameTime)
         ftime = gParams.maxFrameTime;
   }

   if (gParams.scaleFrameTime > 0.0)
      ftime *= gParams.scaleFrameTime;

   return ftime;
}



//------------------------------------------------------------
// API FUNCTIONS
//

tSimTime GetSimTime(void)
{
   return gSimTime.nTicks;
}

tSimTime GetSimFrameTime(void)
{
   return gSimTime.dTicks;
}

tSimTime GetSimFileTime(void)
{
   return gFileTicks;
}

void SetSimTime(tSimTime time)
{
   gSimTime.nTicks = time;
   gSimTime.dTicks = 0;
}

void SetSimTimePassing(BOOL passing)
{
   SimStateSetFlags(kSimTimePasses,passing);
}

BOOL IsSimTimePassing(void)
{
   return SimStateCheckFlags(kSimTimePasses) == kSimTimePasses;
}

void SetSimTimingParams(const sSimTimingParams* params)
{
   gParams = *params;
}

const sSimTimingParams*  GetSimTimingParams(void)
{
   return &gParams;
}

void SetSimTimeScale(float scale)
{
   gParams.scaleFrameTime = scale;
}

void SetPlayerSpeedScale(float scale)
{
   RemoveSpeedScale("SimTimeScale");
   AddSpeedScale("SimTimeScale", scale, scale);
}

////////////////////////////////////////////////////////////
// TIME COMMANDS
//

#ifndef SHIP
EXTERN void show_sim_time(void)
{
   char buf[80] = "Sim Time: ";
   char *s = buf + strlen(buf);
   tSimTime time = GetSimTime();
   if (time > SIM_TIME_HOUR)  // more than an hour
   {
      sprintf(s,"%d:",time/SIM_TIME_HOUR);
      s += strlen(s);
      time %= SIM_TIME_HOUR;
   }
   // minutes
   sprintf(s,"%02d:",time/SIM_TIME_MINUTE);
   s += strlen(s);
   time %= SIM_TIME_MINUTE;

   // seconds
   sprintf(s,"%02d",time/SIM_TIME_SECOND);
   s += strlen(s);
   time %= SIM_TIME_SECOND;

   // hundredths
   sprintf(s,".%02d",time*100/SIM_TIME_SECOND);

   Status(buf);
}

static void time_passes_cmd(bool val)
{
   SetSimTimePassing(val);
}

static Command commands[] =
{
   { "time_passes", FUNC_BOOL, time_passes_cmd, "Set whether sim time is passing", HK_ALL},
   { "show_sim_time", FUNC_VOID, show_sim_time, "Display the sim time on the status bar.", HK_ALL},
   { "scale_sim_time", FUNC_FLOAT, SetSimTimeScale, "Scale the speed at which the sim runs", HK_ALL},
   { "scale_player_speed", FUNC_FLOAT, SetPlayerSpeedScale, "Scale the player's speed", HK_ALL},
};
#endif

//------------------------------------------------------------

static void simtime_app_init(void)
{
#ifndef SHIP
   COMMANDS(commands,HK_ALL);

   sSimTimingParams params = *GetSimTimingParams();
   config_get_int("max_frame_time",&params.maxFrameTime);
   config_get_int("min_frame_time",&params.minFrameTime);

   config_get_int("force_frame_time", &params.maxFrameTime);
   config_get_int("force_frame_time", &params.minFrameTime);

   config_get_float("scale_sim_time", &params.scaleFrameTime);

   SetSimTimingParams(&params);
#endif
}

static void simtime_app_term(void)
{

}

////////////////////////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//

static TagFileTag TimeTag = { "SIM_TIME" };
static TagVersion TimeVersion = { 0, 1};

static void move_sim_time(ITagFile* file)
{
   TagVersion v = TimeVersion;
   if (SUCCEEDED(file->OpenBlock(&TimeTag,&v)))
   {
      sSimTime time = gSimTime;
      file->Move((char*)&time,sizeof(time));
      if (file->OpenMode() == kTagOpenRead)
         gSimTime = time;

      file->CloseBlock();
   }
}


static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   AutoAppIPtr_(SimManager,pSimMan);

   switch(DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         pSimMan->StopSim();
         SetSimTime(0);
         gFileTicks = 0;

         break;
      case kDatabaseLoad:
      case kDatabaseSave:
         if (msg->subtype & kDBMission)
         {
            move_sim_time(data.load);
            gFileTicks = gSimTime.nTicks;
         }
         break;
   }
   pSimMan->DatabaseMsg(msg->subtype,data.load);

}


/////////////////////////////////////////////////////////////
// SIM TIME LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//



#define MY_GUID   LOOPID_SimTime

//
//
// My context data
typedef void Context;

//
//
// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
   BOOL spew;
} StateRecord;

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

static void SlowFrame(long nMilliseconds)
{
   long nStart = timeGetTime();
   while ((timeGetTime()-nStart) < nMilliseconds)
   {}
}


static int nSlowFrameTime = 0;
static long nLastRealFrameTime = 0;

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;
   long nCurrentRealFrameTime;
   long nRealTimeDelta;


   info.raw = hdata;

   switch(msg)
   {
      case kMsgEnterMode:
      case kMsgResumeMode:
      {
         config_get_int("SlowFrame", &nSlowFrameTime);
         state->spew = config_is_defined("spew_sim_time");
         // Check whether this is a multi-player game whenever we enter the mode
         gpNetMan = AppGetObj(INetManager);

         nLastRealFrameTime = timeGetTime(); // tm_get_millisec_unrecorded();
         break;
      }
      case kMsgBeginFrame:
         // Update sim time at start of frame
         if (IsSimTimePassing())
            gSimTime.dTicks = clamp_frame_time(info.frame->dTicks);
         else
            gSimTime.dTicks = 0;
         gSimTime.nTicks += gSimTime.dTicks;
#ifdef DBG_ON
         if (state->spew && gSimTime.dTicks > 0)
            mprintf("[%d] ",gSimTime.nTicks);
#endif

         if (nSlowFrameTime > 0)
         {
//            nCurrentRealFrameTime = tm_get_millisec_unrecorded();
            nCurrentRealFrameTime = timeGetTime();
            nRealTimeDelta = nCurrentRealFrameTime-nLastRealFrameTime;
            if (nRealTimeDelta < nSlowFrameTime)
               SlowFrame(nSlowFrameTime-nRealTimeDelta);
            nLastRealFrameTime = timeGetTime();
         }
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgAppInit:
         simtime_app_init();
         break;

      case kMsgAppTerm:
         simtime_app_term();
         break;


      case kMsgEnd:
         Free(state);
         break;
   }
   return result;
}


////////////////////////////////////////////////////////////
// Frome here on in is boiler plate code.
// Nothing need be changed.
//


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

   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//



sLoopClientDesc SimTimeLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Sim Time Client",                    // NAME
   kPriorityAppLibrary,                    // PRIORITY
   kMsgEnd | kMsgsMode | kMsgDatabase| kMsgsFrame | kMsgsAppOuter,   // INTERESTS

   kLCF_Callback,
   _CreateClient,

   NO_LC_DATA,

   {
      {kNullConstraint} // terminator
   }
};

