/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkinvui.cpp,v 1.78 2000/03/19 16:09:37 patmac Exp $

#include <lg.h>

#include <appagg.h>

#include <simtime.h>
#include <drkinvui.h>
#include <drkplinv.h>
#include <frobctrl.h>
#include <playrobj.h>

#include <command.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <contain.h>
#include <prjctile.h>
#include <iobjsys.h>
#include <traitman.h>
#include <2d.h>
#include <inv_rend.h>
#include <invrndpr.h>
#include <rect.h>
#include <guistyle.h>
#include <statbar.h>
#include <imgsrc.h>
#include <objhp.h>
#include <drkbreth.h>
#include <uigame.h>
#include <matrixs.h>
#include <phprop.h>
#include <phmods.h>
#include <phcore.h>
#include <phmod.h>
#include <physapi.h>
#include <drkinvpr.h>
#include <gamestr.h>
#include <resapi.h>
#include <respaths.h>
#include <scrnman.h>
#include <drkvo.h>
#include <propface.h>
#include <propman.h>
#include <propbase.h>
#include <vismeter.h>
#include <playrobj.h>
#include <plyrmode.h>
#include <pick.h>
#include <objlight.h>

// Pickpocketing
#include <aiapi.h>
#include <aiapiiai.h>
#include <aiaware.h>
#include <aiprcore.h>
#include <weapcb.h>
#include <quest.h>

// Hacks
#include <esnd.h>
#include <creatext.h>
#include <creature.h>
#include <objpos.h>
#include <wrtype.h>
#include <port.h>
#include <objshape.h>
#include <playrobj.h> // compass
#include <drkamap.h> // more compass
#include <fix.h> // and more compass
#include <camera.h>
#include <rendprop.h>
#include <weapon.h>

// object name stuff
#include <str.h>
#include <edittool.h>

// for selecting loot in inventory:
#include <lootprop.h>

#ifdef NEW_NETWORK_ENABLED
#include <netman.h>
#include <iobjnet.h>
#include <netmsg.h>
#include <phnet.h>
#endif

// stats
#include <drkstats.h>

#include <config.h>

#include <dbmem.h>

////////////////////////////////////////////////////////////
// DARK INVENTORY UI COMMANDS & API
//

static IInventory* pInventory = NULL;
static IContainSys* pContainSys = NULL;

// last object player picked up
static ObjID gLastObjPickedUp = OBJ_NULL;

//------------------------------------------------------------
// OBJECT "EXPIRATION"
//

//
// How many msec before our current item display goes away
//
#define NO_EXPIRATION 0xFFFFFFFF

static tSimTime gSelTimeOut[kNumItemSels] =  { 0, 0 };
static tSimTime gSelExpired[kNumItemSels] = { NO_EXPIRATION, NO_EXPIRATION };
static tSimTime gSelLastTime[kNumItemSels] = { NO_EXPIRATION, NO_EXPIRATION };

static inline void set_expiration(int sel)
{
   if (sel == kCurrentItem && pInventory->WieldingJunk())
      gSelExpired[sel] = NO_EXPIRATION;
   else
      gSelExpired[sel] = GetSimTime() + gSelTimeOut[sel];
   gSelLastTime[sel] = gSelExpired[sel]; //This guarantees that it hasn't just expired,
                                         //unless it really should have for some inexplicable
                                         //reason.  AMSD
}

static inline BOOL has_just_expired(int sel)
{
   if (gSelTimeOut[sel] == 0)
     return FALSE;
   if (gSelExpired[sel] == NO_EXPIRATION)
     return FALSE;
   if ((gSelLastTime[sel]) <= gSelExpired[sel])
     return ((gSelLastTime[sel] = GetSimTime()) > gSelExpired[sel]);
   else
     {
       gSelLastTime[sel] = GetSimTime();
       return FALSE;
     }        

}

static inline BOOL is_expired(int sel)
{
   if (gSelTimeOut[sel] == 0)
      return FALSE;
   if (gSelExpired[sel] == NO_EXPIRATION)
      return FALSE;
   return gSelTimeOut[sel] != 0 && GetSimTime() > gSelExpired[sel];
}

//------------------------------------------------------------
// Modal tool frob
//

static BOOL gToolFrobCenter = FALSE; // whether to center tool frobs
static BOOL gToolFrobModal = TRUE; // whether you need to click to center tool

//------------------------------------------------------------
// INTERNALS & HELPERS
//

//
// compare two frob structs
//

static inline BOOL same_frob(const sFrobActivate& a, const sFrobActivate& b)
{
   return a.src_loc == b.src_loc
      &&  a.src_obj == b.src_obj
      &&  a.dst_loc == b.dst_loc
      &&  a.dst_obj == b.dst_obj
      &&  a.frobber == b.frobber;

   // we don't check ms_down, because we want the begin to match the end.
}

//
// Build the frob struct for an item frob
// encapsulates logic that decides between tool/world
//

static inline void build_item_frob(ObjID inv, ObjID world, sFrobActivate* pfrob)
{
   pfrob->flags = 0;
   // check for tool frob
   if (IsToolFrob(inv,world))
   {
      pfrob->src_loc = kFrobLocInv;
      pfrob->src_obj = inv;
      pfrob->dst_loc = kFrobLocWorld;
      pfrob->dst_obj = world;
   }
   else if (world != OBJ_NULL) // world frob
   {
      pfrob->src_loc = kFrobLocWorld;
      pfrob->src_obj = world;
      pfrob->dst_loc = kFrobLocNone;
      pfrob->dst_obj = OBJ_NULL;
   }
   else // inv frob
   {
      pfrob->src_loc = kFrobLocInv;
      pfrob->src_obj = inv;
      pfrob->dst_loc = kFrobLocNone;
      pfrob->dst_obj = OBJ_NULL;
   }
}

//------------------------------------------------------------
// GLOBAL FROB STATE
//

#define NO_START_TIME 0xFFFFFFFF

static tSimTime gItemStartTime = NO_START_TIME;
static sFrobActivate gItemStartFrob;

static tSimTime gWeaponStartTime = NO_START_TIME;
static sFrobActivate gWeaponStartFrob;

static BOOL gAutoFrob = FALSE;


//------------------------------------------------------------
// USE ITEM
//

static void use_item_command(BOOL finish)
{
   tSimTime now = GetSimTime();

   gAutoFrob = FALSE;

   if (finish && gItemStartTime == NO_START_TIME)
      return; // we were aborted or something

   ObjID world = frobWorldSelectObj;
   ObjID inv = pInventory->Selection(kCurrentItem);

   // special case, no world frobs if we have junk
   if (pInventory->WieldingJunk())
      world = OBJ_NULL;

   // special case: item has timed out
   if (is_expired(kCurrentItem))
      inv = OBJ_NULL;

   // Begin modal tool frob mode
   if (gToolFrobModal && !gToolFrobCenter)
   {
      BOOL center_tool = (world == OBJ_NULL) ? IsTool(inv) : IsToolFrob(inv,world);

      if (center_tool)
      {
         gToolFrobCenter = TRUE;
         set_expiration(kCurrentItem);
         gAutoFrob = TRUE;
         return ;
      }
   }

   if (world == OBJ_NULL && inv == OBJ_NULL) // Nothing to frob!
      return ;

   sFrobActivate frob;    // add 1 so finish always has time > 0
   frob.flags = 0;
   frob.ms_down = (finish) ? now - gItemStartTime + 1: 0;
   frob.frobber = PlayerObject();

   build_item_frob(inv,world,&frob);


   BOOL tool_frob = frob.dst_loc == kFrobLocWorld
                    && frob.src_loc == kFrobLocInv;

   if (inv != OBJ_NULL)
      if (finish)
         set_expiration(kCurrentItem);
      else
         // middle of frob, stay up forever.
         gSelExpired[kCurrentItem] = NO_EXPIRATION;

   // Don't start a tool frob while a weapon frob is going on.
#ifdef TOOL_HAND_ART
   if (!finish
       && gWeaponStartTime != NO_START_TIME
       && tool_frob)
      return;
#endif

   // inconsistent start state, abort
   if (finish && !same_frob(frob,gItemStartFrob))
   {
      FrobAbort(kFrobLocNone,&gItemStartFrob);
      gItemStartTime = NO_START_TIME;

      return;
   }

   // put the object in hand, please
#ifdef TOOL_HAND_ART
   if (!finish && tool_frob)
      pInventory->Wield(kCurrentItem);
#endif

   gItemStartFrob = frob;

   if (finish)
   {
      gItemStartTime = NO_START_TIME;
      if (gToolFrobModal)
         gToolFrobCenter = FALSE;
   }
   else
   {
      gItemStartTime = now;
   }

   FrobExecute(&frob);

}

//------------------------------------------------------------
// ATTACK COMMAND
//

static void use_weapon_command(BOOL finish)
{
   tSimTime now = GetSimTime();

   if (finish && gWeaponStartTime == NO_START_TIME)
      return; // we were aborted or something

   ObjID weapon = pInventory->Selection(kCurrentWeapon);

   BOOL expired = is_expired(kCurrentWeapon);

   set_expiration(kCurrentWeapon);

   if (expired || weapon == OBJ_NULL) return;

   // Don't start a weapon frob if there's a tool frob going on
   if (!finish
       && gItemStartTime != NO_START_TIME
       && gItemStartFrob.dst_loc == kFrobLocWorld
       && gItemStartFrob.src_loc == kFrobLocInv)
      return;


   // If we're carrying a junk item, gotta throw it first.
   if (pInventory->WieldingJunk() && !finish)
   {
      ObjID junk = pInventory->WieldedObj();
      sFrobActivate junkfrob = { SIM_TIME_SECOND, junk, OBJ_NULL, kFrobLocInv, kFrobLocNone, PlayerObject(), 0 };
      FrobInstantExecute(&junkfrob);

      // That might not have gotten rid of the junk
      if (pInventory->WieldingJunk())
         return ;
   }

   // Now build the frob - add 1 so that Ups always have time > 0
   int ms_down = (finish) ? now - gWeaponStartTime + 1: 0;
   sFrobActivate frob = { ms_down, weapon, OBJ_NULL, kFrobLocInv, kFrobLocNone, PlayerObject(), 0 };


   if (finish && !same_frob(frob,gWeaponStartFrob))
   {
      FrobAbort(kFrobLocNone,&gWeaponStartFrob);
      gWeaponStartTime = NO_START_TIME;
      return;
   }

   // put the weapon in hand
   pInventory->Wield(kCurrentWeapon);

   gWeaponStartFrob = frob;

   if (finish)
      gWeaponStartTime = NO_START_TIME;
   else
      gWeaponStartTime = now;

   FrobExecute(&frob);


}

//------------------------------------------------------------
// CYCLE WEAPON COMMAND
//

void cycle_weapon_command(int dir)
{
   BOOL expired = is_expired(kCurrentWeapon);
   set_expiration(kCurrentWeapon);

   ObjID weapon = OBJ_NULL;
   if (expired)
      weapon = pInventory->Selection(kCurrentWeapon);

   if (weapon == OBJ_NULL)
      weapon = pInventory->CycleSelection(kCurrentWeapon,(eCycleDirection)dir);

   if (gWeaponStartTime != NO_START_TIME
       && gWeaponStartFrob.src_obj != weapon)
   {
      FrobAbort(kFrobLocNone,&gWeaponStartFrob);
      gWeaponStartTime = NO_START_TIME;
   }

   // if there's no item frob going on, wield the weapon
   if (gItemStartTime == NO_START_TIME)
      pInventory->Wield(kCurrentWeapon);
}

void next_weapon_command( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      cycle_weapon_command( dir );
   }
}

void prev_weapon_command( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? 1 : -1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      cycle_weapon_command( dir );
   }
}

//------------------------------------------------------------
// CYCLE ITEM COMMAND
//

void cycle_item_command(int dir)
{
   BOOL expired = is_expired(kCurrentItem);
   set_expiration(kCurrentItem);

//@HACK: Return player camera to himself
   Camera *cam = PlayerCamera();
   if (CameraIsRemote(cam))
     CameraAttach(cam,PlayerObject());

   ObjID item = OBJ_NULL;
   if (expired)
    {
      item = pInventory->Selection(kCurrentItem);
      FrobSendInvSelectionMsg(kFrobLocInv,kFrobLevFocus,OBJ_NULL,item);
    }

   if (item == OBJ_NULL) {
      if ( dir != 0 ) {
         item = pInventory->CycleSelection(kCurrentItem,(eCycleDirection)dir);
      } else {
         item = pInventory->Selection( kCurrentItem );
      }
   }

   if (gItemStartTime != NO_START_TIME
       && gItemStartFrob.src_loc == kFrobLocInv
       && gItemStartFrob.src_obj != item)
   {
      FrobAbort(kFrobLocNone,&gItemStartFrob);
      gItemStartTime = NO_START_TIME;
   }
}

void next_item_command( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? -1 : 1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      cycle_item_command( dir );
   }
}

void prev_item_command( int count )
{
   int i, dir;

   if ( count == 0 ) {
      count = 1;
   }

   dir = ( count < 0 ) ? 1 : -1;
   count = abs( count );

   for ( i = 0; i < count; i++ ) {
      cycle_item_command( dir );
   }
}


//----------------------------------------

// Don't we wish we had an array at this time
static void abort_frob(int which)
{
   if (which == kCurrentWeapon)
   {
      if (gWeaponStartTime != NO_START_TIME)
         FrobAbort(kFrobLocNone,&gWeaponStartFrob);
      gWeaponStartTime = NO_START_TIME;
   }
   else
   {
      if (gItemStartTime != NO_START_TIME)
         FrobAbort(kFrobLocNone,&gItemStartFrob);
      gItemStartTime = NO_START_TIME;
   }
}

//------------------------------------------------------------
// SELECT OBJECT COMMAND
//

static void select_command(char* arg)
{
   // blast trailing whitespace
   char* s = arg + strlen(arg) - 1;
   for (; s >= arg && isspace(*s); s--)
      *s = '\0';

   AutoAppIPtr_(ObjectSystem,pObjSys);
   ObjID obj = pObjSys->GetObjectNamed(arg);
   if (obj == OBJ_NULL)
   {
      Warning(("select_command: No object named %s\n",arg));
      return ;
   }

   ObjID select = OBJ_NULL;

   if (pContainSys->IsHeld(PlayerObject(),obj) != ECONTAIN_NULL)
      select = obj;
   else
   {
      AutoAppIPtr_(TraitManager,pTraitMan);

      sContainIter* iter = pContainSys->IterStart(PlayerObject());
      for (; !iter->finished; pContainSys->IterNext(iter))
      {
         if (pTraitMan->ObjHasDonor(iter->containee,obj))
         {
            select = iter->containee;
            break;
         }
      }
      pContainSys->IterEnd(iter);
   }

   if (select !=OBJ_NULL)
   {
      Camera *cam = PlayerCamera();
      if (CameraIsRemote(cam))
	CameraAttach(cam,PlayerObject());

      BOOL weapon = pInventory->GetType(select) == kInvTypeWeapon;
      eWhichInvObj which = weapon ? kCurrentWeapon : kCurrentItem;
      set_expiration(which);
      if (select != pInventory->Selection(which))
      {
         abort_frob(which);
         pInventory->Select(select);
      }
      // Auto-wield sword if possible
      if (weapon && gItemStartTime == NO_START_TIME)
         pInventory->Wield(kCurrentWeapon);
   }
}


// from drkloot:
extern ILootProperty* gpLootProp;

static void select_loot_command()
{
   ObjID select = OBJ_NULL;

   sContainIter* iter = pContainSys->IterStart(PlayerObject());
   for (; !iter->finished; pContainSys->IterNext(iter))
   {
      if (gpLootProp->IsRelevant(iter->containee))
      {
         select = iter->containee;
         break;
      }
   }
   pContainSys->IterEnd(iter);

   if (select !=OBJ_NULL)
   {
      Camera *cam = PlayerCamera();
      if (CameraIsRemote(cam))
      	CameraAttach(cam,PlayerObject());

      eWhichInvObj which = kCurrentItem;
      set_expiration(which);
      if (select != pInventory->Selection(which))
      {
         abort_frob(which);
         pInventory->Select(select);
      }
   }
}


//------------------------------------------------------------
// DROP COMMAND
//


static BOOL throw_obj(ObjID thrower, ObjID obj, float power)
{
   if (CreatureExists(obj))
   {
      mxs_matrix orien;
      mxs_vector launch_pos;
      mxs_vector launch_dir;
      mxs_angvec launch_fac;

      launch_fac = ObjPosGet(thrower)->fac;
      launch_fac.tx = 0;
      launch_fac.ty = 0;
      mx_ang2mat(&orien, &launch_fac);

      mx_copy_vec(&launch_dir, &orien.vec[0]);

      mx_scale_add_vec(&launch_pos, &ObjPosGet(thrower)->loc.vec, &launch_dir, 1.0);

      launch_dir.z = 2.0;

      ObjPosUpdate(obj, &launch_pos, &launch_fac);

      VALIDATE_CREATURE_POS(FALSE);

      CreatureMakeBallistic(obj, kCMB_Compressed);

      if (!PhysObjValidPos(obj, NULL))
      {
         CreatureMakeNonBallistic(obj);
         CreatureMakeNonPhysical(obj);

         VALIDATE_CREATURE_POS(TRUE);

         return FALSE;
      }

      VALIDATE_CREATURE_POS(TRUE);

      // Set the make-non-physical-on-sleep flag
      //#define REMOVE_ON_SLEEP
#ifdef REMOVE_ON_SLEEP
      cPhysTypeProp *pTypeProp;
      g_pPhysTypeProp->Get(obj, &pTypeProp);
      pTypeProp->remove_on_sleep = TRUE;
      g_pPhysTypeProp->Set(obj, pTypeProp);
#endif

      launchProjectile(thrower, obj, power / 6,PRJ_FLG_MASSIVE | PRJ_FLG_FROMPOS, NULL, &launch_dir, NULL);

      //used to get launchpos, now we give flag PRJ_FLG_FROMPOS and location
      //as null so bodies don't stick up in the air wierdly when they land.
      //AMSD 10/25/99
      //&launch_pos);

      // Kind of a hack, bodies get "launched"
      cTagSet SoundEvent("Event Launch");
      ESndPlayObj(&SoundEvent, obj, OBJ_NULL);

      return TRUE;
   }
   else
   {
#ifdef NEW_NETWORK_ENABLED
      // @HACK: This really ought to wait until we see whether we have
      // successfully launched the thing before doing this. But the timing
      // of networking currently requires us to tell everyone that we're
      // physicalizing before we launch. We also need to super-hack this
      // with ForceContained, because otherwise the physicalize message
      // won't go through:
      PhysNetForceContainedMsgs(TRUE);
//      PhysRegisterSphereDefault(obj);
#endif

      BOOL launched = (launchProjectile(thrower,obj,power,PRJ_FLG_PUSHOUT|PRJ_FLG_MASSIVE,NULL,NULL,NULL) != OBJ_NULL);

#ifdef NEW_NETWORK_ENABLED
      PhysNetBroadcastObjPosByObj(obj);
      PhysNetForceContainedMsgs(FALSE);
#endif

      return launched;
   }
}

static IBoolProperty* gNoDropProp = NULL;

static sPropertyDesc no_drop_desc =
{
   "NoDrop",
   0, // flags
   NULL, 0, 0, // constraints, version
   { "Inventory", "Can't Drop This" },
};

static void create_no_drop()
{
   gNoDropProp = CreateBoolProperty(&no_drop_desc,kPropertyImplSparseHash);
}

// @NOTE: for now, when a client player (in multiplayer) drops something,
// he retains ownership of it. This *should* be okay, but it is
// different from what Shock did -- Shock would immediately hand the
// object back to the world host, and have the host deal with the
// projectile behaviour.
//
// If we decide that we need to do the Shocklike thing, some timing
// problems will have to be resolved. In particular, the client will
// probably need to figure out whether the throw is legal *before*
// removing it from inventory and giving it to the host, because otherwise
// the client will have a full roundtrip in which he thinks he no longer
// has the thing. In this case, he might, say, pick up another object
// before the host comes back and says, "No, you still have this one."
static void drop_command(void)
{
   ObjID obj = pInventory->Selection(kCurrentItem);

   // special case: item has timed out  (this is what use_item uses,
   // so I made this behaviour consistent AMSD)
   if (is_expired(kCurrentItem))
      obj = OBJ_NULL;

   if (obj != OBJ_NULL)
   {
      BOOL no_drop = FALSE;
      if (gNoDropProp->Get(obj,&no_drop) && no_drop)
         return;

      // if it's a stack, remove a single object from it
      obj = pContainSys->RemoveFromStack(obj);
      pInventory->Remove(obj);
      // @TODO: get a real velocity constant
      if (!throw_obj(PlayerObject(),obj,0.05))
         pInventory->Add(obj);

      if ( obj == gLastObjPickedUp ) {
         gLastObjPickedUp = OBJ_NULL;
      }
   }
}

//------------------------------------------------------------
// CLEAR SELECTIONS
//

static void clear_selection_internal(eWhichInvObj sel)
{
   if (gSelTimeOut[sel]>0)
      gSelExpired[sel]=gSelLastTime[sel]; //so we can get unfocus messages.
   else
      pInventory->ClearSelection(sel);
   abort_frob(sel);
}

static void clear_weapon_command(void)
{
   clear_selection_internal(kCurrentWeapon);
}

static void clear_item_command(void)
{
   if (!pInventory->WieldingJunk())
      clear_selection_internal(kCurrentItem);
}

static void clear_both_command(void)
{
   clear_weapon_command();
   clear_item_command();
}

// select the last item the user picked up
static void
select_newest_command( void )
{
   if ( gLastObjPickedUp != OBJ_NULL ) {
      pInventory->Select( gLastObjPickedUp );
      cycle_item_command( 0 );
   }
}


//------------------------------------------------------------
// COMMAND LIST
//

static Command commands[] =
{
   { "use_item", FUNC_INT, use_item_command, "use item  0 = start, 1 = finish", HK_GAME_MODE },
   { "use_weapon", FUNC_INT, use_weapon_command, "weapon item  0 = start, 1 = finish",  HK_GAME_MODE },
   { "next_item", FUNC_INT, next_item_command, "next item  1 = forward, -1 = backward", HK_GAME_MODE },
   { "prev_item", FUNC_INT, prev_item_command, "previous item  -1 = forward, 1 = backward", HK_GAME_MODE },
   { "next_weapon", FUNC_INT, next_weapon_command, "next weapon  1 = forward, -1 = backward", HK_GAME_MODE },
   { "prev_weapon", FUNC_INT, prev_weapon_command, "previous weapon  -1 = forward, 1 = backward", HK_GAME_MODE },
   { "inv_select", FUNC_STRING, select_command, "Select an inventory object by name (or archetype name)", HK_GAME_MODE },
   { "loot_select", FUNC_VOID, select_loot_command, "Select loot in inventory", HK_GAME_MODE },
   { "drop_item", FUNC_VOID, drop_command, "Drop (actually, throw) your currently selected item", HK_GAME_MODE },

   { "clear_item", FUNC_VOID, clear_item_command, "Clear the player's 'current item' selection.", HK_GAME_MODE },
   { "clear_weapon", FUNC_VOID, clear_weapon_command, "Clear the player's 'current item' selection.", HK_GAME_MODE },
   { "clear_weapon_and_item", FUNC_VOID, clear_both_command, "Clear both the player's 'current item' and 'current weapon.'", HK_GAME_MODE },
   { "select_newest_item", FUNC_VOID, select_newest_command, "Select the most recently picked up object", HK_GAME_MODE },
};

/////////////////////////////////////////////////////
//
// Networking support for pickup
//
// When a client picks up an object, the owner of that object (usually
// the world host), has to arbitrate whether they actually get it.
// So we need some network communication involved.
//

static IBoolProperty* gpBeingTakenProp = NULL;

#ifdef NEW_NETWORK_ENABLED
static cNetMsg *gpFrobPickupMsg = NULL;
static void handleFrobPickup(ObjID owner, ObjID obj)
{
   if (obj == OBJ_NULL)
   {
      // We weren't allowed to pick the thing up. We don't currently do
      // anything in this case, but might want to provide some sort of
      // feedback in the future.
   } else {
      AutoAppIPtr(NetManager);
      if (pNetManager->Networking()) {
         // Take possession of the object:
         AutoAppIPtr(ObjectNetworking);
         pObjectNetworking->ObjTakeOver(obj);
      }

      // Now that we've asserted control, we don't need this any more:
      gpBeingTakenProp->Set(obj, FALSE);

      // remember the last object player picked up
      AutoAppIPtr(ObjectSystem);
      AutoAppIPtr(TraitManager);
      ObjID crystalArch = pObjectSystem->GetObjectNamed( "Crystal" );
      if ( !( (pInventory->GetType(obj) == kInvTypeWeapon)
              || ((crystalArch != OBJ_NULL) && pTraitManager->ObjHasDonor(obj, crystalArch)) ) ) {
         gLastObjPickedUp = obj;
         // handle case where the object will combine with stack already in inventory
         sContainIter *pContIter = pContainSys->IterStart( owner );
         while ( !pContIter->finished ) {
            if ( pContainSys->CanCombine( pContIter->containee, obj, kContainTypeGeneric ) ) {
               gLastObjPickedUp = pContIter->containee;
               break;
            }
            pContainSys->IterNext( pContIter );
         }
         pContainSys->IterEnd( pContIter );
      }

      pContainSys->Add(owner,obj,kContainTypeGeneric,CTF_COMBINE);

   }
}

// The network message telling the client to pick the thing up.
//
// Note that Shock actually sends the container involved as well, but
// I don't *think* we need that for Thief.
static sNetMsgDesc sFrobPickupDesc =
{
   kNMF_SendToObjOwner,
   "FrobPickup",
   "Frob Pickup Obj",
   NULL,
   handleFrobPickup,
   {{kNMPT_ReceiverObjID, kNMPF_SendToOwner, "Frobber"},
    // Can't have the assert here, because the object could be
    // OBJ_NULL:
    {kNMPT_SenderObjID, kNMPF_AllowObjNull, "Obj"},
    {kNMPT_End}}
};

// Give the frobbed object to the requesting player.
// Host Code.
static void send_frob_pickup(ObjID obj, ObjID owner)
{
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      // Make sure we're not sending heartbeats for this thing any more
      PhysNetSetSleep(obj, TRUE);
   }

   // If owner already owned the obj, then this will just loopback:
   gpFrobPickupMsg->Send(OBJ_NULL, owner, obj);
}

static BOOL allow_object_pickup(ObjID obj, ObjID owner)
{
   BOOL beingTaken = FALSE;

   AutoAppIPtr(ObjectNetworking);
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking() && !pObjectNetworking->ObjHostedHere(obj))
   {
      // I don't own it any more, so the request has become invalid:
      send_frob_pickup(OBJ_NULL, owner);
      return(FALSE);
   }

   if (gpBeingTakenProp->Get(obj, &beingTaken) && beingTaken)
   {
      // Someone else has already laid claim to this object:
      send_frob_pickup(OBJ_NULL, owner);
      return FALSE;
   }

   // Okay, acknowledge the claim:
   gpBeingTakenProp->Set(obj, TRUE);

   // If we've borrowed this object from the world, stop worrying about
   // that. Otherwise, deregistering it will return the object
   // automatically:
   PhysNetDiscardBorrow(obj);

   // Tell the client that they have it:
   send_frob_pickup(obj, owner);

   return TRUE;
}
#endif

//------------------------------------------------------------
// FROB CALLBACKS
//

static BOOL rem_obj_CB(ObjID obj)
{
   return SUCCEEDED(pInventory->Remove(obj));
}

static BOOL try_pickpocket(ObjID obj, ObjID frobber)
{
   ObjID who = pContainSys->GetContainer(obj);
   if (who == OBJ_NULL)
      return TRUE;

   // Only AI's can be aware
   // AI it's dead, but not telling the damage model.
   AutoAppIPtr_(AIManager,pAIMan);
   cAutoIPtr<IAI> p_AI = pAIMan->GetAI(who);
   if ((IAI*)p_AI == NULL)
      return TRUE;

   cAutoIPtr<IInternalAI> pAI(IID_IInternalAI,p_AI);
   const sAIAwareness* aware = pAI->GetAwareness(frobber);
   if (aware->level <= kAIAL_Low)
   {
      if (AIGetMode(who)!=kAIM_Dead)
         DarkStatInc(kDarkStatPickPocket);
      return TRUE;
   }
   else
      if (AIGetMode(who)!=kAIM_Dead)
         DarkStatInc(kDarkStatPickPocketFail);

   // We failed,
   // @HACK: Threaten the AI by spoofing a "weapon windup"
   pAI->NotifyWeapon(kStartWindup,who,frobber);

   return FALSE;
}


static BOOL take_obj_CB(ObjID obj, ObjID owner)
{
   if (!try_pickpocket(obj,owner))
      return FALSE;

#ifdef NEW_NETWORK_ENABLED
   return allow_object_pickup(obj, owner);
#else
   pContainSys->Add(owner,obj,kContainTypeGeneric,CTF_COMBINE);
   return TRUE;
#endif
}

static BOOL throw_obj_CB(ObjID obj, ObjID thrower)
{
   // @TODO: get a real velocity constant
   if (throw_obj(thrower,obj,30.0))
   {
      pContainSys->Remove(thrower,obj);
      return TRUE;
   }
   else
   {
      pContainSys->Add(thrower, obj, kContainTypeGeneric, CTF_COMBINE);
      return FALSE;
   }
}

static BOOL tool_cursor_CB(ObjID )
{
   return FALSE;
}

static BOOL on_select_CB(eFrobLoc loc, eFrobLev lev, ObjID newobj, ObjID oldobj)
{
   if (loc == kFrobLocWorld && newobj != oldobj &&
       ((gItemStartFrob.src_loc==kFrobLocWorld)||(gItemStartFrob.dst_loc==kFrobLocWorld)))
      abort_frob(kCurrentItem);  // dont allow you to look away but keep using

   if (newobj != NULL && loc == kFrobLocWorld)
      DarkVoiceOverEvent(newobj, kVoiceWorldHighlight);

   if (newobj != NULL && loc == kFrobLocInv)
   {
      int type = pInventory->GetType(newobj);
      eWhichInvObj sel = (type == kInvTypeWeapon) ? kCurrentWeapon : kCurrentItem;
      // We only care about item to item or weapon to weapon
      abort_frob(sel);
   }

   return TRUE;
}

static BOOL deselect_CB(eFrobLoc loc, ObjID obj, ObjID frobber)
{
   if (IsPlayerObj(frobber))
   {
      int type = pInventory->GetType(obj);
      eWhichInvObj sel = (type == kInvTypeWeapon) ? kCurrentWeapon : kCurrentItem;
      clear_selection_internal(sel);
      return TRUE;
   }
   return FALSE;
}

static void install_frob_CBs()
{
   frobInvRemObj = rem_obj_CB;
   frobInvTakeObjFromWorld = take_obj_CB;
   frobInvThrowBackToWorld = throw_obj_CB;
   frobInvToolCursor = tool_cursor_CB;
   frobOnSetCurrent = on_select_CB;
   frobInvDeselect = deselect_CB;
}

//------------------------------------------------------------
// INVENTORY RENDERING
//

//
// Rendering state structure, one for weapon and one for item
// Needless to say, this should be a class
//

struct sRendState
{
   invRendState* render;
   Rect last;  // last position
   ulong last_msec;  // time at which last was computed
   Rect target; // target position
   ulong target_msec;  // ETA last->next
   int flags;  // special inv rend flags
   ObjID obj;

   // never too late to have member functions

   // set the target obj, blasting render as necessary
   void set_obj(ObjID o)
   {
      if (o != obj && render)
      {
         invRendFreeState(render);
         render = NULL;
      }
      obj = o;
   }

   // invalidate the "last" field
   void invalidate_last(void)
   {
      last.lr.x = -1;
      last.ul.x = 1;
      last.lr.y = -1;
      last.ul.y = 1;
   }

   // test for validity
   BOOL last_valid()
   {
      return RectWidth(&last) >= 0;
   }

   // set target rect, modifying eta as appropriate
   void set_target(const Rect* r, ulong ETA)
   {
      // are we going a different UL?
      if (obj == OBJ_NULL || !last_valid())
         last = *r;
      else if (target.ul.x != r->ul.x || target.ul.y != r->ul.y)
      {
         last_msec = 0;
         target_msec = ETA;
      }

      target = *r;
   }


   sRendState()
   {
      // zero out
      memset(this,0,sizeof(*this));
      invalidate_last();
   }
};

static sRendState gRendState[kNumItemSels];

static void interp_rect(Rect* out, const Rect* r1, long w1, const Rect* r2, long w2)
{
   long sum = w1+w2;
   out->ul.x = (r1->ul.x*w1 + r2->ul.x*w2)/sum;
   out->ul.y = (r1->ul.y*w1 + r2->ul.y*w2)/sum;
   out->lr.x = (r1->lr.x*w1 + r2->lr.x*w2)/sum;
   out->lr.y = (r1->lr.y*w1 + r2->lr.y*w2)/sum;
}


// @TODO: re-write this to use the non-existent string system, not edittools
static cStr get_object_name(ObjID obj)
{
   AutoAppIPtr(GameStrings);
   return pGameStrings->FetchObjString(obj,GAME_NAME_TABLE);
}

//
// Draw an object's name and count in its display rectangle.
//

void AnnotateObject(ObjID obj, int wid, int hgt)
{
   guiStyleSetupFont(NULL,StyleFontNormal);
   gr_set_fcolor(guiStyleGetColor(NULL,StyleColorFG));

   cStr namestr = get_object_name(obj);
   // un-const-ify it
   char* name = (char*)(const char*)namestr;
   short w, h;
   gr_font_string_wrap(gr_get_font(),name,wid);
   gr_string_size(name,&w,&h);
   // center the string at bottom
   short x = (wid - w)/2;
   short y = hgt - h;
   if (x < 0) x = 0;
   gr_string(name,x,y);

   gr_font_string_unwrap(name);

   // Now draw the object count
   cAutoIPtr<IIntProperty> pStackProp ( pContainSys->StackCountProp() );
   int cnt = 0;
   pStackProp->Get(obj,&cnt);
   if (cnt > 0)
   {
      char buf[16];
      itoa(cnt,buf,10);
      gr_string_size(buf,&w,&h);
      // top right corner
      gr_string(buf,3*wid/4-w,hgt/4);
   }

   guiStyleCleanupFont(NULL,StyleFontNormal);
}

//
// Actually render the transparent item and weapon selections
//

static void render_selection_display(ulong time)
{
   int i;

   for (i = 0; i < kNumItemSels; i++)
   {
      sRendState& state = gRendState[i];

      // no obj, no render state
      if (state.obj == OBJ_NULL)
      {
          gSelExpired[i]=NO_EXPIRATION; //blank items cannot expire.
          if (state.render)
          {
             invRendFreeState(state.render);
             state.render = NULL;
          }
          state.last_msec = 0;
          continue;
      }


      AutoAppIPtr_(ObjectSystem,pObjSys);
      AutoAppIPtr_(TraitManager,pTraitMan);

#ifdef PLAYTEST
      ObjID compass1_arch = pObjSys->GetObjectNamed("Compass");
      BOOL compass1=pTraitMan->ObjHasDonor(state.obj,compass1_arch);
#else
      #define compass1 FALSE
#endif
      ObjID compass2_arch = pObjSys->GetObjectNamed("Compass2");
      BOOL compass2=pTraitMan->ObjHasDonor(state.obj,compass2_arch);

      if (is_expired(i))
         continue;

      // translate rects
      if (time >= state.target_msec || !state.last_valid())
      {
         state.last = state.target;
         state.target_msec = 0;
         state.last_msec = 0;
      }
      else if (state.last_msec != 0)
      {
         ulong dt = time - state.last_msec;
         tSimTime remaining = state.target_msec - time;
         interp_rect(&state.last,&state.target,dt,&state.last,remaining);
      }

      state.last_msec = time;

      // compute canvas, perhaps this should be a GUISetup?
      const Rect& draw = state.last;
      Rect screen = { 0, 0, grd_canvas->bm.w, grd_canvas->bm.h } ;

      //      if (!RECT_TEST_SECT(&draw,&screen))

      // fix for sliding over to the other side of the screen
      if (!RECT_ENCLOSES(&screen,&draw))
         continue;

      grs_canvas* pcanv = gr_alloc_canvas(grd_canvas->bm.type,RectWidth(&draw),RectHeight(&draw));
      // @TODO: fix clipping (?) bug so that we can go to a sub-canvas
      // of grd_canvas rather than an off-screen canvas
      // What we really want is to render using hardware. To do this,
      // we need to set the flag INVREND_HARDWARE_QUEUE, _not_ render to a back buffer,

      // and be sure that our object really is a 3d model and no a faux bitmap object.

      gr_push_canvas(pcanv);
      gr_clear(0);
      gr_pop_canvas();

      if (compass1||compass2)  // why cant i set this once?
         state.flags&=~(INVREND_ROTATE|INVREND_DISTANT|INVREND_DIMMED);

      if (state.render)
         invRendUpdateState(state.render, state.flags, state.obj, &draw, NULL);
      else
         state.render = invRendBuildState(state.flags, state.obj, &draw, NULL);

#ifdef PLAYTEST
      if (compass1)
      {
         sInvRendView view;
         Camera *cam=PlayerCamera();
         invRendGetView(state.render,&view);
         view.off_ang.ty=-0x4000;
         view.off_ang.tz=-0x4000;
         view.cam_dist=0.65;
         invRendSetView(state.render,&view);
         float new_parms[6];
         new_parms[0]=(-(cam->ang.tz+0x8000))*360.0/65536.0;
	 new_parms[0]-=GetCompassOffset();
         ObjSetJointPos(state.obj,new_parms);
         gSelExpired[i] = NO_EXPIRATION;
      }
#endif

      if (compass2)
      {
         sInvRendView view;
         Camera *cam=PlayerCamera();
         int pitch_factor=(((int)((short)(cam->ang.ty)))/4);
#ifdef CAPPED_COMPASS
         if (pitch_factor<-0x600) pitch_factor=-0x600;
#else  // this is the "SMOOTHED" compass
         if (pitch_factor<-0x400)
            pitch_factor=((pitch_factor+0x400)*5)+(-0x400);
         else if (pitch_factor<-0x600)
            pitch_factor=((pitch_factor+0x600)*2)+(-0xE00);
         else if (pitch_factor>0x400)
            pitch_factor=((pitch_factor-0x400)*2)+(0x400);
#endif // CAPPED_COMPASS
         invRendGetView(state.render,&view);
         view.off_ang.tz = (cam->ang.tz+0x4000);
	 view.off_ang.tz += degrees_to_fixang(GetCompassOffset());
         view.off_ang.ty = -0x1200 - pitch_factor;
         view.cam_dist = 0.74;
         invRendSetView(state.render,&view);
         gSelExpired[i] = NO_EXPIRATION;
      }

      // draw the object
      invRendDrawUpdate(state.render);

      // draw the object name
      gr_push_canvas(pcanv);
      AnnotateObject(state.obj,RectWidth(&draw),RectHeight(&draw));
      gr_pop_canvas();

      pcanv->bm.flags |= BMF_TRANS;
      gr_set_cliprect(0,0,grd_canvas->bm.w,grd_canvas->bm.h);
      gr_bitmap(&pcanv->bm,draw.ul.x,draw.ul.y);
      gr_free_canvas(pcanv);
   }
}

// Dims of objects, in % of screen

static int gObjW = 20;
static int gObjH = 20;

// How long it takes a rect to get where it's going
static ulong gTransitTime = 300;
//
// Figure out what the hell it is we need to render and in what state
//

static void setup_selection_display(ulong time, short bottom_margin)
{
   int i;

   // compute dimensions
   int w = (grd_canvas->bm.w * gObjW)/100;
   int h = (grd_canvas->bm.h * gObjH)/100;

   for (i = 0; i < kNumItemSels; i++)
   {
      ObjID obj = pInventory->Selection((eWhichInvObj)i);

      // if we've changed, don't expire until later.
      if (obj != gRendState[i].obj)
      {
//really, this set_expiration is so redundantly called elsewhere... there
//isn't a lot of point in calling it AGAIN here.  This is to fix things
//reselecting when you come back from metagame.  

//so Now we call set_expiration only if the item hasn't expired, and it's
//changed.  And we newly set things (on EnterMode) to NO_EXPIRATION, so
//really they won't have expired yet, and this will make it all right.
//Or, if we really are changing to a new object, we make sure to set this.
//HACK!
	 if ((!is_expired(i)) || (gRendState[i].obj!=OBJ_NULL))
	   {
	     set_expiration(i);
	   }
         if (gToolFrobModal && i == kCurrentItem)
            gToolFrobCenter = FALSE;
      }
      gRendState[i].set_obj(obj);
      gRendState[i].flags = INVREND_ROTATE|INVREND_SET|INVREND_HARDWARE_QUEUE;
 
      //support for scripts knowing that things have timed out
      if (has_just_expired(i))
        FrobSendInvSelectionMsg(kFrobLocInv,kFrobLevFocus, obj, OBJ_NULL);
        
   }

   // special case: we have junk
   if (pInventory->WieldingJunk())
   {
      // slide weapon off the left edge
      Rect weapr = { -2*w, bottom_margin - h, -w, bottom_margin };
      gRendState[kCurrentWeapon].set_target(&weapr,time + gTransitTime);

      // Put the rect in the bottom middle of the screen
      Rect r;
      r.ul.x = (grd_canvas->bm.w - w)/2;
      r.lr.x = r.ul.x + w;
      r.lr.y = bottom_margin;
      r.ul.y = r.lr.y - h;

      gRendState[kCurrentItem].set_target(&r,time + gTransitTime);

   }
   else // Normal case
   {
      // Weapon goes on the left.
      if (gRendState[kCurrentWeapon].obj != OBJ_NULL)
      {
         Rect r = { 0, bottom_margin - h, w, bottom_margin };
         gRendState[kCurrentWeapon].set_target(&r,time + gTransitTime);
      }
      else
      {
         // teleport it off
         Rect r = { -2*w, bottom_margin - h, -w, bottom_margin };
         gRendState[kCurrentWeapon].set_target(&r,time - 1);
      }

      BOOL slide_off = TRUE;
      // Item goes on the right
      if (gRendState[kCurrentItem].obj != OBJ_NULL)
      {
         // Sadly, this computation is totally redundant in the modal tool
         // frob case.
         BOOL tool_frob = IsToolFrob(gRendState[kCurrentItem].obj,frobWorldSelectObj);

         slide_off = FALSE;

         // If a tool frob, put it right in the middle
         BOOL center = gToolFrobCenter;
         if (!gToolFrobModal)
            center = center && tool_frob;

         if (center)
         {
            Rect r = { (grd_canvas->bm.w - w)/2, (grd_canvas->bm.h - h)/2 };

            r.lr.x = r.ul.x + w;
            r.lr.y = r.ul.y + h;
            gRendState[kCurrentItem].set_target(&r,time + gTransitTime);
         }
         else
         {
            Rect r = { grd_canvas->bm.w - w, bottom_margin - h,
                       grd_canvas->bm.w, bottom_margin };
            gRendState[kCurrentItem].set_target(&r,time + gTransitTime);

            if (!tool_frob && frobWorldSelectObj)
               gRendState[kCurrentItem].flags = INVREND_SET|INVREND_HARDWARE_QUEUE|INVREND_DISTANT|INVREND_DIMMED;
         }
      }

      if (slide_off)
      {
         Rect r = { grd_canvas->bm.w + w, bottom_margin - h,
                    grd_canvas->bm.w + 2*w, bottom_margin };
         gRendState[kCurrentItem].set_target(&r,time - 1);
      }
   }

   // @TODO: make sure last rects are not below bottom margin

}

//------------------------------------------------------------
// STAT BAR RENDERING
//


typedef cStatusBar SB;

//
// Hitpoint bar
//


static sStatusBarDesc hp_desc =
{
   SB::kTransparent,
   { uiRGB(255,0,0) },  // colors
   0, // tolerance
   100, // refresh (ms)
};

// Horizontal extents, in 640,480 coords

enum { kExtentX = 640, kExtentY = 480, };
static int hp_x[2] = { 1, 275 };


class cHPBar : public cStatusBar
{
public:
   cHPBar()
      : cStatusBar(hp_desc), rends(NULL), nNumRends(0), bDidStart(FALSE)
   {
   RenderType.type = kInvRenderModel;
   strcpy(RenderType.resource.text,"shield2");
   }

   ~cHPBar()
   {
      if (rends)
         free(rends);
   }

   invRendState* BuildState()
   {
      invRendState *rend;
      Rect r = {0, 0, grd_canvas->bm.w, grd_canvas->bm.h};
      rend = invRendBuildStateFromType(nBaseFlags,&RenderType,&r,NULL);

      sInvRendView view;
      invRendGetView(rend,&view);
      view.off_ang.tz += ang*FIXANG_PI*0.005555555555556; // aka "/180";
      view.cam_dist *= 1.0*zoom*0.010000;  // aka "/100"

      invRendSetView(rend,&view);

      return rend;
   }

   void Start()
   {
      if (bDidStart)
         return;

      bDidStart = TRUE;

      Reset();
      nBaseFlags = INVREND_HARDWARE_QUEUE;
      if (config_is_defined("hpbar_spin"))
         nBaseFlags |= INVREND_ROTATE;

      config_get_raw("hpbar_model",RenderType.resource.text,sizeof(RenderType.resource.text));

      ang = 0;
      config_get_int("hpbar_ang",&ang);
      zoom = 100;
      config_get_int("hpbar_zoom",&zoom);
   }

   void Stop()
   {
      if (rends)
         {
         for (int i = 0; i < nNumRends; i++)
            invRendFreeState(rends[i]);
         free(rends);
         rends = 0;
         nNumRends = 0;
         }
   }

protected:
   invRendState **rends;
   int nNumRends;
   uint nBaseFlags;
   sInvRenderType RenderType;
   int ang;
   int zoom;
   BOOL bDidStart;

   BOOL MakeDrawCanvas()   { return FALSE; }

   void UpdateRends(int nNum)
   {
      int nStartIx = nNumRends;

      // Sad. We only grow.
      if (nNum <= nNumRends)
         return;

      if (!rends)
         rends = (invRendState **)malloc(sizeof(invRendState *)*nNum);
      else
         rends = (invRendState **)realloc(rends, sizeof(invRendState *)*nNum);

      nNumRends = nNum;
      for (int i = nStartIx; i < nNumRends; i++)
         rends[i] = BuildState();
   }

   void DoDraw(const Rect& fillme)
   {
      int val = mVal[kCur];
      int max = mVal[kMax];
      if (val <= 0) return;

      Start();

      sInvRendView view;
      int h = RectHeight(&fillme);
      int w = h;  // make them roughly square.  They might overlap, but who really cares..
      int rw = RectWidth(&mScreenRect) - w;
      int n = (max > 1) ? max - 1 : 1;
      Rect UseMe;
      UseMe = mScreenRect;

      // Make sure we have all we need.
      UpdateRends(val);

      invRendGetView(rends[0],&view); // just use first view since they will all be same.

      for (int i = 0; i < val; i++)
      {
         UseMe.ul.x = mScreenRect.ul.x+rw*i/n;
         UseMe.lr.x = UseMe.ul.x+w;

         view.lighting = 0.50 + 0.50*(i+1)/val;
         invRendSetView(rends[i],&view);
         invRendSetRect(rends[i],&UseMe);
         invRendUpdateState(rends[i], 0, OBJ_NULL, NULL, NULL);
         invRendDrawUpdate(rends[i]);
      }
   }


};

//
// AIR BAR
//

static sStatusBarDesc air_desc =
{
   SB::kTransparent|SB::kRtoL|SB::kHideAtMax,
   { uiRGB(0,0,255) },  // colors
   500, // tolerance
   1000, // refresh (ms)
};

// horizontal extents, in 640,480 coords
static int air_x[2] = { 365, 639 };

static const char* bubble_names[] = { "bubb1", "bubb2", "bubb3", "bubb4", "bubb5" };

#define NUM_BUBBLES (sizeof(bubble_names)/sizeof(bubble_names[0]))

class cAirBar : public cStatusBar
{
public:
   cAirBar() :cStatusBar(air_desc)
   {
   };

   void Start()
   {
      for (int i = 0; i < NUM_BUBBLES; i++)
      {
         bubble_src[i] = CreateResourceImageSource("intrface",bubble_names[i]);
         bubbles[i] = (grs_bitmap*)bubble_src[i]->Lock();
         Assert_(bubbles[i]);
      }
      config_get_int("airbar_tolerance",&tolerance);
      sparseness = 100;
      config_get_int("airbar_sparseness", &sparseness);
      if (sparseness < 1) sparseness = 1;
   }

   void Stop()
   {
      for (int i = 0; i < NUM_BUBBLES; i++)
      {
         bubble_src[i]->Unlock();
         SafeRelease(bubble_src[i]);
      }
   }

protected:

      int sparseness;
      IImageSource* bubble_src[NUM_BUBBLES];
      grs_bitmap* bubbles[NUM_BUBBLES];


   void DoDraw(const Rect& fillme)
   {
      if (mVal[kCur] <= 0) return;

      int x = fillme.ul.x;
      int h = RectHeight(&fillme);
      while (x < fillme.lr.x)
      {
         // pick a bubble
         int b = rand()%NUM_BUBBLES;
         // bias towards smaller bubbles
         if (bubbles[b]->h > h/2 || rand()%4 == 0)
            b = rand()%(b + 1);
         // keep trying until we get a bm that fits
         while (b > 0 && bubbles[b]->h > h)
            b = rand() % b; // pick one smaller than b


         grs_bitmap* bm = bubbles[b];
         int y = fillme.ul.y;
         if (bm->h < h) // pick a random height
            y += rand()%(h - bm->h+2) -1;

         // draw the bubble
         gr_bitmap(bm,x,y);

         // translate a random amount, no less than 1
         int max_move = 1 + bm->w * 100/sparseness;
         x += rand()%max_move + 1;
      }
   }


};

//
// Stat Bar objects
//


static cHPBar  hpbar;
static cAirBar airbar;

static void render_stat_bars(ulong time, int top_margin, int bot_margin)
{

   // Hitpoints

   int hp;
   if (ObjGetHitPoints(PlayerObject(),&hp))
      hpbar.SetVal(cStatusBar::kCur,hp);
   if (ObjGetMaxHitPoints(PlayerObject(),&hp))
      hpbar.SetVal(cStatusBar::kMax,hp);

#ifdef MONOCHROME_HPBAR
   int rgb[3] = { 255, 0, 0};
   int cnt = 3;
   config_get_value("inv_hpbar_color",CONFIG_INT_TYPE,(void*)rgb,&cnt);
   hpbar.colors[cStatusBar::kFG] = uiRGB(rgb[0],rgb[1],rgb[2]);
#endif

   int w = grd_canvas->bm.w;
   int h = grd_canvas->bm.h;

   Rect r= { hp_x[0]*w/kExtentX, top_margin, hp_x[1]*w/kExtentX, bot_margin };

   hpbar.SetScreenRect(r);
   hpbar.Update(time);

   // Air

   const sBreathConfig* cfg = GetObjBreathConfig(PlayerObject());

   if (cfg)
   {
      int air = GetObjAirSupply(PlayerObject());
      airbar.SetVal(cStatusBar::kCur,air);
      airbar.SetVal(cStatusBar::kMax,cfg->max_air);

      r.ul.x = air_x[0]*w/kExtentX;
      r.lr.x = air_x[1]*w/kExtentX;

      airbar.SetScreenRect(r);
      airbar.Update(time);
   }
}


static void reset_render_state(int i)
{
   sRendState& state = gRendState[i];
   state.set_obj(NULL);
   if (state.render)
   {
      invRendFreeState(state.render);
   }
   memset(&state,0,sizeof(state));
   // invalidate last rect
   state.invalidate_last();
}

////////////////////////////////////////////////////////////

static IFloatProperty* gpToolReachProp = NULL;


static sPropertyDesc tool_reach_desc =
{
   "ToolReach",
   0,  // flags
   NULL, //constraints
   0, 0, // version
   {
      "Inventory",
      "Tool Reach",
   }
};



////////////////////////////////////////////////////////////

BOOL enable_inv = TRUE;

static float gStatusH = 0.06; // %of screen
static float gStatusMargin = 1.0/480; // %of screen




void InvUIInit(void)
{
   pInventory = AppGetObj(IInventory);
   pContainSys = AppGetObj(IContainSys);
   COMMANDS(commands,HK_ALL);
   install_frob_CBs();


   IStringProperty* prop = CreateGameNameProperty();
   SafeRelease(prop);

   gpToolReachProp = CreateFloatProperty(&tool_reach_desc,kPropertyImplSparseHash);

   gpBeingTakenProp = CreateBeingTakenProperty();

   create_no_drop();

#ifdef NEW_NETWORK_ENABLED
   gpFrobPickupMsg = new cNetMsg(&sFrobPickupDesc);
#endif
}

void InvUITerm(void)
{
   SafeRelease(pContainSys);
   SafeRelease(pInventory);
   SafeRelease(gNoDropProp);
   SafeRelease(gpToolReachProp);
   SafeRelease(gpBeingTakenProp);

#ifdef NEW_NETWORK_ENABLED
   delete gpFrobPickupMsg;
#endif
}

void InvUIClearState()
{
   InvUIAbortAllFrobs();

   pInventory->ClearSelection(kCurrentWeapon);
   pInventory->ClearSelection(kCurrentItem);
}

void InvUIEnterMode(BOOL resuming)
{
   gItemStartTime = NO_START_TIME;
   gWeaponStartTime = NO_START_TIME;

   // Shut them all down, artoo!
   g_PickCurrentObj = OBJ_NULL;
   highlit_obj = OBJ_NULL;
   frobWorldSelectObj = OBJ_NULL;

   InvUIRefresh();

   if (!resuming)
     {
       memset(gSelExpired,0,sizeof(gSelExpired));

       //so this is kind of a hack.  We set the (nonexistent) items to not 
       //expire at the beginning.  Then the render_inventory display will note 
       //that the items haven't expired yet, and will reset their expirations.
       //Those items that HAVE expired will not have theirs reset...
       int i;
       for (i=0;i<kNumItemSels;i++) 
	 gSelExpired[i]=NO_EXPIRATION;

       gLastObjPickedUp = OBJ_NULL;
     }

   hpbar.Start();
   airbar.Start();
   VisMeterEnterMode();
   gToolFrobModal = !config_is_defined("disable_modal_tool_frob");

   if (!gToolFrobModal)
      gToolFrobCenter = config_is_defined("tool_frob_center");

   if (pInventory->WieldingJunk())
      set_expiration(kCurrentItem);

   if (!resuming)
      g_pPlayerMode->InstallOnDeathCallback(InvUIClearState);

   config_get_int("inv_obj_width",&gObjW);
   config_get_int("inv_obj_height",&gObjH);
   config_get_int("inv_transit_time",&gTransitTime);
   config_get_int("inv_item_timeout",&gSelTimeOut[kCurrentItem]);
   config_get_float("inv_status_height",&gStatusH);
   config_get_float("inv_status_margin",&gStatusMargin);

   config_get_int("inv_enabled",&enable_inv);
}

void InvUIRefresh(void)
{
   reset_render_state(kCurrentWeapon);
   reset_render_state(kCurrentItem);
}

void InvUIRefreshObj(ObjID obj)
{
   if (IsPlayerObj(pContainSys->GetContainer(obj)))
   {
      BOOL weapon = pInventory->GetType(obj) == kInvTypeWeapon;
      eWhichInvObj which = weapon ? kCurrentWeapon : kCurrentItem;
      reset_render_state(which);
	  set_expiration(which); //@HACK!  So that when you pick up something that combines
			              //but you've already timed out, it still works.  kill me now.
						  //All to fix one lousy bug.  AMSD
   }
}

void InvUIAbortAllFrobs(void)
{
   abort_frob(kCurrentWeapon);
   abort_frob(kCurrentItem);
}

void InvUIExitMode(BOOL suspending)
{
   InvUIRefresh();
   hpbar.Stop();
   airbar.Stop();
   VisMeterExitMode();
   gAutoFrob = FALSE;
   if (suspending)
      InvUIAbortAllFrobs();
   else
      InvUIClearState();
}

void InvUIRender(ulong msec)
{
   if (enable_inv)
   {
      ScrnLockDrawCanvas();

      // @TODO: compute top and bottom margins for real
      int bot = (int)(grd_canvas->bm.h * (1.0 - gStatusMargin));
      if (bot > grd_canvas->bm.h - 1) bot = grd_canvas->bm.h -1;
      int top = bot-(int)(grd_canvas->bm.h*gStatusH);
      render_stat_bars(msec,top,bot);

      // compute rect for vis meter
      Rect vis_r = { 0, top, 0, bot};
      vis_r.ul.x = hp_x[1]*grd_canvas->bm.w/kExtentX;
      vis_r.lr.x = air_x[0]*grd_canvas->bm.w/kExtentX;
      VisMeterUpdate(msec,&vis_r);

      setup_selection_display(msec,top);

      if (gToolFrobModal && gToolFrobCenter)
      {
         // we made it to the center, autofrob
         if (gAutoFrob && msec >= gRendState[kCurrentItem].target_msec)
            if (IsToolFrob(gRendState[kCurrentItem].obj,frobWorldSelectObj))
               use_item_command(FALSE);

         if (is_expired(kCurrentItem)) // we've been centered too long, uncenter
         {
            gToolFrobCenter = FALSE;
            set_expiration(kCurrentItem);
         }
      }

      render_selection_display(msec);
      ScrnUnlockDrawCanvas();
   }

   // Move the player's current item to his location.
   // This does't really belong in something called "render", but it works
   ObjID item = pInventory->Selection(kCurrentItem);
   if (item != OBJ_NULL)
   {
      ObjPos* pos = ObjPosGet(PlayerObject());
      if (pos)
         ObjPosCopyUpdate(item, pos);
   }
}

//
// Figure out if an object is hilight-able
//

BOOL ValidWorldFocusTarget(ObjID world)
{
   if (pInventory->WieldingJunk())
      return FALSE;
   //can't focus on objects when in remote camera mode.
   if (CameraIsRemote(PlayerCamera()))
     return FALSE;
   ObjID item = pInventory->Selection(kCurrentItem);

   if (item == OBJ_NULL || is_expired(kCurrentItem))
      return TRUE;

   float max_dist = 10000.0;
   // Use the pick dist of the item to limit tool frobs
   BOOL check_dist = !gToolFrobModal || gToolFrobCenter;
   if (check_dist && IsToolFrob(item,world) && gpToolReachProp->Get(item,&max_dist))
   {
      ObjPos* pos1 = ObjPosGet(PlayerObject());
      ObjPos* pos2 = ObjPosGet(world);
      if (pos1 && pos2)
      {
         float distsq = mx_dist2_vec(&pos1->loc.vec,&pos2->loc.vec);
         if (distsq > max_dist*max_dist)
            return FALSE;
      }
   }


#ifdef ITEM_OVERRIDES_WORLD
   // In modal tool frob mode, tools don't allow highlight unless they're centered.
   if (gToolFrobModal)
   {
      if (!gToolFrobCenter && IsTool(item))
         return FALSE;
   }

   // if items can expire, then they override world items
   if (gSelTimeOut[kCurrentItem] != 0 && !IsToolFrob(item,world))
      return FALSE;
#endif

   return TRUE;
}

