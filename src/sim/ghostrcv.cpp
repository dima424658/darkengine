/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostrcv.cpp,v 1.17 1999/08/26 12:53:02 dc Exp $
// receive systems for ghosts

#include <ghost.h>
#include <ghostphy.h>
#include <ghostrcv.h>
#include <ghostlst.h>
#include <ghostmvr.h>
#include <ghostapi.h> // for EXTERN declare of GhostWeaponObjAttach callback
#include <ghostwep.h>

#include <simtime.h>
#include <rand.h>

#include <minmax.h> 

#include <objpos.h>
#include <physapi.h>
#include <phcore.h>
#include <phmods.h>   // phys models (all of them) methods (i.e. for the list)
#include <phmod.h>    // phys model methods (for an individual model)
#include <phcontct.h>

#include <mmanuver.h> // for mocap zaniness
#include <mcoord.h>
#include <mclntapi.h>
#include <motdesc.h>
#include <motset.h>
#include <motprop.h>
#include <mschutil.h>
#include <motmngr.h>
#include <creatapi.h>
#include <creatext.h>
#include <cretprop.h>
#include <weapcb.h>

//#include <objpos.h>

#include <plyrmode.h> // umm, player mode
#include <wrtype.h>   // for Position, for ObjPos stuff

#include <dbmem.h>

////////////////////////
// globals - from ghostapi.h

void (*GhostWeaponObjAttach)(ObjID obj, int weapon)=NULL;

////////////////////////
// receiver calls - on actually getting a packet...

#define SLEEP_DIST (0.05*0.05)

BOOL _GhostAllowedToSleep(sGhostRemote *pGR)
{
   cPhysModel *pModel = g_PhysModels.Get(pGR->obj);
   if (pModel == NULL)
      return TRUE;

   mxs_vector delta;
   mx_sub_vec(&delta, &pModel->GetLocationVec(), &pGR->info.pred.pos.pos);

   if (_GhostIsType(pGR->cfg.flags,IsObj) && PhysObjOnGround(pGR->obj))
      delta.z = 0;

   return (mx_mag2_vec(&delta) < SLEEP_DIST);
}

// returns whether to "disable" the ghost
BOOL _GhostPerFrameModeUpdate(sGhostRemote *pGR)
{
   if (pGR->info.last.pos.mode==kGhostModeSleep)
      if (_GhostAllowedToSleep(pGR))
      {
         if (_ghost_warnings())
            if (pGR->cfg.flags&kGhostCfDontRun)
               _ghost_mprintf(("Hey, sleep for %s, but i was asleep...\n",ObjWarnName(pGR->obj)));
         pGR->cfg.flags|=kGhostCfDontRun;
         _GhostPhysSleep(pGR);
         if (_ghost_watch_events())
            _ghost_mprintf(("Ghost %s to sleep\n",ObjWarnName(pGR->obj)));
         return TRUE;
      }
   return FALSE;
}

void _GhostPacketModeUpdate(sGhostRemote *pGR)
{
   if (pGR->info.last.pos.mode==kGhostModeSleep)
   {
      pGR->info.pred.pos.pos = pGR->info.last.pos.pos;
      mx_zero_vec(&pGR->info.pred.pos.vel);
      _GhostPerFrameModeUpdate(pGR);   // run a "frame" to try and go to sleep
      return;
   }
   if (pGR->info.last.pos.mode==kGhostModeRevive)
   {
      if (_ghost_warnings())
         if (pGR->critter.r_state&kGhostRcvDieNext)
            _ghost_mprintf(("Revive %s while waiting to die...\n",ObjWarnName(pGR->obj)));
      pGR->critter.r_state=0; // sure, kick its butt, eh?
      pGR->info.last.pos.mode=kGhostModeStand;
      if (pGR->cfg.flags&kGhostCfDontRun)
      {
         pGR->cfg.flags&=~kGhostCfDontRun;
         pGR->info.last.pos.flags|=kGhostHBTeleport;
         if (_ghost_watch_events())
            _ghost_mprintf(("Ghost %s revive\n",ObjWarnName(pGR->obj)));
      }
      else if (_ghost_warnings())
         _ghost_mprintf(("Hey, mode revive update for %s, but i wasnt dont run...\n",ObjWarnName(pGR->obj)));
      return;
   }
   if (pGR->info.last.pos.mode!=pGR->info.pred.pos.mode)
      if (pGR->info.pred.pos.mode==kPM_Jump)
         _GhostJumpOff(pGR->obj);  // we were jumping
      else if (pGR->info.last.pos.mode==kPM_Jump)
         _GhostJumpOn(pGR->obj);   // we are now jumping
}

BOOL CheckValidSequence(int arrived, int stored)
{
   if (arrived>stored)
      return TRUE;
   if (arrived+65536-stored<32768)   // handle the wrap case
      return TRUE;                   // we assume we will never get 32K packets behind
   return FALSE;                     // ??? who knows, i guess
}

// on packet recv, just go and update goals in the structures
void GhostRecvPacket(ObjID ghost, int seq_id, ObjID rel_obj, sGhostHeartbeat *pGH, sGhostMoCap *pGMC)
{
   sGhostRemote  *pGR=GhostGetRemote(ghost);
   if (pGR==NULL)
   {
      Warning(("GhostRecv called on %s which is not in the remote hash\n",ObjWarnName(ghost)));
      return;
   }
   _GhostDebugSetupRemote(pGR);
   
   pGR->cfg.flags&=~kGhostCfNoPacket;
   sGhostRecvInfo *pGRP=&pGR->info;

   // the ugly part (well, ok, an ugly part) here is that even if we get out
   //  of sequence packets... if the newly arrived (that is, the old) one is
   //  guaranteed, we had better deal with its mode/mocap data, since that is
   //  probably why it has been sent guaranteed

   BOOL valid_packet=TRUE, valid_pos;
   if (!CheckValidSequence(seq_id,pGRP->seq_id))
   {
      _ghost_mprintf(("%s out of sequence: arrive %d saved %d\n",ObjWarnName(pGR->obj),seq_id,pGRP->seq_id));
      if ((pGH->flags&kGhostHBUseG)==0)
         return;           // we can just blow this off, as it isnt a G packet
      if (!CheckValidSequence(seq_id,pGRP->last_g_seq))
      {
         _ghost_mprintf(("ERROR: %s Guaranteed packet out of sequence: arrive %d saved %d (ng save %d)\n",
                         ObjWarnName(pGR->obj),seq_id,pGRP->last_g_seq,pGRP->seq_id));
         return;           // we are somehow older than last g packet?
      }
      valid_packet=FALSE;  // otherwise, have to do partial update horror
   }
   else
      pGRP->seq_id=seq_id;
   if (pGH->flags&kGhostHBUseG)
      pGRP->last_g_seq=seq_id;  // update last G packet as well

   // these are independant of valid or not, if we are here
   // we are either a new packet, or most recent G, so these "count"
   if (pGH->flags&kGhostHBObjRel)
   {
      _GhostRemoteRelObjPhys(ghost, rel_obj, pGRP->last.rel_obj);
      pGRP->last.rel_obj = rel_obj;
   }
   else
      pGRP->last.rel_obj = pGR->info.pred.rel_obj;

   // if this packet pos thinks its on a relobj, and we arent, skip our pos
   valid_pos=(pGH->flags&kGhostHBOnObj)?
      pGRP->last.rel_obj!=OBJ_NULL:pGRP->last.rel_obj==OBJ_NULL;

   if (valid_packet)
   {
      if (!valid_pos)  // this pos is not valid compared to the rel_obj, so skip it
         pGH->pos=pGRP->last.pos.pos;
      pGRP->last.pos     = *pGH;
      pGRP->last.time    = GetSimTime();
      if (valid_pos && ((pGH->flags&(kGhostHBNoZPos|kGhostHBAngOnly))==0))
         _ghost_pos_centerconvert(ghost,&pGRP->last.pos.pos);
   }
   else // ok, this is a G packet from before now, keep its mode
      pGRP->last.pos.mode=pGH->mode;

   if ((pGH->flags&kGhostHBWeap)==0)
      pGRP->last.pos.weap= pGR->info.pred.pos.weap;
   else
   {
      if (_ghost_track_weapons())
         _ghost_mprintf(("on packet recv %s is attaching weapon %d\n",ObjWarnName(pGR->obj),pGH->weap));
      if (GhostWeaponObjAttach)
         (*GhostWeaponObjAttach)(pGR->obj,pGH->weap-1);
   }

   pGH->flags&=~(kGhostHBAngOnly|kGhostHBNoZPos);   // for now?

   // do we need to change mode to react to this packet
   _GhostPacketModeUpdate(pGR);

#ifdef GHOST_DEBUGGING
   // talk about rel_objs
   if (_ghost_track_relobj())
      if (pGH->flags&kGhostHBObjRel)
         _ghost_mprintf(("%s recv new relobj %s\n",ObjWarnName(pGR->obj),ObjWarnName(rel_obj)));
   
   // warn if getting packets but: not running or no phys and didnt just get that way
   if (((pGR->cfg.flags&kGhostCfDontRun)&&(pGR->info.last.pos.mode!=kGhostModeSleep))||
       (!PhysObjHasPhysics(pGR->obj)))
      if (_ghost_warnings())
         _ghost_mprintf(("Ghost %s receiving packet but %s\n",ObjWarnName(pGR->obj),
                         pGR->cfg.flags&kGhostCfDontRun?"DontRun":"NoPhys"));

   if (_ghost_show_recv_pred()) // show the old position on recv
      _GhostPrintGhostPos(pGR->obj,&pGRP->pred.pos,pGR->cfg.flags,"old",-1);

   // @TBD: compute average "wrongness" when receiving a packet
   if (_ghost_show_recv_delta())
   {
      mxs_vector vdiff, pdiff;
      short ang_diffs[3];
      mx_sub_vec(&pdiff,&pGRP->last.pos.pos,&pGRP->pred.pos.pos);
      mx_sub_vec(&vdiff,&pGRP->last.pos.vel,&pGRP->pred.pos.vel);
      for (int i=0; i<3; i++)
         ang_diffs[i]=pGRP->last.pos.angle_info.fac.el[i]-pGRP->pred.pos.angle_info.fac.el[i];
      ObjPos *pPos=ObjPosGet(pGR->obj);
      mxs_vector cvel,rpos=pPos->loc.vec;
      PhysGetVelocity(pGR->obj,&cvel);
      mx_subeq_vec(&cvel,&pGRP->last.pos.vel);
      mx_subeq_vec(&rpos,&pGRP->last.pos.pos);
      char buf[256];
      sprintf(buf," delv %.4f %.4f %.4f (rv %.4f %.4f %.4f) da %d %d %d\n delp %.4f %.4f %.4f (rp %.4f %.4f %.4f) for %s\n",
              vdiff.el[0],vdiff.el[1],vdiff.el[2],-cvel.el[0],-cvel.el[1],-cvel.el[2],ang_diffs[0],ang_diffs[1],ang_diffs[2],
              pdiff.el[0],pdiff.el[1],pdiff.el[2],-rpos.el[0],-rpos.el[1],-rpos.el[2],ObjWarnName(pGR->obj));
      mprintf(buf);
   }
#endif
   
   // for now, not sure of real model for pred update? (bet this isnt it, though)
   // well, in practice, it almost is, the only thing is we want to keep
   //   the old v if new packet has a control vel, dippy dippy dippy, basically
   mxs_vector old_vel=pGRP->pred.pos.vel;
   pGRP->pred      = pGRP->last;
   if ((pGRP->pred.pos.flags&kGhostHBAbsVel)==0)
      pGRP->pred.pos.vel=old_vel;
   // may i just say how embarassingly dumb this all is?

   // and the mocap hookup
   if (pGMC)
      if ((pGR->critter.cur_mocap.schema_idx!=pGMC->schema_idx)||
          (pGR->critter.cur_mocap.motion_num!=pGMC->motion_num))
      {  // really need the "irq" flag or something here
         pGR->critter.cur_mocap=*pGMC;
         if (_ghost_track_mocap_pkt())
            _ghost_mprintf(("%s got packet saying %d %d (%s)\n",
                            ObjWarnName(ghost),pGMC->schema_idx,pGMC->motion_num,
                            (pGMC->motion_num>0)?(char *)g_pMotionSet->GetName(pGMC->motion_num):"???"));
      }

   if (_ghost_show_recvs())
      _ghost_mprintf(("Recv message from %s at time %d\n",ObjWarnName(ghost),GetSimTime()));
   if (_ghost_show_full_recvs())
   {
      _GhostPrintGhostPos(pGR->obj,pGH,pGR->cfg.flags,"r",seq_id);
      if (pGMC || (pGH->flags&kGhostHBObjRel))
         _ghost_mprintf(("   rel %s mocap %d %d\n",
                         pGRP->last.rel_obj!=OBJ_NULL?ObjWarnName(pGRP->last.rel_obj):"None",
                         pGMC?pGMC->schema_idx:-1,pGMC?pGMC->motion_num:-1));
   }
}

//////////////////////////////
// remote per frame code

// updates the position data of pPos, which is the predicted position
//  using an ultra-simplified physics model
//  this new target will be aimed at by the "real" ghost model
void _UpdateGhostPrediction(sGhostRemote *pGR, float dt)
{
   BOOL gravity = _GhostGravRemote(pGR->obj, &pGR->info.last, pGR->cfg.flags);

   _GhostApproxPhys(pGR->obj,&pGR->info.last,&pGR->info.pred,dt,gravity);
   if (!_is_zero_vec(&pGR->info.pred.pos.vel))
      _GhostBleedVelocity(pGR->obj, &pGR->info.pred, pGR->cfg.flags, pGR->info.last.time, dt);
}  // currently only do this (bleed) for the recv, which is not right

// helper function to smooth the ghosts heading towards the target heading
void _SmoothHeadingNonPhysical(ObjID ghost, sGhostHeartbeat *pHeart, float dt)
{
   mxs_angvec ang;
   PhysGetModRotation(ghost, &ang);
   const mxs_ang goal_facing = pHeart->angle_info.tz;  // since tz's are now unified
   const short delta         = goal_facing-ang.tz;

   if (delta==0) return;  // since we assume we set this last?
   
   const short rot_sign  = (delta>0)?1:-1;  // which way...
   const int   max_rot = dt*0xC000, min_rot = dt*0x1000; // slower than this is criminal
   short       cmp_rot   = delta * dt * 8;  // base computed from delta - x by who knows...

#ifdef TRY_PACKET_ROT   
   if ((pHeart->flags&kGhostHBFullAngs)==0)
   {
      short packet_rot = FIXVEL_TO_DTANG(pHeart->angle_info.dtz,dt*1000.0);
      if ((int)packet_rot*(int)cmp_rot>0)  // same sign, so pay attention
      {
         if (abs(abs(packet_rot)-abs(cmp_rot))>0x400) // they are fairly different
            cmp_rot=(short)(((int)cmp_rot+(int)packet_rot)/2);  // unclear if this is good or bad...
      }
      // else _ghost_mprintf(("Hey, opposite signs %d %d in packet rot\n",packet_rot,cmp_rot));
   }
#endif
   
   if (cmp_rot)
      if (abs(cmp_rot)<min_rot)
         cmp_rot=rot_sign*min_rot; // but which direction?
      else if (abs(cmp_rot)>max_rot)
         cmp_rot=rot_sign*max_rot;

   if (abs(delta) < abs(cmp_rot))
      ang.tz  = goal_facing;
   else
      ang.tz += cmp_rot;
   if (_ghost_track_heading())
      _ghost_mprintf(("%s ang.tz now %x, max_rot %x goal %x\n",ObjWarnName(ghost),ang.tz,cmp_rot,goal_facing));
   PhysSetModRotation(ghost, &ang);
   PhysControlRotation(ghost, &ang);
}

float _generate_ghost_rating(sGhostRemote *pGR)
{
   return 0.0;
}

// recomputes phys forces to drive us towards the target
void _ReAimGhostPhysics(sGhostRemote *pGR, float dt)
{
   // aim at the ghost position
   ObjID ghost=pGR->obj;
   sGhostPos *pTarg=&pGR->info.pred;
   cPhysModel *pModel = g_PhysModels.Get(ghost);

   if (pModel==NULL)
   {  // since our physmodel is destroyed too soon sometimes 
      Warning(("Reaim %s but no longer has physmodels\n",ObjWarnName(ghost)));
      return;
   }

   // ok, now lets set the controls for the phys model
   const mxs_vector *p_curpos = &pModel->GetLocationVec();

   mxs_vector world_targ_pos = pTarg->pos.pos;
   _ghost_pos_reltoworld(ghost,pTarg->rel_obj,&world_targ_pos);

   if (_ghost_track_relobj())
      if (pTarg->rel_obj!=OBJ_NULL)
         _ghost_mprintf(("%s rel_obj %s\n",ObjWarnName(pGR->obj),ObjWarnName(pTarg->rel_obj)));

   mxs_vector dpos, dvel;
   mx_sub_vec(&dpos,&world_targ_pos,p_curpos);
   if (_ghost_frame_delta())
      _ghost_mprintf(("%s dp %g %g %g target %g %g %g\n",ObjWarnName(ghost),dpos.x,dpos.y,dpos.z,pTarg->pos.pos.x,pTarg->pos.pos.y,pTarg->pos.pos.z));

   // now check our rating, see how we are doing - need to give up and teleport
   _generate_ghost_rating(pGR);

   // @TBD ok, really someday we will have to do something real
#define TELEPORT_THRESH   12.0*12.0
   if ((pGR->cfg.flags&kGhostCfFirstTPort)||
       (pTarg->pos.flags&kGhostHBTeleport)||
       (mx_mag2_vec(&dpos)>TELEPORT_THRESH))
   {
      if (_GhostTeleport(ghost,&dpos,&world_targ_pos))
      {  // @TBD: this is ugly, really should restuff curpos...
         const mxs_vector *p_newpos = &pModel->GetLocationVec();
         mx_sub_vec(&dpos,&world_targ_pos,p_newpos);
      }
      pTarg->pos.flags&=~kGhostHBTeleport;
      pGR->cfg.flags&=~kGhostCfFirstTPort;
   }

   BOOL fast_packet_vel;

   // if an AI, i dont want to add mag, for now...
   if (_GhostIsType(pGR->cfg.flags,AI))
      fast_packet_vel=FALSE;
   else                        // some slowdown thing here, maybe?
      fast_packet_vel=(mx_mag2_vec(&pTarg->pos.vel)>1.0);

   if (fast_packet_vel)
      if (mx_mag2_vec(&dvel)>1.0)
         mx_add_vec(&dvel,&dpos,&pTarg->pos.vel);
      else
         dvel=pTarg->pos.vel;
   else
      mx_scale_vec(&dvel,&dpos,2.0);      

   // @TBD: rewrite to do "slowing down" w/slam
   // we should care about control v. abs vel
   BOOL slam_vel=_GhostIsType(pGR->cfg.flags,IsObj);
   slam_vel = slam_vel || (!fast_packet_vel || (pTarg->pos.mode==kPM_Jump));

   if (slam_vel)
      PhysSetVelocity(ghost,&dvel);
   PhysControlVelocity(ghost,&dvel);

   if (!_GhostIsType(pGR->cfg.flags,IsObj))
      _SmoothHeadingNonPhysical(ghost,&pTarg->pos,dt);
}

////////////////
// mocap stages
//

void _AimGhostHead(sGhostRemote *pGR)
{
   // doing snazzy head-focus thing
   if (((pGR->info.last.pos.flags & kGhostHBFullAngs) == 0) &&
       ((pGR->cfg.flags & kGhostCfNoCret) == 0))
   {
      if (pGR->info.last.pos.angle_info.p == 0)
      {
         CreatureSetFocusObj(pGR->obj, OBJ_NULL);
         return;
      }

      mxs_vector unit_vec;
      mxs_vector obj_focus_vec;
      mxs_vector world_focus_vec;

      mx_mk_vec(&unit_vec, 100, 0, 0);

      short pitch = ((short)pGR->info.last.pos.angle_info.p) / 2;

      mx_rot_y_vec(&obj_focus_vec, &unit_vec, (mxs_ang)pitch);

      mxs_matrix rot;
      mx_ang2mat(&rot, &ObjPosGet(pGR->obj)->fac);

      mx_mat_mul_vec(&world_focus_vec, &rot, &obj_focus_vec);
      mx_addeq_vec(&world_focus_vec, &ObjPosGet(pGR->obj)->loc.vec);
      
      CreatureSetFocusLoc(pGR->obj, &world_focus_vec);
   }
}

////////////////
// interface with mvr

static int gMotionNumber = -1;

int GetGhostMotionNumber(void)
{
   int ret_val=gMotionNumber;
   gMotionNumber=-1;
   return ret_val;
}

////////////////
// internal calls/structure

// use a tagset to build a high level goal/plan
IMotionPlan *_BuildTagBasedPlan(sGhostRemote *pGR, char *motion)
{
   sMcMoveParams params;
   cTagSet SpecMotion(motion);
   params.tags = SpecMotion;
   params.mask = 0;
   if (_ghost_track_tag_mocap()||_ghost_track_mocap())
      _ghost_mprintf(("%s playing %s tag based\n",ObjWarnName(pGR->obj),motion));
   IMotionPlan *pPlan =
      pGR->critter.pMotCoord->BuildPlan(pGR->critter.pMotCoord->GetNextEndState(),&params);
   return pPlan;
}

// using the secret number setting method, build a specific motion
IMotionPlan *_BuildSchemaOffsetPlan(sGhostRemote *pGR, int schema_idx, int mot_num)
{
   gMotionNumber = mot_num;

   cMotionSchema *pMotSchema;
   if (!g_pMotionDatabase->GetSchema(schema_idx, &pMotSchema))
   {
      Warning(("Ghost: No schema at index %d\n", schema_idx));
      return NULL;
   }

   int factory_id;
   if (!MSchUGetMotControllerID(pMotSchema, &factory_id))
   {
      Warning(("Ghost: No motor controller for schema %d\n", schema_idx));
      return NULL;
   }

   sMcMoveParams params;
   params.mask = 0;

   if (_ghost_track_idx_mocap()||_ghost_track_mocap())
      _ghost_mprintf(("%s playing %d %d (%s) idx based\n",
                      ObjWarnName(pGR->obj),schema_idx,mot_num,
                      mot_num>0?(char *)g_pMotionSet->GetName(mot_num):"???"));

   IMotionCoordinator *pMotCoord = pGR->critter.pMotCoord;

   // wow, this is way ugly, aint it
   cMotionPlan *pMotionPlan = 
      g_ManeuverFactoryList[factory_id]->CreatePlan(pMotSchema, 
                                                    *((cMotionCoordinator *)pMotCoord)->GetInterMnvrState(),
                                                    *pMotCoord->GetNextEndState(),
                                                    params, 
                                                    CreatureGetMotorInterface(pGR->obj),
                                                    (cMotionCoordinator *)pMotCoord);
   
   if (pMotionPlan)
   {
      IManeuver *pMnvr=pMotionPlan->GetFirstManeuver();
      pMnvr->SetAppData(factory_id);    // store this for load/save purposes
   }
   return pMotionPlan;
}

// go run the next correct mocap for this ghost...
// using either the motion tags or the internal numbers sent over the wire
void _MocapRunMe(sGhostRemote *pGR, char *motion)
{
   IMotionPlan *pPlan=NULL;
   if (motion[0]!='\0')
      pPlan=_BuildTagBasedPlan(pGR,motion);
   else
      pPlan=_BuildSchemaOffsetPlan(pGR,pGR->critter.cur_mocap.schema_idx,
                                       pGR->critter.cur_mocap.motion_num);
   
   if (pPlan)
   {
      IManeuver *pManeuver = pPlan->PopFirstManeuver();
      pGR->critter.pMotCoord->SetCurrentManeuver(pManeuver);
      delete pPlan;
   }
}

void _AnalyzeVelocity(sGhostRemote *pGR, eGhostMotionSpeed *spd, int *dir)
{
   mxs_vector ovel;

   // not quite right, want some sort of hybrid or something?
   PhysGetVelocity(pGR->obj,&ovel);
   // ovel=pGR->info.last.pos.vel;  // lets see how this looks?
   
   if (pGR->info.last.pos.mode!=kGhostModeClimb)
      ovel.z=0;
   float raw_spd_2=mx_mag2_vec(&ovel);

   // Find delta of raw speed and the speed of the thing we're standing on
   sGhostRecvInfo *pGRP = &pGR->info;
   if ((pGRP->last.rel_obj != OBJ_NULL) && PhysObjHasPhysics(pGRP->last.rel_obj))
   {
      mxs_vector tvel;
      PhysGetVelocity(pGRP->last.rel_obj, &tvel);
      raw_spd_2 -= mx_mag2_vec(&tvel);
   }

   *dir=3; // forward is the default
   if (raw_spd_2>0.5)  // see if we are moving, really
   {  // if moving, compare facing to velocity vector, choose quadrant we are pointing at
      mxs_angvec angvec;
      PhysGetModRotation(pGR->obj, &angvec);
      float vel_ang=atan2(ovel.y,ovel.x);
      if (vel_ang<0) vel_ang+=MX_REAL_2PI;
      fixang vel_fixang=(vel_ang*MX_ANG_PI)/MX_REAL_PI;

      static int result_dir[]={3,3,3,2,2,4,4,4,4,4,4,1,1,3,3,3};
      // check for sides and back, left=1, right=2, back=4 (front is 3, go figure)
      fixang ang_diff=angvec.tz-vel_fixang;
      int cone_index=((int)ang_diff)>>12;   // should be 0-15 now
      *dir=result_dir[cone_index];
   }

   if ((*dir)!=3)
      raw_spd_2*=2;  // was 4 - since backstep and sidestep are slower
   if (raw_spd_2>1.2*1.2)
      if (raw_spd_2>12.0*12.0)
         *spd=kGhostSpeedFast;
      else
         *spd=kGhostSpeedNorm;
   else
      *spd=kGhostSpeedNone;
}

char *_ChooseCapture(sGhostRemote *pGR, BOOL cur_idle)
{
   uchar             next_mode=kGhostModeNone;
   eGhostMotionSpeed cur_speed=kGhostSpeedNone;
   char             *c_str=NULL;
   int               ghost_dir, init_state=pGR->critter.r_state;
   pGR->critter.r_state&=~(kGhostRcvStand|kGhostRcvLoco);  // always clear them

   _AnalyzeVelocity(pGR,&cur_speed,&ghost_dir);

   if (_GhostIsType(pGR->cfg.flags,Player))
   {
      switch (pGR->info.last.pos.mode)
      {
         case kGhostModeCrouch: // hmmm
            next_mode=kGhostModeCrouch;
            break;
         case kGhostModeJumpBase:
         {
            BOOL not_jump=((pGR->critter.mmode!=kGhostModeJumpStart)&&
                           (pGR->critter.mmode!=kGhostModeJumping));
            if (not_jump)
               next_mode=kGhostModeJumpStart;
            else if (cur_idle)
               next_mode=kGhostModeJumping;
            else
               return NULL;
            break;
         }
         case kGhostModeDead:
            //            return NULL;        // is this the right thing? for "do nothing"
            break;
         case kGhostModeCarry:
         case kGhostModeSlide:
            Warning(("Unsupported mode %d in player ghost rcv\n",pGR->info.last.pos.mode));
         case kGhostModeSwim:
            break;
         default:  // just leave the mode to get through
            next_mode=pGR->info.last.pos.mode;
            break;
      }
   }
   else
   {
      next_mode=pGR->info.last.pos.mode;
   }

   switch (next_mode)
   {
      case kGhostModeNone:
      case kGhostModeStand:
         if (cur_speed==kGhostSpeedNone)
         {
            if ((init_state&kGhostRcvStand)&&(!cur_idle))
               return NULL;
            BOOL idle=FALSE;
            if (pGR->critter.stand_cnt==0)
               pGR->critter.stand_cnt=16+(Rand()&0xf);
            if ((--pGR->critter.stand_cnt)==1)
            {
               c_str="idlegesture";
               pGR->critter.stand_cnt=0;
            }
            else
               c_str="stand";
            pGR->critter.r_state|=kGhostRcvStand;
         }
         else
         {
            if ((init_state&kGhostRcvLoco)&&(!cur_idle))
               return NULL;
            if (ghost_dir!=3)
            {  
               if (ghost_dir==1)
                  c_str="locomote, direction 1";
               else if (ghost_dir==2)
                  c_str="locomote, direction 2";
               else if (ghost_dir==4)
                  c_str="locomote, direction 4";
               else
                  c_str="locomote";
            }
            else if (cur_speed==kGhostSpeedFast)
               c_str="locomote, locourgent";
            else
               c_str="locomote";
            pGR->critter.r_state|=kGhostRcvLoco;
         }
         break;
      case kGhostModeLeanLeft:
         c_str="stand,leaningleft";
         break;
      case kGhostModeLeanRight:
         c_str="stand,leaningright";
         break;
      case kGhostModeJumpStart:
#if 1
         if (cur_speed==kGhostSpeedNone)
            c_str="jumping";
         else
            c_str="jumping, locourgent, direction 3";
#else
         c_str="stand";         
#endif         
         break;
      case kGhostModeJumping:
         c_str="stand"; // for now, at least
         break;
      case kGhostModeSwim:         
         c_str="locomote, climbing";
         break;
      case kGhostModeClimb:
         if (cur_speed==kGhostSpeedNone)
            c_str="stand";
         else
            c_str="locomote, climbing";
         break;
      case kGhostModeCrouch:
         if (cur_speed==kGhostSpeedNone)
            c_str="stand, crouching";
         else
            c_str="locomote, crouching";
         break;
      default:
         if (_ghost_warnings())
            _ghost_mprintf(("Next mode %d for %s (in %d idle %d)\n",next_mode,ObjWarnName(pGR->obj),pGR->info.last.pos.mode,cur_idle));
         break;
   }

   pGR->critter.mmode=next_mode;
   return c_str;
}

BOOL _NewMocapReady(sGhostRemote *pGR)
{
   return (pGR->critter.cur_mocap.schema_idx>0);
}

// spin the standing capture in place...
void _MocapEval(sGhostRemote *pGR, float dt)
{
   if (_ghost_no_mocap())
      return;
   if (_NewMocapReady(pGR))
   {
      _MocapRunMe(pGR,"");
      pGR->critter.cur_mocap.schema_idx=kGhostMotSchemaNoCustom;
      pGR->critter.r_state|= kGhostRcvCustom;
      pGR->critter.r_state&=~kGhostRcvDidWeap;
      if (pGR->info.last.pos.flags&kGhostHBDead)
      {
         pGR->critter.r_state|= kGhostRcvDieNext;      
         if (_ghost_watch_events())
            _ghost_mprintf(("Ghost %s set dead at moment\n",ObjWarnName(pGR->obj)));
      }
   }
   else  // either playing, or need to generate a non-custom
   {
      BOOL gloco_interrupt = ((pGR->critter.r_state&kGhostRcvCustom) && 
                              (pGR->critter.cur_mocap.schema_idx==kGhostMotSchemaGLoco));

      BOOL cur_idle=gloco_interrupt||(pGR->critter.pMotCoord->GetStatus()==kMCoord_Idle);

      if (gloco_interrupt)
      {
         if (_ghost_watch_events())
            _ghost_mprintf(("Ghost %s forced idle from gloco\n",ObjWarnName(pGR->obj)));
         // clear, so we don't keep interrupting
         pGR->critter.cur_mocap.schema_idx=kGhostMotSchemaNoCustom;
      }

      if (pGR->critter.r_state&kGhostRcvCustom)
      {
         // amazingly horrifying hack to test horribly hypothesis about weaponevents
         // check to see if we should call StartWeaponSwing here?  or just on packet edge trigger?
         if (!_GhostIsType(pGR->cfg.flags,Player))
            if (pGR->info.last.pos.flags&kGhostHBStartSw)  // lets start the swing?
               if ((pGR->critter.r_state&kGhostRcvDidWeap)==0)
               {
                  if (_ghost_track_weapons())
                     _ghost_mprintf(("delayed set up weapon event for %s\n",ObjWarnName(pGR->obj)));
                  WeaponEvent(kStartAttack, pGR->obj, OBJ_NULL);
                  pGR->critter.r_state|=kGhostRcvDidWeap;
               }  // i think, though who knows, maybe we should do this, eh?
         if (cur_idle)
         {
            pGR->critter.r_state&=~kGhostRcvCustom;
            if (pGR->critter.r_state&kGhostRcvDieNext)
               if (pGR->critter.r_state&kGhostRcvHasDied)
                  _ghost_mprintf(("Hey: Ghost %s already dead\n",ObjWarnName(pGR->obj)));
               else  // deregister our phys models locally - and return - only once, though
               {
                  pGR->critter.r_state&=~kGhostRcvDieNext;
                  pGR->critter.r_state|= kGhostRcvHasDied;
                  if (_ghost_watch_events())
                     _ghost_mprintf(("Ghost %s dead/dereg\n",ObjWarnName(pGR->obj)));

                  if (_GhostIsType(pGR->cfg.flags,Player))                  
                     pGR->cfg.flags|=kGhostCfDontRun;  // we should stop doing for a bit, eh?
                  else
                     PhysDeregisterModel(pGR->obj);    // woo woo
                  return;
               }
         }
         else           // never interrupt a custom capture with a generated
            return;     // at least, not for now
      }
      if (ObjIsPosed(pGR->obj))  // ???? - fix Dr. Watts, basically
         return;
      char *_capture_str=_ChooseCapture(pGR,cur_idle);
      if (_capture_str==NULL)
         return;        // early exit
      if (!cur_idle)
         if (strncmp(_capture_str,pGR->critter.mocap_name,strlen(_capture_str))==0)
            return;
      if (_ghost_track_tag_mocap()||_ghost_track_mocap())
         _ghost_mprintf(("Hope to run <%s>, idle %d, on %s (s %d)\n",_capture_str,cur_idle,ObjWarnName(pGR->obj),pGR->critter.stand_cnt));
      _MocapRunMe(pGR,_capture_str);
      strncpy(pGR->critter.mocap_name,_capture_str,MAX_MOCAP_STR_LEN);
   }
}

// actually do a remote frame
void _GhostFrameProcessRemote(sGhostRemote *pGR, float dt)
{
   _GhostDebugSetupRemote(pGR);
   if (!PhysObjHasPhysics(pGR->obj))
      return;  // dont run anything if we are non-physical
   if (pGR->cfg.flags&(kGhostCfDontRun|kGhostCfDisable))
      return;  // and, umm, dont run if we are not supposed to run... duh
   
   if (pGR->cfg.flags&kGhostCfNew)
   {
      if (!_GhostAllowedToBuildModels(pGR))  // It's not ready yet
         return;
      _GhostBuildMotionCoord(pGR);
      _GhostConfigureRemoteModels(pGR);
      pGR->cfg.flags&=~kGhostCfNew;
   }
   
   // for now, we make sure we have some real data and are supposed to do phys
   if ((pGR->cfg.flags&kGhostCfNoPacket)==0)
   {
      if (_ghost_show_recv_prefr())
         _GhostPrintGhostPos(pGR->obj,&pGR->info.pred.pos,pGR->cfg.flags,"rpre",-1);
      
      // actually update local physics setup and so forth
      _UpdateGhostPrediction(pGR,dt);
      _ReAimGhostPhysics(pGR,dt);
      _AimGhostHead(pGR);
      _GhostPerFrameModeUpdate(pGR);

#ifdef GHOST_DEBUGGING      
      if (_ghost_show_recv_frame())
         _GhostPrintGhostPos(pGR->obj,&pGR->info.pred.pos,pGR->cfg.flags,"rprd",-1);

      if (_ghost_show_recv_real())
      {
         ObjPos *pPos=ObjPosGet(pGR->obj);
         mxs_vector cvel;
         PhysGetVelocity(pGR->obj,&cvel);
         char buf[255];
         sprintf(buf," realv: %.4f %.4f %.4f at %d\n realp: %.4f %.4f %.4f fac %x %x %x Obj %d\n",
                 cvel.x,cvel.y,cvel.z,GetSimTime(),
                 pPos->loc.vec.x,pPos->loc.vec.y,pPos->loc.vec.z,
                 pPos->fac.tx,pPos->fac.ty,pPos->fac.tz,pGR->obj);
         mprint(buf);
      }
#endif
   }

   // now we need mocap joy
   // run this always, so we dont get crucifixes at start...
   if ((pGR->cfg.flags & kGhostCfNoCret) == 0)
      _MocapEval(pGR,dt);
}
