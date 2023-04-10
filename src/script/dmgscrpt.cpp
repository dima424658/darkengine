/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/dmgscrpt.cpp,v 1.6 1998/10/05 17:23:04 mahk Exp $

#include <lg.h>
#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>
#include <scrptmsg.h>
#include <damgscrm.h>
#include <damgscrs.h>
#include <objscrt.h>
#include <osystype.h>

#include <dmgbase.h>
#include <dmgmodel.h>
#include <lazyagg.h>

// must be last header
#include <dbmem.h>

#define kDamageMsgVer 1

IMPLEMENT_SCRMSG_PERSISTENT(sDamageScrMsg)
{
   PersistenceHeader(sScrMsg,kDamageMsgVer);

   Persistent(kind);
   Persistent(damage);
   PersistentObject(culprit); 

   return TRUE; 
}

#define kSlayMsgVer 1

IMPLEMENT_SCRMSG_PERSISTENT(sSlayMsg)
{
   PersistenceHeader(sScrMsg,kSlayMsgVer);
   PersistentObject(culprit); 
   Persistent(kind); 

   return TRUE; 
}

////////////////////////////////////////////////////////////
// Damage script service 
//

LazyAggMember(IDamageModel) pDamage; 

DECLARE_SCRIPT_SERVICE_IMPL(cDamageSrv,Damage)
{
public:

   STDMETHOD(Damage)(object victim, object culprit, integer amount, integer kind)
      {
         sDamage damage = { amount, kind}; 
         if (victim != object(0))
            pDamage->DamageObject(victim,culprit,&damage); 
         return S_OK; 
      }

   STDMETHOD(Slay)(object victim, object culprit)
      {
         if (victim != object(0))
            pDamage->SlayObject(victim,culprit); 
         return S_OK; 
      }

   STDMETHOD(Resurrect)(object victim, object culprit)
      {
         if (victim != object(0))
            pDamage->ResurrectObject(victim,culprit); 
         return S_OK; 
      }

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDamageSrv, Damage); 
