/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/motion/crexp.cpp,v 1.7 1999/10/26 16:53:08 BFarquha Exp $
//

//
// Exploding creatures
// To make a creature explode, add CorpsePart links from it to the body parts
// you want to appear. Each body part should be an md model with a vhot that
// indicates where it will appear relative to the dead object. Note that placing
// such a link will cause the original model to be destroyed when it is terminated.
//

// @TODO: I wonder if the trait query is slowing this down

#include <lg.h>
#include <mprintf.h>

#include <appagg.h>

#include <iobjsys.h>
#include <objdef.h>
#include <objpos.h>
#include <osetlnkq.h>
#include <traitbas.h>
#include <traitman.h>
#include <wrtype.h>

#include <dmgbase.h>
#include <dmgmodel.h>

// for vhot phys model placement
#include <md.h>
#include <mds.h>
#include <mnamprop.h>
#include <objmodel.h>
#include <physapi.h>

// for the link stuff
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>

// for physics/testing in world
#include <phcore.h>
#include <phmods.h>
#include <phmod.h>
#include <phmodsph.h>
#include <sphrcst.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////
//
// CorpsePart Relation
//

IRelation *g_pCorpseParts;

static sRelationDesc CorpsePartRDesc =
{
   "CorpsePart",
};

static sRelationDataDesc CorpsePartDDesc = { "None", 0 };

void CreateCorpsePartRelation(void)
{
   g_pCorpseParts = CreateStandardRelation(&CorpsePartRDesc, &CorpsePartDDesc, kQCaseSetSourceKnown);
}

////////////////////////////////////////
//
// CorpsePart iterator
//

class cCorpsePartIter
{
public:
   cCorpsePartIter(ObjID objID, IRelation *pRelation);
   ~cCorpsePartIter(void);

   ObjID GetObj(void);
   int GetJointNum(void);
   BOOL Finished(void);
   void Next(void);

private:
   ILinkQuery *m_pQuery;
   ObjID m_startObj;    // we store off the first obj we find links from
};

///////////////////////////////////////////////

cCorpsePartIter::cCorpsePartIter(ObjID objID, IRelation *pRelation):
   m_startObj(OBJ_NULL)
{
   AutoAppIPtr(TraitManager);
   // @TODO: use trait cache
   IObjectQuery* donors = pTraitManager->Query(objID, kTraitQueryAllDonors);
   cLinkQueryFactory* factory = CreateSourceSetQueryFactory(pRelation, LINKOBJ_WILDCARD);

   m_pQuery = CreateObjSetLinkQuery(donors,factory);
   Assert_(m_pQuery != NULL);
   // set start object
   if (!Finished())
   {
      sLink link;
      m_pQuery->Link(&link);
      if (m_startObj == OBJ_NULL)
         m_startObj = link.source;
   }
   SafeRelease(donors);
}

///////////////////////////////////////////////

cCorpsePartIter::~cCorpsePartIter(void)
{
   SafeRelease(m_pQuery);
}

///////////////////////////////////////////////

ObjID cCorpsePartIter::GetObj(void)
{
   ObjID result = OBJ_NULL;

   Assert_(m_pQuery != NULL);
   if (!Finished())
   {
      sLink link;
      m_pQuery->Link(&link);
      result = link.dest;
   }
   return result;
}

///////////////////////////////////////////////

int cCorpsePartIter::GetJointNum(void)
{
   int result = -1;

   Assert_(m_pQuery != NULL);
   if (!Finished())
   {
      sLink link;
      m_pQuery->Link(&link);
      result = *((int*)(m_pQuery->Data()));
   }
   return result;
}

///////////////////////////////////////////////

BOOL cCorpsePartIter::Finished(void)
{
   Assert_(m_pQuery != NULL);
   if (!m_pQuery->Done())
   {
      // if query not done, check that we are still looking at links
      // from the initial object (to ensure that we don't return links
      // from archetypes higher up in the hierarchy)
      if (m_startObj == OBJ_NULL)
         return FALSE;
      sLink link;
      m_pQuery->Link(&link);
      return (m_startObj != link.source);
   }
   return TRUE;
}

///////////////////////////////////////////////

void cCorpsePartIter::Next(void)
{
   Assert_(m_pQuery != NULL);
   m_pQuery->Next();
}

///////////////////////////////////////////////

eDamageResult LGAPI CreatureExplodeDamageListener(const sDamageMsg* pMsg, tDamageCallbackData data)
{
   cCorpsePartIter iter(pMsg->victim, g_pCorpseParts);

   if (!iter.Finished())
   {
      mxs_matrix mat;
      char model_name[80];
      int modelIdx;
      mds_model* pModel;
      ObjID partID;
      ObjPos pos;
      float radius;
      AutoAppIPtr(ObjectSystem);

      while (!iter.Finished())
      {
         // setup
         pos = *ObjPosGet(pMsg->victim);
         UpdateChangedLocation(&pos.loc);
         radius = 0;
         partID = pObjectSystem->Create(iter.GetObj(), kObjectConcrete);

         // find model
         if (ObjGetModelName(partID, model_name))
         {
            modelIdx = objmodelGetIdx(model_name);
            if (objmodelGetModelType(modelIdx) == OM_TYPE_MD)
            {
               // add vhot location to obj pos, if we have one
               pModel = (mds_model*)objmodelGetModel(modelIdx);
               if (pModel->vhots>0)
               {
                  mds_vhot *pVhots = md_vhot_list(pModel);
                  mx_ang2mat(&mat, &pos.fac);

                  for (int i=0; i<3; i++)
                     mx_scale_addeq_vec(&pos.loc.vec, &mat.vec[i], pVhots->v.el[i]);
               }
            }
            else
            {
               Warning(("CreatureExplodeDamageListener: Object %d model is not an md model\n", partID));
               iter.Next();
               continue;
            }
         }
         else
            Warning(("CreatureExplodeDamageListener: Object %d has no model\n", partID));

         // check that new object position is OK
         // get physics size
         cPhysModel *pModel;

         if ((pModel = g_PhysModels.Get(partID)) != NULL)
         {
            if ((pModel->GetType(0) == kPMT_Sphere) ||
                (pModel->GetType(0) == kPMT_SphereHat))
            {
               radius = ((cPhysSphereModel *)pModel)->GetRadius(0);
            }
         }

         // this is not quite right, but we need to create the object to find out about its
         // model, so it's easier this way
         BOOL valid_pos = TRUE;

         if (radius > 0.0)
         {
            if (!SphrSphereInWorld(&pos.loc, radius * 1.05, 0))
               valid_pos = FALSE;
         }
         else
         {
            if (ComputeCellForLocation(&pos.loc) == CELL_INVALID)
               valid_pos = FALSE;
         }

         if (valid_pos)
            ObjPosUpdate(partID, &pos.loc.vec, &pos.fac);
         else
            pObjectSystem->Destroy(partID);

         iter.Next();
      }
      // remove original model
#ifdef SHOCK
      return kDamageTerminate;
#else
      return kDamageDestroy; // Originally in Thief, added back for Thief2 so zombie bodys will disappear after explosion.
#endif
   }
   return kDamageNoOpinion;
}

///////////////////////////////////////////////

void CreatureExplodeInit(void)
{
   AutoAppIPtr(DamageModel);
   pDamageModel->Listen(kDamageMsgTerminate, CreatureExplodeDamageListener, NULL);
   CreateCorpsePartRelation();
}

