/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/creatapi.h,v 1.2 2000/01/31 09:50:05 adurant Exp $
// This used for getting class pointers for creatures
// Should only be included by cpp files that want to think of creature
// as mesh or motor
#pragma once

#ifndef __CREATAPI_H
#define __CREATAPI_H

#include <meshtype.h>
#include <motrtype.h>
#include <objtype.h>
// why include these?
#include <meshapi.h>
#include <motorapi.h>

IMesh *CreatureGetMeshInterface(const ObjID objID);
IMotor *CreatureGetMotorInterface(const ObjID objID);


#endif
