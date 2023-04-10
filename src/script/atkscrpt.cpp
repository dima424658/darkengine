/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/atkscrpt.cpp,v 1.1 1998/10/20 21:16:14 CCAROLLO Exp $
//

#include <lg.h>

#include <scrptapi.h>
#include <atkscrpt.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////
//
// Weapon attack scripting message implementation
//

IMPLEMENT_SCRMSG_PERSISTENT(sAttackMsg)
{
   PersistenceHeader(sScrMsg, kAttackMsgVer);
   PersistentEnum(weapon);
   return TRUE;
}
