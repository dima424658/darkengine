/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghost.cpp,v 1.6 1999/08/26 12:52:47 dc Exp $
// central ghost contoller, configuration and setup for ghosts

#include <ghost.h>
#include <ghostapi.h>
#include <ghostlst.h>
#include <ghostrcv.h>
#include <ghostsnd.h>
#include <ghostmsg.h>
#include <ghosthst.h>

#include <command.h>

#include <simtime.h>
#include <config.h>

#include <dbmem.h>

static int ghost_local_process_ms=0;
static int ghost_local_last_run=0;

//
// go through all objects in the ghost system
//   - if local, see if we need to send a new packet due to time settings
//   - if ghost, update controls, state, appropriately
// 
void GhostFrame(int dt)
{
   if (dt==0) return; // nothing happening, so who cares...
   float flt_dt=(float)dt/1000.0;

   if (ghost_local_process_ms==0 || 
       (ghost_local_last_run+ghost_local_process_ms<GetSimTime()))
   {
      tHashSetHandle local_iter;    // go through local ghosts
      sGhostLocal *pGL=gGhostLocals.GetFirst(local_iter);
      while (pGL)
      {
         _GhostFrameProcessLocal(pGL,flt_dt);    // ghostsnd.cpp
         pGL=gGhostLocals.GetNext(local_iter);
      }
      ghost_local_last_run=GetSimTime();
   }

   tHashSetHandle remote_iter;   // go through remote ghosts
   sGhostRemote *pGR=gGhostRemotes.GetFirst(remote_iter);
   while (pGR)
   {
      pGR->cfg.flags &= ~kGhostCfLoaded;
      _GhostFrameProcessRemote(pGR,flt_dt);   // ghostrcv.cpp
      pGR=gGhostRemotes.GetNext(remote_iter);
   }
}

//////////////////////////
// commands

#ifdef GHOST_DEBUGGING

extern int ghost_local_frame_rate;

static Command g_GhostCommands[] =
{
   {"ghostspew_type", VAR_INT, &targ_type, },
   {"ghostspew_obj", VAR_INT, &targ_obj, },
   {"ghost_frame_rate", VAR_INT, &ghost_local_frame_rate },
   {"ghost_process_rate", VAR_INT, &ghost_local_process_ms },
#ifdef PLAYTEST   
   {"ghostspew_show_hist", FUNC_STRING, _ghost_histo_show},
   {"ghostspew_clear_hist", FUNC_VOID, _ghost_histo_clear},
#endif   
};
void GhostSetupCommands(void)
{
   COMMANDS(g_GhostCommands, HK_ALL);
   config_get_int("ghostspew_targ_type",&targ_type);
   config_get_int("ghostspew_targ_obj",&targ_obj);
}
#else
#define GhostSetupCommands()
#endif   

//////////////////////////
// init/close

void GhostInit(void)
{
   GhostListInit();
   GhostMsgsInit();
   GhostSendInit();
   GhostSetupCommands();
   config_get_int("ghost_local_process_ms",&ghost_local_process_ms);
}

void GhostTerm(void)
{
   GhostMsgsTerm();
   GhostListTerm();
   GhostSendTerm();
}

void GhostDBReset(void)
{
   GhostListReset();
}
