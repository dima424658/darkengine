/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/twqscrpt.cpp,v 1.11 1999/05/27 17:25:18 Justin Exp $
// 

#include <string.h>

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <propstor.h>

// for script service/persistance
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

// tweq itself
#include <tweqbase.h>
#include <tweqctrl.h>
#include <twqscrpt.h>
#include <tweqenum.h>
#include <tweqexp.h>
#include <twqprop_.h>

// for a/r reaction interface
#include <reaction.h>
#include <propreac.h>
#include <reacbase.h>

#include <dbmem.h>

IMPLEMENT_SCRMSG_PERSISTENT(sTweqMsg)
{
   PersistenceHeader(sScrMsg,kTweqMsgVer);
   PersistentEnum(Op);
   PersistentEnum(Dir);
   PersistentEnum(Type);
   return TRUE;
}

// convienience function, since tweqctrl isnt CPP
void SendTweqMessage(ObjID to, eTweqType type, eTweqOperation op, eTweqDirection dir)
{
   AutoAppIPtr_(ScriptMan,pScriptMan);
   sTweqMsg*  tweqmsg = new sTweqMsg(to,type,op,dir);
   pScriptMan->PostMessage(tweqmsg);
   SafeRelease(tweqmsg); 
}

//------------------------------------------------------------
// TweqControl Reaction
//

// modify the state property in place from action and bits
static void _do_modify_state(sTweqBaseState *state, eTweqDo action, int xtra_bits)
{
   BOOL change=TRUE;
   short bits=state->flg_anim;
   switch (action)
   {
      case kTweqDoDefault:
         if (bits&TWEQ_AS_ONOFF) // it is on
            if (bits&TWEQ_AS_REVERSE)
               bits&=~TWEQ_AS_REVERSE;
            else
               bits|=TWEQ_AS_REVERSE;
         else
            bits|=TWEQ_AS_ONOFF;
         break;
      case kTweqDoContinue:
         if (bits&TWEQ_AS_ONOFF) // it is on
            ; // ignore it, in motion
         else
            bits|=TWEQ_AS_ONOFF;
         break;
      case kTweqDoActivate:
         if (bits&TWEQ_AS_ONOFF)
            change=FALSE;
         else
            bits|=TWEQ_AS_ONOFF;
         break;
      case kTweqDoHalt:
         if (bits&TWEQ_AS_ONOFF)
            bits&=~TWEQ_AS_ONOFF;
         else
            change=FALSE;
         break;
      case kTweqDoReset:
         bits&=~TWEQ_AS_ONOFF;
         bits|=TWEQ_AS_RESYNCH|TWEQ_AS_GOEDGE;
         break;
      case kTweqDoForward:
         bits|=TWEQ_AS_ONOFF;
         bits&=~TWEQ_AS_REVERSE;
         break;
      case kTweqDoReverse:
         bits|=TWEQ_AS_ONOFF|TWEQ_AS_REVERSE;
         break;
   }
   bits|=xtra_bits;
   state->flg_anim=bits;
}

// get the right state property - prepare to do the the work of the tweq here...
static void _do_generic_props(ObjID obj, IProperty *prop, eTweqDo action, int xtra_bits)
{
   sTweqBaseState *state;
   IPropertyStore *store;
   Verify(SUCCEEDED(prop->QueryInterface(IID_IPropertyStore,(void **)&store)));
   sDatum dat; 
   if (!store->Get(obj,&dat))
   {
      prop->Create(obj);
      if (!store->Get(obj,&dat))
         return;
   }

   state=(sTweqBaseState *)dat.value;
   _do_modify_state(state,action,xtra_bits);
   store->Set(obj,dat);
   prop->Touch(obj);  // call listeners
}

// the joint properties
static void _do_joints_props(ObjID obj, IProperty *prop, eTweqDo action, int xtra_bits, int xtra_parm)
{
   ITweqJointsStateProperty *jprop=(ITweqJointsStateProperty *)prop;
   sTweqJointsConfig *jcfg;
   sTweqJointsState *jstate;
   sTweqBaseState *bstate;
   BOOL prop_found=TRUE;
   int primary;
   
   AssertMsg(jprop==tweq_jointsstate_prop,"JointProp not finding itself..");
   if (!jprop->Get(obj,&jstate))
      prop_found=FALSE;
   if (tweq_joints_prop->Get(obj,&jcfg))
   {
      if (!prop_found)
      {
         jstate=(sTweqJointsState *)malloc(sizeof(sTweqJointsState));
         memset(jstate,0,sizeof(sTweqJointsState));
      }
      primary=jcfg->primary_joint-1;
      if (primary>=MAX_TWEQ_JOINTS) primary=0;
      if (primary==-1)  // special master control, not clear we want/need it...
         bstate=&jstate->state;
      else
         bstate=&jstate->jnt_flgs[primary];
      _do_modify_state(bstate,action,xtra_bits);
      jprop->Set(obj,jstate);
      if (!prop_found)
         free(jstate);
   }
   else
      Warning(("TweqReact for %d joint prop not found\n",obj));
}

// for the "generic" type, ie. find something to do and do it
static void _do_all_props(ObjID obj, eTweqDo action, int xtra_bits, int xtra_parm)
{
   int cur_type;
   for (cur_type=kTweqTypeScale; cur_type<=kTweqTypeFlicker; cur_type++)
   {  // cycle through each property
      IProperty *prop=tweq_prop_states[cur_type];
      IProperty *cfg_prop=tweq_prop_config[cur_type];
      if (cfg_prop->IsRelevant(obj))
         _do_generic_props(obj,prop,action,xtra_bits);
   }
}

// base tweq control setup
static eReactionResult LGAPI tweq_control_func(sReactionEvent* event, 
                                               const sReactionParam* param, 
                                               tReactionFuncData )
{
   sTweqControl *tweqc = (sTweqControl*)&param->data;
   ObjID obj = param->obj[kReactDirectObj];
   IProperty *prop=NULL;
   IProperty *cfg_prop=NULL;

   if (obj == OBJ_NULL) obj = event->sensor_obj;
   // YO! watcom sucks, since Scale==0. types is an enum (uint)
   //   it warns that uints are never not >= 0... however, id kinda like
   //   the test, in case the enum moves around... so, here we are, woo woo
   if ((((int)tweqc->type)>=kTweqTypeScale)&&(tweqc->type<=kTweqTypeFlicker))
   {
      prop=tweq_prop_states[tweqc->type];
      cfg_prop=tweq_prop_config[tweqc->type];
      if (!cfg_prop->IsRelevant(obj))
      {
         Warning(("No TweqConfig for tweq %d\n",tweqc->type));
         return kReactionNormal;
      }
   }
   if (prop==NULL)
      _do_all_props(obj,tweqc->action,tweqc->xtra_bits,tweqc->xtra_parm);
   else if ((tweqc->type==kTweqTypeJoints)&&(tweqc->xtra_parm>0))
      _do_joints_props(obj,prop,tweqc->action,tweqc->xtra_bits,tweqc->xtra_parm);
   else
      _do_generic_props(obj,prop,tweqc->action,tweqc->xtra_bits);
   return kReactionNormal;
}

void TweqProcessAll(ObjID obj, eTweqDo action)
{
   _do_all_props(obj,action,0,0);
}

static sReactionDesc tweq_control_rdesc =
{
   REACTION_TWEQ_CONTROL,
   "Tweq Control",
   REACTION_PARAM_TYPE(sTweqControl), 
   kReactionHasDirectObj|kReactionWorkOnProxy,
};

ReactionID create_tweq_control_reaction(void)
{
   AutoAppIPtr(Reactions);
   return pReactions->Add(&tweq_control_rdesc,tweq_control_func,NULL);
}
