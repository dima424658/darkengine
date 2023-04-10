/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __PROJLINK_H
#define __PROJLINK_H

#include <lnkbyobj.h>

typedef struct sProjectileData
{
  int m_order;
  int m_setting;
} sProjectileData;

class cOrderedLinksByIntData : public LinksByObj
{
 protected:
  int CompareLinks(LinkID l1, LinkID l2);
};

BOOL MatchesSetting(sProjectileData* pProjData, int setting);

class cProjectileQuery
{
 public:
  cProjectileQuery(ObjID gunID);

  ~cProjectileQuery(void);

  ObjID GetFirst(void);

  ObjID GetNext(void);

  int GetOrder(void);

 private:
  ObjID m_gunArchID;
  int m_setting;
  ILinkQuery* m_pQuery;

  ObjID GetNextMatch(void);
  
};

void ProjectileLinksInit();
void ProjectileLinksTerm();

ObjID GetProjectile(ObjID gunID);
void SetProjectile(ObjID gunID, ObjID projID);
ObjID GetNextProjectile(ObjID gunID, ObjID currentProjID);

#endif // __PROJLINK_H
