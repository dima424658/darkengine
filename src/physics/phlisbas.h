/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phlisbas.h,v 1.2 2000/01/29 13:32:47 adurant Exp $
#pragma once

#ifndef __PHLISBAS_H
#define __PHLISBAS_H

#include <types.h>

#include <comtools.h>

#include <phscrpt.h>

//
// Physics listeners get these
//
struct sPhysListenMsg : sPhysMsgBase
{
   ePhysScriptMsgType type;

   // Collision
   sPhysListenMsg(int submod, ePhysCollisionType type, ObjID coll_object, 
      int coll_submod, const mxs_vector &normal, mxs_real momentum, const mxs_vector &pt):
      type(kCollisionMsg),
      sPhysMsgBase(submod, type, coll_object, coll_submod, normal, momentum, pt)
   {
   }
};

#endif