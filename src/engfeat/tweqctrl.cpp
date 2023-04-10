/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/tweqctrl.cpp,v 1.56 2000/02/20 18:11:26 adurant Exp $
// tweq system controllers

#include <math.h>
#include <stdlib.h>

#include <lg.h>
#include <lgassert.h>
#include <md.h>
#include <mprintf.h>

#include <rand.h>
#include <cfgdbg.h>

#include <prjctile.h>

#include <wr.h>
#include <objmodel.h>
#include <objpos.h>
#include <objscale.h>
#include <objedit.h>
#include <osysbase.h>
#include <objshape.h>
#include <rendobj.h>
#include <simtime.h>
#include <dmgmodel.h>

#include <iobjsys.h>
#include <appagg.h>
#include <objquery.h>

#include <mnumprop.h>
#include <mnamprop.h>
#include <rendprop.h>

#include <property.h>
#include <propbase.h>
#include <proptype.h>

#include <tweqbase.h>
#include <tweqprop.h>
#include <tweqenum.h>
#include <tweqexp.h>
#include <tweqflgs.h>
#include <twqprop_.h>
#include <tweqctrl.h>

#include <lockprop.h> // for fix_my_locks
#include <command.h>

#include <camera.h>   // for activity center, for now
#include <playrobj.h>

#include <dlistsim.h>
#include <dlisttem.h>

#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>

#include <dbmem.h>
#include <vhot.h>

#define min(x,y) ((x) < (y) ? (x) : (y))

static IObjectSystem *cur_osys=NULL;
static IDamageModel  *cur_dmgmodel=NULL;

//////////
//
// NETWORKING CODE
//
// Tweq-specific network messages.
//

// The network controller objects:
static INetManager *g_pNetMan;
static IObjectNetworking *g_pObjNet;

//
// @HACK: Stop TweqEmit
//
// This is a horrible hack, so that when we're supposed to do a one-shot
// tweq, and then be done with it, the clients don't keep trying to do it.
// Not only is this wasteful of cycles, we wind up doing the tweq again
// if the client bumps into the object (and thus temporarily borrows it).
// Someday, we need to figure out a much better way to do this.
//
static cNetMsg *g_pStopTweqEmitMsg = NULL;

static void handleStopTweqEmit(ObjID obj)
{
   sTweqSimpleState *tweq_simple;
   if (tweq_emitterstate_prop->Get(obj, &tweq_simple))
   {
      tweq_simple->state.flg_anim &= ~TWEQ_AS_ONOFF;
      tweq_emitterstate_prop->TouchValue(obj, tweq_simple);
   }
}

static sNetMsgDesc sStopTweqEmitDesc =
{
   kNMF_Broadcast,
   "StopEmit",
   "Stop Tweq Emit",
   NULL,
   handleStopTweqEmit,
   {{kNMPT_SenderObjID, kNMPF_IfHostedHere, "Obj"},
    {kNMPT_End}}
};

void BroadcastTweqEmitStop(ObjID obj)
{
#ifdef NEW_NETWORK_ENABLED
   g_pStopTweqEmitMsg->Send(OBJ_NULL, obj);
#endif
}

//////////

// cribbed whole hack/hog from rendtest.c, perhaps should be somewhere useful
// needs to be a recorded rand for recordings!!
static float frand_hack(void)
{
   float tmp=(float)((Rand()&0x7fff)-16384);
   tmp/=16384;
   return tmp;
}

////////////
// misc help functions

static mxs_vector cur_activity_center_vec; // where activity is, so we can see if we are close to it

//#define simple_process_needed(cfg,obj) ((cfg->flg_anim&TWEQ_AC_SIM)||rendobj_object_visible(obj))

#define simple_process_needed(cfg,obj) check_basics(cfg,obj)

inline static BOOL check_basics(sTweqBaseConfig *cfg, ObjID obj)
{
   if (cfg->flg_anim&TWEQ_AC_SIM) return TRUE;
   if (cfg->flg_anim&TWEQ_AC_OFFSCRN)
      return !rendobj_object_visible(obj);
   else
      return  rendobj_object_visible(obj);
}

// call on an object you want to query about whether you are supposed to run it this frame
// does sim and on screen testing, for now
// Also checks whether this is a network proxy, and we're not supposed
// to affect it.
static BOOL tweq_process_needed(sTweqBaseConfig *cfg, ObjID obj)
{
   if (cfg->flg_anim&(TWEQ_AC_SIMRADSM|TWEQ_AC_SIMRADLG))
   {  // have to get our pos, grim
      if (simple_process_needed(cfg,obj))
         return TRUE;
      mxs_vector *myloc=ObjPosGetLocVector(obj);
      float use_rad2=20.0*20.0;;
      if (cfg->flg_anim&TWEQ_AC_SIMRADLG)
         use_rad2=80.0*80.0;
      return (mx_dist2_vec(myloc,&cur_activity_center_vec)<use_rad2);
   }
   if (cfg->flg_misc & TWEQ_MC_HOSTONLY)
   {
#ifdef NEW_NETWORK_ENABLED
      // This should only be used for stuff that's done
      // occasionally, not frame-by-frame. If we *do* need it for
      // frame-by-frame, then we'll need to do some serious
      // buffering.
      if (g_pNetMan->Networking())
      {
         if (g_pObjNet->ObjIsProxy(obj)) {
            return FALSE;
         }
      }
#endif
   }
   return simple_process_needed(cfg,obj);
}

// call on an object you have decided is done tweq'ing
// returns code for halt, remove, kill
static int tweq_test_haltkill(sTweqBaseConfig *cfg)
{
   return cfg->halt_act;
}

static BOOL tweq_halt_is_killtype(int halt_act)
{
   return ((halt_act==TWEQ_HALT_KILL)||(halt_act==TWEQ_HALT_SLAY));
}

static void _tweq_do_reverse(sTweqBaseState *state)
{
   if (state->flg_anim&TWEQ_AS_REVERSE)  // should be == top_edge, but not quite sure?
      state->flg_anim&=~TWEQ_AS_REVERSE;
   else
      state->flg_anim|= TWEQ_AS_REVERSE;
}

//////////
// general vector support

// remember, limits are rate-low-high
#define TweqLimitValid(limits) ((limits)->el[TWEQ_LIMIT_RATE]!=0.0)

// call before doing the reverse, for now
// has to all get rewritten for LapOne and stuff soon...
static BOOL end_condition(int cfg_flg, int state_flg)
{
   if ((cfg_flg&TWEQ_AC_1BOUNCE)==TWEQ_AC_1BOUNCE)
      if ((state_flg&TWEQ_AS_REVERSE)==0)
         return FALSE;
   return TRUE;
}

// call this with valid limits!
// returns TRUE if the axis is still ok and val is good
// FALSE means it exceeded limits and was killed off
BOOL processTweqAxis(float *val, sTweqBaseConfig *cfg, sTweqBaseState *state, mxs_vector *limits, int ms)
{
   float eff_rate=limits->el[TWEQ_LIMIT_RATE], new_val=*val, step=ms/100.0;;
   AssertMsg(TweqLimitValid(limits),"Calling processTweqAxis with invalid limit\n");
   // first, lets update the val
   if (state->flg_anim&TWEQ_AS_REVERSE)
      eff_rate*=-1.0;
   if (cfg->flg_curve&TWEQ_CC_MUL)   // how do we make this time invariant, anyway???
   {
      if (cfg->flg_curve&TWEQ_CC_JITTER)
      {
         float delta=0.05+fabs(1-eff_rate);  // add 0.05 to make sure we get some jitter
         float fac=cfg->flg_curve&TWEQ_CC_JITTER;        // fac is now 1-3
         delta=1.0+(delta*fac*frand_hack()/2.0);
         eff_rate*=delta;
      }
      new_val*=eff_rate;
   }
   else
   {
      new_val+=eff_rate*step;
      if (cfg->flg_curve&TWEQ_CC_JITTER)      
      {
         float fac=cfg->flg_curve&TWEQ_CC_JITTER;  // fac is now 1-3
         new_val+=(eff_rate*frand_hack()*fac*step/2.0);
      }
   }
   if ((cfg->flg_anim&TWEQ_AC_NOLIMIT)==0)
   {  // not no limit, in other words - now clip verse limits
      int clip=0;   // 1 means low, 2 means high
      if (new_val<limits->el[TWEQ_LIMIT_LOW])  
         clip=1;
      else if (new_val>limits->el[TWEQ_LIMIT_HIGH])
         clip=2;
      if (clip!=0)
      {
         BOOL is_end=end_condition(cfg->flg_anim,state->flg_anim);
         int halt_action=tweq_test_haltkill(cfg);
         if (tweq_halt_is_killtype(halt_action))
            if (is_end) return halt_action;
         if (cfg->flg_anim&TWEQ_AC_WRAP)   // could track overflow here, but messy
         {                                 //   ie. do LIMIT_HIGH+(new_val-LIMIT_LOW)
            new_val=limits->el[3-clip];    //   but what if still above/below limit? - so that is commented out for now
         }  // new_val=limits->el[3-clip]+(new_val-limits->el[clip]);
         else                         
         {
            new_val=limits->el[clip];      // limit overflow! MUST DO @TODO
            _tweq_do_reverse(state);
         }  // new_val=limits->el[clip]-(new_val-limits->el[clip]);
         *val=new_val;
         if (is_end)
            return halt_action;
      }
   }
   *val=new_val;   // we made it, so make the change
   return TWEQ_STATUS_QUO;
}

static int processTweqVector(mxs_vector *vec, sTweqVectorState *vec_state, sTweqVectorConfig *vec_cfg, int ms)
{
   int i, rv=TWEQ_STATUS_QUO;
   if (vec_cfg->primary_axis)  // copy global state bits to primary joint specific
      vec_state->axis_flgs[vec_cfg->primary_axis-1]=vec_state->state;
   for (i=0; i<3; i++)
   {
      mxs_vector *jnt_limit=&vec_cfg->limits[i];
      if (TweqLimitValid(jnt_limit))
      {
         int newrv=processTweqAxis(&vec->el[i],&vec_cfg->cfg,&vec_state->axis_flgs[i],jnt_limit,ms);
         if ((vec_cfg->primary_axis==0)||(i==vec_cfg->primary_axis-1))
            rv=newrv;
         if (rv!=TWEQ_STATUS_QUO)
            break;
      }
   }
   if (vec_cfg->primary_axis)  // copy global state bits to primary joint specific
      vec_state->state=vec_state->axis_flgs[vec_cfg->primary_axis-1];
   return rv;
}

////////////
// rotate

#define fixang_to_float_degrees(ang) ((float)ang*360.0/65536.0)
#define float_degrees_to_fixang(d)   ((fixang)(d*(65536.0/360.0)))

int processTweqRotate(ObjID obj, void *state, int ms)
{
   sTweqVectorState* vec_state = (sTweqVectorState*)state;
   sTweqVectorConfig *vec_cfg;
   ObjPos *our_pos=ObjPosGet(obj);
   mxs_vector vec;
   int rv=TWEQ_STATUS_QUO;

   if (!PROPERTY_GET(tweq_rotate_prop,obj,&vec_cfg))
   {
      Warning(("Cant get RotateCfg for obj %s\n",ObjEditName(obj)));
      return rv;
   }
   if (!tweq_process_needed(&vec_cfg->cfg,obj)) return rv;
   AssertMsg(our_pos,"Hey, no pos for TweqRotate object\n");
   vec.el[0]=fixang_to_float_degrees(our_pos->fac.el[0]);
   vec.el[1]=fixang_to_float_degrees(our_pos->fac.el[1]);
   vec.el[2]=fixang_to_float_degrees(our_pos->fac.el[2]);
   rv=processTweqVector(&vec,vec_state,vec_cfg,ms);
   our_pos->fac.el[0]=float_degrees_to_fixang(vec.el[0]);
   our_pos->fac.el[1]=float_degrees_to_fixang(vec.el[1]);
   our_pos->fac.el[2]=float_degrees_to_fixang(vec.el[2]);
   ObjRotate(obj,&our_pos->fac);
   return rv;
}

static void get_anchor(ObjID obj,int flag,mxs_vector *pos)
{
   if (flag&TWEQ_MC_ANCHOR) {
      mxs_vector dummy;
      ObjGetObjOffsetBBox(obj,pos,&dummy);
      // zero out so we can use full vector for delta
      // in truth this bounding box stuff is kind of suspect, since
      // only the z changes, when in fact, all x,y,z should be used
      // So that if we took this line out, it should actually do a better
      // job of keeping the models aligned, but I fear changing
      // existing behavior
      pos->x = pos->y  = 0;
   } else {
      // vhot based
      ObjGetObjOffsetVhot(obj,kVHotAnchor,pos);
   }
}

static void finalize_anchor(ObjID obj, int flag,mxs_vector *old_anchor)
{
   ObjPos* pos = ObjPosGet(obj);
   if (pos!=NULL)
   {
      mxs_vector new_anchor;
      get_anchor(obj,flag,&new_anchor);

      mx_addeq_vec(&pos->loc.vec,old_anchor);
      mx_subeq_vec(&pos->loc.vec,&new_anchor);
      ObjPosUpdateUnsafe(obj,&pos->loc.vec,&pos->fac);
   }
}

int processTweqScale(ObjID obj, void *state, int ms)
{
   sTweqVectorState* vec_state = (sTweqVectorState*)state;
   sTweqVectorConfig *vec_cfg;
   mxs_vector scale;
   int rv=TWEQ_STATUS_QUO;
   int misc_flag;
   
   if (!PROPERTY_GET(tweq_scale_prop,obj,&vec_cfg))
   {
      Warning(("Cant get ScaleCfg for obj %s\n",ObjEditName(obj)));   
      return rv;
   }
   if (!tweq_process_needed(&vec_cfg->cfg,obj)) return rv;
   if (!ObjGetScale(obj,&scale))
      scale.el[0]=scale.el[1]=scale.el[2]=1.0;
   rv=processTweqVector(&scale,vec_state,vec_cfg,ms);

   misc_flag = vec_cfg->cfg.flg_misc;
   if (misc_flag&(TWEQ_MC_ANCHOR|TWEQ_MC_VHOT)) {
      mxs_vector init_anchor;
      get_anchor(obj,misc_flag,&init_anchor);
      ObjSetScale(obj,&scale);
      finalize_anchor(obj,misc_flag,&init_anchor);
   } else {
      ObjSetScale(obj,&scale);
   }

   return rv;
}

//////////////
// joints

// misc helper/setup code
static int _model_count_joints(ObjID obj)
{
   int idx;
   mds_model *m;
   if (ObjGetModelNumber(obj,&idx))
      if ((m=(mds_model *)objmodelGetModel(idx))!=NULL)
         if (m->parms) // update appropriate joints, i think
            return m->parms;
   return 0;
}

static int _do_single_joint(mxs_vector *lim, ObjID obj, sTweqBaseConfig *cfg, sTweqBaseState *state, int ms, float *val)
{
   if (state->flg_anim&TWEQ_AS_ONOFF)   // if we are off, dont run
      if (tweq_process_needed(cfg,obj))
         if (TweqLimitValid(lim))
            return processTweqAxis(val,cfg,state,lim,ms);
   return TWEQ_STATUS_QUO;
}                           
                            
// need to add "auto-constraints" as well... ick
int processTweqJoints(ObjID obj, void *state, int ms)
{
   sTweqJointsState *jnt_state = (sTweqJointsState*)state; 
   sTweqJointsConfig *jnt_cfg;
   BOOL no_update=FALSE;
   int param_count=0, i, rv=TWEQ_STATUS_QUO;

   param_count=_model_count_joints(obj);
   if (param_count)
   {
      float *parms=ObjJointPos(obj), use_parms[MAX_REND_JOINTS];
      
      memcpy(use_parms,parms,MAX_REND_JOINTS*sizeof(float));
      if (!PROPERTY_GET(tweq_joints_prop,obj,&jnt_cfg))
      {
         Warning(("Cant get JointCfg for obj %s\n",ObjEditName(obj)));
         return rv;
      }
      if (jnt_cfg->primary_joint)  // copy global state bits to primary joint specific
         jnt_state->jnt_flgs[jnt_cfg->primary_joint-1]=jnt_state->state;
      for (i=0; i<min(param_count,MAX_TWEQ_JOINTS); i++)
      {
         mxs_vector *limits=&jnt_cfg->jnt_data[i].limits;
         sTweqBaseConfig *jnt_x_cfg=&jnt_cfg->jnt_data[i].flags;
         jnt_x_cfg->halt_act=jnt_cfg->cfg.halt_act;
         rv=_do_single_joint(limits,obj,jnt_x_cfg,&jnt_state->jnt_flgs[i],ms,&use_parms[i]);
         if (rv!=TWEQ_STATUS_QUO)                        
            if (i!=jnt_cfg->primary_joint-1) // if we arent the primary joint
               jnt_state->jnt_flgs[i].flg_anim&=~TWEQ_AS_ONOFF; // turn us off
            else
            {
               rv=tweq_test_haltkill(jnt_x_cfg);  // reget for our master config
               no_update=tweq_halt_is_killtype(rv);
               if (rv!=TWEQ_STATUS_QUO)
                  break;
            }
      }
      if (!no_update)
         ObjSetJointPos(obj,use_parms);
      if (jnt_cfg->primary_joint)  // copy back into primary state
         jnt_state->state=jnt_state->jnt_flgs[jnt_cfg->primary_joint-1];
   }
   else
   {
      Warning(("TweqJoints on unjointed model!! obj %d\n",obj));
      return TWEQ_HALT_STOP;
   }
   return rv;
}

////////////
// locks (single aux joint, basically)

// basic processing function
int processTweqLock(ObjID obj, void *state, int ms)
{
   sTweqLockState  *lock_state = (sTweqLockState*)state; 
   sTweqLockConfig *lock_cfg;
   BOOL no_update=FALSE;
   int param_count=0, rv=TWEQ_STATUS_QUO;

   param_count=_model_count_joints(obj);
   if (param_count)
   {
      float *parms=ObjJointPos(obj), use_parms[MAX_REND_JOINTS];
      memcpy(use_parms,parms,MAX_REND_JOINTS*sizeof(float));
      
      if (!PROPERTY_GET(tweq_lock_prop,obj,&lock_cfg))
      {
         Warning(("Cant get LockCfg for obj %s\n",ObjEditName(obj)));
         return rv;
      }
      sTweqBaseConfig *_x_cfg=&lock_cfg->cfg;
      int use_joint=lock_cfg->lock_joint;
      if (use_joint!=0) use_joint--;

      mxs_vector limits=lock_cfg->jnt_data.limits;   
      // now do the horrible limit hacking, using target vectors
      if (parms[use_joint]>lock_state->targ_ang) // past it
         limits.el[1]=lock_state->targ_ang;
      else        // if target_ang > joint_pos, set limit[2], else limit[1] to target ang, i think
         limits.el[2]=lock_state->targ_ang;
      rv=_do_single_joint(&limits,obj,_x_cfg,&lock_state->state,ms,&use_parms[use_joint]);
#if 0
      mprintf("got %d for joint %d targ %g now %g from %g, limits %g %g %g\n",
              rv,use_joint,lock_state->targ_ang,use_parms[use_joint],parms[use_joint],
              limits.el[0],limits.el[1],limits.el[2]);
#endif
      if (rv!=TWEQ_STATUS_QUO)
      {
         rv=tweq_test_haltkill(&lock_cfg->cfg);
         no_update=tweq_halt_is_killtype(rv);
      }
      if (!no_update)
         ObjSetJointPos(obj,use_parms);
   }
   else
   {
      Warning(("TweqLock on unjointed model!! obj %d\n",obj));
      return TWEQ_HALT_STOP;
   }
   return rv;
}

// controller calls for locks
// go and change the limit data for this lock to be
// where is 0-1.0, 0 totally locked, 1.0 totally locked
BOOL LockAnimSetTarget(ObjID obj, float where)
{  // go grind the lock data to get a target angle
   sTweqLockState  *lock_state;
   sTweqLockConfig *lock_cfg;
   if (tweq_lockstate_prop->Get(obj,&lock_state))
      if (tweq_lock_prop->Get(obj,&lock_cfg))
      {
         int use_joint=lock_cfg->lock_joint;
         if (use_joint!=0) use_joint--;
         float *parms=ObjJointPos(obj);         
         float total_delta=lock_cfg->jnt_data.limits.el[2]-lock_cfg->jnt_data.limits.el[1];
         lock_state->targ_ang=lock_cfg->jnt_data.limits.el[1]+(total_delta*where);
         float targ_delta=lock_state->targ_ang-parms[use_joint];
         lock_state->state.flg_anim|=TWEQ_AS_ONOFF;
         if (targ_delta*lock_cfg->jnt_data.limits.el[0]>0)  // want to be going forward
            lock_state->state.flg_anim&=~TWEQ_AS_REVERSE;
         else
            lock_state->state.flg_anim|=TWEQ_AS_REVERSE;
         return TRUE;
      }
      // 5/29/99 Xemu: turned into configspew since this is called by the
      // listener whether or not you care about this feature
   ConfigSpew("TweqSpew",("Couldnt find TweqLock Props for %s\n",ObjEditName(obj)));
   return FALSE;
}

// Function for setting lock joints on all Objects with StTweqLock to the proper positions.
void SetAllLockJoints()
{
	   
	sTweqLockConfig *lock_cfg;
	AutoAppIPtr(ObjectSystem);
	IObjectQuery* objquery = pObjectSystem->Iter(kObjectConcrete);

	//ok, so we have this STTweqLockProp.  Let's iterate over concrete
	//objects and do our thing.
	
	//there are probably more efficient ways to do this, but really this is
	//run as a command, and should take order 1 second, so how much faster
	//do you need?

	for (; !objquery->Done(); objquery->Next())
	{
		ObjID targ = objquery->Object();
        if (!(tweq_lock_prop->Get(targ,&lock_cfg)))
			continue; //don't have prop, return.

		//ok, so we have the prop.  Now lets do our thing.
		//Most of this is copied from AnimSetTarget.

		int use_joint=lock_cfg->lock_joint;
		int where =  (ObjSelfLocked(targ)?0.0:1.0); //standard lock setting.0 locked 1 unlocked.
        if (use_joint!=0) use_joint--; //array offsetting?
        float *parms=ObjJointPos(targ);     
		float targ_ang = (where ? lock_cfg->jnt_data.limits.el[2] : lock_cfg->jnt_data.limits.el[1]);
		//targ_ang is the position I want, either jnt_data.l
        parms[use_joint] = targ_ang; //force it.
		ObjSetJointPos(targ,parms);
		mprintf("Updating joint params on %d\n",targ);
	}

	objquery->Release(); //Like a good little boy.
}

//set up tweqctrl keys.

Command tweqctrl_keys[] =
{
   { "fix_my_locks", FUNC_STRING, SetAllLockJoints, "Set all my lock joint positions appropriately." },
};

void tweqctrlCommandRegister(void)
{
   COMMANDS(tweqctrl_keys, HK_BRUSH_EDIT);
}

////////////
// models

static int tweqGetTop(sTweqModelsConfig *mds_cfg)
{
   int i=MAX_TWEQ_MODELS-1;
   while ((i>0)&&(mds_cfg->names[i].text[0]=='\0'))
      i--;
   return i;
}

// returns TRUE if it should be deleted now that it hit the edge, whatever that means
// it will act on the edge hit if it doesnt toast, ie. set the new frame id
static int tweqHitEdge(sTweqSimpleState *twq_state, sTweqModelsConfig *mds_cfg, BOOL top_edge)
{
   sTweqBaseConfig *twq_cfg=&mds_cfg->cfg;
   if (((twq_cfg->flg_anim&TWEQ_AC_1BOUNCE)==0)||
        (twq_state->state.flg_anim&TWEQ_AS_REVERSE))
   {
      int rv=tweq_test_haltkill(twq_cfg);
      if (rv!=TWEQ_STATUS_QUO) return rv;
   }
   if (twq_cfg->flg_anim&TWEQ_AC_WRAP)
      twq_state->tm_info.cur_frame=top_edge?0:tweqGetTop(mds_cfg);
   else
   {
      twq_state->tm_info.cur_frame=top_edge?tweqGetTop(mds_cfg)-1:1;
      _tweq_do_reverse(&twq_state->state);
   }
   return TWEQ_STATUS_QUO;
}

static void chooseRandomFrame(sTweqSimpleState *twq_state, sTweqModelsConfig *mds_cfg)
{
   int i, new_fr;   // recount the frames every damn time...
   for (i=0; i<MAX_TWEQ_MODELS-1; i++)
      if (mds_cfg->names[i].text[0]=='\0')
         break;
   if (i<2)
    { new_fr=0; Warning(("Yo! no frames for random tweqModel\n")); }
   else       // now go to a random one
      do {
         new_fr=Rand()%i;
      } while (twq_state->tm_info.cur_frame==new_fr);
   twq_state->tm_info.cur_frame=new_fr;
}

static void addTime(ushort *overflow, int ms, sTweqBaseConfig *twq_cfg)
{
   if (twq_cfg->flg_curve&TWEQ_CC_JITTER)   // play with ms here...
      ;
   *overflow+=ms;
}

#define TWEQ_DURATION(cfg) (((cfg)->rate)+1)
#define TWEQ_FRAME(state)  ((state)->tm_info.cur_frame)
   
int processTweqModels(ObjID obj, void* state , int ms)
{
   sTweqSimpleState *twq_state = (sTweqSimpleState*)state;
   sTweqModelsConfig *mds_cfg;
   int toast_me=TWEQ_STATUS_QUO;

#ifdef NEW_NETWORK_ENABLED
   // Don't do tweqing of proxy objects.
   // @TBD: this is potentially called frame-by-frame; should we store
   // pNetManager and pObjectSystem?
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      AutoAppIPtr(ObjectNetworking);
      if (pObjectNetworking->ObjIsProxy(obj)) {
         return toast_me;
      }
   }
#endif

   if (!PROPERTY_GET(tweq_models_prop,obj,&mds_cfg))
   {
      Warning(("Cant get ModelCfg for obj %s\n",ObjEditName(obj)));      
      return toast_me;
   }
   if (!tweq_process_needed(&mds_cfg->cfg,obj)) return toast_me;
   addTime(&twq_state->tm_info.cur_time,ms,&mds_cfg->cfg);  //need to pass jitter/curve flags in here!
   while ((toast_me==TWEQ_STATUS_QUO)&&(twq_state->tm_info.cur_time>TWEQ_DURATION(&mds_cfg->cfg)))
   {  // go to next model
      twq_state->tm_info.cur_time-=TWEQ_DURATION(&mds_cfg->cfg);
      if (mds_cfg->cfg.flg_misc&TWEQ_MC_RANDOM)
         chooseRandomFrame(twq_state,mds_cfg);
      else if (twq_state->state.flg_anim&TWEQ_AS_REVERSE)
         if (TWEQ_FRAME(twq_state)>0)
            TWEQ_FRAME(twq_state)--;
         else
            toast_me=tweqHitEdge(twq_state,mds_cfg,FALSE);
      else
         if ((TWEQ_FRAME(twq_state)<MAX_TWEQ_MODELS-1)&&
             (mds_cfg->names[TWEQ_FRAME(twq_state)+1].text[0]!='\0'))
            TWEQ_FRAME(twq_state)++;
         else
            toast_me=tweqHitEdge(twq_state,mds_cfg,TRUE);
   }
   if (!tweq_halt_is_killtype(toast_me))
   {  // turn the name into a model number, set the property, all here
      int misc_flag = mds_cfg->cfg.flg_misc;
      if (misc_flag&(TWEQ_MC_ANCHOR | TWEQ_MC_VHOT))
      {
         mxs_vector init_anchor;
         get_anchor(obj,misc_flag,&init_anchor);
         ObjSetModelName(obj,mds_cfg->names[TWEQ_FRAME(twq_state)].text);
         finalize_anchor(obj,misc_flag,&init_anchor);
      }
      else
         ObjSetModelName(obj,mds_cfg->names[TWEQ_FRAME(twq_state)].text);
   }
   return toast_me;
}

//////////
// emitter

struct sLaunchData 
{
   ObjID obj;
   ObjID arch; 
   int flags; 
   mxs_vector vec; 
}; 

typedef cSimpleDList<sLaunchData> cLaunchList; 

static cLaunchList gLaunchList; 

static ITweqEmitterProperty *emit_cfg_prop;
int processTweqEmitter(ObjID obj, void* state, int ms)
{
   sTweqSimpleState *twq_state = (sTweqSimpleState*)state;
   sTweqEmitterConfig *emit_cfg;

   int toast_me=TWEQ_STATUS_QUO;
   if (!PROPERTY_GET(emit_cfg_prop,obj,&emit_cfg))
   {
      Warning(("Cant get EmitterCfg for obj %s\n",ObjEditName(obj)));         
      return toast_me;
   }

   if (!tweq_process_needed(&emit_cfg->cfg,obj)) return toast_me;

   addTime(&twq_state->tm_info.cur_time,ms,&emit_cfg->cfg);  //need to pass jitter/curve flags in here!
   while ((toast_me==TWEQ_STATUS_QUO)&&(twq_state->tm_info.cur_time>TWEQ_DURATION(&emit_cfg->cfg)))
   {
      twq_state->tm_info.cur_time-=TWEQ_DURATION(&emit_cfg->cfg);
      if ((emit_cfg->cfg.flg_anim&TWEQ_AC_NOLIMIT)==0) // we have limits
         if (twq_state->tm_info.cur_frame>=emit_cfg->max_frames)  // time to die
            toast_me=tweq_test_haltkill(&emit_cfg->cfg);
      if (toast_me==TWEQ_STATUS_QUO)
      {
         ObjID our_arch=IObjectSystem_GetObjectNamed(cur_osys,emit_cfg->emittee.text);
         mxs_vector launchVec=emit_cfg->vel;
         int flags=0;
         
         twq_state->tm_info.cur_frame++;
         if (our_arch==OBJ_NULL)
            continue;  // no point in getting the null projectile ready, that is for sure
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_RANDOM)
         {  // perterb launchVec here
            launchVec.el[0]=frand_hack();
            launchVec.el[1]=frand_hack();
            launchVec.el[2]=frand_hack();
            mx_scaleeq_vec(&launchVec,emit_cfg->vel.el[0]);
         }
         else if (emit_cfg->cfg.flg_curve&TWEQ_CC_JITTER)
         {  // this is just not the best way...
            float fac=(emit_cfg->cfg.flg_curve&TWEQ_CC_JITTER)/3.0, mag;   // should use angular displacements!!! yes!!!
            mxs_vector ran;  // in msvc, you could do this in declaration
            int i;
            
            for (i=0; i<3; i++) ran.el[i]=frand_hack();
            mag=mx_mag_vec(&launchVec);
            mx_scale_addeq_vec(&launchVec,&ran,mag*fac);
         }
         if ((emit_cfg->rand.el[0]!=0) || (emit_cfg->rand.el[1]!=0) || (emit_cfg->rand.el[2]!=0))
         {
            // apply random angular deviation for launch velocity
            mxs_angvec fac;
            mxs_vector temp;
            mxs_matrix rotMat;
            
            for (int i=0; i<3; i++)
               fac.el[i] = ushort((float(2*Rand())/32767.0)*emit_cfg->rand.el[i])-emit_cfg->rand.el[i];
            mx_ang2mat(&rotMat, &fac);   
            mx_mat_mul_vec(&temp, &rotMat, &launchVec);
            mx_copy_vec(&launchVec, &temp);
         }
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_RELVEL)
         {
            // make launch velocity object relative
            mxs_angvec fac = ObjPosGet(obj)->fac;
            mxs_vector temp;
            mxs_matrix rotMat;
            
            mx_ang2mat(&rotMat, &fac);   
            mx_mat_mul_vec(&temp, &rotMat, &launchVec);
            mx_copy_vec(&launchVec, &temp);
         }
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_GRAV)    flags|=PRJ_FLG_GRAVITY;
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_ZEROVEL) flags|=PRJ_FLG_ZEROVEL;
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_TELLAI)  flags|=PRJ_FLG_TELLAI;
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_PUSHOUT) flags|=PRJ_FLG_PUSHOUT;
         if (emit_cfg->cfg.flg_misc&TWEQ_MC_NOPHYS)  flags|=PRJ_FLG_NOPHYS;
         sLaunchData launch = { obj, our_arch, flags, launchVec.x, launchVec.y,launchVec.z }; 
         gLaunchList.Append(launch); 
      }
   }
   // @HACK: This is a total horror of a hack, but needed for Shock. What
   // we *really* want to do is detect HALT_STOP down in actOnObj for *any*
   // tweq, and broadcast that. However, it really isn't clear to me how
   // the heck to get my hands on all the relevant info down there, like
   // something telling me what tweq I'm actually dealing with. So for now,
   // we'll solve the immediate problem, and cogitate further on how this
   // really should work, as part of the general issue of tweq networking.
   if (toast_me == TWEQ_HALT_STOP)
      BroadcastTweqEmitStop(obj);

   return toast_me;
}

static void do_launches()
{
   cLaunchList::cIter iter = gLaunchList.Iter(); 
   for (; !iter.Done(); iter.Next())
   {
      sLaunchData& l = iter.Value(); 
      launchProjectile(l.obj,l.arch,1.0,l.flags,&l.vec,NULL,NULL);
      gLaunchList.Delete(iter.Node()); 
   }
}

///////////
// delete

int processTweqDelete(ObjID obj, void* state, int ms)
{
   sTweqSimpleState *twq_state = (sTweqSimpleState*)state;
   sTweqSimpleConfig *twq_cfg;
   if (!PROPERTY_GET(tweq_delete_prop,obj,&twq_cfg))
   {
      Warning(("Cant get DeleteCfg for obj %s\n",ObjEditName(obj)));
      return TWEQ_STATUS_QUO;
   }
   if (!tweq_process_needed(&twq_cfg->cfg,obj)) return TWEQ_STATUS_QUO;
   addTime(&twq_state->tm_info.cur_time,ms,&twq_cfg->cfg);  //need to pass jitter/curve flags in here!

#ifdef NEW_NETWORK_ENABLED
   // We really only want to run tweqdelete on an object's host. However,
   // we do want to keep processing the timers, in case we get handed
   // the object.
   if (g_pNetMan->Networking() && g_pObjNet->ObjIsProxy(obj))
   {
      return TWEQ_STATUS_QUO;
   }
#endif

   if (twq_state->tm_info.cur_time>twq_cfg->cfg.rate)
   {
      int halt_act=tweq_test_haltkill(&twq_cfg->cfg);
      if (tweq_halt_is_killtype(halt_act))
         return halt_act;
      Warning(("Delete tweq with non fatal halt - fatalizing\n"));
      return TWEQ_HALT_KILL;
   }
   return TWEQ_STATUS_QUO;
}

///////////
// flicker

int processTweqFlicker(ObjID obj, void* state, int ms)
{
   sTweqSimpleState *twq_state = (sTweqSimpleState*)state;
   sTweqSimpleConfig *twq_cfg;
   if (!PROPERTY_GET(tweq_flicker_prop,obj,&twq_cfg))
   {
      Warning(("Cant get FlickerCfg for obj %s\n",ObjEditName(obj)));
      return TWEQ_STATUS_QUO;
   }
   if (!tweq_process_needed(&twq_cfg->cfg,obj)) return TWEQ_STATUS_QUO;
   addTime(&twq_state->tm_info.cur_time,ms,&twq_cfg->cfg);  //need to pass jitter/curve flags in here!
   if (twq_state->tm_info.cur_time>twq_cfg->cfg.rate)
   {  // turn the flicker to opposite state!
      twq_state->tm_info.cur_time=0;
      if ((twq_cfg->cfg.flg_anim&TWEQ_AC_NOLIMIT)==0) // we have limits
         if ((--twq_state->tm_info.cur_frame)==0)
            return tweq_test_haltkill(&twq_cfg->cfg);
      return TWEQ_FRAME_EVENT;
   }
   return TWEQ_STATUS_QUO;
}

//////////////
// frame update

// cur_osys, the current IObjSys, is up top
static IProperty     *cur_prop;
static int            cur_ms;
static enum eTweqType cur_type;

// hmmm... maybe this should be a #define
// especially as we want the flags field to know if we are a sim object, and should filter on were
//   we drawn in the world... hmmm....
static BOOL actOnObj(ObjID obj, sTweqBaseState *state, int (*process_call)(ObjID obj, void *data, int ms))
{
   BOOL remains=TRUE; // is the obj still around
   
   if (!OBJ_IS_ABSTRACT(obj))
      if (state->flg_anim&TWEQ_AS_ONOFF)   // if we are off, dont run
      {
         int rv=(*process_call)(obj,state,cur_ms);
         if (rv!=TWEQ_STATUS_QUO)
         {
            if (rv==TWEQ_HALT_STOP)
            {  // Clean up flags - BUG - need to deal with vector property hell
               state->flg_anim&=~TWEQ_AS_ONOFF;
            }
            if (cur_type != kTweqTypeNull)
               SendTweqMessage(obj,cur_type,(enum eTweqOperation)rv,
                            (state->flg_anim&TWEQ_AS_REVERSE)?kTweqDirForward:kTweqDirReverse);
            switch (rv)  // someday, the send might change this, or something, i think
            {
               case TWEQ_HALT_KILL:   cur_osys->Destroy(obj); remains=FALSE; break;
               case TWEQ_HALT_SLAY:   cur_dmgmodel->SlayObject(obj,OBJ_NULL); remains=FALSE; break;
               case TWEQ_HALT_REM:    cur_prop->Delete(obj); remains=FALSE; break;
               case TWEQ_HALT_STOP:   break;  // already dealt with
               case TWEQ_STATUS_QUO:  break;  // @TBD:script might have set it back...
               case TWEQ_FRAME_EVENT: break;  // just a frame boundary
            }
         }
      }
   return remains;
}

// do the thing!
void RunAllTweqs(int ms)
{
   sTweqVectorState  *tweq_vector;  // for rotate and scale
   sTweqJointsState  *tweq_joints;
   sTweqSimpleState  *tweq_simple;  // for emit and delete
   sTweqLockState    *tweq_lock;
   sPropertyObjIter   iter;
   ObjID              obj;
   int                i;

   if (!IsSimTimePassing()||(ms==0))  // nothing will happen, so dont bother
      return;

   // get the activity center vector, for simrad tweqs
   CameraGetLocation(PlayerCamera(),&cur_activity_center_vec,NULL);

   cur_osys=AppGetObj(IObjectSystem);
   cur_dmgmodel=AppGetObj(IDamageModel);
   cur_ms=ms;

   cur_osys->Lock(); // we will queue object launches, then do them after all processing

   cur_type=kTweqTypeScale;
   cur_prop=(IProperty *)tweq_scalestate_prop;   // Scale 
   IProperty_IterStart(tweq_scalestate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_scalestate_prop,&iter,&obj,&tweq_vector))
      if (actOnObj(obj,&tweq_vector->state,processTweqScale))
         tweq_scalestate_prop->TouchValue(obj,tweq_vector);
   IProperty_IterStop(tweq_scalestate_prop,&iter);
   
   cur_type=kTweqTypeRotate;
   cur_prop=(IProperty *)tweq_rotatestate_prop;  // Rotate
   IProperty_IterStart(tweq_rotatestate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_rotatestate_prop,&iter,&obj,&tweq_vector))
      if (actOnObj(obj,&tweq_vector->state,processTweqRotate))
         tweq_rotatestate_prop->TouchValue(obj,tweq_vector);   
   IProperty_IterStop(tweq_rotatestate_prop,&iter);

   cur_type=kTweqTypeJoints;
   cur_prop=(IProperty *)tweq_jointsstate_prop;  // Joints
   IProperty_IterStart(tweq_jointsstate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_jointsstate_prop,&iter,&obj,&tweq_joints))
      if (actOnObj(obj,&tweq_joints->state,processTweqJoints))
         tweq_jointsstate_prop->TouchValue(obj,tweq_joints);
   IProperty_IterStop(tweq_jointsstate_prop,&iter);           

   cur_type=kTweqTypeModels;   
   cur_prop=(IProperty *)tweq_modelsstate_prop;  // models
   IProperty_IterStart(tweq_modelsstate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_modelsstate_prop,&iter,&obj,&tweq_simple))
      if (actOnObj(obj,&tweq_simple->state,processTweqModels))
         tweq_modelsstate_prop->TouchValue(obj,tweq_simple);
   IProperty_IterStop(tweq_modelsstate_prop,&iter);

   cur_type=kTweqTypeDelete;   
   cur_prop=(IProperty *)tweq_deletestate_prop;  // delete
   IProperty_IterStart(tweq_deletestate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_deletestate_prop,&iter,&obj,&tweq_simple))
      if (actOnObj(obj,&tweq_simple->state,processTweqDelete))
         tweq_deletestate_prop->TouchValue(obj,tweq_simple);
   IProperty_IterStop(tweq_deletestate_prop,&iter);


   cur_type=kTweqTypeFlicker;
   cur_prop=(IProperty *)tweq_flickerstate_prop; // flicker
   IProperty_IterStart(tweq_flickerstate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_flickerstate_prop,&iter,&obj,&tweq_simple))
      if (actOnObj(obj,&tweq_simple->state,processTweqFlicker))
         tweq_flickerstate_prop->TouchValue(obj,tweq_simple);
   IProperty_IterStop(tweq_flickerstate_prop,&iter);

   cur_type=kTweqTypeLock;
   cur_prop=(IProperty *)tweq_lockstate_prop; // lock
   IProperty_IterStart(tweq_lockstate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_lockstate_prop,&iter,&obj,&tweq_lock))
      if (actOnObj(obj,&tweq_lock->state,processTweqLock))
         tweq_lockstate_prop->TouchValue(obj,tweq_lock);
   IProperty_IterStop(tweq_lockstate_prop,&iter);


   cur_type=kTweqTypeEmitter;   
   cur_prop=(IProperty *)tweq_emitterstate_prop; // emitter
   emit_cfg_prop = tweq_emitter_prop;
   IProperty_IterStart(tweq_emitterstate_prop,&iter);
   while (PROPERTY_ITER_NEXT_VALUE(tweq_emitterstate_prop,&iter,&obj,&tweq_simple))
      if (actOnObj(obj,&tweq_simple->state,processTweqEmitter))
         tweq_emitterstate_prop->TouchValue(obj,tweq_simple);   
   IProperty_IterStop(tweq_emitterstate_prop,&iter);

   for (i=0; i < NUM_EXTRA_EMITTERS; ++i)
   {
      cur_type=kTweqTypeNull;
      cur_prop=(IProperty *)tweq_extra_emitterstate_prop[i]; // extra emitters
      emit_cfg_prop = tweq_extra_emitter_prop[i];
      IProperty_IterStart(cur_prop,&iter);
      while (PROPERTY_ITER_NEXT_VALUE(tweq_extra_emitterstate_prop[i],
                   &iter,&obj,&tweq_simple))
         if (actOnObj(obj,&tweq_simple->state,processTweqEmitter))
            tweq_extra_emitterstate_prop[i]->TouchValue(obj,tweq_simple);   
      IProperty_IterStop(cur_prop,&iter);
   }

   do_launches(); 
   cur_osys->Unlock(); 

   SafeRelease(cur_osys);
   SafeRelease(cur_dmgmodel);
}

/////////////
// setup/closedown
void SetupAllTweqs(void)
{
   TweqPropsInit();  // hey, now the secret globals are set

#ifdef EDITOR
   tweqctrlCommandRegister(); //fix_my_locks.
#endif

#ifdef NEW_NETWORK_ENABLED
   // Set up the horrible hack network message:
   g_pStopTweqEmitMsg = new cNetMsg(&sStopTweqEmitDesc);
   // Save out the network managers, so we don't need to AutoAppIPtr
   // all the time:
   g_pNetMan = AppGetObj(INetManager);
   g_pObjNet = AppGetObj(IObjectNetworking);
#endif
}

void TerminateTweqs(void)
{
   TweqPropsTerm();
#ifdef NEW_NETWORK_ENABLED
   delete g_pStopTweqEmitMsg;
   SafeRelease(g_pNetMan);
   SafeRelease(g_pObjNet);
#endif
}


