/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/cretprop.h,v 1.11 2000/01/29 13:22:02 adurant Exp $
#pragma once

#ifndef __CRETPROP_H
#define __CRETPROP_H

#include <property.h>
#include <creature.h>
#include <objtype.h>
#include <label.h>

//------------------------------------------------------------
// Creature Property
//

#undef INTERFACE
#define INTERFACE ICreatureProperty
DECLARE_PROPERTY_INTERFACE(ICreatureProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sCreatureHandle*); 
};

#define PROP_CREATURE_NAME "Creature"


//
// Creature pose property
//

typedef struct sCreaturePose
{
   int type;
   char motion[80];
   float frac;
   float scale;
   BOOL ballistic;

   sCreaturePose()
   {
      type = 0;
      memset(motion, 0, sizeof(char) * 80);
      frac = 0.0;
      scale = 1.0;
      ballistic = TRUE;
   };

} sCreaturePose;

enum eCreaturePoseType
{
   kCPT_Tags,
   kCPT_MotionName,
   kCPT_Invalid=0xffffffff,
};

#define PROP_CREATUREPOSE_NAME "CretPose"

F_DECLARE_INTERFACE(ICreaturePoseProperty);
#undef INTERFACE
#define INTERFACE ICreaturePoseProperty

DECLARE_PROPERTY_INTERFACE(ICreaturePoseProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sCreaturePose*);
};

//
//  Creature "non-physical" property
//

#define PROP_CREATURENONPHYS_NAME "NonPhysCreature"

//
// functions
//

EXTERN ICreatureProperty *CreaturePropertiesInit(int nCreatureTypes, const char **pCreatureTypeNames);

EXTERN void CreaturePropertiesTerm();

EXTERN void ObjSetCreatureType(ObjID obj, int type);

EXTERN BOOL ObjGetCreatureType(ObjID obj, int *pType);

EXTERN BOOL ObjIsNonPhysicalCreature(ObjID obj);

EXTERN BOOL ObjIsPosed(ObjID obj);

#endif
