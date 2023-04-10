/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostsnd.cpp,v 1.12 1999/08/26 12:52:58 dc Exp $
// send packets/local control systems for ghosts

#include <appagg.h>

#include <ghost.h>
#include <ghostapi.h>
#include <ghostmsg.h>
#include <ghostphy.h>
#include <ghostlst.h>
#include <ghostsnd.h>
#include <ghostrcv.h>
#include <ghostmvr.h>
#include <ghosthst.h>

#include <simtime.h>
#include <config.h>

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>   // phys models (all of them) methods (i.e. for the list)
#include <phmod.h>    // phys model methods (for an individual model)
#include <phmoapi.h>  // leaning

#include <objpos.h>
#include <plyrmode.h> // umm, player mode
#include <wrtype.h>   // for Position, for ObjPos stuff
#include <aiman.h>    // argh
#include <motmngr.h>
#include <motset.h>

#include <weapcb.h>   // so we can track weapon state, to include it in packets

#include <dbmem.h>

//////////////////////////////
// globals

// really, if we punt impulse, we can just give up on this...
static IAINetServices *g_pAINetServ=NULL;

// from ghostapi.h
int (*GhostPlayerActionCallback)(ObjID ghost, int mode, int state)=NULL;

//////////////////////////////
// misc dumb helpers, why isnt this in a library

static BOOL AngvecMatch(mxs_angvec *vec1, mxs_angvec *vec2, int eps)
{
#define eps_test(ang,eps) (abs((short)(vec1->ang-vec2->ang))<eps)
   if (eps==0)
      return ((vec1->tx == vec2->tx) &&
              (vec1->ty == vec2->ty) &&
              (vec1->tz == vec2->tz));
   else
      return (eps_test(tx,eps) && eps_test(ty,eps) && eps_test(tz,eps));
}

////////////////////////
// core packet generation

// actually store into the local ghost state for send during next packet....
// for now called directly from motion code (mvr*), prototyped in ghostmvr
void GhostSendMoCap(ObjID ghost, int schema, int motion, BOOL is_gloco)
{
   sGhostLocal *pGL=GhostGetLocal(ghost);
   _GhostDebugSetupLocal(pGL);
   if (is_gloco)
   {
      // notify about gloco (for interruption purposes) by setting the 
      // schema index to kGhostMotSchemaGLoco
      pGL->playing.schema_idx=kGhostMotSchemaGLoco;
      if (_ghost_track_idx_mocap()||_ghost_track_mocap())
         _ghost_mprintf(("%s setting send gloco\n", ObjWarnName(ghost)));
   }
   else
   {
      pGL->playing.schema_idx=schema;
      pGL->playing.motion_num=motion;
      if (_ghost_track_idx_mocap()||_ghost_track_mocap())
         _ghost_mprintf(("%s setting send %d %d (%s) idx based\n",
                         ObjWarnName(ghost),schema,motion,
                         motion>0?(char *)g_pMotionSet->GetName(motion):"???"));
   }
}

//#define MINIPACKET_STATS
#ifdef MINIPACKET_STATS
static int failed_dpZ, failed_dvZ, failed_mag_vZ, failed_dFac, failed_mag_dtz, failed_dp, failed_player_mag_dtz, failed_flags, failed_dvel, failed_dpos;
static int allow_mini_hb, allow_rot_hb, allow_ai_hb, nogo_ai_hb, allow_obj_hb, nogo_obj_hb, total_hb, total_ng_hb;
#endif

static void _GhostAnalyzeAndSendMiniPacket(sGhostLocal *pGL, sGhostPos *pNew)
{
#ifdef MINIPACKET_STATS
   BOOL no_dp=abs((((int)pNew->pos.angle_info.p)+100000)-(((int)pGL->info.last.pos.angle_info.p)+100000))<0x80;
   BOOL no_dpz=fabs(pNew->pos.pos.z-pGL->info.last.pos.pos.z)<0.2;
   BOOL no_dvz=fabs(pNew->pos.vel.z-pGL->info.last.pos.vel.z)<0.2;
   BOOL no_dvel=mx_is_identical(&pNew->pos.vel,&pGL->info.last.pos.vel,0.02);
   BOOL no_dpos=mx_is_identical(&pNew->pos.pos,&pGL->info.last.pos.pos,0.02);   
   BOOL no_magvz=fabs(pNew->pos.vel.z)<=0.2;
   BOOL no_magdtz=(abs(pNew->pos.angle_info.dtz)<0x080);
   BOOL no_dfac=AngvecMatch(&pNew->pos.angle_info.fac,&pGL->info.last.pos.angle_info.fac,0x80);
   int der_diff_flags=pNew->pos.flags^pGL->info.last.pos.flags;
   der_diff_flags&=~(kGhostHBHaveCap|kGhostHBUseG);  // what others dont we care about
   BOOL der_same_flags=der_diff_flags==0;
   BOOL allowed=FALSE;
   // do some minipacket analysis, eh?
   if (no_dvel && no_dpos && der_same_flags)
    { allow_rot_hb++; allowed=TRUE; }
   else if (_GhostIsType(pGL->cfg.flags,AI) && no_dp && no_dpz && no_dvz && der_same_flags )
    { allow_ai_hb++; allowed=TRUE; }
   else if (no_dp && no_magvz && no_magdtz)
    { allow_mini_hb++; allowed=TRUE; }
   else if (_GhostIsType(pGL->cfg.flags,IsObj) && no_dfac && der_same_flags)
    { allow_obj_hb++; allowed=TRUE; }
   if (!allowed)
   {
      if (_GhostIsType(pGL->cfg.flags,AI))
         nogo_ai_hb++;
      if (_GhostIsType(pGL->cfg.flags,IsObj))
         nogo_obj_hb++;
      if (!no_dp)     failed_dp++;
      if (!no_magvz)  failed_mag_vZ++;
      if (!no_dvel)   failed_dvel++;
      if (!no_dpos)   failed_dpos++;
      if (!no_magdtz)
      {
         failed_mag_dtz++;
         if (_GhostIsType(pGL->cfg.flags,Player))
            failed_player_mag_dtz++;
      }
      if (_GhostIsType(pGL->cfg.flags,AI))  // specials for AI/Obj only sitchiations...
      {
         if (!no_dpz)         failed_dpZ++;
         if (!no_dvz)         failed_dvZ++;
         if (!der_same_flags) failed_flags++;
      }
      if (_GhostIsType(pGL->cfg.flags,IsObj))
      {
         if (!no_dfac)        failed_dFac++;
         if (!der_same_flags) failed_flags++;
      }
   }
   total_ng_hb++;
#endif
   // actual minipacket send logic...
#define ALLOW_MINIPACKETS
#ifdef ALLOW_MINIPACKETS
   short delta_p = (short)((ushort)pNew->pos.angle_info.p - (ushort)pGL->info.last.pos.angle_info.p);
   int diff_flags=pNew->pos.flags^pGL->info.last.pos.flags;
   diff_flags&=~(kGhostHBHaveCap|kGhostHBUseG);  // what others dont we care about
   BOOL same_flags=diff_flags==0;

   // would this be faster as subtract and zero vec test? ???
   if (same_flags&&
       mx_is_identical(&pNew->pos.vel,&pGL->info.last.pos.vel,0.02)&&
       mx_is_identical(&pNew->pos.pos,&pGL->info.last.pos.pos,0.02))
   {  // wow, we can use a rot packet, i guess?
      sGhostRotMiniHB RMH;
      RMH.angle_info.fac=pNew->pos.angle_info.fac;
      g_pGhostRotHeartbeatNGMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,&RMH);
      return;
   }
   if (abs(delta_p)<0x200)
   {
      if (_GhostIsType(pGL->cfg.flags,AI))
      {
         BOOL nodpz=fabs(pNew->pos.pos.z-pGL->info.last.pos.pos.z)<0.2;
         BOOL nodvz=fabs(pNew->pos.vel.z-pGL->info.last.pos.vel.z)<0.2;
         if (nodpz && nodvz && same_flags)
         {
            sGhostAIMiniHB AHB;
            AHB.posx=pNew->pos.pos.x; AHB.posy=pNew->pos.pos.y;
            AHB.velx=pNew->pos.vel.x; AHB.vely=pNew->pos.vel.y;
            AHB.tz=pNew->pos.angle_info.tz; AHB.dtz=pNew->pos.angle_info.dtz;
            g_pGhostAIHeartbeatNGMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,&AHB);
            return;
         }
      }
      BOOL nomagvz=fabs(pNew->pos.vel.z)<=0.2;
      BOOL nomagdtz=(abs(pNew->pos.angle_info.dtz)<0x080);
      // somehow, have to make sure this isnt a go to dtz 0 frame too!
      if (nomagvz && nomagdtz)
      {
         sGhostMiniHeartbeat MHB;
         MHB.pos=pNew->pos.pos; MHB.flags=pNew->pos.flags;
         MHB.velx=pNew->pos.vel.x; MHB.vely=pNew->pos.vel.y;
         MHB.tz=pNew->pos.angle_info.tz;
         g_pGhostMiniHeartbeatNGMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,&MHB);
         return;
      }
   }
   if (_GhostIsType(pGL->cfg.flags,IsObj))   
   {
      if (same_flags && AngvecMatch(&pNew->pos.angle_info.fac,&pGL->info.last.pos.angle_info.fac,0x80))
      {
         sGhostObjMiniHB OHB;
         OHB.pos=pNew->pos.pos;
         OHB.vel=pNew->pos.vel;
         g_pGhostObjHeartbeatNGMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,&OHB);
         return;
      }
   }
#endif  // ALLOW_MINIPACKETS
   g_pGhostHeartbeatTerseNGMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,&pNew->pos);
}   

// actually send a ghost update packet
void _GhostSendHeartbeat(sGhostLocal *pGL, sGhostPos *pNew)
{
   pGL->info.seq_id++; // if it wraps, so what, clients problem
   if (pGL->playing.schema_idx>0) // this isnt quite right, but hey... ignore gLoco too...
      pNew->pos.flags|=kGhostHBHaveCap;  // for now, this is safest place to do this
   if (mx_mag2_vec(&pGL->info.last.pos.vel)>mx_mag2_vec(&pNew->pos.vel)+5.0) // some slack
      pNew->pos.flags|=kGhostHBSlowDown; // im going to slow down, watch me... see

   if (!_ghost_no_sends()) // go ahead and send it
      if (pNew->pos.flags&(kGhostHBObjRel|kGhostHBHaveCap))
         g_pGhostHeartbeatFullMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,pNew->rel_obj,&pNew->pos,&pGL->playing);
      else if (pNew->pos.flags&kGhostHBUseG)  // send pNew->rel_obj always, or only if bit is set???
         g_pGhostHeartbeatNormMsg->Send(OBJ_NULL,pGL->obj,pGL->info.seq_id,&pNew->pos);
      else    // @TBD: minipackets here as well?
         _GhostAnalyzeAndSendMiniPacket(pGL,pNew);
#ifdef MINIPACKET_STATS
   total_hb++;
#endif
   
   // fill last, update times, etc...
   pGL->info.last          = *pNew;
   pGL->info.last.time     = GetSimTime();
   pGL->cfg.flags         &= ~kGhostCfForce;  // we sent, force no longer needed

   // do same horrible thing we do on receiver... i hate this
   mxs_vector old_vel=pGL->info.pred.pos.vel;
   pGL->info.pred          = pGL->info.last;  // start predicting from the packet we just sent
   if ((pGL->info.pred.pos.flags&kGhostHBAbsVel)==0)
      pGL->info.pred.pos.vel=old_vel;
   
   // set/clear edge triggers
   pGL->info.rel_obj       = pNew->rel_obj;
   pGL->playing.schema_idx = kGhostMotSchemaNoCustom;

   // did we state change?
   if (pNew->pos.mode==kGhostModeSleep) // we are gonna go to sleep too, since we sent it
      pGL->cfg.flags|=kGhostCfDontRun;

   // misc debugging fun
   if (_ghost_show_sends())
      _ghost_mprintf(("Sent message %d from %s at time %d\n",pGL->info.seq_id,ObjWarnName(pGL->obj),GetSimTime()));
   if (_ghost_show_full_sends())
      _GhostPrintGhostPos(pGL->obj,&pNew->pos,pGL->cfg.flags,"snd",pGL->info.seq_id);
}

////////////////////////
// Frame Control

// @TBD: as you might imagine, this is not a particularly good implementation at this point
static int _ComputeHeartFrequencyMax(sGhostLocal *pGL, sGhostPos *pNew)
{
   // fast moving things want more
   // type
   // detail slider
   return 30000;  // 30s, sure, why not...
}

static int _ghost_HF_Min[]={0,500,250,100};

static int _ComputeHeartFrequencyMin(sGhostLocal *pGL, sGhostPos *pNew)
{
   // fast moving things want more, near player as well
   // and should listen to the detail slider too
   
   // type
   return _ghost_HF_Min[_GhostGetType(pGL->cfg.flags)];
}


////////////////////////////////////////
// detail systems

static int _ghost_detail_mul[]={1.0,1.5,1.0,0.5};

static float _ComputeGhostDetail(sGhostLocal *pGL)
{
   return pGL->cfg.detail;
}

// these are all totally bogus, need a core of estimate/predition code
#define kGhostEpsPos           (20.0) // w30 // distance from the last 
#define kGhostEpsApproxPos      (4.0) // w8
#define kGhostEpsAbsVel         (4.0)
#define kGhostEpsControlVel     (4.0)
#define kGhostEpsFromZeroVel    (1.0) 
#define kGhostEpsHeading     (0x2000)
#define kGhostEpsPitch       (0x1000)
#define kGhostEpsHeadApprox  (0x1000)
#define kGhostEpsHeadVel     (0x2800)  // really, i almost never want this to fire
#define kGhostDtzMinCap       (0x800)

// use detail implicitly
#define _fltEps(type) ((float)((kGhostEps##type##))*_ComputeGhostDetail(pGL))
#define _intEps(type) ((int)((kGhostEps##type##))*_ComputeGhostDetail(pGL))

////////////////////////////////////////
// do the wierd stuff we do for dtz computation
static void _GhostClearDtz(sGhostLocal *pGL)
{
   pGL->info.dtz_dat.s_ptr=pGL->info.dtz_dat.s_cnt=pGL->info.dtz_dat.dtz_sum=0;
}

// actually go compute a current windowed average dtz and update streak counts
static short _GhostFindDtz(sGhostLocal *pGL, mxs_ang cur_tz)
{
   int base_time, cur_time=GetSimTime();
   sGhostDtzInfo *pDtz=&pGL->info.dtz_dat;
   int prev_samp=(pDtz->s_ptr+NUM_DTZ_SAMPLES-1)&MASK_DTZ_SAMPLES;
   mxs_ang last_tz;

   if (pDtz->s_cnt)
      if (pGL->last_fr!=pDtz->samples[prev_samp].s_time)
      {  // invalidating all old data
         pDtz->s_ptr=pDtz->s_cnt=pDtz->dtz_sum=0; // pDtz->streak=0;
         last_tz=pGL->info.last.pos.angle_info.tz;
      }
      else
      {
         last_tz=pDtz->samples[prev_samp].s_tz;
      }
   else
      last_tz=pGL->info.last.pos.angle_info.tz;
   short cur_dtz=cur_tz-last_tz;
   
   if (pDtz->s_cnt==NUM_DTZ_SAMPLES)
   {
      pDtz->dtz_sum-=pDtz->samples[pDtz->s_ptr].s_dtz;  // sub out old value
      base_time=pDtz->samples[pDtz->s_ptr].s_time;
   }
   else // still working up to full set of samples
   {
      base_time=pGL->info.last.time;
      pDtz->s_cnt=pDtz->s_ptr+1;
   }
      
   pDtz->samples[pDtz->s_ptr].s_tz   = cur_tz;
   pDtz->samples[pDtz->s_ptr].s_time = cur_time;
   pDtz->samples[pDtz->s_ptr].s_dtz  = cur_dtz;

   pDtz->dtz_sum += cur_dtz;
   pDtz->s_ptr=(pDtz->s_ptr+NUM_DTZ_SAMPLES+1)&MASK_DTZ_SAMPLES;

   if (cur_dtz==0)
      if (pDtz->streak>0) pDtz->streak=-1;
      else pDtz->streak--;
   else
      if (pDtz->streak<0) pDtz->streak= 1;
      else pDtz->streak++;

   short real_dtz=DTANG_TO_FIXVEL(pDtz->dtz_sum,cur_time-base_time);

   if (_ghost_track_heading())
      if (_GhostIsType(pGL->cfg.flags,Player))
         _ghost_mprintf(("%s real %d from sum %d times %d %d streak %d (cnt %d ptr %d)\n",
                         ObjWarnName(pGL->obj),real_dtz,pDtz->dtz_sum,cur_time,base_time,pDtz->streak,pDtz->s_cnt,pDtz->s_ptr));
   return real_dtz;
}

#ifdef GHOST_DEBUGGING
static int _ghost_new_packet_reason;
#define Return_NeedPacketReason(reason) { _ghost_new_packet_reason=reason; return TRUE; }
#else
#define    _ghost_new_packet_reason 0
#define Return_NeedPacketReason(reason) return TRUE
#endif

// local host, do we need to send packet updates
// first, go through and build pNew, since we need to know our state
// then, look at pNew verse last sent packet to determine what to do
static BOOL _IsNewPacketNeeded(sGhostLocal *pGL, sGhostPos *pNew)
{
   cPhysModel   *pModel = g_PhysModels.Get(pGL->obj);
   if (pModel==NULL)
   {  // since we check for phys outside, shouldnt happen, but ....
      Warning(("Hey! GhostIsNewPacketNeeded called on %s, which has no phys models\n",ObjWarnName(pGL->obj)));
      return FALSE;
   }
   
   cPhysCtrlData *pCtrl = pModel->GetControls();
   cPhysDynData   *pDyn = pModel->GetDynamics();

   if (pCtrl==NULL || pDyn==NULL)
   {  // this really should never fire, but hey, lets be safety pup approved
      Warning(("Hey! GhostIsNewPacketNeeded called on %s, which has pCtrl %x pDyn %x\n",ObjWarnName(pGL->obj),pCtrl,pDyn));
      return FALSE;
   }
   
   pNew->pos.flags = 0;

   // do weapon and mode, and any other player specific things
   if (_GhostIsType(pGL->cfg.flags,Player))
   {
      switch (PlayerMotionGetActive())
      {
      case kMoLeanLeft:  pNew->pos.mode = kGhostModeLeanLeft;  break;
      case kMoLeanRight: pNew->pos.mode = kGhostModeLeanRight; break;
      default:           pNew->pos.mode = GetPlayerMode();     break;
      }
      pNew->pos.flags |= kGhostHBHaveMode;
      pNew->pos.weap   = pGL->nWeapon;
      if (pNew->pos.weap!=pGL->info.last.pos.weap)
      {
         pNew->pos.flags |= kGhostHBWeap; // counts on any change going G
         if (_ghost_track_weapons())
            _ghost_mprintf(("Send %s setting pNew weap to %d\n",ObjWarnName(pGL->obj),pNew->pos.weap));
      }
   }
   else 
      pNew->pos.mode  = 0;

   // lets assume we dont need to force an update w/these, since a mocap will be soon
   if (pGL->state&kGhostStSwinging)  
      pNew->pos.flags |= kGhostHBStartSw;
   if (pGL->state&(kGhostStDead|kGhostStDying))
      pNew->pos.flags |= kGhostHBDead;
   if (_GhostGravLocal(pGL->obj, pNew, pGL->cfg.flags))
      pNew->pos.flags |= kGhostHBGravity;

   pNew->pos.pos   = pModel->GetLocationVec();

   // do the world-to-rel here, so we're always comparing relative location
   pNew->rel_obj = OBJ_NULL;
   _ghost_pos_worldtorel(pGL->obj,&pNew->rel_obj,&pNew->pos.pos,pGL->cfg.flags);
   if (pNew->rel_obj!=OBJ_NULL)
      pNew->pos.flags |= kGhostHBOnObj;      

   // get velocity data...
   if (_GhostIsType(pGL->cfg.flags,AI))
   {                             // as AI's seem to not set vels at all
      sAIImpulse ourImp;
      g_pAINetServ->GetTargetVel(pGL->obj,&ourImp);
#if 0      
      if (pGL->state&kGhostStSwinging)   // dont look at impulse during combat???
         mx_zero_vec(&pNew->pos.vel);    // probably a bad idea...
      else
#endif         
         pNew->pos.vel    = ourImp.vec;
      // mprintf("Impulse %g %g %g\n",ourImp.vec.el[0],ourImp.vec.el[1],ourImp.vec.el[2]);
      pNew->pos.flags |= kGhostHBAbsVel;  // should this be treated as control or abs???
   }
   else if (pModel->IsVelocityControlled()&&pNew->pos.mode!=kPM_Jump)
      pNew->pos.vel    = pCtrl->GetControlVelocity();
   else
   {
      pNew->pos.vel    = pDyn->GetVelocity();
      _ghost_vel_worldtorel(pNew->rel_obj, &pNew->pos.vel);
      pNew->pos.flags |= kGhostHBAbsVel;
   }
   //   mprintf("Got %g %g %g\n",pNew->pos.vel.x,pNew->pos.vel.y,pNew->pos.vel.z);

   // now decide which Angular model to use
   if (pGL->cfg.flags&kGhostCfObjPos || _GhostIsType(pGL->cfg.flags,Player))
   {
      pNew->pos.angle_info.fac=ObjPosGet(pGL->obj)->fac;
      if ((pNew->pos.angle_info.fac.ty|pNew->pos.angle_info.fac.tx)==0)
      {  // as angle_info.fac.tz overlaps the other, this gets easier
         pNew->pos.angle_info.dtz = _GhostFindDtz(pGL,pNew->pos.angle_info.tz);

         // don't forget about p: gets real val for player, zeroed for everyone else
         if (_GhostIsType(pGL->cfg.flags,Player))
            pNew->pos.angle_info.p = pModel->GetRotation(PLAYER_HEAD).ty;
         else
            pNew->pos.angle_info.p = 0;
      }
      else
         pNew->pos.flags |= kGhostHBFullAngs;
   }
   else
   {
      mxs_vector rvel = pDyn->GetRotationalVelocity();
      if (mx_mag2_vec(&rvel)<EPS_ZERO_VEC)
      {
         pNew->pos.angle_info.fac=pModel->GetRotation();
         pNew->pos.flags |= kGhostHBFullAngs;
      }
      else
      {
         if (fabs(rvel.x)+fabs(rvel.y)>0.05)
         {
            // _ghost_mprintf(("%s losing rotational data %g %g %g\n",ObjWarnName(pGL->obj),rvel.x,rvel.y,rvel.z));
            pNew->pos.flags |= kGhostHBAxisRot;
         }
#define RADVEL_TO_FIXVEL(x) (short)(((x) / MX_REAL_PI) * 0x8000)
         pNew->pos.angle_info.tz  = pModel->GetRotation().tz;
         pNew->pos.angle_info.dtz = RADVEL_TO_FIXVEL(rvel.z);
         pNew->pos.angle_info.p   = pModel->GetRotation().ty;
      }
   }

#ifdef GHOST_DEBUGGING
   if ((pGL->state&(kGhostStSleep|kGhostStRevive))==(kGhostStSleep|kGhostStRevive))
      _ghost_mprintf(("GHOST SND CONFUSED, %s is %x, Sleeping and Reviving at once\n",ObjWarnName(pGL->obj),pGL->state));

   // so we can show the packet each frame
   if (_ghost_show_pNew())
   {
      _GhostPrintGhostPos(pGL->obj,&pNew->pos,pGL->cfg.flags,"pnew",-1);
      if ((pNew->pos.flags&kGhostHBAbsVel)==0)
      {
         mxs_vector tmp = pDyn->GetVelocity();
         if (__ghost_dbg_rel())
            _ghost_mprintf((" ps. current vel %g %g %g\n",tmp.x,tmp.y,tmp.z));
      }
   }
   if (_ghost_show_send_pred())
      _GhostPrintGhostPos(pGL->obj,&pGL->info.pred.pos,pGL->cfg.flags,"sprd",-1);
   if (_ghost_show_send_last())
      _GhostPrintGhostPos(pGL->obj,&pGL->info.last.pos,pGL->cfg.flags,"slst",-1);
#endif

   if (pNew->rel_obj != pGL->info.rel_obj)
   {
      if (_ghost_track_relobj())
         _ghost_mprintf(("%s sending relobj %s (old %s)\n",
                         ObjWarnName(pGL->obj),ObjWarnName(pNew->rel_obj),ObjWarnName(pGL->info.rel_obj)));
      pNew->pos.flags|=kGhostHBObjRel;   // relobj edge trigger - so it sends
   }

   ///////////////
   // reasons which require guaranteed packets first
   // do we want to set guarantee, or anything?
   if (pGL->state&kGhostStSleep)         // next packets will be sleep as well
   {
      pNew->pos.mode=kGhostModeSleep;    // have to do this before mode !=, and before needreason
      pGL->state&=~kGhostStSleep;
      pGL->state|= kGhostSendStSleeping; // umm, not sure this is right, but hey
      pNew->pos.flags|=kGhostHBHaveMode; // hey, havemode here, so we trigger the last thing..
   }

   if (pGL->cfg.flags&kGhostCfNew)
   {
      pNew->pos.flags|=kGhostHBUseG;     // always use G packet for first from a New Ghost
      pGL->cfg.flags&=~kGhostCfNew;      // and clear the New field
      Return_NeedPacketReason(1);
   }

   // check mode change
   if (pNew->pos.flags&kGhostHBHaveMode)
      if (pNew->pos.mode!=pGL->info.last.pos.mode)
      {
         pNew->pos.flags|=kGhostHBUseG;  // always use G packets for mode changes?
         Return_NeedPacketReason(2);
      }

   // do the revive thing
   if (pGL->state&kGhostStRevive)
   {
      pNew->pos.mode=kGhostModeRevive;
      pNew->pos.flags|=kGhostHBHaveMode|kGhostHBUseG;
      pGL->state=0;  // clear all state, why not, live it up...
      Return_NeedPacketReason(4);
   }
   if (pGL->state&kGhostStDying)   // set internal im dead state
      pGL->state|=kGhostStDead;    // wont be cleared till revive

   // player mocap hell - for now player only
   if (_GhostIsType(pGL->cfg.flags,Player)&&(pGL->state&kGhostStIrqChecks))
   {  // add "do" to pNew flags, if any left... - actually mode or pNew, woo woo
      int new_schema=kGhostMotSchemaNoCustom, state_bit=1, i;
      if (_ghost_watch_events())
         _ghost_mprintf(("Handle state IRQ init state %x...",pGL->state));
      if (GhostPlayerActionCallback)    // if no cback, cant play motions, what is our point
         for (i=0; state_bit<=kGhostStLastPerFrame; i++, state_bit<<=1)
            if ((new_schema==kGhostMotSchemaNoCustom)&&(pGL->state&state_bit))
            {
               new_schema=(*GhostPlayerActionCallback)(pGL->obj,pNew->pos.mode,state_bit);
               pGL->state&=~state_bit;
            }
            else if (new_schema!=kGhostMotSchemaNoCustom)    // if we found something
               pGL->state&=~state_bit;  // clear all less important state
      if (new_schema!=kGhostMotSchemaNoCustom)  // setting new schema will trigger a send due to mocap
      {
         if (_ghost_watch_events())
            _ghost_mprintf(("final state %x\n",pGL->state));
         pGL->playing.schema_idx=new_schema;
      }
      else
         Warning(("MoCap Int Flag (%x) on player, but no schema\n",pGL->state));
   }

   if (pGL->state&kGhostStDying)   // if dying - clear it out
      pGL->state&=~kGhostStDying;
   
   if (pGL->playing.schema_idx>0)
   {  // always use G packets for custom captures)
      pNew->pos.flags|=kGhostHBUseG;
      Return_NeedPacketReason(5);
   }

   if (pNew->pos.flags&kGhostHBWeap)
   {
      pNew->pos.flags|=kGhostHBUseG;  // always use G packets for weap
      Return_NeedPacketReason(6);
   }

   // check flag
   if (pGL->cfg.flags&kGhostCfForce)
   {
      pNew->pos.flags|=kGhostHBUseG;  // always use G packets for forces
      Return_NeedPacketReason(7);
   }

   // early out if we are sending too often - ???
   if (pGL->info.last.time+_ComputeHeartFrequencyMin(pGL,pNew)>GetSimTime())
      return FALSE;

   // check transition from relative to non-relative
   if (pNew->pos.flags&kGhostHBObjRel)
   {
      if (_ghost_watch_events())
         _ghost_mprintf(("Ghost %s relobj edge %d -> %d\n",ObjWarnName(pGL->obj),pGL->info.rel_obj,pNew->rel_obj));
      pNew->pos.flags|=kGhostHBUseG;     // always use G packet for rel-world transition
      Return_NeedPacketReason(3);
   }
   
   ////////////////
   // transition packets which are only G to make sure we hear them

   // going to Zero velocity
   if (_is_zero_vec(&pNew->pos.vel))
      if (!_is_zero_vec(&pGL->info.last.pos.vel))
      {
         pNew->pos.flags|=kGhostHBUseG;  // dont want to miss hearing about go to zero
         Return_NeedPacketReason(11);
      }

   if ((pNew->pos.flags&kGhostHBFullAngs)==0)
      if (pGL->info.last.pos.angle_info.dtz != 0) // last send non-zero
         if (pGL->info.dtz_dat.streak <= -2)      // 2 zero frames
         {
            pNew->pos.angle_info.dtz=0;
            _GhostClearDtz(pGL);
            if (_ghost_math_sends())
               _ghost_mprintf(("%d dtz 2 Frames at %x pred %x\n",pGL->obj,
                               pNew->pos.angle_info.dtz,pGL->info.pred.pos.angle_info.dtz));
            pNew->pos.flags|=kGhostHBUseG;  // dont want to miss hearing about go to zero
            Return_NeedPacketReason(18);    
         }

   ////////////////
   // now "normal" reasons to require packets

#define constantFlags (kGhostHBFullAngs|kGhostHBAbsVel)
   // if we switched how we are reporting angles or velocity, send an update
   if ((pNew->pos.flags&(constantFlags))!=(pGL->info.last.pos.flags&(constantFlags)))
      Return_NeedPacketReason(8);
   
   // check timeout
   if (pGL->info.last.time+_ComputeHeartFrequencyMax(pGL,pNew)<GetSimTime())
   {
      // no timeout if we are in the EXACT SAME PLACE - should check rotation too...
      if (!mx_is_identical(&pNew->pos.pos,&pGL->info.last.pos.pos,0.02))
         if (!AngvecMatch(&pNew->pos.angle_info.fac,&pGL->info.last.pos.angle_info.fac,0x100))
            Return_NeedPacketReason(9);
   }

   float vel_eps=(pNew->pos.flags&kGhostHBAbsVel)?_fltEps(AbsVel):_fltEps(ControlVel);
   
   // check new velocity
   if (!mx_is_identical(&pNew->pos.vel,&pGL->info.last.pos.vel,vel_eps))
   {
      if (_ghost_math_sends())
         _ghost_mprintf(("%d n:%g %g %g o:%g %g %g e:%g f:%x\n",
                         pGL->obj,pNew->pos.vel.el[0],pNew->pos.vel.el[1],pNew->pos.vel.el[2],
                         pGL->info.last.pos.vel.el[0],pGL->info.last.pos.vel.el[1],pGL->info.last.pos.vel.el[2],
                         vel_eps,pNew->pos.flags));
      Return_NeedPacketReason(10);
   }

   if (_is_zero_vec(&pGL->info.last.pos.vel))
      if (mx_mag2_vec(&pNew->pos.vel)>_fltEps(FromZeroVel)*_fltEps(FromZeroVel))
      {
         if (_ghost_math_sends())
            _ghost_mprintf(("%d from zero vel n:%g %g %g e:%g f:%x\n",
                            pGL->obj,pNew->pos.vel.el[0],pNew->pos.vel.el[1],pNew->pos.vel.el[2],
                            vel_eps,pNew->pos.flags));
         Return_NeedPacketReason(12);
      }
   
   // check position delta
   if (!mx_is_identical(&pNew->pos.pos,&pGL->info.last.pos.pos,_fltEps(Pos)))
      Return_NeedPacketReason(13);
   if (!mx_is_identical(&pNew->pos.pos,&pGL->info.pred.pos.pos,_fltEps(ApproxPos)))
   {
      if (_ghost_math_sends())
         _ghost_mprintf(("%d cur %g %g %g pred %g %g %g\n",
                         pGL->obj,pNew->pos.pos.el[0],pNew->pos.pos.el[1],pNew->pos.pos.el[2],
                         pGL->info.pred.pos.pos.el[0],pGL->info.pred.pos.pos.el[1],pGL->info.pred.pos.pos.el[2]));
      Return_NeedPacketReason(14);
   }

   // check angular delta
   if (pNew->pos.flags&kGhostHBFullAngs)
   {
      if (!AngvecMatch(&pNew->pos.angle_info.fac,&pGL->info.last.pos.angle_info.fac,_intEps(Heading)))
      {
         if (_ghost_math_sends())
            _ghost_mprintf(("%d cur %x %x %x pred %x %x %x\n",
                            pGL->obj, pNew->pos.angle_info.fac.el[0],
                            pNew->pos.angle_info.fac.el[1], pNew->pos.angle_info.fac.el[2],
                            pGL->info.last.pos.angle_info.fac.el[0], // ack
                            pGL->info.last.pos.angle_info.fac.el[1],
                            pGL->info.last.pos.angle_info.fac.el[2]));
         Return_NeedPacketReason(15);
      }
   }
   else
   {
      if (abs((short)(pNew->pos.angle_info.tz - pGL->info.pred.pos.angle_info.tz)) > _intEps(HeadApprox))
      {
         if (_ghost_math_sends())
            _ghost_mprintf(("%d tz at %x pred %x\n",pGL->obj,
                            pNew->pos.angle_info.tz,pGL->info.pred.pos.angle_info.tz));
         Return_NeedPacketReason(16);
      }

      if (pGL->info.last.pos.angle_info.dtz == 0) // last sent at zero, only care about up
      {
         if (pGL->info.dtz_dat.streak > 2)        // 2 positive frames
            if (pNew->pos.angle_info.dtz>kGhostDtzMinCap)
            {
               if (_ghost_math_sends())
                  _ghost_mprintf(("%d Dtz non-zero at %x pred %x\n",pGL->obj,
                                  pNew->pos.angle_info.dtz,pGL->info.pred.pos.angle_info.dtz));
               Return_NeedPacketReason(17);            
            }
      }
      
      // and dtz delta itself, as well
      if (abs((short)(pNew->pos.angle_info.dtz - pGL->info.last.pos.angle_info.dtz)) > _intEps(HeadVel))
      {
         if (_ghost_math_sends())
            _ghost_mprintf(("%d dtz at %x pred %x\n",pGL->obj,
                            pNew->pos.angle_info.dtz,pGL->info.pred.pos.angle_info.dtz));
         Return_NeedPacketReason(19);
      }

      if (abs((short)(pNew->pos.angle_info.p - pGL->info.last.pos.angle_info.p)) > _intEps(Pitch))
      {
         if (_ghost_math_sends())
            _ghost_mprintf(("%d p at %x pred %x\n",pGL->obj,
                            pNew->pos.angle_info.p,pGL->info.pred.pos.angle_info.p));
         Return_NeedPacketReason(20);
      }
   }
   
   return FALSE;
}

#define NUM_SEND_REASONS 21 // ???? Keep this in Synch, this is hateful, so shoot me

int ghost_local_frame_rate=0;      // ms between frames 
 
// go through our local ghosts and see what we care about updating
void _GhostFrameProcessLocal(sGhostLocal *pGL, float dt)
{
   if ((ghost_local_frame_rate)&&(pGL->last_fr+ghost_local_frame_rate>GetSimTime()))
      return;  // this ghost was last run at a time under our frame rate, so skip
   _GhostDebugSetupLocal(pGL);   
   if (pGL->state&kGhostStRevive)        // if user revives us
      pGL->cfg.flags&=~kGhostCfDontRun;  // allow ourselves to run again
   if (!PhysObjHasPhysics(pGL->obj)||(pGL->cfg.flags&(kGhostCfDontRun|kGhostCfDisable)))
      return;  // dont run anything if we are non-physical, or dontrun|disabled
   sGhostPos new_pos;

   BOOL gravity = pGL->info.last.pos.flags & kGhostHBGravity;

   _GhostApproxPhys(pGL->obj,&pGL->info.last,&pGL->info.pred,dt,gravity);
   if (!_is_zero_vec(&pGL->info.pred.pos.vel))  // hmmm, perhaps the right thing
      _GhostBleedVelocity(pGL->obj, &pGL->info.pred, pGL->cfg.flags, pGL->info.last.time, dt);

   if (_IsNewPacketNeeded(pGL,&new_pos))
   {
      if (_ghost_explain_sends()) // show reasons for the new packet
         _ghost_mprintf(("ghost %s sending reason %d time %d\n", ObjWarnName(pGL->obj), _ghost_new_packet_reason, GetSimTime()));
      _ghost_histo_add(pGL->obj,_ghost_new_packet_reason,GetSimTime());
      _GhostSendHeartbeat(pGL,&new_pos);
   }
   pGL->last_fr=GetSimTime();
}

static void _GhostWeaponCallback(eWeaponEvent event, ObjID victim, ObjID culprit, void *data)
{
   _GhostDebugSetupObj(culprit);
   if (IsLocalGhost(culprit))
   {
      if (event==kStartAttack)
      {
         sGhostLocal *pGL=GhostGetLocal(culprit);
         pGL->state|=kGhostStSwinging;
         if (_ghost_track_weapons())
            _ghost_mprintf(("ghost setting StartAttack for %s (%x)\n",ObjWarnName(culprit),pGL->state));
      }
      else if (event==kEndAttack)
      {
         sGhostLocal *pGL=GhostGetLocal(culprit);
         pGL->state&=~kGhostStSwinging;
         if (_ghost_track_weapons())
            _ghost_mprintf(("ghost clearing StartAttack for %s (%x)\n",ObjWarnName(culprit),pGL->state));
      }
   }
#ifdef DBG_ON
   else if (IsRemoteGhost(culprit))
   {
      if (_ghost_track_weapons())
         _ghost_mprintf(("GWC: seeing %x for %s\n",event,ObjWarnName(culprit)));
   }
#endif   
}

// get the appagg to get the aimanager
void GhostSendInit(void)
{
   g_pAINetServ = AppGetObj(IAINetServices);
   RegisterWeaponEventCallback(kStartEndEvents,_GhostWeaponCallback,NULL);
   _ghost_histo_init(NUM_SEND_REASONS,4000);  // 4s is "long"

   config_get_int("ghost_local_frame_rate",&ghost_local_frame_rate);

#ifdef GHOST_DEBUGGING
   sGhostHeartbeat HBTest;     // horrible hack for compiler ordering?
   HBTest.angle_info.fac.tz=0xdead;
   AssertMsg(HBTest.angle_info.tz==0xdead,"Ghost TZ Union alignment failure!!");
#endif
}

void GhostSendTerm(void)
{
   SafeRelease(g_pAINetServ);
   DeregisterWeaponEventCallback(kStartEndEvents,_GhostWeaponCallback);
   _ghost_histo_term();
#ifdef MINIPACKET_STATS
   if (total_hb)
   {
      _ghost_mprintf(("\nGhostAllow mini %d, rot %d, ai %d/%d, obj %d/%d out of %d ng (%d total)\n",
                      allow_mini_hb, allow_rot_hb, allow_ai_hb, nogo_ai_hb, allow_obj_hb, nogo_obj_hb, total_ng_hb, total_hb));
      _ghost_mprintf(("Failures dp %d dpZ %d dvZ %d magvZ %d dFac %d magdtZ %d flags %d dv %d dpos %d (play mdtz %d)\n\n",
                      failed_dp, failed_dpZ, failed_dvZ, failed_mag_vZ, failed_dFac, failed_mag_dtz, failed_flags, failed_dvel, failed_dpos, failed_player_mag_dtz));
   }
#endif
}
