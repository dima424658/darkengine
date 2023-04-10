/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/sndprop.cpp,v 1.4 1999/12/01 20:12:22 BFarquha Exp $
// Collision schema properties

#include <sndprop.h>
#include <propbase.h>

// Must be last header
#include <dbmem.h>


// Collide with arrow sound
ILabelProperty *pPropCollideWithArrowSnd;

static sPropertyDesc CollideWithArrowSndDesc =
{
   PROP_COLLIDE_WITH_ARROW_SND, 0,
};

void CollideWithArrowSndPropertyInit(void)
{
   pPropCollideWithArrowSnd = CreateLabelProperty(&CollideWithArrowSndDesc, kPropertyImplDense);
}

// get collision sound
Label *CollideWithArrowSndGet(ObjID objID)
{
   Label *pCollideWithArrowSndLabel = NULL;

   pPropCollideWithArrowSnd->Get(objID, &pCollideWithArrowSndLabel);
   return pCollideWithArrowSndLabel;
}

// Collide with projectile sound
ILabelProperty *pPropCollideWithProjectileSnd;

static sPropertyDesc CollideWithProjectileSndDesc =
{
   PROP_COLLIDE_WITH_PROJECTILE_SND, 0,
};

void CollideWithProjectileSndPropertyInit(void)
{
   pPropCollideWithProjectileSnd = CreateLabelProperty(&CollideWithProjectileSndDesc, kPropertyImplDense);
}

// get collision sound
Label *CollideWithProjectileSndGet(ObjID objID)
{
   Label *pCollideWithProjectileSndLabel = NULL;

   pPropCollideWithProjectileSnd->Get(objID, &pCollideWithProjectileSndLabel);
   return pCollideWithProjectileSndLabel;
}

// Hey, it's a property that's a property name!
ILabelProperty *pPropCollideSndProperty;

static sPropertyDesc CollideSndPropertyDesc =
{
   PROP_COLLIDE_SND_PROPERTY, 0,
};

void CollideSndPropertyPropertyInit(void)
{
   pPropCollideSndProperty = CreateLabelProperty(&CollideSndPropertyDesc, kPropertyImplDense);
}

Label *CollideSndPropertyGet(ObjID objID)
{
   Label *pCollideSndPropertyLabel = NULL;

   pPropCollideSndProperty->Get(objID, &pCollideSndPropertyLabel);
   return pCollideSndPropertyLabel;
}


void SoundPropsInit(void)
{
   CollideWithArrowSndPropertyInit();
   CollideWithProjectileSndPropertyInit();
   CollideSndPropertyPropertyInit();
}

void SoundPropsClose(void)
{
   SafeRelease(pPropCollideWithArrowSnd);
   SafeRelease(pPropCollideWithProjectileSnd);
   SafeRelease(pPropCollideSndProperty);
}
