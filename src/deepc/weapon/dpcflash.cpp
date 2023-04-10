/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// gun flashes
//

#include <dpcflash.h>

#include <fix.h>
#include <cfgdbg.h>

#include <appagg.h>
#include <autolink.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <traitman.h>
#include <matrixc.h>
#include <objpos.h>
#include <objsys.h>
#include <objdef.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <wrtype.h>

// projectile like-flashes
#include <camera.h>
#include <playrobj.h>
#include <rand.h>
#include <dpcproj.h>

#include <dpcvhot.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////

enum eFlashFlags_ {kFlashProjectile = 0x0001, kFlashRandBank = 0x0002,};

typedef struct sFlashData
{
   int m_vhot;
   int m_flags;   // this wierd ordering is to preserve the previous version's data where the was just a vhot
} sFlashData;

///////////////////////////////////////////////////////
// gun flash link

IRelation *g_pGunFlashLinks;

void CreateGunFlashes(ObjID archetypeID, ObjID gunID, 
                      mxs_ang facingOffset, BOOL isGhost)
{
   cAutoLinkQuery pFlashQuery(g_pGunFlashLinks, archetypeID, LINKOBJ_WILDCARD);
   ObjPos flashPos;

   // Create flash objects
   while (!pFlashQuery->Done())
   {
      ObjID newID;
      sFlashData* pData = (sFlashData*)(pFlashQuery->Data());
      if (pData != NULL)
      {
         // Get VHot loc
         flashPos.loc.vec.x = 0;
         flashPos.loc.vec.y = 0;
         flashPos.loc.vec.z = 0;
         VHotGetLoc(&flashPos.loc.vec, gunID, *((int*)(pFlashQuery->Data())));
      }
      else
         flashPos.loc.vec = ObjPosGet(gunID)->loc.vec;

      if ((pData->m_flags)&kFlashProjectile)
      {
         // We don't want to spit actual projectiles out from ghost weapons.
         // This is for things like shell casings and such.
         if (!isGhost)
         {
            sLaunchParams launchParams = g_defaultLaunchParams;
            launchParams.flags = 
               kLaunchLocOverride|kLaunchPitchOverride|
               kLaunchRelativeVelocity|kLaunchCheck;
            if ((pData->m_flags)&kFlashRandBank)
            {
               launchParams.flags |= kLaunchBankOverride;
               launchParams.bank = Rand() << 1;
            }
            // setup remainder of launch parameters
            launchParams.loc = flashPos.loc.vec;
            launchParams.pitch = PlayerCamera()->ang.ty;
            // actually launch the damn things
            newID = DPCLaunchProjectile(PlayerObject(), 
                                          pFlashQuery.GetDest(),
                                          &launchParams);
         }
      }
      else
      {
         newID = BeginObjectCreate(pFlashQuery.GetDest(), kObjectConcrete);   
         if (newID != OBJ_NULL)
         {
            flashPos.fac = ObjPosGet(gunID)->fac;
            flashPos.fac.tz += facingOffset;
            if ((pData->m_flags)&kFlashRandBank)
               flashPos.fac.tx += Rand()<<1;
            ObjPosUpdate(newID, &flashPos.loc.vec, &flashPos.fac);
            EndObjectCreate(newID);
         }
      }
      pFlashQuery->Next();
   }
}

/////////////////////////////////////////////////////
//
// The flash link
//

static char* flashFlags[] = 
{
   "Projectile Flash",
   "Random Bank", 
};

static sFieldDesc flashFields[] = 
{
   { "VHot", kFieldTypeInt, FieldLocation(sFlashData, m_vhot), kFieldFlagNone, },
   { "Flags", kFieldTypeBits, FieldLocation(sFlashData, m_flags), kFieldFlagNone, 0, 2, 2, flashFlags,},
};


static sStructDesc flashSDesc = StructDescBuild(sFlashData, kStructFlagNone, flashFields);

void GunFlashInit(void)
{
   sRelationDesc gunFlashDesc = { "GunFlash", 0, 0, 0 };
   sRelationDataDesc gunFlashDDesc = LINK_DATA_DESC_FLAGS(sFlashData, kRelationDataAutoCreate);

   // links
   StructDescRegister(&flashSDesc);
   g_pGunFlashLinks = CreateStandardRelation(&gunFlashDesc, &gunFlashDDesc, kQCaseSetSourceKnown);
}

void GunFlashTerm(void)
{
   SafeRelease(g_pGunFlashLinks);
}
