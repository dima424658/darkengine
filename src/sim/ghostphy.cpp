/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostphy.cpp,v 1.25 1999/08/26 13:25:22 ccarollo Exp $
// physics functions for ghosts

#include <ghost.h>
#include <ghostphy.h>
#include <ghostmvr.h>
#include <ghostlst.h>
#include <ghostapi.h> // for the extern of dereg
#include <ghostwep.h> // so we can unequip on exit

#include <minmax.h>   // duh

#include <physapi.h>
#include <phcore.h>
#include <phmods.h>   // phys models (all of them) methods (i.e. for the list)
#include <phmod.h>    // phys model methods (for an individual model)
#include <phprop.h>   // g_pPhysTypeProp
#include <phcontct.h> // reltoworld and worldtorel
#include <physcast.h> // the dread physcaster...
#include <phclimb.h>
#include <phconst.h>
#include <mclntapi.h> // MotSysCreateMotionCoordinator()
#include <creatext.h> // CreatureMakeBallistic()
#include <creatapi.h> // CreatureGetMotorInterface()
#include <netprops.h> // gAvatarHeightOffsetProp
#include <cretprop.h> // ObjGetCreatureType()
#include <netman.h>   // SuspendMessaging()
#include <plyrmode.h> // kPM_

#include <dbmem.h>

// for offset property
#define DEFAULT_Z_OFFSET (0) // (1.41)

////////////////////////
// actual do things to the underlying models/object/physics controls

BOOL GhostIsFloored(ObjID obj)
{  // @TODO: standing on moving terrain?
   sGhostRemote *pGR = GhostGetRemote(obj);
   if (pGR)       // dont do this if we are a MoCap only ghost!?
      if (PhysObjHasPhysics(obj))  // if no phys, sure, floor us
         if ((PhysGetGravity(obj) > 0.0) || ObjIsPosed(obj) ||
             (pGR->info.last.pos.mode == kGhostModeClimb))
            return FALSE;
   return TRUE;
}

void _GhostJumpOn(ObjID obj)
{
   PhysSetGravity(obj,1.0);
}

void _GhostJumpOff(ObjID obj)
{
   PhysSetGravity(obj,0.0);
   PhysSetBaseFriction(obj,1.0);
}

BOOL _GhostHaveGrav(ObjID obj)
{
   return PhysGetGravity(obj)!=0.0;
}

BOOL _GhostGravRemote(ObjID obj, sGhostPos *pPos, int cfg_flags)
{
   if (cfg_flags & kGhostCfIsObj)
   {
      cPhysModel *pModel = g_PhysModels.Get(obj);

      if (pModel == NULL)
         return FALSE;

      return((pPos->pos.flags & kGhostHBGravity) &&
              !PhysObjOnGround(obj) &&
              !PhysObjInWater(obj) && 
              !pModel->IsLocationControlled() &&
              pPos->pos.mode != kGhostModeSleep);
   }
   else
      return ((pPos->pos.mode == kPM_Jump) ||
              (pPos->pos.flags & kGhostHBGravity));
}

BOOL _GhostGravLocal(ObjID obj, sGhostPos *pPos, int cfg_flags)
{
   if (_GhostIsType(cfg_flags,IsObj))
   {
      cPhysModel *pModel = g_PhysModels.Get(obj);

      if (pModel == NULL)
         return FALSE;

      // We might want to remove the OnGround and InWater checks, so we don't
      // prevent our predictors from stopping gravity prematurely just because
      // locally we've stopped.

      return ((PhysGetGravity(obj) > 0.0) &&
              !pModel->IsLocationControlled() &&
              pPos->pos.mode != kGhostModeSleep);
   }
   else
   if (_GhostIsType(cfg_flags,AI))
      return FALSE;
   else
      return ((pPos->pos.mode == kPM_Jump) ||
              !PhysObjOnGround(obj));
}

void _GhostFixupGrav(sGhostRemote *pGR)
{
   BOOL t_grav=_GhostGravRemote(pGR->obj, &pGR->info.last, pGR->cfg.flags);
   if (t_grav!=_GhostHaveGrav(pGR->obj))
      if (t_grav)
         _GhostJumpOff(pGR->obj);
      else
         _GhostJumpOn(pGR->obj);
}

void _GhostRemoteRelObjPhys(ObjID ghost, ObjID rel_obj, ObjID last_rel_obj)
{
   if (rel_obj == OBJ_NULL)
   {
      if (last_rel_obj != OBJ_NULL)
         DestroyObjectContact(ghost, 0, g_PhysModels.Get(ghost), last_rel_obj, 2, g_PhysModels.Get(last_rel_obj));
   }
   else
      CreateObjectContact(ghost, 0, g_PhysModels.Get(ghost), rel_obj, 2, g_PhysModels.Get(rel_obj));
}

void _GhostPhysSleep(sGhostRemote *pGR)
{
   PhysStopControlVelocity(pGR->obj);

   // need to teleport when going to sleep
   mxs_vector world_targ_pos = pGR->info.last.pos.pos;
   _ghost_pos_reltoworld(pGR->obj,pGR->info.last.rel_obj,&world_targ_pos);
   _GhostTeleport(pGR->obj,NULL,&world_targ_pos);
}

void _GhostConfigureRemoteModels(sGhostRemote *pGR)
{
   ObjID obj = pGR->obj;

   PhysSetGravity(obj,0.0);
   PhysSetBaseFriction(obj,2.0);

   _GhostSetupInitialPhysRemote(pGR);

   if (_GhostIsType(pGR->cfg.flags,Player))
      PhysSetFlag(obj,kPMF_Avatar,TRUE);
}

void (*GhostInstallFireListenFunc)(ObjID obj) = NULL;

void _GhostBuildMotionCoord(sGhostRemote *pGR)
{
   int creatureType;
   if (ObjGetCreatureType(pGR->obj, &creatureType))
   {
      if (pGR->critter.pMotCoord==NULL)
      {
         pGR->critter.pMotCoord=MotSysCreateMotionCoordinator();
         IMotor *pMotor = CreatureGetMotorInterface(pGR->obj);
         if (pMotor==NULL)
            Warning(("Hey, couldnt create motor for %s\n",ObjWarnName(pGR->obj)));
         pGR->critter.pMotCoord->SetMotor(pMotor);
      }
      // The Posed style doesn't muck with the physics models so much
      CreatureMakeBallistic(pGR->obj, kCMB_Posed);

      // players - Init to no weapon in hand and listener for motion flags
      if (_GhostIsType(pGR->cfg.flags,Player)&&GhostInstallFireListenFunc)
         GhostInstallFireListenFunc(pGR->obj);
   }
   else
   {  // It's not actually a creature, so stub things out
      pGR->critter.pMotCoord = NULL;
      pGR->cfg.flags |= kGhostCfNoCret;
   }
}

// We can't build the physics for creatures until the motor exists:
BOOL _GhostAllowedToBuildModels(sGhostRemote *pGR)
{
   int creatureType;
   if (ObjGetCreatureType(pGR->obj, &creatureType))
   {
      // Note that, despite being an interface, this doesn't actually
      // cause a ref, so we don't release it:
      IMotor *pMotor = CreatureGetMotorInterface(pGR->obj);
      if (pMotor == NULL)
         return FALSE;
      else
         return TRUE;
   } else {
      // It's not a creature, so there shouldn't be a problem:
      return TRUE;
   }
}

// Undo all of the above horrors that we've done to the physics model.
// Needed for making this thing into a local ghost.
void _GhostRepairRemoteModels(sGhostRemote *pGR)
{
   AutoAppIPtr(ObjectSystem);
   if (!pObjectSystem->Exists(pGR->obj))
      // We don't need to do anything to repair the models of something
      // that doesn't exist...
      return;

   // Put the physics back to whatever they should normally inherit:
   if (_GhostIsType(pGR->cfg.flags,IsObj))
   {  // We don't need to do much to this:
      if (PhysObjHasPhysics(pGR->obj))
      {
         PhysSetFlag(pGR->obj, kPMF_TrueGhost, FALSE);
      }
   }
   else if (pGR->cfg.flags & kGhostCfNoCret)
   {
      g_pPhysTypeProp->Delete(pGR->obj);
      g_pPhysTypeProp->Create(pGR->obj);
   }
   else
   {
      // @TBD: does this need a test in case the creature is *supposed*
      // to be ballistic?
      CreatureMakeNonBallistic(pGR->obj);
      // @HACK: Unfortunately, we don't want these physics changes to be
      // networked; they proceed to screw up the nicely-setup ghosts on
      // the other side. So we specifically don't network these calls.
      // There surely must be a more elegant way to fix this bug, but
      // damned if I know what it is...
      AutoAppIPtr(NetManager);
      pNetManager->SuspendMessaging();
      CreatureMakeNonPhysical(pGR->obj);
      CreatureMakePhysical(pGR->obj);
      pNetManager->ResumeMessaging();
   }
   if (pGR->weap.nWeapon!=-1)  // need a better way to do this...
   {
      if (_ghost_track_weapons())
         _ghost_mprintf(("Detach in RepairRemoteModels, %s had a %s...\n",ObjWarnName(pGR->obj),ObjWarnName(pGR->weap.weaponObj)));
      if (GhostWeaponObjAttach)
         (*GhostWeaponObjAttach)(pGR->obj,-1);
   }
}

void _GhostFindValidLoc(ObjID ghost, mxs_vector *pDesired, mxs_vector *pLast, mxs_vector *pResult)
{
   PhysSetModLocation(ghost, pDesired);
   mx_copy_vec(pResult, pDesired);

   // test desired
   if (PhysObjValidPos(ghost, NULL))
   {
      if (_ghost_watch_validloc())
         _ghost_mprintf(("%s (desired) Valid loc found at %g %g %g\n", ObjWarnName(ghost), pResult->x, pResult->y, pResult->z));
      return;
   }

   // test last
   PhysSetModLocation(ghost, pLast);
   mx_copy_vec(pResult, pLast);

   if (PhysObjValidPos(ghost, NULL))
   {
      if (_ghost_watch_validloc())
         _ghost_mprintf(("%s (last) Valid loc found at %g %g %g\n", ObjWarnName(ghost), pResult->x, pResult->y, pResult->z));
      return;
   }

   // try moving along major axes
   float dist;
   int x, y, z;

   mxs_vector test_vec;
   mxs_matrix ident;
   mx_identity_mat(&ident);

   for (dist = 0.2; dist < 1.01; dist += 0.2)
   {
      for (x=0; x<2; x++)
      {
         for (y=0; y<2; y++)
         {
            for (z=0; z<2; z++)
            {
               test_vec = *pDesired;

               mx_scale_addeq_vec(&test_vec, &ident.vec[0], dist * ((x==0) ? 1 : -1));
               mx_scale_addeq_vec(&test_vec, &ident.vec[1], dist * ((y==0) ? 1 : -1));
               mx_scale_addeq_vec(&test_vec, &ident.vec[2], dist * ((z==0) ? 1 : -1));
                        
               PhysSetModLocation(ghost, &test_vec);
               mx_copy_vec(pResult, &test_vec);

               if (PhysObjValidPos(ghost, NULL))
               {
                  if (_ghost_watch_validloc())
                     _ghost_mprintf(("%s (%d %d %d) Valid loc found at %g %g %g\n", 
                                     ObjWarnName(ghost), x, y, z, pResult->x, pResult->y, pResult->z));
                  return;
               }
            }
         }
      }
   }

   if (_ghost_watch_validloc())
      _ghost_mprintf(("Couldn't find valid location for %s\n", ObjWarnName(ghost)));
}

void _GhostSetupInitialPhysRemote(sGhostRemote *pGR)
{
   cPhysModel *pModel = g_PhysModels.Get(pGR->obj);

   if (pModel == NULL)
      return;

   if (_GhostIsType(pGR->cfg.flags,IsObj))
      PhysSetFlag(pGR->obj,kPMF_TrueGhost,TRUE);
   else
      PhysSetFlag(pGR->obj,kPMF_FancyGhost,TRUE);

   // lets not put all the AIs and players to dont run, eh?
   if (_GhostIsType(pGR->cfg.flags,IsObj))   
      if (pModel->IsLocationControlled())
      {
         pGR->cfg.flags|=kGhostCfDontRun;
         if (_ghost_watch_events())
            _ghost_mprintf(("Ghost %s initted to sleep\n",ObjWarnName(pGR->obj)));
      }

   if (_GhostIsType(pGR->cfg.flags,AI) || _GhostIsType(pGR->cfg.flags,Player))
      PhysSetFlag(pGR->obj,kPMF_Special,TRUE);
}

void _GhostSetupInitialPhysWeap(sGhostRemote *pGR)
{
   if (_GhostIsType(pGR->cfg.flags,Player)&&GhostWeaponObjAttach)
   {
      if (_ghost_track_weapons())
         _ghost_mprintf(("SetupInitialPhysRemote detach weapon for %s\n",ObjWarnName(pGR->obj)));
      (*GhostWeaponObjAttach)(pGR->obj,-1);
   }
}

void _GhostSetupInitialPhysLocal(sGhostLocal *pGL)
{
   // Disable if it's location controlled
   cPhysModel *pModel = g_PhysModels.Get(pGL->obj);
   if (pModel == NULL)
      return;

   if (_GhostIsType(pGL->cfg.flags,IsObj))
      if (pModel->IsLocationControlled())
         pGL->cfg.flags|= kGhostCfDontRun;
}

////////////////////////
// position/relobj code/decode

void _ghost_vel_worldtorel(ObjID rel_obj, mxs_vector *vel)
{
   if (rel_obj == NULL)
      return;

   mxs_vector tvel;
   PhysGetVelocity(rel_obj, &tvel);
   mx_subeq_vec(vel, &tvel);
}
  

// acknowledge wacky phys model center/obj center/mocap center/center hell
void _ghost_pos_centerconvert(ObjID ghost, mxs_vector *pos)
{
   float offset=DEFAULT_Z_OFFSET;
   gAvatarHeightOffsetProp->Get(ghost,&offset);
   pos->z+=offset;
}

// takes a ghost and its world position
// if we are in contact w/MovTerr
// set *rel_obj to the terrain object, *pos to the reltive position
BOOL _ghost_pos_worldtorel(ObjID ghost, ObjID *rel_obj, mxs_vector *pos, int ghost_flags)
{
   cPhysSubModelInst *pContacts;

   int submod;

   if (_GhostIsType(ghost_flags,Player))
      submod = PLAYER_FOOT;
   else
      submod = 0;

   if (GetObjectContacts(ghost, submod, g_PhysModels.Get(ghost), &pContacts))
   {
      while (pContacts != NULL)
      {
         if (pContacts->GetPhysicsModel()->IsMovingTerrain())
         {
            *rel_obj = pContacts->GetObjID();
            mx_sub_vec(pos, &ObjPosGet(ghost)->loc.vec, &ObjPosGet(*rel_obj)->loc.vec);
            return TRUE;
         }
         
         pContacts = pContacts->GetNext();
      }
   }

   *rel_obj = OBJ_NULL;
   return FALSE;
}

// give ghost, and the obj is it rel_obj too, and its pos (world or rel)
// if rel_obj != NULL, pos is a rel pos
// so this should make pos a world pos again
// or do whatever the hell we are going to do to deal w/moving terrain
BOOL _ghost_pos_reltoworld(ObjID ghost, ObjID rel_obj, mxs_vector *pos)
{
   if (rel_obj != OBJ_NULL)
   {
      mx_addeq_vec(pos, &ObjPosGet(rel_obj)->loc.vec);
      return TRUE;
   }

   return FALSE;
}

///////////////////////
// misc helper funcs

BOOL _GhostFacingGround(ObjID obj, const mxs_vector *vel)
{
   cPhysModel *pModel = g_PhysModels.Get(obj);   
   cFaceContactList *pFaceContactList;
   
   if (pModel->GetFaceContacts(0, &pFaceContactList))
   {
      cFaceContact *pFaceContact = pFaceContactList->GetFirst();
      
      while (pFaceContact)
      {
         if (mx_dot_vec(vel,&pFaceContact->GetNormal())<=0)
            return TRUE;
         pFaceContact = pFaceContact->GetNext();
      }
   }
   return FALSE;
}

void _GhostBleedVelocity(ObjID obj, sGhostPos *pPred, int cfg_flags, int last_time, float dt)
{
   float bleed_factor=1.0;
   if (pPred->pos.mode == kGhostModeDead)
      bleed_factor = 5.0;
   else if (pPred->pos.mode == kGhostModeSleep)
      return;
   else if ((pPred->pos.flags&kGhostHBAbsVel)==0)    // dont bleed off control velocities
      return;
   else if (_GhostIsType(cfg_flags,IsObj))
   {  // attempt to only do this if facing the ground, eh?
      if (PhysObjOnGround(obj)&&_GhostFacingGround(obj, &pPred->pos.vel))
         bleed_factor = 15.0;  // bleed more if we're on the ground, and none if we're not
      else
         return;
   }
   else if (pPred->pos.flags&kGhostHBSlowDown)
      bleed_factor=3.0;
   else
      return;

   float bleed_amt = min(1.0, bleed_factor * (dt / 2));

   mx_scaleeq_vec(&pPred->pos.vel,1.0-bleed_amt);
}

BOOL _GhostTeleport(ObjID ghost, mxs_vector *delta, const mxs_vector *target)
{
   mxs_vector local_delta;
   if (delta==NULL)
   {
      mxs_vector *cur=&ObjPosGet(ghost)->loc.vec;
      mx_sub_vec(&local_delta,target,cur);
      delta=&local_delta;
   }
#ifdef GHOST_DEBUGGING
   if (_ghost_warnings())
   {
      float delta_dist_2=mx_mag2_vec(delta);
      if (delta_dist_2>32*32)
         _ghost_mprintf(("%s: Ghost Long Teleport by %4g %4g %4g to %d %d %d\n",ObjWarnName(ghost),delta->x,delta->y,delta->z,(int)target->x,(int)target->y,(int)target->z));
   }
#endif   
   if (PhysObjValidPos(ghost,delta))
   {
      ObjTranslate(ghost,target);
      if (_ghost_loud_teleport())
         _ghost_mprintf(("%s: Ghost Teleport by %g %g %g to %d %d %d\n",ObjWarnName(ghost),delta->x,delta->y,delta->z,(int)target->x,(int)target->y,(int)target->z));
      return TRUE;
   }
   else    // really, should translate to a known "good" position from last, i guess
      Warning(("%s: Ghost Teleport bad to %g %g %g\n",ObjWarnName(ghost),target->x,target->y,target->z));
   return FALSE;
}

///////////////////////
// and the physcaster hookup to the approx system...

// using state in netPos, and current data in ghostPos, incrementally updates ghostPos by dt
void _GhostApproxPhys(ObjID obj, sGhostPos *netPos, sGhostPos *ghostPos, float dt, BOOL gravity)
{
   mxs_vector delta_pos, try_pos;
   sGhostPos *pPos=ghostPos;

   if (gravity)
      mx_scale_addeq_vec(&pPos->pos.vel,&kGravityDir,kGravityAmt*dt);

   if (PhysObjInWater(obj))
      mx_scale_addeq_vec(&pPos->pos.vel,&kGravityDir,-(kGravityAmt * dt) / (PhysGetDensity(obj) * PhysGetMass(obj)));

   if ((pPos->pos.flags&kGhostHBAbsVel)==0)
   {  // this is where we do control vel, i guess?
      mxs_vector delta_v;
      mx_sub_vec(&delta_v,&netPos->pos.vel,&pPos->pos.vel); // delta to desired control velocity
      float scale_fac=dt/0.150;
      if (scale_fac>=1.0) scale_fac=1.0;
      mx_scale_addeq_vec(&pPos->pos.vel,&delta_v,scale_fac);
   }
      
   mx_scale_vec(&delta_pos,&pPos->pos.vel,dt);  // this is dumb
   mx_add_vec(&try_pos,&pPos->pos.pos,&delta_pos);

#ifdef DBG_ON   
   if (!config_is_defined("no_physcast"))
#endif      
   {
      // first, we should cast from old->pos by delta_pos, to see if we can
      Location sourceLoc, targetLoc, hitLoc, *objLoc=&ObjPosGet(obj)->loc;
      ObjID hitObjID=OBJ_NULL;
      
      // hint off the objpos, i think...
      MakeHintedLocationFromVector(&sourceLoc,&pPos->pos.pos,objLoc);
      MakeHintedLocationFromVector(&targetLoc,&try_pos,objLoc);
      
      // physcast joy....
      int hit_result=PhysRaycast(sourceLoc,targetLoc,&hitLoc,&hitObjID,0.0,kCollideTerrain|kCollideOBB);
      if (hit_result!=kCollideNone)
      {
         if (_ghost_watch_physcast())
            _ghost_mprintf(("Hit %s: giving up on %g %g %g gonna do %g %g %g\n",ObjWarnName(hitObjID),
                            try_pos.x,try_pos.y,try_pos.z,hitLoc.vec.x,hitLoc.vec.y,hitLoc.vec.z));

         if (_is_zero_vec(&hitLoc.vec))
         {
            if (_ghost_watch_physcast())
               _ghost_mprintf(("Hey! raycaster gave us zero vec\n"));
            try_pos=pPos->pos.pos;  // so just dont move... 
         }
         else
            try_pos=hitLoc.vec;     // i guess?
      }
   }

   // the only time we do a heading approx with vel is when we're not using
   // full angles
   if ((ghostPos->pos.flags & kGhostHBFullAngs) == 0)
      ghostPos->pos.angle_info.tz += FIXVEL_TO_DTANG(ghostPos->pos.angle_info.dtz,(long)(dt*1000));

   // ok, lets do the job
   pPos->pos.pos=try_pos;    // hah- take that line of code, eh?
}

BOOL _is_zero_vec(const mxs_vector *vec)
{
   return ((*((int *)&vec->el[0])|*((int *)&vec->el[1])|*((int *)&vec->el[2]))&(~0x80000000))==0;
}
