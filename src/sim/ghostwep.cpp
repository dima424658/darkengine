/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostwep.cpp,v 1.3 1999/08/13 16:15:29 dc Exp $

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

#include <crattach.h>
#include <ghost.h>
#include <ghostapi.h>
#include <ghostlst.h>
#include <ghostrcv.h>
#include <ghostsnd.h>
#include <ghostmsg.h>
#include <ghostwep.h>
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
#include <traitman.h>

#include <dbmem.h>

#define kChargeOff (1<<0)
#define kChargeOn  (1<<1)
#define kChargeAny (kChargeOff|kChargeOn)

struct sGhostWeapon {
   char szObjName[32];      // ummm, alignment, maybe?
   char szGhostObjName[32];
   float vScale;
   int charge_flags;
   mxs_trans trans;
};

static struct sGhostWeapon GhostWeaponMod[] =
{
   {"Pistol","Avatar Pistol", 0.6, kChargeAny,
      { 0,0,-1,
      0,1,0,
      1,0,0,
      0.2,-0.6,0}},
   {"Assault Rifle","Avatar Rifle", 0.5, kChargeAny,
      { 0,0,-1,
      0,1,0,
      1,0,0,
      0.2,-0.9,0}},
   {"Shotgun","Avatar Shotgun", 0.5, kChargeAny,
      { 0,0,1,
      0,-1,0,
      1,0,0,
      0.1,-1.1,-0.1}},
   {"Hybrid Shotgun","Avatar Shotgun", 0.5, kChargeAny,
      { 0,0,1,
      0,-1,0,
      1,0,0,
      0.1,-1.1,-0.1}},
   {"Wrench","Avatar Wrench", 0.5, kChargeAny,
      { 0,1,0,
      0,0,1,
      1,0,0,
      0.1,-1.2,-0.1}},
   {"Laser Pistol","Avatar Laser", 0.5, kChargeAny,
      { 0,1,0,
      0,0,1,
      1,0,0,
      0.2,-0.7,-0.1}},
   {"EMP Rifle","Avatar EMP", 0.5, kChargeAny,
      { -3.65277e-008,1,-1.67635e-009,
      2.18557e-008,7.94827e-018,1,
      1,3.35269e-009,-2.37071e-009,
      0.2,-1,0}},
   {"Electro Shock","Avatar Electro", 0.5, kChargeAny,
      { 1,0,0,
      0,-2.37071e-009,1,
      0,-1,-2.37071e-009,
      0.1,-1.1,0}},
   {"Gren Launcher","Avatar Grenade", 0.5, kChargeAny,
      { 6.40025e-008,2.37071e-009,-1,
      0,1,2.37071e-009,
      1,-5.62028e-018,2.37071e-009,
      0.3,-1.1,-0.1}},
   {"Stasis Field Generator","Avatar Stasis", 0.3, kChargeAny,
      { -2.18557e-008,4.9099e-010,1,
      1.67635e-009,-1,1.67635e-009,
      1,2.8617e-009,2.37071e-009,
      0.2,-0.7,0.1}},
   {"Fusion Cannon","Avatar Fusion", 0.5, kChargeAny,
      { 6.40025e-008,-1,-2.37071e-009,
      0,2.37071e-009,-1,
      1,2.37071e-009,5.62028e-018,
      0.4,-1,-0.1}},
   {"Worm Launcher","Avatar Worm", 0.5, kChargeAny,
      { -2.02912e-008,-5.62028e-018,1,
      -6.40025e-008,-1,-5.62028e-018,
      1,-2.37071e-009,-2.37071e-009,
      -0.3,-0.3,0}},
   {"Crystal Shard","Avatar Shard", 1.0, kChargeAny,
      { 6.88394e-008,-1,-4.38232e-008,
      0.707107,-5.62028e-018,-0.707107,
      0.707107,-2.37071e-009,0.707107,
      0.3,-2,0.2 }},
   {"Viral Prolif","Avatar Prolif", 0.5, kChargeAny,
      { 2.3259e-007,-2.37071e-009,1,
      2.02912e-008,-1,-2.37071e-009,
      1,-2.37071e-009,-2.37071e-009,
      0.3,-0.8,0}},                                
   {"Psi Amp","Avatar Amp", 0.5, kChargeOff,
      { 0.707107,-0.5,-0.5,
      -1.67635e-009,-0.707107,0.707107,
      -0.707107,-0.5,-0.5,
      0.2,-0.4,0}},
   {"Psi Amp","Avatar Charging Amp", 0.5, kChargeOn,
      { 0.707107,-0.5,-0.5,
      -1.67635e-009,-0.707107,0.707107,
      -0.707107,-0.5,-0.5,
      0.2,-0.4,0}},
   // The psi sword is currently a laser shock. This will have to be modified.
   {"PsiSword", "Avatar PsiSword", 0.5, kChargeAny,
      { -2.02912e-008,-2.37071e-009,1,
      -1,-2.37071e-009,2.37071e-009,
      2.37071e-009,-1,-2.37071e-009,
      0.2,-1.6,0}},
};

int GhostGetWeaponIdCharging(ObjID o, BOOL charging)
{
   if (o==OBJ_NULL) return -1;
   AutoAppIPtr_(ObjectSystem,pObjSys);
   
   int i, targ_charge=charging?kChargeOn:kChargeOff;
   ObjID name_obj=o;

   AutoAppIPtr_(TraitManager,pTrait);
   while ((name_obj!=OBJ_NULL) && (OBJ_IS_CONCRETE(name_obj)))
      name_obj = pTrait->GetArchetype(name_obj);
   const char *szName = pObjSys->GetName(name_obj);

   if (szName==NULL)
   {
      Warning(("Couldnt get name for %s\n",ObjWarnName(name_obj)));
      return -1;
   }

   for (i=0;i<(sizeof(GhostWeaponMod)/sizeof(GhostWeaponMod[0]));i++)
      if (strcmp(GhostWeaponMod[i].szObjName,szName) == 0)
         if (GhostWeaponMod[i].charge_flags&targ_charge)
            return i;
   
   return -1;
}

int GhostGetWeaponId(ObjID o)
{
   return GhostGetWeaponIdCharging(o, FALSE);
}

ObjID GhostGetWeaponArch(int id)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);

   return pObjSys->GetObjectNamed(GhostWeaponMod[id].szObjName);
}

// set the weapon attached to a given creature...
void GhostAttachWeapon(sGhostRemote *pCreat, int nWeapon)
{
   if (_ghost_track_weapons())
      _ghost_mprintf(("Called GAW for %s old %d %d, passed %d\n",ObjWarnName(pCreat->obj),
                      pCreat->weap.nWeapon,pCreat->weap.weaponObj,nWeapon));
   
   if (pCreat->weap.nWeapon == nWeapon)
      return;
   
   AutoAppIPtr_(ObjectSystem,pObjSys);

   if (pCreat->weap.weaponObj != OBJ_NULL)
   {
      //mprintf("GhostWep destroy %s\n",ObjWarnName(pCreat->weap.weaponObj));
      pObjSys->Destroy(pCreat->weap.weaponObj);
      pCreat->weap.weaponObj = OBJ_NULL;
   }
   
   pCreat->weap.nWeapon = nWeapon;

   // -1 or lower means no weapon equiped.
   if (nWeapon < 0)
      return;
   
   AssertMsg1(nWeapon < (sizeof(GhostWeaponMod)/sizeof(GhostWeaponMod[0])),
			  "Ghost set weapon: Weapon %d is more than the maximum weapon value",nWeapon);
   
   if(nWeapon >= (sizeof(GhostWeaponMod)/sizeof(GhostWeaponMod[0])) )
      return;

   struct sGhostWeapon *pGhostWeapon = GhostWeaponMod+nWeapon;

   int nObjID = pObjSys->GetObjectNamed(pGhostWeapon->szGhostObjName);
   if(nObjID == OBJ_NULL)
   {
      Warning(("GhostSetWeapon: Could not find a weapon named '%s'\n",pGhostWeapon->szGhostObjName));
      return;
   }

   pCreat->weap.weaponObj = BeginObjectCreate(nObjID,kObjectConcrete);
   if(pCreat->weap.weaponObj == OBJ_NULL)
   {
      Warning(("GhostSetWeapon: Failed to create weapon no %d, object %d did not create\n",nWeapon,nObjID));
      return;
   }

   EndObjectCreate(pCreat->weap.weaponObj);

   sCreatureAttachInfo Info;
   memset(&Info,0,sizeof(Info));

   Info.joint = 10;
   mx_copy_trans(&Info.relTrans,&(pGhostWeapon->trans));

   CreatureAttachItem(pCreat->obj,pCreat->weap.weaponObj,&Info);

   // mprintf("Attach %s to %s\n",ObjWarnName(pCreat->weap.weaponObj),ObjWarnName(pCreat->obj));
}

///////////////////////
// warning
//   this has all essentially been desupported
//   in that the code above has changed to work in the real code
//   but none of this was changed for new calling conventions, names, etc...

#if 0

int nWeaponObjId;

static sGhostRemote GhostRemote;

void GhostWeaponSetObj(int nObjid)
{
   nWeaponObjId = nObjid;
   GhostRemote.obj = nObjid;
}

void GhostWeaponSet(int nWeapon)
{
   GhostAttachWeapon(&GhostRemote,nWeapon);
}   

void 
GhostWeaponRotX(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Rotate X by %g\n",vD);
   sMxMatrix matrix;

   mx_copy_mat(&matrix,&(pInfo->relTrans.mat));
   mx_rot_x_mat_rad(&(pInfo->relTrans.mat),&matrix,(vD/180.0)*MX_REAL_PI);
   
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[0].x,pInfo->relTrans.mat.vec[0].y,pInfo->relTrans.mat.vec[0].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[1].x,pInfo->relTrans.mat.vec[1].y,pInfo->relTrans.mat.vec[1].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[2].x,pInfo->relTrans.mat.vec[2].y,pInfo->relTrans.mat.vec[2].z);
}

void 
GhostWeaponRotY(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Rotate Y by %g\n",vD);
   sMxMatrix matrix;

   mx_copy_mat(&matrix,&(pInfo->relTrans.mat));
   mx_rot_y_mat_rad(&(pInfo->relTrans.mat),&matrix,(vD/180.0)*MX_REAL_PI);
   
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[0].x,pInfo->relTrans.mat.vec[0].y,pInfo->relTrans.mat.vec[0].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[1].x,pInfo->relTrans.mat.vec[1].y,pInfo->relTrans.mat.vec[1].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[2].x,pInfo->relTrans.mat.vec[2].y,pInfo->relTrans.mat.vec[2].z);
   
}

void GhostWeaponRotZ(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Rotate Z by %g\n",vD);
   sMxMatrix matrix;

   mx_copy_mat(&matrix,&(pInfo->relTrans.mat));
   mx_rot_z_mat_rad(&(pInfo->relTrans.mat),&matrix,(vD/180.0)*MX_REAL_PI);

   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[0].x,pInfo->relTrans.mat.vec[0].y,pInfo->relTrans.mat.vec[0].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[1].x,pInfo->relTrans.mat.vec[1].y,pInfo->relTrans.mat.vec[1].z);
   mprintf("%g,%g,%g\n",pInfo->relTrans.mat.vec[2].x,pInfo->relTrans.mat.vec[2].y,pInfo->relTrans.mat.vec[2].z);
}

void GhostWeaponTransX(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Translate X by %g\n",vD);
   pInfo->relTrans.vec.x += vD;
}

void GhostWeaponTransY(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Translate Y by %g\n",vD);
   pInfo->relTrans.vec.y += vD;
}

void GhostWeaponTransZ(float vD)
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
   {
      mprintf("No info\n");      
      return;

   }
   mprintf("Translate Z by %g\n",vD);
   pInfo->relTrans.vec.z += vD;
}

void
GhostWeaponDump()
{
   sCreatureAttachInfo *pInfo = CreatureAttachmentsGetAttach(nWeaponObjId,0);
   if(pInfo == NULL)
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
