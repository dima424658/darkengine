/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/collprop.cpp,v 1.13 1998/10/24 19:52:22 CCAROLLO Exp $

//
// Collision property
//

#include <collprop.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <sdestool.h>

// must be last header
#include <dbmem.h>

static IIntProperty* collisionprop = NULL;

////////////////////////////////////////////////////////////
// COLLISION PROPERTY CREATION 
//

#define COLLISIONPROP_IMPL kPropertyImplDense

static sPropertyDesc collisionprop_desc =
{
   PROP_COLLISION_NAME,
   kPropertyInstantiate,
   NULL, 0, 0,
   { "Physics: Misc", "Collision Type" }
};

static const char* coll_bits[] =
{ 
   "Bounce", 
   "Destroy on Impact", 
   "Slay on Impact", 
   "No Collision Sound",
   "No Result",
   "Full Collision Sound",
};
 
#define COLLTYPE_NAME "CollType"

static sFieldDesc coll_field[] = 
{
   { "", kFieldTypeBits, sizeof(int), 0, kFieldFlagUnsigned, 0, 6, 6, coll_bits },
}; 

static sStructDesc coll_sdesc = 
{
   COLLTYPE_NAME, 
   sizeof(int),
   kStructFlagNone,
   sizeof(coll_field)/sizeof(coll_field[0]),
   coll_field,
}; 

static sPropertyTypeDesc coll_tdesc = { COLLTYPE_NAME, sizeof(int)}; 
   

// Init the property
void CollisionTypePropInit(void)
{
   StructDescRegister(&coll_sdesc);
   collisionprop = CreateIntegralProperty(&collisionprop_desc, &coll_tdesc, COLLISIONPROP_IMPL);
   
}

// get and set functions
BOOL ObjGetCollisionType(ObjID obj, int *num)
{
   Assert_(collisionprop);
   int temp;
   BOOL retval = collisionprop->Get(obj, &temp);
   if (retval)
      *num = temp;
   else
      *num = COLLISION_BOUNCE;
   return TRUE;
}

void ObjSetCollisionType(ObjID obj, int num)
{
   collisionprop->Set(obj, num);
}

