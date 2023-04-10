/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <appagg.h>
#include <sdesc.h>
#include <sdesbase.h>

#include <autolink.h>
#include <bintrait.h>
#include <linkman.h>
#include <linkint.h>
#include <lnkbyobj.h>
#include <lnkquery.h>
#include <relation.h>
#include <objdef.h>
#include <osetlnkq.h>
#include <osetlnkq.h>
#include <traitman.h>
#include <traitbas.h>
#include <iobjsys.h>
#include <prjctile.h>
#include <playrobj.h>
#include <psnd.h>
#include <matrixc.h>
#include <rendprop.h>

#include <gunflash.h>
#include <gunprop.h>
#include <projprop.h>
#include <gunproj.h>
#include <gunapi.h>

#include <esnd.h>
#include <ctagset.h>

#include <command.h>
#include <scrptapi.h>

#include <projlink.h>

#include <dbmem.h>


//moving projectile link out of the shkgun.... since it's more generic


////////////////////////////////////////////
// Projectile link

// query database - sorts links by data field filed
int cOrderedLinksByIntData::CompareLinks(LinkID l1, LinkID l2)
    {
      const int val1 = ((sProjectileData*)LinkMan()->GetData(l1))->m_order;
      const int val2 = ((sProjectileData*)LinkMan()->GetData(l2))->m_order;
      return val1-val2;
    }


static sFieldDesc projectileFields[] =
{
  { "Setting", kFieldTypeInt, FieldLocation(sProjectileData, m_setting), kFieldFlagNone },
  { "Order", kFieldTypeInt, FieldLocation(sProjectileData, m_order), kFieldFlagNone },
};

static sStructDesc projectileSDesc = StructDescBuild(sProjectileData, kStructFlagNone, projectileFields);

IRelation * g_pProjectileLinks;

BOOL MatchesSetting(sProjectileData* pProjData, int setting)
{
  return ((pProjData->m_setting<0) || (pProjData->m_setting == setting));
}

sRelationDesc projectileRelationDesc =
{"Projectile",
 0,
 0,
 0};

sCustomRelationDesc desc;

sRelationDataDesc projectileDataDesc = LINK_DATA_DESC_FLAGS(sProjectileData, kRelationDataAutoCreate);

void ProjectileLinksInit()
{

  StructDescRegister(&projectileSDesc);
  g_pProjectileLinks = CreateCustomRelation(&projectileRelationDesc,&projectileDataDesc,&desc);
}

void ProjectileLinksTerm()
{
  SafeRelease(g_pProjectileLinks);
}

/////////////////////////////
//
// Query class for getting projectile links appropriate to the current gun setting
// GetNext() should just be called repeatedly.  It will return OBJ_NULL when the query is finished.
//

ILinkQuery* m_pQuery;
ObjID m_gunArchID;

cProjectileQuery::cProjectileQuery(ObjID gunID):
    m_setting(GunStateGetSetting(gunID)),
    m_pQuery(NULL)
      {
	AutoAppIPtr(TraitManager);
	m_gunArchID = pTraitManager->GetArchetype(gunID);
      }

cProjectileQuery::~cProjectileQuery(void)
    {
      SafeRelease(m_pQuery);
    }

ObjID cProjectileQuery::GetFirst(void)
    {
      SafeRelease(m_pQuery);
      m_pQuery = g_pProjectileLinks->Query(m_gunArchID, LINKOBJ_WILDCARD);
      return GetNextMatch();
    }

ObjID cProjectileQuery::GetNext(void)
    {
      if (m_pQuery == NULL)
	return OBJ_NULL;
      m_pQuery->Next();
      return GetNextMatch();
    }

int cProjectileQuery::GetOrder(void)
    {
      if (m_pQuery == NULL)
	return -1;
      return ((sProjectileData*)m_pQuery->Data())->m_order;
    }

ObjID cProjectileQuery::GetNextMatch(void)
    {
      Assert_(m_pQuery != NULL);
      while (!m_pQuery->Done() && !MatchesSetting((sProjectileData*)m_pQuery->Data(), m_setting))
	m_pQuery->Next();
      if (m_pQuery->Done())
	return OBJ_NULL;
      sLink link;
      m_pQuery->Link(&link);
      return link.dest;
    }


//end of class, some helper functions...

/// Get current projectile for concrete gun from link
/// can return OBJ_NULL
//

ObjID GetProjectile(ObjID gunID)
{
  cAutoLinkQuery query(g_pProjectileLinks, gunID, LINKOBJ_WILDCARD);
  
  if (!query->Done())
      return query.GetDest();
  // if no link, setup default link
  ObjID projID = GetNextProjectile(gunID, OBJ_NULL);
  SetProjectile(gunID, projID);
  return projID;
}

////////////////////////////////////////////////////////////////
// Set a new projectile link
void SetProjectile(ObjID gunID, ObjID projID)
{
   AutoAppIPtr(LinkManager);
   ILinkQuery *pQuery = pLinkManager->Query(gunID, LINKOBJ_WILDCARD, g_pProjectileLinks->GetID());

   while (!pQuery->Done())
   {
      pLinkManager->Remove(pQuery->ID()); 
      // should be only one link, but...
      pQuery->Next();
   }
   SafeRelease(pQuery);
   if (projID != OBJ_NULL)
      pLinkManager->Add(gunID, projID, g_pProjectileLinks->GetID());
}

//////////////////////////////////////////////////////
// Given a gun archetype & current projectile get the next projectile for the gun
// This can return OBJ_NULL if the currentProjID is not in the list of projectiles for the 
// current setting or if the links are just set up badly.
//     
ObjID GetNextProjectile(ObjID gunID, ObjID currentProjID)
{
   AutoAppIPtr(TraitManager);
   cProjectileQuery query(gunID);  
   ObjID projID;

   projID = query.GetFirst();

   // if no current projectile, return default
   if (currentProjID == OBJ_NULL)
      return projID;

   // find the link to the current projectile
   while ((projID != OBJ_NULL) && (projID != currentProjID))
      projID = query.GetNext();

   // now get the next proj
   if (projID != OBJ_NULL)
      projID = query.GetNext();

   // wrap around if we are at the last ordered link 
   if (projID == OBJ_NULL)
      projID = query.GetFirst();
   return projID;

}
