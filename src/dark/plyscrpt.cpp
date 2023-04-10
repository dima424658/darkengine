/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/plyscrpt.cpp,v 1.1 1998/05/13 10:38:07 kate Exp $

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <plyscrpt.h>
#include <plyrhdlr.h>
#include <drkcret.h>

#include <mprintf.h>

// must be last header
#include <dbmem.h>

// @TODO: get this info from a property lookup or something
static int ObjGetPlayerLimbHandler(ObjID item)
{
   return kPlayerModeBodyCarry;
}

static int g_CurHandler=kPlayerModeInvalid;

////////////////////////////////////////////////////////////
// Player Item Script Service

DECLARE_SCRIPT_SERVICE_IMPL(cPlayerLimbsSrv, PlayerLimbs)
{
public:        

   STDMETHOD(Equip)(object item)
   {
      return PlayerHandlerEquip(ObjGetPlayerLimbHandler(item),item);
   }

   STDMETHOD(UnEquip)(object item)
   {
      return PlayerHandlerUnEquip(ObjGetPlayerLimbHandler(item),item);
   }

   STDMETHOD(StartUse)(object item)
   {
      return PlayerHandlerStartUse(ObjGetPlayerLimbHandler(item));
   }

   STDMETHOD(FinishUse)(object item)
   {
      return PlayerHandlerFinishUse(ObjGetPlayerLimbHandler(item));
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cPlayerLimbsSrv, PlayerLimbs);











