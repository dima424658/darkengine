/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndprop.h,v 1.4 2000/01/31 10:02:37 adurant Exp $
// Properties that go on objects to trigger sounds (schemas)
#pragma once

#ifndef __SNDPROP_H
#define __SNDPROP_H

#include <propface.h>

// Collision with arrow schema property
#define PROP_COLLIDE_WITH_ARROW_SND "CSArrow"
EXTERN ILabelProperty *gPropCollideWithArrowSnd;
EXTERN void CollideWithArrowSndPropInit(void);
EXTERN Label *CollideWithArrowSndGet(ObjID objID);
#define COLLISION_SCHEMA_SET(objID, schemaID) PROPERTY_SET(gPropCollideWithArrowSnd, (objID), (schemaID))

// What collide property do I look for property?
#define PROP_COLLIDE_SND_PROPERTY "CSProperty"
EXTERN ILabelProperty *gPropCollideSndProperty;
EXTERN void CollideSndPropertyPropInit(void);
EXTERN Label *CollideSndPropertyGet(ObjID objID);
#define COLLISION_SND_PROPERTY_SET(objID, schemaID) PROPERTY_SET(gPropCollideSndProperty, (objID), (schemaID))

// Collision with projectile schema property
#define PROP_COLLIDE_WITH_PROJECTILE_SND "CSProjectile"
EXTERN ILabelProperty *gPropCollideWithProjectileSnd;
EXTERN void CollideWithProjectileSndPropInit(void);
EXTERN Label *CollideWithProjectileSndGet(ObjID objID);
#define COLLISION_PROJECTILE_SET(objID, schemaID) PROPERTY_SET(gPropCollideWithProjectileSnd, (objID), (schemaID))

EXTERN void SoundPropsInit(void);
EXTERN void SoundPropsClose(void);

#endif
