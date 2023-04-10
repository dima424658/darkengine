/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhome.cpp,v 1.5 2000/02/19 13:25:23 toml Exp $
//
// Homing projectiles
// Various props & code support
// 

#include <shkhome.h>

#include <float.h>
#include <math.h>

#include <appagg.h>
#include <fix.h>
#include <matrix.h>
#include <mxang.h>

#include <dataops_.h>
#include <linkbase.h>
#include <matrixc.h>
#include <objhp.h>
#include <objpos.h>
#include <objquery.h>
#include <phoprop.h>
#include <physapi.h>
#include <playrobj.h>
#include <port.h>
#include <prcniter.h>
#include <propbase.h>
#include <propert_.h>
#include <prophash.h>
#include <propstor.h>
#include <rendprop.h>
#include <relation.h>
#include <sdesc.h>
#include <sdesbase.h>
#include <wrtype.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define min(x, y) (((x)<(y))?(x):(y))
#define max(x, y) (((x)>(y))?(x):(y))

///////////////////////////////////////////
//
// Target type prop
//

IIntProperty* g_pTargetTypeProperty;
#define PROP_TARGET_TYPE "TargetType"

////////////////////////////////////////////////
//
// Homing property
//

// data ops
class cHomingDataOps: public cClassDataOps<sHoming>
{
};

// storage class
class cHomingStore: public cHashPropertyStore<cHomingDataOps>
{
};

// property implementation class
class cHomingProperty: public cSpecificProperty<IHomingProperty, &IID_IHomingProperty, sHoming*, cHomingStore>
{
   typedef cSpecificProperty<IHomingProperty, &IID_IHomingProperty, sHoming*, cHomingStore> cParent; 

public:
   cHomingProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sHoming); 

};

static char *targetTypeNames[] = 
{ 
   "Type 1",
   "Type 2",
   "Type 3",
   "Type 4",
   "Type 5",
};

static sFieldDesc targetTypeField[] = 
{ 
   {"Target Type", kFieldTypeBits, sizeof(int), 0, kFieldFlagNone, 0, 5, 5, targetTypeNames},
};

static sStructDesc targetTypeSDesc = 
{
   PROP_TARGET_TYPE, 
   sizeof(int),
   kStructFlagNone,
   sizeof(targetTypeField)/sizeof(targetTypeField[0]),
   targetTypeField,
}; 

static sFieldDesc homingFields[] = 
{
   {"Target Type", kFieldTypeBits, FieldLocation(sHoming, m_targetType), kFieldFlagNone, 0, 5, 5, targetTypeNames},
   {"Distance Filter", kFieldTypeFloat, FieldLocation(sHoming, m_distFilter),},
   {"Heading Filter", kFieldTypeInt, FieldLocation(sHoming, m_headingFilter), kFieldFlagHex},
   {"Max Turn", kFieldTypeInt, FieldLocation(sHoming, m_maxTurn), kFieldFlagHex},
   {"Update Freq (msec)", kFieldTypeInt, FieldLocation(sHoming, m_updateFreq), },
};

IHomingProperty *g_pHomingProperty;

static sStructDesc homingStructDesc = 
   StructDescBuild(sHoming, kStructFlagNone, homingFields);

IHomingProperty *CreateHomingProperty(sPropertyDesc *desc, ePropertyImpl impl)
{
   StructDescRegister(&homingStructDesc);
   return new cHomingProperty(desc);
}

static sPropertyDesc homingDesc = 
{
   PROP_HOMING, 
   kPropertyInstantiate,
   NULL, 
   1,    // version
   0,
   {"Gun", "Homing Projectile"},
};

///////////////////////////////////////////
// 
// Target link
//
IRelation * g_pTargetLinks;

void ProjectileTargetInit(void)
{
   sRelationDesc targetRelationDesc  = { "Target", 0, 0, 0 };
   sRelationDataDesc noDataDesc = { "None", 0 };
   g_pTargetLinks = CreateStandardRelation(&targetRelationDesc, &noDataDesc, kQCaseSetSourceKnown);

   sPropertyTypeDesc targetTypeDesc = {PROP_TARGET_TYPE, sizeof(int)}; 
   sPropertyDesc targetDesc = { PROP_TARGET_TYPE, 0, NULL, 1, 0, {"Gun", "Target Type"}, };
   g_pTargetTypeProperty = CreateIntegralProperty(&targetDesc, &targetTypeDesc, kPropertyImplLlist);
   StructDescRegister(&targetTypeSDesc);

   g_pHomingProperty = CreateHomingProperty(&homingDesc, kPropertyImplDense);
}

void ProjectileTargetTerm(void)
{
   SafeRelease(g_pTargetLinks);
   SafeRelease(g_pTargetTypeProperty);
   SafeRelease(g_pHomingProperty);
}

// Scan for targets
ObjID TargetScan(ObjID projID)
{
   sHoming* pHoming;
   cConcreteIter iter(g_pTargetTypeProperty);
   int targetType;
   ObjID objID;
   ObjID targetID = OBJ_NULL;
   Location* pTargetLoc;
   Position* pMyPos = ObjPosGet(PlayerObject());
   mxs_vector *pMyLoc = &(pMyPos->loc.vec);
   fixang myHeading = pMyPos->fac.tz;
   fixang myPitch = pMyPos->fac.ty;
   mxs_vector delta;
   fixang heading;
   fixang headingDelta;
   fixang pitch;
   fixang pitchDelta;
   fixang angularDelta;
   fixang minAngularDelta = 0xffff;
   Location hitLoc;
   int hp;

   // get the homing data from the projectile archetype
   if (!g_pHomingProperty->Get(projID, &pHoming))
   {
      Warning(("proj %d has no homing property\n", projID));
      return OBJ_NULL;
   }
   // now iterate over all objects with the target type property
   iter.Start();   
   while (iter.Next(&objID))
   {
      if (!OBJ_IS_CONCRETE(objID) || !ObjHasRefs(objID))
         continue;
      // check that target type matches ... we should always have this prop if we got here
      g_pTargetTypeProperty->Get(objID, &targetType);
      if (!(targetType&(pHoming->m_targetType)))
         continue;
      // ignore dead creatures...
      if (!ObjGetHitPoints(objID, &hp) || (hp<=0))
         continue;
      // get loc
      pTargetLoc = &ObjPosGet(objID)->loc;
      mx_sub_vec(&delta, &pTargetLoc->vec, pMyLoc);
      // pre-filter on city block dist
      if ((delta.x<pHoming->m_distFilter) && (delta.y<pHoming->m_distFilter) && (delta.z<pHoming->m_distFilter))
      {
         // check heading 
         heading = fix_atan2(fix_from_float(delta.y), fix_from_float(delta.x));
         headingDelta = min(fixang(myHeading-heading), fixang(heading-myHeading));
         if (headingDelta<pHoming->m_headingFilter)
         {
            // check pitch
            pitch = fix_atan2(fix_from_float(delta.z), fix_from_float(sqrt(delta.x*delta.x+delta.y*delta.y)));
            pitchDelta = min(fixang(myPitch-pitch), fixang(pitch-myPitch));
            if (pitchDelta<pHoming->m_headingFilter)
            {
               // check min sum
               if (pitchDelta<0x8000)
                  angularDelta = pitchDelta;
               else
                  angularDelta = -pitchDelta;
               if (headingDelta<0x8000)
                  angularDelta += headingDelta;
               else
                  angularDelta -= headingDelta;
               if (angularDelta<minAngularDelta)
               // finally, raycast... argh
               if (PortalRaycast(&pMyPos->loc, pTargetLoc, &hitLoc, 0))
               {
                  minAngularDelta = angularDelta;
                  targetID = objID;
               }
            }
         }
      }
   }
   iter.Stop();
   return targetID;
}

void Home(ObjID projID, ObjID targetID)
{
   cMxsVector* pProjVec = (cMxsVector*)&ObjPosGet(projID)->loc.vec;
   cMxsVector* pTargetVec = (cMxsVector*)&ObjPosGet(targetID)->loc.vec;
   cMxsVector deltaVec;
   cMxsVector velocity;
   cMxsVector newVelocity;
   cMxsVector* pInitVelocity;
   fixang targetHeading, targetPitch;
   fixang projHeading, projPitch;
   fixang deltaHeading, deltaPitch;
   sHoming* pHoming;
   mxs_matrix mat;

   if (!g_pHomingProperty->Get(projID, &pHoming))
   {
      Warning(("proj %d has no homing property\n", projID));
      return;
   }

   mx_sub_vec(&deltaVec, pTargetVec, pProjVec);
   targetHeading = fix_atan2(fix_from_float(deltaVec.y), fix_from_float(deltaVec.x));
   // we have to take sqrt here to avoid underflow problems with fixes
   targetPitch = fix_atan2(-fix_from_float(deltaVec.z), fix_from_float(sqrt(deltaVec.y*deltaVec.y+deltaVec.x*deltaVec.x)));
   PhysGetVelocity(projID, (mxs_vector*)&velocity);

   // heading delta
   projHeading = fix_atan2(fix_from_float(velocity.y), fix_from_float(velocity.x));
   deltaHeading = targetHeading-projHeading;
   // clockwise or counter-clock?
   if (deltaHeading<0x8000)
      deltaHeading = min(deltaHeading, fixang(pHoming->m_maxTurn));
   else
      deltaHeading = max(deltaHeading, fixang(-pHoming->m_maxTurn));

   // pitch delta
   projPitch = fix_atan2(-fix_from_float(velocity.z), fix_from_float(sqrt(velocity.x*velocity.x+velocity.y*velocity.y)));
   deltaPitch = targetPitch-projPitch;
   // clockwise or counter-clock?
   if (deltaPitch<0x8000)
      deltaPitch = min(deltaPitch, fixang(pHoming->m_maxTurn));
   else
      deltaPitch = max(deltaPitch, fixang(-pHoming->m_maxTurn));

   mxs_angvec orientVec;
   orientVec.tx = 0;
   orientVec.ty = projPitch+deltaPitch;
   orientVec.tz = projHeading+deltaHeading;
   mx_ang2mat(&mat, &orientVec);
   if (g_pPhysInitVelProp->Get(projID, (mxs_vector**)&pInitVelocity))
      newVelocity = *pInitVelocity;
   mx_mat_muleq_vec(&mat, &newVelocity);

   PhysSetVelocity(projID, (mxs_vector*)&newVelocity);
}

