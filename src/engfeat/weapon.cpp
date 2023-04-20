/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapon.cpp,v 1.43 1999/12/15 15:56:19 BODISAFA Exp $
//
// core weapon (hand-to-hand-combat) routines
//

#include <lg.h>
#include <config.h>
#include <comtools.h>
#include <matrixs.h>
#include <appagg.h>
#include <objtype.h>
#include <command.h>
#include <playrobj.h>
#include <timer.h>
#include <headmove.h>
#include <schema.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phctrl.h>

#include <crattach.h>
#include <crwpnlst.h>

#include <weapon.h>
#include <weapprop.h>
#include <weapreac.h>
#include <weapcb.h>
#include <crwpnapi.h>

#include <traitman.h>
#include <iobjsys.h>
#include <objsys.h>
#include <osysbase.h>
#include <linkman.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <bintrait.h>
#include <objedit.h>

#include <relation.h>

#include <rendprop.h>
#include <objpos.h>

#include <sdesbase.h>

#include <mprintf.h>

#include <crjoint.h>
#include <sdesc.h>

#include <initguid.h>
#include <weapguid.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////////////////////////

sRelationDesc sWeaponRelationDesc =
{
   WEAPON_RELATION_NAME,
   0
};

sRelationDataDesc sWeaponRelationDataDesc = { "None", 0 };

IRelation *g_pWeaponRelation = NULL;
ITrait    *gpWeaponTrait = NULL;

////////////////////////////////////////////////////////////////////////////////

void InitWeapon()
{
   // Initialize reactions
   InitWeaponReactions();

   // Init weapon relations
   InitWeaponRelation();
   InitWeaponOffsetRelation();

   // Init weapon damage properties
   InitBaseWeaponDamageProp();
   InitCurWeaponDamageProp();   
   InitWeaponExposureProp();
   InitWeapSwingExposureProp();
   InitWeaponTerrainCollisionProp();
   InitWeaponModeChangeMetaProp();
   InitWeaponModeUnchangeMetaProp();

   // Init the weapon event callback system
   InitWeaponEventCallbacks();

   // Install the damage listener
   InitDamageListener();

   // Create the weapon relation
   Assert_(g_pWeaponRelation == NULL);
   g_pWeaponRelation = CreateStandardRelation(&sWeaponRelationDesc, &sWeaponRelationDataDesc, kQCaseSetAll);
   gpWeaponTrait = MakeTraitFromRelation(g_pWeaponRelation);
}

////////////////////////////////////////

void TermWeapon()
{
   // Terminate weapon damage properties
   TermBaseWeaponDamageProp();
   TermCurWeaponDamageProp();
   TermWeaponExposureProp();
   TermWeapSwingExposureProp();
   TermWeaponTerrainCollisionProp();
   TermWeaponModeUnchangeMetaProp();

   // Terminate weapon relations
   TermWeaponOffsetRelation();
   TermWeaponRelation();

   // Terminate the weapon event callback system
   TermWeaponEventCallbacks();

   Assert_(g_pWeaponRelation != NULL);
   SafeRelease(g_pWeaponRelation);
   SafeRelease(gpWeaponTrait);
}

////////////////////////////////////////////////////////////////////////////////

static const char* WeaponArchName = "Weapon";
static const char* ProjArchName = "Projectile";

BOOL IsWeapon(ObjID weapon)
{
   AutoAppIPtr_(TraitManager, pTraitMan);
   AutoAppIPtr_(ObjectSystem, pObjSys);

   ObjID weapon_arch = pObjSys->GetObjectNamed(WeaponArchName);
   ObjID proj_arch   = pObjSys->GetObjectNamed(ProjArchName);

   if (weapon_arch != OBJ_NULL)
   {
      if (pTraitMan->ObjHasDonor(weapon, weapon_arch))
      {
         return TRUE;
      }
   }
   else
   {
      Warning(("IsWeapon: No weapon_arch object (nothing named \"Weapon\")\n"));
   }
   if (proj_arch != OBJ_NULL)
   {
      if (pTraitMan->ObjHasDonor(weapon, proj_arch))
      {
         return TRUE;
      }
   }
   else
   {
      Warning(("IsWeapon: No proj_arch object (nothing named \"Projectiles\"\n"));
   }
   return FALSE;
}

////////////////////////////////////////

ObjID SetWeapon(ObjID owner, ObjID weapon, int mode)
{
   Assert_(OBJ_IS_CONCRETE(owner));

   // Switch the weapon model to the creature (if appropriate).
   if (CreatureWeaponObjSwizzle)
   {
      ((*CreatureWeaponObjSwizzle)(owner, mode));
   }

   LinkID id = g_pWeaponRel->GetSingleLink(owner,LINKOBJ_WILDCARD); 
   if (id != LINKID_NULL)
   {
      sLink link;
      g_pWeaponRel->Get(id,&link); 
      if (link.dest == weapon)
         return weapon; 

      g_pWeaponRel->Remove(id); 
   }

   // Check that weapon has the root weapon as an archetype
   if (!IsWeapon(weapon))
   {
      Warning(("Specified object [%s] is not a weapon!\n",ObjWarnName(weapon)));
      return OBJ_NULL;
   }

   id = g_pWeaponRel->GetSingleLink(LINKOBJ_WILDCARD,weapon); 
   if (id != LINKID_NULL)
      g_pWeaponRel->Remove(id); 

   // Set link
   int no_swing = 0; 
   g_pWeaponRel->AddFull(owner, weapon, &no_swing);

   return weapon;
}

BOOL StartWeaponSwing(ObjID owner, ObjID weapon, int val)
{
   if (weapon == OBJ_NULL || GetWeaponObjID(owner) != weapon)
   {
      Warning(("Trying to StartSwing an unset weapon (%s a %s, thinks %s)\n",
               ObjWarnName(owner),ObjWarnName(weapon),ObjWarnName(GetWeaponObjID(owner))));
      return FALSE;
   }
   
   LinkID weapLink = g_pWeaponRel->GetSingleLink(owner,weapon);
   if (weapLink != LINKID_NULL)
   {

#ifdef PLAYTEST      
      int *myState=(int *)g_pWeaponRel->GetData(weapLink);
      if (myState&&(*myState==val))
         Warning(("StartSwing: %s already thinks %s is swinging it with val %d\n",
                  ObjWarnName(weapon),ObjWarnName(owner),val));
#endif      
      g_pWeaponRel->SetData(weapLink,&val);
   }

   return TRUE;
}

BOOL FinishWeaponSwing(ObjID owner, ObjID weapon)
{
   if (GetWeaponObjID(owner) != weapon)
   {
      Warning(("Trying to FinishSwing an unset weapon (%s a %s, thinks %s)\n",
               ObjWarnName(owner),ObjWarnName(weapon),ObjWarnName(GetWeaponObjID(owner))));
      return FALSE;
   }

   LinkID weapLink = g_pWeaponRel->GetSingleLink(owner,weapon);
   if (weapLink!=LINKID_NULL)
   {
      int doneVal=0;
#ifdef PLAYTEST      
      int *myState=(int *)g_pWeaponRel->GetData(weapLink);
      if ((myState)&&(*myState==FALSE))
            Warning(("FinishSwing: %s doesnt think %s is swinging it\n",
                     ObjWarnName(weapon),ObjWarnName(owner)));
#endif      
      g_pWeaponRel->SetData(weapLink,&doneVal);
   }
   
   return TRUE;
}

int IsWeaponSwinging(ObjID weapon)
{
   LinkID weapLink = g_pWeaponRel->GetSingleLink(LINKOBJ_WILDCARD,weapon);
   if (weapLink!=LINKID_NULL)
   {
      int *myState=(int *)g_pWeaponRel->GetData(weapLink);
      if (myState)
         return *myState;
   }
   
   return FALSE;
}

////////////////////////////////////////

BOOL UnSetWeapon(ObjID owner)
{
   ObjID weapon;

   if ((weapon = GetWeaponObjID(owner)) == OBJ_NULL)
      return FALSE;

   Assert_(g_pWeaponRel);

   // Find and remove the link
   AutoAppIPtr_(LinkManager, pLinkMan);
   AutoIPtr_(LinkQuery,pQuery);
   pQuery = g_pWeaponRel->Query(owner, weapon);
   if (!pQuery->Done())
      pLinkMan->Remove(pQuery->ID());

   return TRUE;
}

////////////////////////////////////////

ObjID GetWeaponObjID(ObjID owner)
{
   AutoIPtr_(LinkQuery, pQuery);
   Assert_(g_pWeaponRel);
   pQuery = g_pWeaponRel->Query(owner, LINKOBJ_WILDCARD);

   if (!pQuery->Done())
   {
      sLink link;
      
      pQuery->Link(&link);

      return link.dest;
   }
   else
      return OBJ_NULL;
}

////////////////////////////////////////

ObjID GetWeaponOwnerObjID(ObjID weapon)
{
   AutoIPtr_(LinkQuery, pQuery);
   Assert_(g_pWeaponRel);
   pQuery = g_pWeaponRel->Query(LINKOBJ_WILDCARD, weapon);

   if (!pQuery->Done())
   {
      sLink link;
      
      pQuery->Link(&link);

      return link.source;
   }
   else
      return OBJ_NULL;
}

////////////////////////////////////////////////////////////////////////////////

//////   AI WEAPON FUNCTIONS

ObjID CreateAIWeapon(ObjID owner)
{
   // @TODO: get rid of this when everyone has weapon links
   AutoAppIPtr_(ObjectSystem, pObjSys);

   ObjID weapon_arch;
   ObjID weapon; 
   // figure out what archetype to use as weapon for given object
   ILinkQuery *query = QueryInheritedLinksSingle(gpWeaponTrait, g_pWeaponRelation, owner, LINKOBJ_WILDCARD);

   if (query->Done())
   {
      Warning(("No weapon link for %d! (defaulting to \"Sword\")\n", owner));
      
      weapon_arch = pObjSys->GetObjectNamed("Sword");
   }
   else
   {
      sLink link;
      query->Link(&link);

      weapon_arch = link.dest;
   }
   SafeRelease(query);

   if(weapon_arch==OBJ_NULL)
      return OBJ_NULL;



   // create a concrete instance of that object
   weapon=pObjSys->BeginCreate(weapon_arch, kObjectConcrete);
   if(weapon!=OBJ_NULL)
   {
      Position *pPos=ObjPosGet(owner);

      AssertMsg1(pPos,"EquipAIWeapon: owner %d has no position",owner);

      ObjPosUpdate(weapon,&pPos->loc.vec,&pPos->fac);
   }
   pObjSys->EndCreate(weapon);

   return weapon; 
}

BOOL  EquipAIWeapon(ObjID owner, ObjID *pWeapon, int mode)
{
   // Switch the weapon model to the creature (if appropriate).
   if (CreatureWeaponObjSwizzle)
   {
      ((*CreatureWeaponObjSwizzle)(owner, mode));
   }

   int   weapon_type = 0; 

   *pWeapon = GetWeaponObjID(owner); 
   if (*pWeapon != OBJ_NULL)
   {
      return TRUE; 
   }

   *pWeapon = CreateAIWeapon(owner); 

   if (*pWeapon==OBJ_NULL)
   {
      return FALSE;
   }

   // SetWeapon it
   SetWeapon(owner,*pWeapon, mode);

   // Set its base damage factor
   SetWeaponDamageFactor(*pWeapon, 1);

   // attach it to the creature
   return CreatureAttachWeapon(owner, *pWeapon, weapon_type);
}

BOOL UnEquipAIWeapon(ObjID owner, ObjID weapon)
{
   BOOL success=TRUE;
   
   if (!ObjExists(weapon))
   {
      return success;
   }
   // @TODO:  Bodisafa 12/1/1999
   // While this seems like a good idea to put it here, it causes weapon popping.
   // Destroy the weapon model to the creature (if appropriate).
   // CreatureWeaponDestroy(owner);

   // UnSetWeapon it

   success=UnSetWeapon(owner)&&success;

   // Clear its base damage factor
   ClearWeaponDamageFactor(weapon);

   // detach from creature
   CreatureDetachWeapon(owner,weapon);

   DestroyObject(weapon);


   return success;
}

BOOL MakeAIWeaponPhysical(ObjID owner, ObjID weapon)
{
   int weapon_type=0; // @TODO: should get this from somewhere
   return CreatureMakeWeaponPhysical(owner, weapon, weapon_type);
}

void  MakeAIWeaponNonPhysical(ObjID owner, ObjID weapon)
{
   CreatureMakeWeaponNonPhysical(owner, weapon);

   WeaponEvent(kEndAttack, owner, NULL);
}


/////////////////////////////////////////////////////////////
// Weapon Offset Link
// Intention:  Define a link for attaching weapons to creatures.
// 
/////////////////////////////////////////////////////////////

// weapon_mode_names:  Allows designers to customize the weapon attachment for different creature modes.
const char *weapon_mode_names[] =
{
   "BLOCK",    // Blocking movements.
   "MELEE",    // Melee combat.
   "RANGED",   // Ranged combat.
   "IDLE",     // Idle (leaning on the weapon, etc...).
};

static sFieldDesc WeaponOffsetFields[] = 
{
   {"Weapon Modes",     kFieldTypeBits,   FieldLocation(sWeaponOffset, m_mode), FullFieldNames(weapon_mode_names)},
   {"Transform1",       kFieldTypeVector, FieldLocation(sWeaponOffset, m_transforms[0]),},
   {"Transform2",       kFieldTypeVector, FieldLocation(sWeaponOffset, m_transforms[1]),},
   {"Transform3",       kFieldTypeVector, FieldLocation(sWeaponOffset, m_transforms[2]),},
   {"Transform4",       kFieldTypeVector, FieldLocation(sWeaponOffset, m_transforms[3]),},
   {"Launch Joint",     kFieldTypeEnum,   FieldLocation(sWeaponOffset, m_joint), kFieldFlagNone, 0, kCJ_NumCreatureJoints, kCJ_NumCreatureJoints, g_pJointNames },
};

static sStructDesc WeaponOffsetStructDesc = StructDescBuild(sWeaponOffset, kStructFlagNone, WeaponOffsetFields);

static sRelationDesc weapon_offset_desc = 
{
   "WeaponOffset",
   kRelationNetworkLocalOnly,
};


IRelation *g_pWeaponOffsetRelation = NULL;
static sRelationDataDesc weapon_offset_data_desc = LINK_DATA_DESC_FLAGS(sWeaponOffset, kRelationDataAutoCreate);

void InitWeaponOffsetRelation(void)
{
   Assert_(g_pWeaponOffsetRelation == NULL);
   g_pWeaponOffsetRelation = CreateStandardRelation(&weapon_offset_desc, &weapon_offset_data_desc, kQCaseSetAll);
   
   StructDescRegister(&WeaponOffsetStructDesc);
   
   ClearWeaponOffsetCache();
}

void TermWeaponOffsetRelation(void)
{
   ClearWeaponOffsetCache();
   Assert_(g_pWeaponOffsetRelation != NULL);
   SafeRelease(g_pWeaponOffsetRelation);
   g_pWeaponOffsetRelation = NULL;
}

// ObjHasWeaponOffset Determines if any WeaponOffset links exist for this object or the archetype.
//
// Assumption:  That WeaponOffset links are at least on the leaf archetype.
//              Otherwise, this function could get quickly expensive.
// Note: While this assumption is used for the trivial reject case,
//       the full tree is examined for weapon offset swizzling.
//
BOOL ObjHasWeaponOffset(ObjID objID)
{
   Assert_(g_pWeaponOffsetRelation);
   
   if (!g_pWeaponOffsetRelation->AnyLinks(objID, LINKOBJ_WILDCARD))
   {
      // Check the archetype.
      AutoAppIPtr_(TraitManager, pTraitMan);
      ObjID archID = pTraitMan->GetArchetype(objID);
      if (archID != OBJ_NULL)
      {
         return g_pWeaponOffsetRelation->AnyLinks(archID, LINKOBJ_WILDCARD);
      }
      return FALSE;
   }
   return TRUE;
}

BOOL WeaponOffsetGet(ObjID objID, int mode, sWeaponOffset **ppWeaponOffsetResult, ObjID* pDestObjID)
{
   AutoAppIPtr_(LinkManager, pLinkMan);
   ILinkQuery *query = QueryInheritedLinksSingleUncached(g_pWeaponOffsetRelation, objID, LINKOBJ_WILDCARD);
   //g_pWeaponOffsetRelation->Query(objID, LINKOBJ_WILDCARD);

   Assert_(query);
   for (; !query->Done(); query->Next())
   {
      LinkID id = query->ID();
      
      sWeaponOffset *pWeaponOffset = (sWeaponOffset *)pLinkMan->GetData(id);
      
      Assert_(pWeaponOffset);
      if (mode & pWeaponOffset->m_mode)
      {
         sLink link;
         query->Link(&link);
         *pDestObjID = link.dest;
         
         *ppWeaponOffsetResult = pWeaponOffset;
         SafeRelease(query);
         return TRUE;         
      }
   }
   
   *ppWeaponOffsetResult = NULL;
   return FALSE;
}

// sWeaponOffsetCache - used for caching weapon offset list searches.
struct sWeaponOffsetCache
{
   inline void Set(sWeaponOffset* inWeaponOffset, const ObjID& inSourceID, const ObjID& inDestID, const int& inMode);
   
   sWeaponOffset* m_pWeaponOffset;  // Cached Weapon Offset information.
   ObjID          m_sourceID;       // Cached Weapon Offset ObjectId
   ObjID          m_destID;         // Cached Weapon Offset Destination ObjectId
   int            m_mode;           // Cached Weapon Offset mode.
};

inline void
sWeaponOffsetCache::Set(sWeaponOffset* inWeaponOffset, const ObjID& inSourceID, const ObjID& inDestID, const int& inMode)
{
   m_pWeaponOffset = inWeaponOffset;
   m_sourceID      = inSourceID;
   m_destID        = inDestID;
   m_mode          = inMode;
}

static sWeaponOffsetCache weaponOffsetCache;

#define IsInWeaponOffsetCache(id) (weaponOffsetCache.m_pWeaponOffset && weaponOffsetCache.m_sourceID == id)

sWeaponOffset * GetCachedWeaponOffset(ObjID objID)
{
   if (IsInWeaponOffsetCache(objID))
   {
      return weaponOffsetCache.m_pWeaponOffset;
   }
#ifndef SHIP
   else
   {
      Warning(("GetCachedWeaponOffset:  Didn't call WeaponOffsetSetActive for obj %d - recovering...\n", objID));
      
      // Grab the idle mode.
      if (WeaponOffsetSetActive(objID, kWeaponModeIdle))
      {
         return weaponOffsetCache.m_pWeaponOffset;
      }

      // @HACK
      // Attempt recovery.  Grab the any valid weapon mode and set it to active. 
      for (int i = kWeaponModeInvalid + 1; i < kNumWeaponModes; ++i)
      {
         if (WeaponOffsetSetActive(objID,  (1 << i)))
         {
            return weaponOffsetCache.m_pWeaponOffset;
         }
      }
      Warning(("GetCachedWeaponOffset:  WeaponOffsetSetActive failed.\n", objID));
   }
#endif // !SHIP
   return NULL;
}

int WeaponOffsetGetMode(ObjID objID)
{
   return GetCachedWeaponOffset(objID)->m_mode;
}

mxs_vector* WeaponOffsetGetTransform(ObjID objID)
{
   return GetCachedWeaponOffset(objID)->m_transforms;
}

int WeaponOffsetGetJoint(ObjID objID)
{
   return GetCachedWeaponOffset(objID)->m_joint;
}

ObjID WeaponOffsetGetDest(ObjID objID)
{
   if (IsInWeaponOffsetCache(objID))
   {
      return weaponOffsetCache.m_destID;
   }
   
   Warning(("WeaponOffsetGetDest:  Didn't call WeaponOffsetSetActive for obj %d.\n", objID));
   return OBJ_NULL;   
}

// WeaponOffsetSetActiveOffset: Sets the active WeaponOffset for a given mode.
// Caches mode->WeaponOffset translation.
// TRUE:  An appropriate weapon offset exists for this mode.
BOOL WeaponOffsetSetActive(ObjID objID, int mode)
{
   if (IsInWeaponOffsetCache(objID) && weaponOffsetCache.m_mode == mode)
   {
      // Nothing new to do.
      return TRUE;
   }
   
   sWeaponOffset *pWeaponOffset = NULL;
   ObjID destObjID = OBJ_NULL;
   if (WeaponOffsetGet(objID, mode, &pWeaponOffset, &destObjID))
   {
      weaponOffsetCache.Set(pWeaponOffset, objID, destObjID, mode);
      return TRUE;
   }
   
   // Invalidate cache.
   ClearWeaponOffsetCache();
   return FALSE;
}

void ClearWeaponOffsetCache(void)
{
//   Warning(("ClearWeaponOffsetCache:  Clearing...\n"));
   weaponOffsetCache.Set(NULL, OBJ_NULL, OBJ_NULL, kWeaponModeInvalid);
}
