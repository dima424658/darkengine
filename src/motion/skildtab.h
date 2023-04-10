/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/skildtab.h,v 1.10 2000/01/31 09:49:59 adurant Exp $
#pragma once

#ifndef __SKILDTAB_H
#define __SKILDTAB_H

#include <mskilset.h>
#include <motdesc.h>
#include <skilltab.h>
#include <mseltype.h>

typedef enum eMControllerType
{
   kTeleportController,
   kMultipedPlayController,
   kGlideGroundLocoController,
   kMultipedGroundLocoController,
   kPlayerStandardController,
   kPlayerFlexBowController,
   kCombatController,
   kControllerInvalid=0xff,
} eMControllerType;


typedef struct sDefaultFullDesc
{
   int controllerID;
} sDefaultFullDesc;


typedef struct sSinglePlayFullDesc
{
   int controllerID;
} sSinglePlayFullDesc;

typedef struct sCombatFullDesc
{
   int controllerID;
} sCombatFullDesc;

typedef struct sLocoFullDesc
{
   int controllerID;
   sMLocoSelectDesc selDesc;
   sMGaitSkillData skillData;
} sLocoFullDesc;

typedef struct sPlayerSkillFullDesc
{
   int controllerID;
   ePlayerTransition trans;
   sSkillConnectDesc connect;
   sMPlayerSkillData skillData;
   char *modelName;
   char *schemaName;
   float timeToMax; 
   BOOL shoot;
   int mouseSpeed;
   int mouseZone;
   float slewSpeedScale;
   float rotateSpeedScale;
   BOOL IsIdleMotion;
} sPlayerSkillFullDesc;


EXTERN sLocoFullDesc g_aLocoDesc[];
EXTERN sPlayerSkillFullDesc g_aPlayerSkillDesc[];
EXTERN sDefaultFullDesc g_aDefaultDesc[];
EXTERN sSinglePlayFullDesc g_aSinglePlayDesc[];
EXTERN sCombatFullDesc g_aCombatDesc[];

#endif
