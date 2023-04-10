/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/ccrhuman.cpp,v 1.2 1998/10/05 17:20:39 mahk Exp $

#include <creature.h>
#include <ccrhuman.h>
#include <matrixs.h>
#include <math.h>

// must be last header
#include <dbmem.h>

class cHumanoidCreatureFactory : public cCreatureFactory
{
   virtual cCreature *Create(int type, ObjID objID) 
        { return new cHumanoidCreature(type,objID); }
   virtual cCreature *CreateFromFile(int type, ObjID objID, fCreatureReadWrite func, int version)
        { return new cHumanoidCreature(type, objID, func, version); }
};

static cHumanoidCreatureFactory g_HumanoidCreatureFactory;
cCreatureFactory *g_pHumanoidCreatureFactory=&g_HumanoidCreatureFactory;
