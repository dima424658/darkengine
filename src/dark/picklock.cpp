/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/picklock.cpp,v 1.22 2000/01/29 20:22:07 adurant Exp $
// lockpick service, properties, the whole thing...

#include <lg.h>
#include <mprintf.h>
#include <cfgdbg.h>
#include <stdlib.h>

// link stuff
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

// property stuff
#include <property.h>
#include <propface.h>
#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>

// sdesc fun
#include <sdesc.h>
#include <sdesbase.h>

////////////
// headers for this system

// pick stuff
#include <pickbase.h>
#include <picklock.h>


//scripts
#include <pickscrm.h>
#include <scrptapi.h>

// player handler
#include <plyrhdlr.h>
#include <drkcret.h>
#include <plycbllm.h>

#include <playrobj.h>

// sound stuff
#include <schbase.h>
#include <schema.h>
#include <esnd.h>

// locks
#include <lockprop.h>
#include <tweqctrl.h> // for LockAnim
#include <rand.h>

#include <drkstats.h>

// report stuff
#include <report.h>
#ifdef REPORT
#include <drkrphlp.h>
#include <objedit.h>
#endif

// Must be last header
#include <dbmem.h>

// the properties we use
static IIntProperty       *pPickSrcProp=NULL;
static IPickCfgProperty   *pPickCfgProp=NULL;
static IAdvPickTransCfgProperty *pAdvPickTransCfgProp=NULL;
static IAdvPickStateCfgProperty *pAdvPickStateCfgProp=NULL;
static IAdvPickSoundCfgProperty *pAdvPickSoundCfgProp=NULL;
static IPickStateProperty *pPickStateProp=NULL;
static IRelation          *pPickStateRel=NULL;

// horrible sound hack horror
static int schema_idx=0;

F_DECLARE_INTERFACE(IScriptMan);

//////////////
// actual thinking about the situation

BOOL _IsAdvancedLock(ObjID lock)
{
  sAdvPickStateCfg *pPSC;

  if (!pAdvPickStateCfgProp->Get(lock,&pPSC))
    return FALSE;
  return pPSC->enable;
}

int _LockTransitionDestination(int PickBits, int curstate, ObjID lock)
{
  sAdvPickStateCfg *pPSC;
  sAdvPickTransCfg *pPST;
  int deststate;

  deststate = -1;
  
  if (!pAdvPickTransCfgProp->Get(lock,&pPST))
    return deststate;

  if (!pAdvPickStateCfgProp->Get(lock,&pPSC))
    return deststate;
  
  if (pPSC->firstpickbits&PickBits)
    deststate = pPST->picktransfsm[curstate].firstpickstate;

  if (pPSC->secondpickbits&PickBits)
    deststate = pPST->picktransfsm[curstate].secondpickstate;

  if (pPSC->thirdpickbits&PickBits)
    deststate = pPST->picktransfsm[curstate].thirdpickstate;

  if (deststate == curstate)
    deststate = -1; //no self loops!

  if ((deststate < 0) || (deststate >= NUMPICKFSMSTATES))
    deststate = -1; //bounds check

  ConfigSpew("PickTrack",("Lock %d state check %d to %d\n",lock,curstate,deststate));

  return deststate;

}

void ResetPickState(ObjID lock)
{
  sPickState locPS, *pPS;
  //if we have a pick state, reset it.  woohoo
  if (pPickStateProp->Get(lock,&pPS))
    {
      memset(&locPS,0,sizeof(sPickState));
      pPS=&locPS;
      ConfigSpew("PickTrack",("Resetting pick state on %d\n",lock));
      pPickStateProp->Set(lock,pPS);
    }
}

void _BuildInitialPickState(ObjID picker, ObjID pick, ObjID lock)
{
   sPickState locPS, *pPS;

   pPickStateRel->AddFull(pick,lock,(void *)&picker);
   if (!pPickStateProp->Get(lock,&pPS))
   {
      memset(&locPS,0,sizeof(sPickState));
      pPS=&locPS;
      //if this really is a new prop, set this up...
      if (_IsAdvancedLock(lock))
	if (ObjComputeLockedState(lock))
	  pPS->cur_tumbler=0; //set state to 0;
	else
	  pPS->cur_tumbler=8; //unlocked, so make sure unlocked.
      ConfigSpew("PickTrack",("Clean slate state\n",lock));
   } // might need an else "do the right thing" kind of thing
   pPS->picker=picker;    // for now, sure
   if (_IsAdvancedLock(lock))
     pPS->tm_stage = 0; //hey, we clearly had stopped picking, so lets
                        //reset time to 0.
   pPickStateProp->Set(lock,pPS);
   ConfigSpew("PickTrack",("Building Initial Pick State for %d\n",lock));
}

ePickResult _PickCheckThisState(ObjID pick, ObjID lock, sPickState *pPSt, sPickCfg *pPCfg)
{
   int PickBits;
   if (pPSt->done) return kPickComplete;
   if (!pPickSrcProp->Get(pick,&PickBits))
   {
      Warning(("Pick %d has no pickSrc\n",pick));
      return kPickNoMatch;
   }
   if (!_IsAdvancedLock(lock))  //not advanced lock, do old thing
     {
       if ((pPSt->cur_tumbler>=NUM_TUMBLERS)||(pPSt->cur_tumbler<0))
	 return kPickNoMatch;
       sPickStage *this_tumbler=&pPCfg->tumbler[pPSt->cur_tumbler];
       if (this_tumbler->data_bits==0) // no tumblers left
	 return kPickComplete;
       if (this_tumbler->data_bits&PickBits)
	 return kPickMatch;
       return kPickNoMatch;
     }
   else
     {
       int curstate;
       curstate = pPSt->cur_tumbler;
       if ((curstate>=NUMPICKFSMSTATES)||(curstate<0))
	 return kPickNoMatch;
       if (_LockTransitionDestination(PickBits,curstate,lock) == -1)
	 return kPickNoMatch;
       return kPickMatch;
     }
}

static int _PickGetVolPct(sPickState *pPSt, sPickCfg *pPCfg)
{
   int total, cur, i;
   
   for (i=0; i<NUM_TUMBLERS; i++)
   {
      total+=pPCfg->tumbler[i].pins;
      if (i<pPSt->cur_tumbler)
         cur+=pPCfg->tumbler[i].pins;
   }
   cur+=pPSt->pin;
   return 100*cur/total;
}

// pass 0 to mean default volume (current schemas are locked,lockpik,pinset,noluck,unlocked)
static int _PickSound(const char *schName, ObjID snd_loc, int vol_pct)
{
   sSchemaCallParams callDat={SCH_SET_OBJ,};
   if (vol_pct) 
    { callDat.flags|=SCH_SCALE_VOLUME; callDat.volume=vol_pct; }
   callDat.sourceID=snd_loc;
   return SchemaPlay((Label *)schName,&callDat);
}

// ok, now we try the advanced picking for player notification of good/bad
static int _AdvPickSound(ObjID snd_loc, ObjID picker, int vol_pct, char* locktags)
{
  cTagSet TagSet(locktags);

  sSchemaCallParams callDat={SCH_SET_OBJ,};
  callDat.sourceID=snd_loc;
  int iHandle=SCH_HANDLE_NULL;
  iHandle = ESndPlayObj(&TagSet,snd_loc,picker,&callDat);
  if (iHandle != SCH_HANDLE_NULL)
  {
//    return SchemaIDPlay(SchemaGetIDFromHandle(iHandle),&callDat);
  }
  return 0;
}

static int _AdvSoundProgress(ObjID lock, ObjID pick, ObjID picker, sPickState *pPS)
{
  int PickBits;
  int curstate = pPS->cur_tumbler;
  if (!pPickSrcProp->Get(pick,&PickBits))
    {
      Warning(("Trying to play sound, but pick %d has no picksrc!\n",pick));
      return 0;
    }
  sAdvPickSoundCfg *pPSS;
  sAdvPickStateCfg *pPSC;
  char schematags[80];

  if (!pAdvPickSoundCfgProp->Get(lock,&pPSS))
    return 0;
  if (!pAdvPickStateCfgProp->Get(lock,&pPSC))
    return 0;
  
  if (pPSC->firstpickbits&PickBits)
    sprintf(schematags,"Event StateChange, LockState %s",pPSS->picksoundfsm[curstate].firstpicksound);

  if (pPSC->secondpickbits&PickBits)
    sprintf(schematags,"Event StateChange, LockState %s",pPSS->picksoundfsm[curstate].secondpicksound);

  if (pPSC->thirdpickbits&PickBits)
    sprintf(schematags,"Event StateChange, LockState %s",pPSS->picksoundfsm[curstate].thirdpicksound);

  return _AdvPickSound(lock,picker,0,schematags);

}

// duh
static int _PickGetTumblerCount(sPickCfg *pPCfg)
{
   int i;
   for (i=0; i<NUM_TUMBLERS; i++)
      if (pPCfg->tumbler[i].data_bits==0)
         break;
   return i;
}

// set the visual state of the lock - stage means you just hit it, so go straight there
static void SetLockVisuals(sPickState *pPSt, sPickCfg *pPCfg, ObjID lock, BOOL stage)
{
   if (!stage)
      if ((pPSt->tm_stage&0xff)<0x7f)
         return;  // only set new target 1/2thish of the time

   float per_tumbler;
   float pin_cnt;
   float pin_pos;
   float use_pos;

   ConfigSpew("PickView",("Setting Lock Visuals on %d\n",lock));

   if (!_IsAdvancedLock(lock))
     {
       per_tumbler=1.0/(float)_PickGetTumblerCount(pPCfg);
       pin_cnt=(float)pPCfg->tumbler[pPSt->cur_tumbler].pins;
       pin_pos=(float)pPSt->pin/pin_cnt;
       use_pos=(pPSt->cur_tumbler+(pin_pos*0.77))*per_tumbler;
     }
   else
     {
       per_tumbler=1.0/(float)(NUMPICKFSMSTATES-1);
       pin_cnt=1; //really only 1 "pin" per stat0e
       pin_pos=0; //"really only one "pin" per state, so 0 or 1.
       use_pos=(pPSt->cur_tumbler)*per_tumbler;
     }	 
   // want to make this a bit bigger - hmmmm
   if (!stage)
      use_pos+=((Rand()&0x7f)*(1.0/(1.7*pin_cnt*128.0)))*per_tumbler;
   LockAnimSetTarget(lock,use_pos);
 }

BOOL _AdvLockUpdateProgress(sPickState *pPS, sPickCfg *pPCfg, ObjID lock, ObjID pick, ObjID snd_loc, ObjID picker, ulong dt = 0)
{
  sAdvPickStateCfg *pPSC;
  sAdvPickTransCfg *pPST;
  sAdvPickSoundCfg *pPSS;

  if (!pAdvPickTransCfgProp->Get(lock,&pPST))
    {
      Warning(("Lock %d shows up as advanced but has no TransCfg\n",pick));
      return FALSE;
    }
  if (!pAdvPickStateCfgProp->Get(lock,&pPSC))
    {
      Warning(("Lock %d shows up as advanced but has no StateCfg\n",pick));
      return FALSE;
    }
  if (!pAdvPickSoundCfgProp->Get(lock,&pPSS))
    {
      Warning(("Lock %d shows up as advanced but has no SoundCfg\n",pick));
      return FALSE;
    }


  int curstate = pPS->cur_tumbler;
  int state_time = pPSC->pickstatefsm[curstate].time;//time(in ms) to change
  ConfigSpew("PickTiming",("Lock %d has stage time %d and state time %d\n",lock,pPS->tm_stage,state_time));

  //if we've just crossed 1/3rd of the way.  
  if ((pPS->tm_stage>(state_time/3)) && ((pPS->tm_stage-dt)<(state_time/3)))
    _AdvSoundProgress(lock,pick,picker,pPS);

  if (pPS->tm_stage>state_time) // hey, state change!
    {
      ConfigSpew("PickTrack",("State changing on lock %d\n",lock));
      pPS->tm_stage=0; //reset time
      int PickBits;
      if (!pPickSrcProp->Get(pick,&PickBits))
	{
	  Warning(("Pick %d has no pickSrc, but we were picking!\n",pick));
	  return FALSE;
	}
      int nextstate = _LockTransitionDestination(PickBits,curstate,lock);
      if (nextstate == -1)
	{
	  Warning(("Next state on lock %d is -1.  Current state is %d\n",nextstate,curstate));
	  return FALSE;
	}
      pPS->cur_tumbler=nextstate;
      _PickSound("tumblerset",snd_loc,0);

      //send script message
      
      IScriptMan* mpScriptMan = AppGetObj(IScriptMan);
      if (mpScriptMan != NULL)
	{
	  sPickStateScrMsg* scrmsg = new sPickStateScrMsg(lock,curstate,nextstate);
	  mpScriptMan->SendMessage(scrmsg);
	  scrmsg->Release();
	}
      SafeRelease(mpScriptMan);

      if (pPS->cur_tumbler==(NUMPICKFSMSTATES-1))
	{
	  _PickSound("unlocked",snd_loc,0);
	  pPS->done = TRUE; //reached final state.
	  DarkStatInc(kDarkStatLockPick);
	}
      SetLockVisuals(pPS,pPCfg,snd_loc,TRUE);
      PickLockFinishPicking(pick); //so you have to start again after each end of state.
      return pPS->done;
    }
  SetLockVisuals(pPS,pPCfg,snd_loc,FALSE);
  return FALSE;

}

BOOL _LockUpdateProgress(sPickState *pPSt, sPickCfg *pPCfg, ObjID snd_loc)
{  // default time will be 2s
   int stage_pct=100.0+pPCfg->tumbler[pPSt->cur_tumbler].time_pct;
   int stage_time=2*(stage_pct*(100.0+pPSt->tm_rand))/10.0;
   if (pPSt->tm_stage>stage_time)  // past the threshold
   {
      ConfigSpew("PickTiming",("Turn over at %d (%d) [stage time %d r %d]\n",pPSt->tm_stage,pPSt->tm_total,stage_time,pPSt->tm_rand));
      pPSt->tm_stage=0;
      if (pPCfg->tumbler[pPSt->cur_tumbler].flags&PICK_FLG_RANDOM)
         pPSt->tm_rand=(char)((Rand()&0x7f)-0x40);
      else
         pPSt->tm_rand=0;
      if (++pPSt->pin<pPCfg->tumbler[pPSt->cur_tumbler].pins)
      {  // finished pin, same tumbler
         _PickSound("pinset",snd_loc,0);
         ConfigSpew("PickTrack",("Now on tumbler %d Pin %d\n",pPSt->cur_tumbler,pPSt->pin));
      }
      else  // hey, we have gone past our tumbler
      {  // ok, new tumbler -  do animation here!
         _PickSound("tumblerset",snd_loc,0);  // for now, no tumbler sound
         pPSt->pin=0;
         ConfigSpew("PickTrack",("Move to tumbler %d Pin %d\n",pPSt->cur_tumbler+1,pPSt->pin));
         if ((++pPSt->cur_tumbler>=NUM_TUMBLERS)||
             (pPCfg->tumbler[pPSt->cur_tumbler].data_bits==0)) // no tumblers left
         {  // whole thing done
            _PickSound("unlocked",snd_loc,0);
            pPSt->done=TRUE;
            ConfigSpew("PickTrack",("Lock done\n"));
            DarkStatInc(kDarkStatLockPick);
         }
      }
      SetLockVisuals(pPSt,pPCfg,snd_loc,TRUE);
      return pPSt->done;
   }
   SetLockVisuals(pPSt,pPCfg,snd_loc,FALSE);
   return FALSE;
}

// returns whether we are done
ePickResult _DoPickOperation(ObjID picker, ObjID pick, ObjID lock, ulong dt)
{
   sPickState *pPSt;
   sPickCfg *pPCfg;   
   ePickResult ret=kPickNoData;

   // if obj somehow unlocked - hit the road
   if (!ObjSelfLocked(lock))
   {
      ConfigSpew("PickTrack",("Hmm. im picking a lock, and someone else opened it\n"));
      return kPickNoLock;  // odd
   }
   if (!pPickStateProp->Get(lock,&pPSt))
   {
      ConfigSpew("PickTrack",("Hmm. this lock (%d) has no pickState\n", lock));
      return ret;
   }
   if ((!pPickCfgProp->Get(lock,&pPCfg)) && (!_IsAdvancedLock(lock)))
   {
     //don't need pickcfg if advanced lock
      ConfigSpew("PickTrack",("Hmm. this lock (%d) has no pickCfg\n", lock));
      return ret;
   }
   if (picker!=pPSt->picker)    // hmmm what to do?
   {
      Warning(("New picker %d (old %d) for lock %d\n",picker,pPSt->picker,lock));
      pPSt->pin=0; pPSt->tm_stage=0; // reset to beginning of this tumbler
   }
   pPSt->tm_total+=dt;
   ret=_PickCheckThisState(pick,lock,pPSt,pPCfg);
   switch (ret)
   {
      case kPickNoMatch:   // do we reset? check the bit...
         ConfigSpew("PickTrack",("Pick doesnt match\n")); 
         _PickSound("noluck",lock,0);
	 if (!_IsAdvancedLock(lock))
	   {
	     if (pPCfg->tumbler[pPSt->cur_tumbler].flags&PICK_FLG_RESET)
	       {  // really only want to do this if we werent already picking
		 int PickBits;
		 pPSt->pin=0; pPSt->tm_stage=0; // reset time and pin
		 if (pPSt->cur_tumbler>0)
		   if (pPickSrcProp->Get(pick,&PickBits))
		     if (pPCfg->tumbler[pPSt->cur_tumbler-1].data_bits&PickBits)
		       {
			 ConfigSpew("PickTrack",("Wrong Pick... matches last, leaving tumbler alone\n"));
			 break;
		       }
		 pPSt->cur_tumbler=0;  // sad....
		 SetLockVisuals(pPSt,pPCfg,lock,TRUE);
		 // @TODO: sound here!?!
		 ConfigSpew("PickTrack",("Wrong Pick... reset to first tumbler\n"));
	       }
	   }
	 else
	   {
	     pPSt->tm_stage = 0; // reset time;
	     ConfigSpew("PickTrack",("Pick has no destination\n"));
	   }
         break;
      case kPickComplete:
         Warning(("Lock already complete?\n"));
         break;
      case kPickMatch:
         pPSt->tm_stage+=dt;
	 if (!_IsAdvancedLock(lock))
	   if (_LockUpdateProgress(pPSt,pPCfg,lock))
	     {
	       ObjSetSelfLocked(lock,FALSE);
	       ret=kPickComplete;
	     }
	   else; //lockupdateprogress
	 else
	   if (_AdvLockUpdateProgress(pPSt,pPCfg,lock,pick,lock,picker,dt))
	     {
	       if (pPSt->cur_tumbler == 0)  //done AND 0... relock it.
		 ObjSetSelfLocked(lock,TRUE); 
	       else
		 ObjSetSelfLocked(lock,FALSE);
	       ret=kPickComplete;
	     }
         break;
   }
   pPickStateProp->Set(lock,pPSt);    // hmmmmmm
   return ret;
}

//////////////
// service elements

ePickResult PickLockMatchLevel(ObjID pick, ObjID lock, int stage)
{
   sPickState *pPSt;
   sPickCfg *pPCfg;
   ePickResult ret;

   if (!ObjSelfLocked(lock))
   {
      Warning(("Hmm. im picking a lock, and someone else opened it\n"));
      return kPickAlreadyUnlocked;  // odd
   }
   if (!pPickStateProp->Get(lock,&pPSt))
   {
      Warning(("Hmm. this lock (%d) has no pickState\n", lock));
      return kPickNoData;
   }
   if (!pPickCfgProp->Get(lock,&pPCfg))
   {
      Warning(("Hmm. this lock (%d) has no pickCfg\n", lock));
      return kPickNoData;
   }
   ret = _PickCheckThisState(pick,lock,pPSt,pPCfg);
   return ret;
}

static void PickLockClearState(ObjID pick)
{
#ifdef WARN_ON
   int cnt=0;
#endif
   ILinkQuery *pickLinks=pPickStateRel->Query(pick,LINKOBJ_WILDCARD);
   for (; !pickLinks->Done(); pickLinks->Next())
   {
#ifdef WARN_ON
      if (++cnt>1) Warning(("HEY! more than one active pickState Link from %d\n",pick));
#endif
      LinkID lnkID=pickLinks->ID();
      pPickStateRel->Remove(lnkID);
   }
   SafeRelease(pickLinks);
   PlayerHandlerSetFrameCallback(kPlayerModeLockPick,NULL,NULL);
   PlayerFinishAction();
}

void PickLockDoStop(ObjID pick)
{
   PickLockClearState(pick);
   if (schema_idx) SchemaPlayHalt(schema_idx);
   schema_idx=0;
}

void PickLockPerFrameCallback(ulong dt)
{  // check for current pick actions
   // would like to do an "if any" first
   ILinkQuery *pickLinks=pPickStateRel->Query(LINKOBJ_WILDCARD,LINKOBJ_WILDCARD);
   for (; !pickLinks->Done(); pickLinks->Next())
   {
      sLink link;
      pickLinks->Link(&link);
      if (_DoPickOperation(*((ObjID *)pickLinks->Data()),link.source,link.dest,dt)!=kPickMatch)
      {  // only if we Match do we keep going
         PickLockDoStop(link.source);
      }  // @TODO: send "completed" message?? with type?
   }
   SafeRelease(pickLinks);
}

// ready or unready the picks themselves
BOOL PickLockReady(ObjID frobber, ObjID pick_obj)
{
#ifdef USE_PLAYER_ARM
   if (frobber==PlayerObject())
      PlayerSwitchMode(kPlayerModeLockPick);
   else
      Warning(("Cant ready picks for non-player!\n"));
#endif
   return TRUE;
}

BOOL PickLockUnReady(ObjID frobber, ObjID pick_obj)
{
#ifdef USE_PLAYER_ARM
   if (frobber==PlayerObject())   
      PlayerSwitchMode(kPlayerModeUnarmed);
   else
      Warning(("Cant unready picks for non-player!\n"));
#endif
   return TRUE;   
}

/// start and stop the actual picking of the lock
BOOL PickLockStartPicking(ObjID frobber, ObjID pick_obj, ObjID locked_obj)
{
   ePickResult res;
   if (!ObjComputeLockedState(locked_obj))
   {
      _PickSound("noluck",locked_obj,0);
      return FALSE;  // cant start to pick an unlocked door.
   }
   if (_IsAdvancedLock(locked_obj))
     ConfigSpew("PickTrack",("%d is an advanced lock.\n",locked_obj));
   else
     ConfigSpew("PickTrack",("%d is a simple lock.\n",locked_obj));
   // see if we match at all first
   _BuildInitialPickState(frobber,pick_obj,locked_obj);
   if (schema_idx!=0) Warning(("Already playing picking loop?"));
   schema_idx=0;
   res=_DoPickOperation(frobber,pick_obj,locked_obj,0);
   if (res==kPickMatch)
   {
#ifdef USE_PLAYER_ARM
      PlayerHandlerSetFrameCallback(kPlayerModeLockPick,PickLockPerFrameCallback,NULL);
      PlayerStartAction();
#endif
      schema_idx=_PickSound("lockpik",locked_obj,40);
      ConfigSpew("PickTrack",("%d is starting to pick %d with %d\n",frobber,locked_obj,pick_obj));
      return TRUE;
   }
   else
   {
      if (res==kPickNoData)
         _PickSound("noluck",locked_obj,0);
      PickLockClearState(pick_obj);
      return FALSE;
   }
}

BOOL PickLockFinishPicking(ObjID pick_obj)
{
   PickLockDoStop(pick_obj);
   return TRUE;
}

// @TODO: someday should warn if cant do now..
BOOL PickLockMotionOnly(BOOL start)
{
#ifdef USE_PLAYER_ARM
   if (start)
      PlayerStartAction();
   else
      PlayerFinishAction();
#endif
   return TRUE;
}

int PickLockCountPickableLockedDoors(void)
{
   sPropertyObjIter iter;
   ObjID obj;
   int cnt=0;

   pPickCfgProp->IterStart(&iter);
   while (pPickCfgProp->IterNext(&iter,&obj))
      if (ObjComputeLockedState(obj))
         cnt++;
   pPickCfgProp->IterStop(&iter);
   return cnt;
}

#ifdef REPORT
///////////////
// report

static void _pickable_report(int WarnLevel, void *data, char *buffer)
{
   BOOL loud=(WarnLevel>=kReportLevel_DumpAll);
   sPropertyObjIter iter;
   sPickCfg *pPCfg;
   char *p=buffer;
   ObjID obj;
   int cnt=0;

   DarkReportHelpersStartUse();
   rsprintf(&p,"\nPickable Objects Report\n");
   pPickCfgProp->IterStart(&iter);
   while (pPickCfgProp->IterNextValue(&iter,&obj,&pPCfg))
   {
      cnt++;
      if (loud)
      {
         rsprintf(&p,"%s%s pickable\n",ObjWarnName(obj),RHLocS(obj));
         for (int i=0; i<NUM_TUMBLERS; i++)
            if (pPCfg->tumbler[i].data_bits!=0)
               rsprintf(&p," Tumbler %d: bits %x, %d pins, time_pct %d flags %x\n",
                        i,pPCfg->tumbler[i].data_bits,pPCfg->tumbler[i].pins,
                        pPCfg->tumbler[i].time_pct,pPCfg->tumbler[i].flags);
      }
   }
   rsprintf(&p,"There are %d pickable objects\n",cnt);
   pPickCfgProp->IterStop(&iter);
   DarkReportHelpersDoneWith();
}
#endif

///////////////
// pick properties

// look, a whole pile of identical stuff
#define PICK_IMPL kPropertyImplSparseHash

typedef cGenericProperty<IAdvPickTransCfgProperty,&IID_IAdvPickTransCfgProperty,sAdvPickTransCfg*> cAdvPickTransCfgPropertyBase;

class cAdvPickTransCfgProperty : public cAdvPickTransCfgPropertyBase
{
  cClassDataOps<sAdvPickTransCfg> mOps;

public:
  cAdvPickTransCfgProperty(const sPropertyDesc* desc, IPropertyStore* store)
    : cAdvPickTransCfgPropertyBase(desc,store)
      {
	SetOps(&mOps);
      }

  cAdvPickTransCfgProperty(const sPropertyDesc* desc, ePropertyImpl impl)
    : cAdvPickTransCfgPropertyBase(desc,CreateGenericPropertyStore(impl))
      {
	SetOps(&mOps);
      }

  STANDARD_DESCRIBE_TYPE(sAdvPickTransCfg);
};

typedef cGenericProperty<IAdvPickStateCfgProperty,&IID_IAdvPickStateCfgProperty,sAdvPickStateCfg*> cAdvPickStateCfgPropertyBase;

class cAdvPickStateCfgProperty : public cAdvPickStateCfgPropertyBase
{
  cClassDataOps<sAdvPickStateCfg> mOps;

public:
  cAdvPickStateCfgProperty(const sPropertyDesc* desc, IPropertyStore* store)
    : cAdvPickStateCfgPropertyBase(desc,store)
      {
	SetOps(&mOps);
      }

  cAdvPickStateCfgProperty(const sPropertyDesc* desc, ePropertyImpl impl)
    : cAdvPickStateCfgPropertyBase(desc,CreateGenericPropertyStore(impl))
      {
	SetOps(&mOps);
      }

  STANDARD_DESCRIBE_TYPE(sAdvPickStateCfg);
};

typedef cGenericProperty<IAdvPickSoundCfgProperty,&IID_IAdvPickSoundCfgProperty,sAdvPickSoundCfg*> cAdvPickSoundCfgPropertyBase;

class cAdvPickSoundCfgProperty : public cAdvPickSoundCfgPropertyBase
{
  cClassDataOps<sAdvPickSoundCfg> mOps;

public:
  cAdvPickSoundCfgProperty(const sPropertyDesc* desc, IPropertyStore* store)
    : cAdvPickSoundCfgPropertyBase(desc,store)
      {
	SetOps(&mOps);
      }

  cAdvPickSoundCfgProperty(const sPropertyDesc* desc, ePropertyImpl impl)
    : cAdvPickSoundCfgPropertyBase(desc,CreateGenericPropertyStore(impl))
      {
	SetOps(&mOps);
      }

  STANDARD_DESCRIBE_TYPE(sAdvPickSoundCfg);
};

typedef cGenericProperty<IPickCfgProperty,&IID_IPickCfgProperty,sPickCfg*> cPickCfgPropertyBase;

class cPickCfgProperty : public cPickCfgPropertyBase
{
   cClassDataOps<sPickCfg> mOps; 

public: 
   cPickCfgProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cPickCfgPropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cPickCfgProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cPickCfgPropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE(sPickCfg);
};

typedef cGenericProperty<IPickStateProperty,&IID_IPickStateProperty,sPickState*> cPickStatePropertyBase;

class cPickStateProperty : public cPickStatePropertyBase
{
   cClassDataOps<sPickState> mOps; 

public: 
   cPickStateProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cPickStatePropertyBase(desc,store)
   {
      SetOps(&mOps); 
   }

   cPickStateProperty(const sPropertyDesc* desc, ePropertyImpl impl)
      : cPickStatePropertyBase(desc,CreateGenericPropertyStore(impl))
   {
      SetOps(&mOps); 
   }

   STANDARD_DESCRIBE_TYPE(sPickState);
};


static sPropertyTypeDesc PickLockSrcTypeDesc = { "PickSrc", sizeof(int), };


static sPropertyDesc AdvPickTransCfgPropDesc =
{PROP_ADVPICKTRANSCFG_NAME,0,NULL,0,0,{"Dark Gamesys","AdvPickTransCfg"}};

static sPropertyDesc AdvPickStateCfgPropDesc =
{PROP_ADVPICKSTATECFG_NAME,0,NULL,0,0,{"Dark Gamesys","AdvPickStateCfg"}};

static sPropertyDesc AdvPickSoundCfgPropDesc =
{PROP_ADVPICKSOUNDCFG_NAME,0,NULL,0,0,{"Dark Gamesys","AdvPickSoundCfg"}};

static sPropertyDesc PickSrcPropDesc = 
   {PROP_PICKSRC_NAME,0,NULL,0,0,{"Dark GameSys","PickSrc"}};
static sPropertyDesc PickCfgPropDesc = 
   {PROP_PICKCFG_NAME,0,NULL,0,0,{"Dark GameSys","PickCfg"}};
static sPropertyDesc PickStatePropDesc = 
   {PROP_PICKSTATE_NAME,0,NULL,0,0,{"Dark GameSys","PickState"}, kPropertyProxyChangable};

// structure descriptor fun
static sFieldDesc pick_src_fields [] =
{
   { "PickBits", kFieldTypeBits, sizeof(int), 0, kFieldFlagUnsigned },
};

static const char *pick_flag_names[] = { "Reset On Fail", "Randomize Time" };

#define DEFINE_TUMBLER_FIELDS(tnumstr,field) \
   { "LockBits "tnumstr,   kFieldTypeBits, FieldLocation(sPickCfg,field##.data_bits) }, \
   { "Pins "tnumstr,       kFieldTypeInt,  FieldLocation(sPickCfg,field##.pins) }, \
   { "TimePct "tnumstr,    kFieldTypeInt,  FieldLocation(sPickCfg,field##.time_pct) }, \
   { "Flags "tnumstr,      kFieldTypeBits, FieldLocation(sPickCfg,field##.flags), FullFieldNames(pick_flag_names) }

#define DEFINE_CFGSTATETRANS_FIELDS(snumstr,field) \
   { "Next State Pick 1-"snumstr,   kFieldTypeInt, FieldLocation(sAdvPickTransCfg,field##.firstpickstate) }, \
   { "Next State Pick 2-"snumstr,   kFieldTypeInt, FieldLocation(sAdvPickTransCfg,field##.secondpickstate) }, \
   { "Next State Pick 3-"snumstr,   kFieldTypeInt, FieldLocation(sAdvPickTransCfg,field##.thirdpickstate) }

#define DEFINE_CFGSTATESTATE_FIELDS(snumstr,field) \
   { "Time "snumstr,       kFieldTypeInt,  FieldLocation(sAdvPickStateCfg,field##.time) }

#define DEFINE_CFGSTATESND_FIELDS(snumstr,field) \
   { "Sound Pick 1-"snumstr,    kFieldTypeString,  FieldLocation(sAdvPickSoundCfg,field##.firstpicksound) }, \
   { "Sound Pick 2-"snumstr,    kFieldTypeString,  FieldLocation(sAdvPickSoundCfg,field##.secondpicksound) }, \
   { "Sound Pick 3-"snumstr,    kFieldTypeString,  FieldLocation(sAdvPickSoundCfg,field##.thirdpicksound) }

static sFieldDesc adv_pick_trans_cfg_fields [] =
{
  DEFINE_CFGSTATETRANS_FIELDS("0",picktransfsm[0]),
  DEFINE_CFGSTATETRANS_FIELDS("1",picktransfsm[1]),
  DEFINE_CFGSTATETRANS_FIELDS("2",picktransfsm[2]),
  DEFINE_CFGSTATETRANS_FIELDS("3",picktransfsm[3]),
  DEFINE_CFGSTATETRANS_FIELDS("4",picktransfsm[4]),
  DEFINE_CFGSTATETRANS_FIELDS("5",picktransfsm[5]),
  DEFINE_CFGSTATETRANS_FIELDS("6",picktransfsm[6]),
  DEFINE_CFGSTATETRANS_FIELDS("7",picktransfsm[7]),
  DEFINE_CFGSTATETRANS_FIELDS("8",picktransfsm[8])
};

static sFieldDesc adv_pick_state_cfg_fields [] =
{
  {"Enable Advanced System", kFieldTypeBool, FieldLocation(sAdvPickStateCfg,enable) },
  {"First Pick Src", kFieldTypeBits, FieldLocation(sAdvPickStateCfg,firstpickbits) },
  {"Second Pick Src", kFieldTypeBits, FieldLocation(sAdvPickStateCfg,secondpickbits) },
  {"Third Pick Src", kFieldTypeBits, FieldLocation(sAdvPickStateCfg,thirdpickbits) },
  DEFINE_CFGSTATESTATE_FIELDS("0",pickstatefsm[0]),
  DEFINE_CFGSTATESTATE_FIELDS("1",pickstatefsm[1]),
  DEFINE_CFGSTATESTATE_FIELDS("2",pickstatefsm[2]),
  DEFINE_CFGSTATESTATE_FIELDS("3",pickstatefsm[3]),
  DEFINE_CFGSTATESTATE_FIELDS("4",pickstatefsm[4]),
  DEFINE_CFGSTATESTATE_FIELDS("5",pickstatefsm[5]),
  DEFINE_CFGSTATESTATE_FIELDS("6",pickstatefsm[6]),
  DEFINE_CFGSTATESTATE_FIELDS("7",pickstatefsm[7]),
  DEFINE_CFGSTATESTATE_FIELDS("8",pickstatefsm[8])
};

static sFieldDesc adv_pick_sound_cfg_fields [] =
{
  DEFINE_CFGSTATESND_FIELDS("0",picksoundfsm[0]),
  DEFINE_CFGSTATESND_FIELDS("1",picksoundfsm[1]),
  DEFINE_CFGSTATESND_FIELDS("2",picksoundfsm[2]),
  DEFINE_CFGSTATESND_FIELDS("3",picksoundfsm[3]),
  DEFINE_CFGSTATESND_FIELDS("4",picksoundfsm[4]),
  DEFINE_CFGSTATESND_FIELDS("5",picksoundfsm[5]),
  DEFINE_CFGSTATESND_FIELDS("6",picksoundfsm[6]),
  DEFINE_CFGSTATESND_FIELDS("7",picksoundfsm[7]),
  DEFINE_CFGSTATESND_FIELDS("8",picksoundfsm[8])
};

static sFieldDesc pick_cfg_fields [] =
{
   DEFINE_TUMBLER_FIELDS("1",tumbler[0]),
   DEFINE_TUMBLER_FIELDS("2",tumbler[1]),
   DEFINE_TUMBLER_FIELDS("3",tumbler[2])
};

static sFieldDesc pick_state_fields [] =
{
   { "CurTumbler/State", kFieldTypeInt,   FieldLocation(sPickState,cur_tumbler) },
   { "Pin",       kFieldTypeInt,   FieldLocation(sPickState,pin) },
   { "Done",      kFieldTypeBool,  FieldLocation(sPickState,done) },
   { "RandTime",  kFieldTypeInt,   FieldLocation(sPickState,tm_rand) },
   { "TotalTime", kFieldTypeInt,   FieldLocation(sPickState,tm_total) },
   { "StageTime", kFieldTypeInt,   FieldLocation(sPickState,tm_stage) },
   { "Picker",    kFieldTypeInt,   FieldLocation(sPickState,picker) },
};

static sStructDesc adv_pick_trans_cfg_struct = StructDescBuild(sAdvPickTransCfg,kStructFlagNone,adv_pick_trans_cfg_fields);
static sStructDesc adv_pick_state_cfg_struct = StructDescBuild(sAdvPickStateCfg,kStructFlagNone,adv_pick_state_cfg_fields);
static sStructDesc adv_pick_sound_cfg_struct = StructDescBuild(sAdvPickSoundCfg,kStructFlagNone,adv_pick_sound_cfg_fields);
static sStructDesc pick_src_struct   =
  { "PickSrc", sizeof(int), kStructFlagNone, sizeof(pick_src_fields)/sizeof(pick_src_fields[0]), pick_src_fields};
static sStructDesc pick_cfg_struct   = StructDescBuild(sPickCfg,kStructFlagNone,pick_cfg_fields);
static sStructDesc pick_state_struct = StructDescBuild(sPickState,kStructFlagNone,pick_state_fields);

// pickstate link data
#define PICKLOCK_REL_QCASES kQCaseSetSourceKnown
static sRelationDesc     PickStateRelDesc  = {"NowPicking",kRelationNoEdit};
static sRelationDataDesc PickStateDataDesc = LINK_DATA_DESC(int);

BOOL PickLockInit(void)
{
   StructDescRegister(&adv_pick_trans_cfg_struct);
   StructDescRegister(&adv_pick_sound_cfg_struct);
   StructDescRegister(&adv_pick_state_cfg_struct);
   StructDescRegister(&pick_src_struct);
   StructDescRegister(&pick_cfg_struct);
   StructDescRegister(&pick_state_struct);      
   pPickSrcProp=CreateIntegralProperty(&PickSrcPropDesc,&PickLockSrcTypeDesc,PICK_IMPL);
   pAdvPickTransCfgProp=new cAdvPickTransCfgProperty(&AdvPickTransCfgPropDesc,PICK_IMPL);
   pAdvPickSoundCfgProp=new cAdvPickSoundCfgProperty(&AdvPickSoundCfgPropDesc,PICK_IMPL);
   pAdvPickStateCfgProp=new cAdvPickStateCfgProperty(&AdvPickStateCfgPropDesc,PICK_IMPL);
   pPickCfgProp=new cPickCfgProperty(&PickCfgPropDesc,PICK_IMPL);
   pPickStateProp=new cPickStateProperty(&PickStatePropDesc,PICK_IMPL);
   pPickStateRel=CreateStandardRelation(&PickStateRelDesc,&PickStateDataDesc,PICKLOCK_REL_QCASES);
   ReportRegisterGenCallback(_pickable_report,kReportAllObj,"Pickable Locks",NULL);   
   return TRUE;
}

BOOL PickLockTerm(void)
{
   SafeRelease(pPickSrcProp);
   SafeRelease(pPickCfgProp);
   SafeRelease(pPickStateProp);
   SafeRelease(pPickStateRel);
   SafeRelease(pAdvPickTransCfgProp);
   SafeRelease(pAdvPickSoundCfgProp);
   SafeRelease(pAdvPickStateCfgProp);
   ReportUnRegisterGenCallback(_pickable_report,kReportAllObj,NULL);
   return TRUE;
}
