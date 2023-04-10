/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Mostly, if not all copied from Dark
//

#include <dpcblood.h>

#include <appagg.h>
#include <comtools.h>

#include <iobjsys.h>
#include <objdef.h>
#include <objpos.h>

#include <dmgbase.h>

#include <phcollev.h>

#include <config.h>

#include <pgrpprop.h>

#include <prjctile.h>

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

// Include this last
#include <dbmem.h>

////////////////////////////////////////////////////////////
// BLOOD TYPE UTILITY CODE

static sPropertyDesc    BloodTypePropDesc= { "BloodType", 0, NULL, 0, 0, { "Game", "BloodType" } };
static IStringProperty *pBloodTypeProp = NULL;

static sPropertyDesc    BloodCausePropDesc= { "BloodCause", 0, NULL, 0, 0, { "Game", "BloodCause" } };
static IBoolProperty  *pBloodCauseProp = NULL;

static sPropertyDesc    BloodMaxDmgPropDesc = { "BloodMaxDmg", 0, NULL, 0, 0, { "Game", "BloodMaxDamage" } };
static IIntProperty   *pBloodMaxDmgProp = NULL;

void DPCBloodInit(void)
{
   pBloodTypeProp   = CreateStringProperty(&BloodTypePropDesc,kPropertyImplSparse);
   pBloodCauseProp  = CreateBoolProperty(&BloodCausePropDesc,kPropertyImplSparse);
   pBloodMaxDmgProp = CreateIntProperty(&BloodMaxDmgPropDesc,kPropertyImplSparse);
}

void DPCBloodTerm(void)
{
   SafeRelease(pBloodTypeProp);
   SafeRelease(pBloodCauseProp);
   SafeRelease(pBloodMaxDmgProp);
}

// im not sure how to get the real collision location for blood?
void DPCReleaseBlood(const sDamageMsg* msg)
{
   ObjID victim = msg->victim;
   ObjID hitter = msg->data.damage->kind; 
   const char *blood_arch;
   BOOL cause_bleeding=FALSE;
   
   if (hitter==OBJ_NULL)
      return;
   pBloodCauseProp->Get(hitter,&cause_bleeding); 

   if (cause_bleeding)
      if (pBloodTypeProp->Get(victim,&blood_arch))
      {
         AutoAppIPtr(ObjectSystem);
         ObjID arch = pObjectSystem->GetObjectNamed(blood_arch);

         mxs_vector dir; 
         dir.x=0.2; dir.y=0.0; dir.z=0.0;

         // Look in the event history for the raw collision event
         sPhysClsnEvent* ev = (sPhysClsnEvent*)msg->Find(kEventKindCollision); 
         if (ev) // It's there, find point of contact
         {
            cPhysClsn* coll = ev->collision; 
               
            mxs_vector pt; 
            pt = coll->GetClsnPt();
             
            ObjID blood = pObjectSystem->BeginCreate(arch,kObjectConcrete); 
            ObjTranslate(blood,&pt); 
            pObjectSystem->EndCreate(blood);

            int maxDmg;
            if (pBloodMaxDmgProp->Get(blood,&maxDmg))
            {  // get the particle property, tweak the data here
               ParticleGroup *pPGroup = ObjGetParticleGroup(blood);
               float scale=(float)msg->data.damage->amount/(float)maxDmg;
               if (scale<0.33) scale=0.33; else if (scale>1.0) scale=1.0;
               pPGroup->n*=scale;
               ObjSetParticleGroup(blood,pPGroup);
            }
         }
         else
            launchProjectile(victim,arch,0.1,PRJ_FLG_PUSHOUT|PRJ_FLG_GRAVITY,NULL,&dir,NULL);
      }
}

