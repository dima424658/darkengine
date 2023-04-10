/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/crattach.h,v 1.8 2000/01/29 13:21:59 adurant Exp $
#pragma once

#ifndef __CRATTACH_H
#define __CRATTACH_H

#include <matrixs.h>
#include <objtype.h>
#include <crettype.h>
#include <relation.h>

////////////////////////////////////////

#define CREATURE_ATTACH_RELATION_NAME "CreatureAttachment"

EXTERN IRelation *g_pCreatureAttachRelation;

struct sCreatureAttachInfo
{
   int  joint;
   BOOL is_weapon;

   mxs_trans relTrans;
   int weapon; // kCrWeap_Invalid if not a weapon
};

////////////////////////////////////////

EXTERN void InitCreatureAttachments();
EXTERN void TermCreatureAttachments();
EXTERN BOOL CreatureAttachItem(ObjID creature, ObjID item, const sCreatureAttachInfo *pAttachInfo);
EXTERN void CreatureDetachItem(ObjID creature, ObjID item);
EXTERN void CreatureDetachAllItems(ObjID creature);
EXTERN void CreatureAttachmentsPhysUpdate(ObjID creature);
EXTERN void CreatureAttachmentsPosUpdate(ObjID creature);
EXTERN BOOL CreatureAttachWeapon(ObjID creature, ObjID weaponObj, int weaponType);
EXTERN void CreatureDetachWeapon(ObjID creature, ObjID weaponObj);
EXTERN BOOL CreatureMakeWeaponPhysical(ObjID creature, ObjID weapon, int weaponType);
EXTERN void CreatureMakeWeaponNonPhysical(ObjID creature, ObjID weapon);
EXTERN void CreatureAttachmentModify(ObjID creature, ObjID weapon, sCreatureAttachInfo *pAttachInfo);
EXTERN sCreatureAttachInfo* CreatureAttachmentGet(ObjID creature, ObjID item, const int& inIndex);
////////////////////////////////////////

#endif
