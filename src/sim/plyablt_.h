/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/plyablt_.h,v 1.4 2000/01/31 10:00:18 adurant Exp $
#pragma once

#ifndef __PLYABLT__H
#define __PLYABLT__H

#include <plyablty.h>
#include <matrixs.h>
#include <mschbase.h>
#include <plyrbase.h>

// this only needs to be used by modules that want to instantiate
// player abilities.  It defines the data structures that get passed in
// to the ability constructor (and are forward declared in plyablty.h)

// this is used instead of skill selection desc for player, since 
// want player to always execute a pre-defined set of maneuvers when
// he start/ends/aborts using an item.  (Aborting using an item is what
// happens when player wants to switch items.)
// We don't need to match skills to best fit arbitrary control inputs.
typedef struct sSkillConnectDesc
{
   int nextDefault; // skill to perform next if no inputs received
   int nextIfStart; // skill to perform next if start action input received
   int nextIfFinish;// skill to perform next if finish action input received
   int nextIfAbort; // skill to perform next if abort action input received
} sSkillConnectDesc;

struct sPlayerSkillFullDesc
{
   int controllerID;
   ePlayerTransition trans;
   sSkillConnectDesc connect;
   sMPlayerSkillData skillData;
   char *tagNames;
   char *sndSchemaName;
   float timeToMax; 
   int mouseSpeed;
   int mouseZone;
   float slewSpeedScale;
   float rotateSpeedScale;
   BOOL IsIdleMotion;
};

#endif


