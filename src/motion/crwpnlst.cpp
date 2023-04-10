/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <string.h>    // memset, apparently

#include <appagg.h>
#include <physapi.h>

#include <simtime.h>

#include <aiapi.h>     // ObjIsAI()

#include <objedit.h>
#include <hashset.h>
#include <hshsttem.h>
#include <crwpnlst.h>

#include <linkbase.h>
#include <lnkquery.h>
#include <crattach.h>
#include <objsys.h>

#include <dbmem.h>

#ifdef _MSC_VER
template cCreatureWeaponTableBase;
#endif

// core data structures
static cCreatureWeaponHash  gCreatureWeapons;

////////////////////////
// List Control

static BOOL _CreatureWeaponRecentlyExisted(ObjID obj)
{
   // will want to store off recently killed objs
   // and return whether this object is in the list
   return FALSE;
}

// core setup for creature weapons.
void CreatureWeaponInitStruct(sCreatureWeapon *pCW)
{
   memset(pCW, 0, sizeof(sCreatureWeapon));
}

////////////////////////////////
// internal api to rest of CreatureWeapon.

sCreatureWeapon *CreatureWeaponGet(ObjID obj)
{
   return gCreatureWeapons.Search(obj);
}

// actual add/remove of creatures from the creature weapon hash.
void CreatureWeaponAdd(ObjID obj)
{
   if (obj < 0)
   {
      Warning(("CreatureWeaponAdd: got a negative object %s?\n", ObjWarnName(obj)));
      return;
   }
   
   if (CreatureWeaponGet(obj))
   {
      return;
   }
   sCreatureWeapon  *pCW = new sCreatureWeapon;
   CreatureWeaponInitStruct(pCW);
   pCW->obj        = obj;
   gCreatureWeapons.Insert(pCW);
}

extern IRelation *g_pWeaponOffsetRelation;
void CreatureWeaponRem(ObjID obj)
{
   sCreatureWeapon *pCW=gCreatureWeapons.RemoveByKey(obj);
   
   if (pCW)
   {
      delete pCW;
   }
   else
   {
      if (!_CreatureWeaponRecentlyExisted(obj))
      {
         Warning(("CreatureWeaponRem: %s not found in hash\n",ObjWarnName(obj)));
      }
   }
      // in debug full spew, might want to warn here anyway
}

// remove all weapons for all creatures.
void CreatureWeaponRemoveWeapons(void)
{
   if (CreatureWeaponObjCreate == NULL)
   {
      return;  // cause if we cant create, this is all pretty silly
   }

   tHashSetHandle cwIter;
   sCreatureWeapon *pGR=gCreatureWeapons.GetFirst(cwIter);
   while (pGR)
   {
      if (pGR->nWeapon != -1)
      {
         (*CreatureWeaponObjDestroy)(pGR->obj);
      }
      pGR=gCreatureWeapons.GetNext(cwIter);
   }
}

////////////////////////////////
// dumb code to memory clean up on destroy

void CreatureWeaponListEmptyAndFree(void)
{
#ifdef WARN_ON
   int count = gCreatureWeapons.GetCount();
   if (count)
   {
      Warning(("CreatureWeapons still has %d members\n", count));
   }
#endif

   tHashSetHandle cwIter;
   sCreatureWeapon *pCW=gCreatureWeapons.GetFirst(cwIter);
   while (pCW)
   {
      CreatureWeaponRem(pCW->obj);
      pCW=gCreatureWeapons.GetNext(cwIter);
   }
}

////////////////
// init/term/reset

void CreatureWeaponListInit(void)
{
}

void CreatureWeaponListTerm(void)
{
   CreatureWeaponListEmptyAndFree();

#ifdef WARN_ON
   int count =gCreatureWeapons.GetCount();
   if (count)
   {
      Warning(("CreatureWeaponHash post empty has %d members\n",count));
   }
#endif

   gCreatureWeapons.DestroyAll();
}

void CreatureWeaponListReset(void)
{
   CreatureWeaponListTerm();  // for now, cause this is easy...
}

