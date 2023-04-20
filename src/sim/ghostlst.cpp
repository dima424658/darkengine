/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostlst.cpp,v 1.19 1999/12/17 15:37:16 PATMAC Exp $
// internal data structures/lists for ghost control

#include <string.h>    // memset, apparently

#include <appagg.h>

#include <ghost.h>
#include <ghostlst.h>
#include <ghostapi.h>
#include <ghostphy.h>  // to build the models
#include <ghostint.h>
#include <ghostmvr.h>
#include <physapi.h>

#include <netman.h>    // pNetMan
#include <simtime.h>

#include <aiapi.h>     // ObjIsAI()

#include <hshsttem.h>

#include <dbmem.h>

////////////////////////
// actual ghost info, global

#ifdef _MSC_VER
template cGhostLocalTableBase;
template cGhostRemoteTableBase;
#endif

// core data structures
cGhostLocalHash  gGhostLocals;
cGhostRemoteHash gGhostRemotes;

// for weapon object solving
int (*GhostWeaponObjSwizzle)(ObjID weapon, BOOL charging)=NULL;

// declare int hash or list or whatever, debug only

////////////////////////
// List Control

static BOOL _GhostRecentlyExisted(ObjID obj)
{
   // will want to store off recently killed objs
   // and return whether this object is in the list
   return FALSE;
}

///////////////////////////////
///////////////////////////////
// these are part of the GLOBAL ghostapi, thus in GHOSTAPI.H, not in GHOSTLST.H

// core setup for local ghosts
void GhostInitLocalStruct(sGhostLocal *pGL)
{
   memset(pGL,0,sizeof(sGhostLocal));
   pGL->playing.schema_idx=pGL->playing.motion_num=-1;
}

// actual add/remove of ghost into the local hash
void GhostAddLocal(ObjID obj, float detail, int flags)
{
   if (obj<0)
   {
      Warning(("Umm.... local got a negative ghost %s?\n",ObjWarnName(obj)));
      return;
   }
   if (gGhostLocals.Search(obj))
   {
      if (_ghost_list_duplicates())
         _ghost_mprintf(("GhostAddLocal: Obj %s already in local ghost table\n",ObjWarnName(obj)));
      return;
   }
   sGhostLocal  *pGL = new sGhostLocal;
   GhostInitLocalStruct(pGL);
   pGL->obj        = obj;
   pGL->cfg.detail = detail; // really, want to multiply by detail for my type
   pGL->cfg.flags  = flags|kGhostCfNew;
   gGhostLocals.Insert(pGL);
   _GhostDebugSetupLocal(pGL);
   _GhostSetupInitialPhysLocal(pGL);
   if (_ghost_list_mods())
      _ghost_mprintf(("ghost list added %s local at %d\n",ObjWarnName(obj),GetSimTime()));
}

void GhostInitRemoteStruct(sGhostRemote *pGR)
{
   memset(pGR,0,sizeof(sGhostRemote));
   pGR->critter.cur_mocap.schema_idx=pGR->critter.cur_mocap.motion_num=-1;
   pGR->weap.nWeapon=-1;
}

// actually add a ghost into the remote hash
void GhostAddRemote(ObjID obj, float detail, int flags)
{
   if (obj<0)
   {
      Warning(("Umm.... remote got a negative ghost %s?\n",ObjWarnName(obj)));
      return;
   }
   if (gGhostRemotes.Search(obj))
   {
      if (_ghost_list_duplicates())
         _ghost_mprintf(("GhostAddRemote: Obj %s already in remote ghost table\n",ObjWarnName(obj)));
      return;
   }
   sGhostRemote *pGR = new sGhostRemote;
   GhostInitRemoteStruct(pGR);
   pGR->obj         = obj;
   pGR->cfg.detail  = detail;
   pGR->cfg.flags   = flags|kGhostCfNew|kGhostCfNoPacket|kGhostCfFirstTPort;
   gGhostRemotes.Insert(pGR);
   _GhostDebugSetupRemote(pGR);
   _GhostSetupInitialPhysRemote(pGR);
   _GhostSetupInitialPhysWeap(pGR);
   if (_ghost_list_mods())
      _ghost_mprintf(("ghost list added %s remote at %d\n",ObjWarnName(obj),GetSimTime()));
}

void GhostRemLocal(ObjID obj)
{
   sGhostLocal *pGL=gGhostLocals.RemoveByKey(obj);
   _GhostDebugSetupLocal(pGL);

   if (pGL)
      delete pGL;
   else
      if (!_GhostRecentlyExisted(obj))
         Warning(("GhostRemLocal: %s not found in hash\n",ObjWarnName(obj)));
      // in debug full spew, might want to warn here anyway
   if (_ghost_list_mods())
      _ghost_mprintf(("ghost list removed %s local\n",ObjWarnName(obj)));
}

void GhostRemRemote(ObjID obj)
{
   sGhostRemote *pGR=GhostGetRemote(obj);
   _GhostDebugSetupRemote(pGR);
   if (pGR)
   {
      if (pGR->cfg.flags & kGhostCfLoaded)
         return;  // We're still in the postload; don't allow deletes yet
      _GhostRepairRemoteModels(pGR);
      gGhostRemotes.RemoveByKey(obj);
      if (pGR->critter.pMotCoord)
         delete pGR->critter.pMotCoord;
      delete pGR;
   }
   else
      if (!_GhostRecentlyExisted(obj))
         Warning(("GhostRemRemote: %s not found in hash\n",ObjWarnName(obj)));
      // in debug full spew, might want to warn here anyway
   if (_ghost_list_mods())
      _ghost_mprintf(("ghost list removed %s remote\n",ObjWarnName(obj)));
}

////////////////////////////////
// dumb code to memory clean up on destroy

void GhostListEmptyAndFree(void)
{
#ifdef WARN_ON
   int count;
   count=gGhostLocals.GetCount();
   if (count)
      Warning(("GhostLocalHash still has %d members\n",count));
   count=gGhostRemotes.GetCount();
   if (count)
      Warning(("GhostRemoteHash still has %d members\n",count));
#endif

   tHashSetHandle local_iter;    // go through local ghosts
   sGhostLocal *pGL=gGhostLocals.GetFirst(local_iter);
   while (pGL)
   {
      GhostRemLocal(pGL->obj);
      pGL=gGhostLocals.GetNext(local_iter);
   }

   tHashSetHandle remote_iter;   // go through remote ghosts
   sGhostRemote *pGR=gGhostRemotes.GetFirst(remote_iter);
   while (pGR)
   {
      GhostRemRemote(pGR->obj);
      pGR=gGhostRemotes.GetNext(remote_iter);
   }
}

////////////////////////////////
// misc state/list maintenance

// really, want to not do these if we arent networking

// notify about deregistration
void GhostNotifyPhysDeregister(ObjID obj)
{
   sGhostRemote *pGR=GhostGetRemote(obj);
   if (pGR)
   {
      if (_ghost_watch_events())
         _ghost_mprintf(("PhysDereg Clearing to CfNew for %s\n",ObjWarnName(obj)));
      pGR->cfg.flags|=kGhostCfNew;
      return;
   }
   sGhostLocal *pGL=GhostGetLocal(obj);
   if (pGL)
   {
      if (_ghost_watch_events())
         _ghost_mprintf(("PhysDereg Clearing to CfNew for %s\n",ObjWarnName(obj)));
      pGL->cfg.flags|=kGhostCfNew;
      return;
   }
}

// are we allowed to send a deregister packet for this obj
BOOL GhostAllowDeregisterPacket(ObjID obj)
{
   sGhostLocal *pGL=GhostGetLocal(obj);
   if (pGL)                // @TBD: WHAT SHOULD WE DO HERE, REALLY
      if (!_GhostIsType(pGL->cfg.flags,Player))
         return ((pGL->state&kGhostStDead)==0);
   return TRUE;
}

#ifdef GHOST_DEBUGGING
static const char *ghost_events[]={"Dying", "Wounded", "Swinging", "Firing", "Sleep", "Revive", "Dead", "Sleeping"};
#define NUM_GHOST_EVENTS (sizeof(ghost_events)/sizeof(ghost_events[0]))
const char *EventString(int x)
{
   for (int i=(1<<0), j=0; i<(1<<NUM_GHOST_EVENTS); i<<=1, j++)
      if ((x&i)==x)
         return ghost_events[j];
   return "None";
}
#else
#define EventString(x) "##x##"
#endif

// notify a ghost about an event
void GhostNotifyState(ObjID obj, int event, BOOL off)
{
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr_(NetManager,pNetMan);
   if (!pNetMan->Networking())
      return;
   sGhostLocal *pGL=GhostGetLocal(obj);
   if (pGL)
   {
      _GhostDebugSetupLocal(pGL);
      if (_ghost_watch_events())
         _ghost_mprintf(("Ghost %s event %s (%x) state %s\n",
                         ObjWarnName(obj),EventString(event),event,off?"Off":"On"));
      // not clear we really want to support "off" - but i will for now
      if (off)
         pGL->state&=~event;
      else
         pGL->state|=event;
      return;
   }
#ifdef DBG_ON
   sGhostRemote *pGR=GhostGetRemote(obj);
   if (pGR)
      Warning(("Hey! Ghost %s notified %s, but im remote AI\n",ObjWarnName(obj),EventString(event)));
   else
      Warning(("Called GhostNotify %s for a non-ghost\n",ObjWarnName(obj),EventString(event)));
#endif
#endif
}

void GhostNotify(ObjID obj, int event)
{
   GhostNotifyState(obj,event,FALSE);
}

void _GhostChangeConfig(ObjID obj, int *cfg_flags, int new_flags, BOOL off)
{
   if (_ghost_watch_events())
      _ghost_mprintf(("Ghost %s old flags %x turning %x %s ",ObjWarnName(obj),*cfg_flags,new_flags,off?"Off":"On"));
   if (off)
      *cfg_flags&=~new_flags;
   else
      *cfg_flags|= new_flags;
   if (_ghost_watch_events())
      _ghost_mprintf(("new val %x\n",*cfg_flags));
}

void GhostChangeLocalConfig(ObjID obj, int flags, BOOL off)
{
   sGhostLocal *pGL=GhostGetLocal(obj);
   if (pGL)
   {
      _GhostDebugSetupLocal(pGL);
      _GhostChangeConfig(pGL->obj,&pGL->cfg.flags,flags,off);
   }
   else
      Warning(("Hey! GhostChangeLocalConfig on %s, who isnt a local ghost\n",ObjWarnName(obj)));
}

void GhostChangeRemoteConfig(ObjID obj, int flags, BOOL off)
{
   sGhostRemote *pGR=GhostGetRemote(obj);
   if (pGR)
   {
      _GhostDebugSetupRemote(pGR);
      _GhostChangeConfig(pGR->obj,&pGR->cfg.flags,flags,off);
   }
   else
      Warning(("Hey! GhostChangeRemoteConfig on %s, who isnt a remote ghost\n",ObjWarnName(obj)));
}

// The flags that we should keep if a ghost gets rebooted due to ownership
// changes:
#define GHOST_KEEP_FLAGS (kGhostCfTypeMask)

// This object has changed hands; see if we need to change its ghost state
void GhostNotifyOwnerChanged(ObjID obj, BOOL iOwnIt)
{
   if (_ghost_watch_events())
      _ghost_mprintf(("Just heard Obj %s changed owner (is %sme)\n",
                      ObjWarnName(obj),iOwnIt?"":"Not "));

   // on any owner change, reset all sequence id data...
   sGhostLocal *pGL = GhostGetLocal(obj);
   if (pGL)
      pGL->info.seq_id=0;
   else
   {
      sGhostRemote *pGR = GhostGetRemote(obj);
      if (pGR)
         pGR->info.seq_id=pGR->info.last_g_seq=0;
      // else we dont have a ghost... why are we here?
   }

   if (ObjIsAI(obj))   // AIs have their own handoff path, so don't mess with them
      return;

   if (IsLocalGhost(obj) && !iOwnIt)
   {
      sGhostLocal *pGL = GhostGetLocal(obj);
      float detail = pGL->cfg.detail;
      int keepFlags = pGL->cfg.flags & GHOST_KEEP_FLAGS;
      GhostRemLocal(obj);
      GhostAddRemote(obj, detail, keepFlags);
   }
   else if (IsRemoteGhost(obj) && iOwnIt)
   {
      sGhostRemote *pGR = GhostGetRemote(obj);
      BOOL is_obj = _GhostIsType(pGR->cfg.flags,IsObj);
      float detail = pGR->cfg.detail;
      int keepFlags = pGR->cfg.flags & GHOST_KEEP_FLAGS;
      GhostRemRemote(obj);
      GhostAddLocal(obj, detail, keepFlags);

      // if we're handing an object back to the default
      // host, remove any residual control velocities
#ifdef NEW_NETWORK_ENABLED
      if (is_obj)
      {
         AutoAppIPtr_(NetManager, pNetMan);
         if (pNetMan->AmDefaultHost())
         {
            PhysSetGravity(obj, 1.0);
            PhysSetBaseFriction(obj, 0.0);
            PhysStopControlVelocity(obj);
         }
      }
#endif
   }
}

// set w/OBJ_NULL to set no weapon
// set w/charging to mean, well, charging
void GhostSetWeapon(ObjID ghost, ObjID weapon, BOOL charging)
{
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr_(NetManager,pNetMan);
   if (!pNetMan->Networking())
      return;
   sGhostLocal *pGL=GhostGetLocal(ghost);
   if (pGL)
   {
      _GhostDebugSetupLocal(pGL);            // weapID returns -1 none, so we add 1
      if (GhostWeaponObjSwizzle)
         pGL->nWeapon=((*GhostWeaponObjSwizzle)(weapon,charging))+1;  // so 0 means none
      if (_ghost_track_weapons())
         _ghost_mprintf(("%s weap idx %d, (weap %s, %s Charging)\n",ObjWarnName(ghost),
                         pGL->nWeapon,ObjWarnName(weapon),charging?"Is":"Not"));
   }
   else
      Warning(("Hey! GhostSetWeapon on %s, who isnt a local ghost\n",ObjWarnName(ghost)));
#endif
}

// remove all weapons for all ghosts (in order to get ready to save to briefcase)
void GhostRemoveWeapons(void)
{
   if (_ghost_track_weapons())
      _ghost_mprintf(("GhostRemoveWeapons called\n"));

   if (GhostWeaponObjAttach==NULL)
      return;  // cause if we cant attach, this is all pretty silly

   tHashSetHandle remote_iter;   // go through remote ghosts
   sGhostRemote *pGR=gGhostRemotes.GetFirst(remote_iter);
   while (pGR)
   {
      if (_GhostIsType(pGR->cfg.flags,Player))
      {
         if (pGR->weap.nWeapon!=-1)
         {
            if (_ghost_track_weapons())
               _ghost_mprintf(("GRM removing weapon %s from %s\n",ObjWarnName(pGR->weap.weaponObj),ObjWarnName(pGR->obj)));
            (*GhostWeaponObjAttach)(pGR->obj,-1);
         }
      }
      pGR=gGhostRemotes.GetNext(remote_iter);
   }
}

////////////////////////////////
// internal api to rest of ghost

sGhostRemote *GhostGetRemote(ObjID obj)
{
   return gGhostRemotes.Search(obj);
}

sGhostLocal *GhostGetLocal(ObjID obj)
{
   return gGhostLocals.Search(obj);
}

// @TBD: can we quick exit FALSE for these if no networking?
// this is also dumb since these are all but the functions above...
BOOL IsLocalGhost(ObjID obj)
{
   return (gGhostLocals.Search(obj) != NULL);
}

BOOL IsRemoteGhost(ObjID obj)
{
   return (gGhostRemotes.Search(obj) != NULL);
}

static int _ghost_type_convert[]={0,kGT_AI,kGT_Object,kGT_Player};

int GetGhostType(ObjID obj)
{
   int type = kGT_None;
   int flags = 0;

   sGhostLocal  *pGL;
   sGhostRemote *pGR;

   if ((pGL = GhostGetLocal(obj)) != NULL)
   {
      type |= kGT_Local;
      flags = pGL->cfg.flags;
   }
   else if ((pGR = GhostGetRemote(obj)) != NULL)
   {
      type |= kGT_Remote;
      flags = pGR->cfg.flags;
   }

   type|=_ghost_type_convert[_GhostGetType(flags)];

   return type;
}

////////////////
// full init/term/reset

void GhostListInit(void)
{

}

void GhostListTerm(void)
{
   GhostListEmptyAndFree();

#ifdef WARN_ON
   int count;
   count=gGhostLocals.GetCount();
   if (count)
      Warning(("GhostLocalHash post empty has %d members\n",count));
   count=gGhostRemotes.GetCount();
   if (count)
      Warning(("GhostRemoteHash post empty has %d members\n",count));
#endif

   gGhostLocals.DestroyAll();
   gGhostRemotes.DestroyAll();
}

void GhostListReset(void)
{
   GhostListTerm();  // for now, cause this is easy...
}

#ifdef GHOST_DEBUGGING
// externed in ghostcfg.h, which this is really for
int cur_obj=0, targ_obj=0, cur_type=0, targ_type=0;

// call with any ONE of the three, if you have the pG, use that, save time
// sets up the debugging globals for ghost spew stuff
void _GhostDebugSetup(ObjID obj, sGhostLocal *pGL, sGhostRemote *pGR)
{
   if ((pGL==NULL)&&(pGR==NULL)&&(obj!=OBJ_NULL))
   {
      pGL=GhostGetLocal(obj);
      if (pGL==NULL)
         pGR=GhostGetRemote(obj);
   }
   // ok, ok, i should have inherited from a common thing, duh, shoot me
   if (pGL)
   {
      cur_type=pGL->cfg.flags;
      cur_obj=pGL->obj;
   }
   else if (pGR)
   {
      cur_type=pGR->cfg.flags;
      cur_obj=pGR->obj;
   }
   else
   {
#ifdef GHOST_DEBUGGING
      if (!config_is_defined("no_network")) {
         _ghost_mprintf(("Cannot setup debug ghost info for %d %x %x\n",obj,pGL,pGR));
      }
#endif
      cur_type=cur_obj=0;
   }
}

void _GhostPrintGhostPos(ObjID obj, sGhostHeartbeat *pGH, int flags, const char *prefix, int seq_id)
{
   char seq_buf[64], buf[1024];
   if (seq_id!=-1)
      sprintf(seq_buf," seq %d",seq_id);
   else
      seq_buf[0]='\0';
   sprintf(buf," %sv: %.4f %.4f %.4f m %d f %x%s @ %d\n %sp: %.4f %.4f %.4f a(%s): %x %x %x\n",
           prefix,pGH->vel.x,pGH->vel.y,pGH->vel.z,pGH->mode,pGH->flags,seq_buf,
           GetSimTime(),prefix,pGH->pos.x,pGH->pos.y,pGH->pos.z,
           (pGH->flags&kGhostHBFullAngs)?"Full":"Dtz",
           pGH->angle_info.tz,pGH->angle_info.dtz,pGH->angle_info.p);
   mprint(buf);
}
#endif
