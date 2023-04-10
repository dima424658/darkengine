/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostcfg.h,v 1.10 1999/08/26 12:52:30 dc Exp $
// ghost system configuration variables/debugging help, etc...

#ifndef __GHOSTCFG_H
#pragma once
#define __GHOSTCFG_H

#include <ghostflg.h>

#ifdef PLAYTEST
#define GHOST_DEBUGGING
#endif

///////////////////////////
// globals and setup for the spew type stuff
#ifdef GHOST_DEBUGGING
EXTERN int cur_type, targ_type, cur_obj, targ_obj;
#define __ghost_dbg_rel() \
 (((targ_obj==0)||(targ_obj==cur_obj))&& \
  ((targ_type==0)||(((cur_type)&kGhostCfTypeMask)==targ_type)))
#else
#define __ghost_dbg_rel() FALSE
#endif

///////////////////////////
// control logic for the spew stuff
#ifdef GHOST_DEBUGGING
#define _ghostspew_master()      (config_is_defined("ghostspew_all"))
#define _ghostspew_config(x)     (config_is_defined(x)||_ghostspew_master())
#define _ghostspew_single(x)     (__ghost_dbg_rel()&&_ghostspew_config(x))
#define _ghost_no_sends()        (config_is_defined("ghost_never_send"))
#define _ghost_no_recvs()        (config_is_defined("ghost_never_recv"))
#define _ghost_no_mocap()        (config_is_defined("disable_ghost_mocap"))
#define _ghost_mprintf(x)        mprintf x
#else  // GHOST_DEBUGGING
#define _ghostspew_single(x)     FALSE
#define _ghost_no_sends()        FALSE
#define _ghost_no_recvs()        FALSE
#define _ghost_no_mocap()        FALSE
#define _ghost_mprintf(x)          
#endif // GHOST_DEBUGGING

///////////////////////////
// actual spews/warns/flags/etc

// these do the right things playtest or not, since they hit the defines above...
#define _ghost_warnings()        (_ghostspew_single("ghostspew_warns"))
#define _ghost_explain_sends()   (_ghostspew_single("ghostspew_send_reasons"))
#define _ghost_math_sends()      (_ghostspew_single("ghostspew_send_math"))
#define _ghost_show_sends()      (_ghostspew_single("ghostspew_sends"))
#define _ghost_show_recvs()      (_ghostspew_single("ghostspew_recvs"))
#define _ghost_show_full_sends() (_ghostspew_single("ghostspew_full_sends"))
#define _ghost_show_full_recvs() (_ghostspew_single("ghostspew_full_recvs"))
#define _ghost_show_pNew()       (_ghostspew_single("ghostspew_pnew_always"))
#define _ghost_show_send_pred()  (_ghostspew_single("ghostspew_pred_always"))
#define _ghost_show_send_last()  (_ghostspew_single("ghostspew_last_always"))
#define _ghost_show_recv_delta() (_ghostspew_single("ghostspew_recv_delta"))
#define _ghost_show_recv_pred()  (_ghostspew_single("ghostspew_recv_pred"))
#define _ghost_show_recv_frame() (_ghostspew_single("ghostspew_recv_always"))
#define _ghost_show_recv_prefr() (_ghostspew_single("ghostspew_recv_preframe"))
#define _ghost_show_recv_real()  (_ghostspew_single("ghostspew_recv_realframe"))    
#define _ghost_list_mods()       (_ghostspew_single("ghostspew_lists"))
#define _ghost_list_load()       (_ghostspew_single("ghostspew_list_load"))
#define _ghost_list_duplicates() (_ghostspew_single("ghostspew_list_dups"))
#define _ghost_frame_delta()     (_ghostspew_single("ghostspew_deltas"))
#define _ghost_track_relobj()    (_ghostspew_single("ghostspew_relobj"))
#define _ghost_track_mocap_pkt() (_ghostspew_single("ghostspew_mocap_packets"))
#define _ghost_track_mocap()     (_ghostspew_single("ghostspew_mocaps"))
#define _ghost_track_idx_mocap() (_ghostspew_single("ghostspew_idx_mocaps"))
#define _ghost_track_tag_mocap() (_ghostspew_single("ghostspew_tag_mocaps"))
#define _ghost_track_heading()   (_ghostspew_single("ghostspew_heading"))
#define _ghost_track_weapons()   (_ghostspew_single("ghostspew_weapons"))
#define _ghost_track_player()    (_ghostspew_single("ghostspew_player"))
#define _ghost_watch_rating()    (_ghostspew_single("ghostspew_rating"))
#define _ghost_watch_death()     (_ghostspew_single("ghostspew_death"))
#define _ghost_watch_events()    (_ghostspew_single("ghostspew_events"))
#define _ghost_watch_physcast()  (_ghostspew_single("ghostspew_physcast"))
#define _ghost_watch_validloc()  (_ghostspew_single("ghostspew_validloc"))
#define _ghost_loud_teleport()   (_ghostspew_single("ghostspew_teleport"))
#define _ghost_approx()          (_ghostspew_single("ghostspew_approx"))

#endif  // __GHOSTCFG_H
