/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/partprop.cpp,v 1.3 1998/10/05 17:26:57 mahk Exp $

//
// Particle property
//

#include <partprop.h>

#include <property.h>
#include <propface.h>
#include <propbase.h>

// Must be last header 
#include <dbmem.h>


static IIntProperty* particleprop = NULL;

////////////////////////////////////////////////////////////
// PARTICLE PROPERTY CREATION 
//

#define PARTICLEPROP_IMPL kPropertyImplDense

static sPropertyDesc particleprop_desc =
{
   PROP_PARTICLE_NAME,
   0, NULL, 0, 0, 
   { "SFX", "Particle" }, 
};

// Init the property
// this really should be a void or BOOL property
void ParticlePropInit(void)
{
   particleprop = CreateIntProperty(&particleprop_desc, PARTICLEPROP_IMPL);
}

// get and set functions
BOOL ObjIsParticle(ObjID obj)
{
   Assert_(particleprop);
   return particleprop->IsRelevant(obj);
}

void ObjSetParticle(ObjID obj)
{
   Assert_(particleprop);
   particleprop->Set(obj, 1);
}
