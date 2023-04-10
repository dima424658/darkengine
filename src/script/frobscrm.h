/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/frobscrm.h,v 1.2 1998/04/07 13:58:10 CCAROLLO Exp $

#pragma once

#ifndef __FROBSCRM_H
#define __FROBSCRM_H

#include <scrptmsg.h>
#include <objtype.h>
#include <frobscrt.h>

/////////////////////////////////////
//
// MESSAGES: Room services
//    RoomEnter, RoomExit, for Player, RemotePlayer, Creature, or Object

#define kFrobMsgVer 1

// self frob will be just a src obj and loc
// tool will have both

struct sFrobMsg : public sScrMsg
{
   // ObjId's for the action
   ObjID    SrcObjId;  // solo or tool obj
   ObjID    DstObjId;  // dst in a tool obj
   ObjID    Frobber;   // who is doing the frobbing
   eFrobLoc SrcLoc;    // solo or tool loc
   eFrobLoc DstLoc;    // dst loc
   float    Sec;       // MS for the up event
   BOOL     Abort;     // an extra StartMsg w/Abort set true is sent   

   sFrobMsg()
      : SrcLoc(kFrobLocNone)
   {
   }

   sFrobMsg(ObjID src, ObjID dest, ObjID frobber, eFrobLoc srcLoc, eFrobLoc dstLoc, int ms, char *type, BOOL abort)
      : sScrMsg(src,type), SrcObjId(src), DstObjId(dest), Frobber(frobber),
                           SrcLoc(srcLoc), DstLoc(dstLoc), Sec(ms/1000.0), Abort(abort)
   {
   }

   DECLARE_SCRMSG_PERSISTENT();
};

#endif  // __FROBSCRM_H
