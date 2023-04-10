/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/


// $Header: r:/t2repos/thief2/src/motion/crwpnapi.cpp,v 1.2 2000/01/07 14:33:15 BODISAFA Exp $

// until this is propertized, this is completely shock/gamesys specific, sadly
// if it were propertized, that would be way less true

// in particular, we would want a nethandweapon property, or something
// which had all the data for a weapon converting from world to handheld
// then at init time, we would scan that property and build our table
// (which would just be obj->obj)

// at that point, we could pass that across, and the actual table data would
// be in the property.

// if we did that, we could probably make this truly ghost, and not have the
// shock dependancy we currently obviously have

#include <lg.h>
#include <comtools.h>
#include <appagg.h>

#include <crattach.h>
#include <crwpnlst.h>
#include <crwpnapi.h>
#include <linkman.h>
#include <linktype.h>
#include <relation.h>
#include <appagg.h>
#include <linkbase.h>
#include <matrix.h>
#include <mxmat.h>
#include <objsys.h>
#include <objdef.h>
#include <iobjsys.h>

#include <cretprop.h>
#include <weapon.h>
#include <objedit.h>
#include <mprintf.h>

#include <refsys.h>

#include <crjoint.h> // Range checking on joints.

#include <creature.h>   // CreatureFromObj

// Drop object
#include <physapi.h>
#include <prjctile.h>
#include <phnet.h>
#include <traitman.h>
#include <traitbas.h>
#include <objquery.h>
#include <aiapi.h>

#include <dbmem.h>

// for weapon object solving
int  (*CreatureWeaponObjSwizzle) (ObjID weapon, int mode) = NULL;
BOOL (*CreatureWeaponObjCreate)  (ObjID id, int mode)     = NULL;
void (*CreatureWeaponObjDestroy) (ObjID id)               = NULL;
void (*CreatureWeaponObjDrop)    (ObjID id)               = NULL;

// @TODO:  Bodisafa 12/9/1999
// This needs to be in Dark & DeepC.
BOOL CreatureWeaponCreate(ObjID id, int mode);
void CreatureWeaponDestroy(ObjID id);
int  CreatureWeaponSwizzle(ObjID id, int mode);
void CreatureWeaponDrop(ObjID id);

// Init/Term
void CreatureWeaponInit(void)
{
   // @TODO:  Bodisafa 11/30/1999
   // This needs to be in Dark & DeepC.
   CreatureWeaponObjCreate  = CreatureWeaponCreate;
   CreatureWeaponObjDestroy = CreatureWeaponDestroy;
   CreatureWeaponObjSwizzle = CreatureWeaponSwizzle;
   CreatureWeaponObjDrop    = CreatureWeaponDrop;
   
   CreatureWeaponListInit();
}

void CreatureWeaponTerm(void)
{
   CreatureWeaponListTerm();
}

// @TODO:  Bodisafa 11/30/1999
// ObjHasWeaponOffset checks should be reduced to the correct code flow.
// Otherwise, we're doing too many redundant checks.
// Assert in the other cases.

// Creature Weapon List

// GetWeaponIdForMode:  Returns the appropriate ObjID for the current mode.
ObjID GetWeaponIdForMode(ObjID id, int mode)
{
   if (id != OBJ_NULL && (ObjHasWeaponOffset(id)))
   {
      if (WeaponOffsetSetActive(id, mode))
      {
         int referenceID =  WeaponOffsetGetDest(id);
         if (referenceID != OBJ_NULL)
         {
            return referenceID;
         }
      }
   }
//   Warning(("GetWeaponIdForMode: No weapon for object '%d' in mode '%d'\n", id, mode));
   return OBJ_NULL;
}

// CreatureWeaponAttachWeapon: Hooks up creatures weapon for a given mode.
// Input:
// ppCreat     - Creature weapon information.
// referenceID - Uses this ID to construct a new weapon.
// mode        - What mode is the creature in?
void CreatureWeaponAttachWeapon(sCreatureWeapon **ppCreat, int referenceID, int mode)
{
   const sCreatureWeapon* pCW = *ppCreat;

   if (!ObjHasWeaponOffset(pCW->obj))
   {
      mprintf("CreatureWeaponAttachWeapon:  returning - !WeaponOffsetProp\n");
      return;
   }

   // Same weapon or we have this weapon attached.
   if (pCW->nWeapon == referenceID || (pCW->weaponObj == referenceID))
   {
//      Warning(("CreatureWeaponAttachWeapon:  returning - nothing to do\n"));
      return;
   }
   
   AutoAppIPtr_(ObjectSystem, pObjSys);

   // If we currently holding a weapon, destroy it.
   if (pCW->weaponObj != OBJ_NULL)
   {
      mprintf("CreatureWeaponAttachWeapon:  destroying (%d,%d)\n", (*ppCreat)->nWeapon, referenceID);
      pObjSys->Destroy(pCW->weaponObj);
      (*ppCreat)->weaponObj = OBJ_NULL;
   }
   
   (*ppCreat)->nWeapon = referenceID;
   (*ppCreat)->weaponObj = BeginObjectCreate(referenceID, kObjectConcrete);
   if ((*ppCreat)->weaponObj == OBJ_NULL)
   {
      Warning(("CreatureWeaponAttachWeapon: Failed to create weapon no object id %d\n", referenceID));
      return;
   }

   EndObjectCreate(pCW->weaponObj);

   sCreatureAttachInfo Info;
   memset(&Info,0,sizeof(Info));

   if (WeaponOffsetSetActive(pCW->obj, mode))
   {
      Info.joint = WeaponOffsetGetJoint(pCW->obj);
      if (Info.joint >= 0 && Info.joint < kCJ_NumCreatureJoints)
      {
         mxs_vector* pTransform = WeaponOffsetGetTransform(pCW->obj);

         Assert_(pTransform != NULL);
         mxs_trans trans = {pTransform[0].x, pTransform[0].y, pTransform[0].z,
                            pTransform[1].x, pTransform[1].y, pTransform[1].z,
                            pTransform[2].x, pTransform[2].y, pTransform[2].z,
                            pTransform[3].x, pTransform[3].y, pTransform[3].z};
   
         mx_copy_trans(&Info.relTrans, &trans);

         CreatureAttachItem(pCW->obj, pCW->weaponObj, &Info);

         // Creatures don't support weapons with physics.
         CreatureMakeWeaponNonPhysical(pCW->obj, pCW->weaponObj);
   
         // mprintf("Attach %s to %s\n",ObjWarnName(pCW->weaponObj),ObjWarnName(pCW->obj));
      }
   }
}

inline BOOL WeaponSwizzle(ObjID id, ObjID referenceID, int mode)
{
   // Nothing to do.
   if (!ObjHasWeaponOffset(id) || (referenceID == OBJ_NULL))
   {
      return FALSE;
   }
   
   // We're on the way to the deathbed...or at least we're not feeling better yet.
   cCreature* pCreat = CreatureFromObj(id);
   if (pCreat == NULL)
   {
      return FALSE;
   }

   IAI* pAI = AppGetObj(IAIManager)->GetAI(id);
   if (!pCreat->IsPhysical() || !PhysObjHasPhysics(id) || (pAI && pAI->IsDying()))
   {
      pAI->Release();
      return FALSE;
   }
   
   if (pAI)
   {
      pAI->Release();
   }

   sCreatureWeapon *pCW = CreatureWeaponGet(id);
   
   // Verify referenceID is valid and creature weapon is in the list.
   if (pCW != NULL)
   {
      CreatureWeaponAttachWeapon(&pCW, referenceID, mode);
   }
   else if (CreatureWeaponObjCreate)
   {
      (*CreatureWeaponObjCreate)(id, mode);
   }
   return TRUE;
}

int CreatureWeaponSwizzle(ObjID id, int mode)
{
   // Get creature's reference ID for this mode.
   ObjID referenceID = GetWeaponIdForMode(id, mode);
   if (referenceID)
   {
      return WeaponSwizzle(id, referenceID, mode);
   }
   return FALSE;
}

BOOL CreatureWeaponCreate(ObjID id, int mode)
{
   if (!ObjHasWeaponOffset(id))
   {
      return FALSE;
   }

   CreatureWeaponAdd(id);
   // Find and swizzle to this weapon.
   int referenceID = GetWeaponIdForMode(id, mode);
   if (referenceID == OBJ_NULL)
   {
      Warning(("CreatureWeaponCreate: Invalid referenceID for Object '%d'\n", id));
      return FALSE;
   }
   return WeaponSwizzle(id, referenceID, mode);
}

void CreatureWeaponDestroy(ObjID id)
{
   if (!ObjHasWeaponOffset(id))
   {
      return;
   }
   
   sCreatureWeapon *pCW = CreatureWeaponGet(id);
   if (pCW)
   {
      LinkID linkID = (g_pCreatureAttachRelation->GetSingleLink(id, pCW->weaponObj));
      if (linkID)
      {
         g_pCreatureAttachRelation->Remove(linkID);
      }

	   CreatureDetachItem(id, pCW->weaponObj);
	   DestroyObject(pCW->weaponObj);
	   pCW->weaponObj = NULL;
      CreatureWeaponRem(id);
   }
   
   // Totally safety - yes it is crude.
   ClearWeaponOffsetCache();
}

#ifdef DEEPC
// EXTERN void ThrowObj(ObjID o, ObjID src);
static BOOL DoThrowObj(ObjID thrower, ObjID obj, float power)
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

#endif // DEEPC

void CreatureWeaponDrop(ObjID id)
{
   if (ObjHasWeaponOffset(id))
   {
      sCreatureWeapon *pCW = CreatureWeaponGet(id);
      Assert_(pCW != NULL);

   // @TODO:  Bodisafa 12/7/1999
   // Game callback setup.

#ifdef DEEPC

      // Using Old & New for old & new weapon.
      IObjectQuery* query = AppGetObj(ITraitManager)->Query(pCW->weaponObj, kTraitQueryAllArchetypes);
      if (query && !query->Done())
      {
         // Grab Old's parent.
         query->Next();

         // We're at the top of the tree.  This should never happen.
         Assert_(!query->Done());

         // Grab the Old's parent object id.
         ObjID parentWeaponId = query->Object();
         
         // Create New as using Old's parent id.
         ObjID newWeaponId = BeginObjectCreate(parentWeaponId, kObjectConcrete);
         if (newWeaponId == OBJ_NULL)
         {
            Warning(("CreatureWeaponDrop: Failed to create throwing weapon object '%d'\n", newWeaponId));
            return;
         }
         EndObjectCreate(newWeaponId);
         
         // Throw New.
         DoThrowObj(id, newWeaponId, 0.5);
      }
      
      else
      {
         Warning(("CreatureWeaponDrop: No parent for weapon object '%d' \n", pCW->weaponObj));
      }

      // Get rid of Old.
      if (CreatureWeaponObjDestroy)
      {
         (*CreatureWeaponObjDestroy)(id);
      }

#else
      CreatureDetachItem(id, pCW->weaponObj);
      // throw_obj()
#endif // DEEPC
   }
}

///////////////////////
// warning
//   this has all essentially been desupported
//   in that the code above has changed to work in the real code
//   but none of this was changed for new calling conventions, names, etc...
// 
// Bodisafa 12/3/1999: This should be happy now.

#if 0

static void CreatureWeaponOutput(sCreatureAttachInfo *pInfo)
{
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[0].x,pInfo->relTrans.mat.vec[0].y,pInfo->relTrans.mat.vec[0].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[1].x,pInfo->relTrans.mat.vec[1].y,pInfo->relTrans.mat.vec[1].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[2].x,pInfo->relTrans.mat.vec[2].y,pInfo->relTrans.mat.vec[2].z);
}

static int nWeaponObjId;

static sCreatureWeapon CreatureWeapon;

void CreatureWeaponSetObj(int nObjid)
{
   nWeaponObjId    = nObjid;
   CreatureWeapon.obj = nObjid;
}

void 
CreatureWeaponRotX(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Rotate X by %g\n",vD);
   sMxMatrix matrix;

   mx_copy_mat(&matrix,&(pInfo->relTrans.mat));
   mx_rot_x_mat_rad(&(pInfo->relTrans.mat),&matrix,(vD/180.0)*MX_REAL_PI);
   
   CreatureWeaponOutput(pInfo);   
}


void 
CreatureWeaponRotY(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Rotate Y by %g\n",vD);
   sMxMatrix matrix;

   mx_copy_mat(&matrix,&(pInfo->relTrans.mat));
   mx_rot_y_mat_rad(&(pInfo->relTrans.mat),&matrix,(vD/180.0)*MX_REAL_PI);
   
   CreatureWeaponOutput(pInfo);
}

void CreatureWeaponRotZ(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Rotate Z by %g\n",vD);
   sMxMatrix matrix;

   mx_copy_mat(&matrix,&(pInfo->relTrans.mat));
   mx_rot_z_mat_rad(&(pInfo->relTrans.mat),&matrix,(vD/180.0)*MX_REAL_PI);

   CreatureWeaponOutput(pInfo);
}

void CreatureWeaponTransX(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Translate X by %g\n",vD);
   pInfo->relTrans.vec.x += vD;
}

void CreatureWeaponTransY(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Translate Y by %g\n",vD);
   pInfo->relTrans.vec.y += vD;
}

void CreatureWeaponTransZ(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Translate Z by %g\n",vD);
   pInfo->relTrans.vec.z += vD;
}

void
CreatureWeaponDump()
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentGet(CreatureWeapon.obj, nWeaponObjId, 0);
   if (pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   
   FILE *pFile = fopen("weapon.txt","a+");
   fprintf(pFile,"************************\n");
   fprintf(pFile,"%g,%g,%g\n",pInfo->relTrans.mat.vec[0].x,pInfo->relTrans.mat.vec[0].y,pInfo->relTrans.mat.vec[0].z);
   fprintf(pFile,"%g,%g,%g\n",pInfo->relTrans.mat.vec[1].x,pInfo->relTrans.mat.vec[1].y,pInfo->relTrans.mat.vec[1].z);
   fprintf(pFile,"%g,%g,%g\n",pInfo->relTrans.mat.vec[2].x,pInfo->relTrans.mat.vec[2].y,pInfo->relTrans.mat.vec[2].z);
   fprintf(pFile,"%g,%g,%g\n",pInfo->relTrans.vec.x,pInfo->relTrans.vec.y,pInfo->relTrans.vec.z);
   fclose(pFile);
}

#endif


