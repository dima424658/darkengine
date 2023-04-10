/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkplpup.cpp,v 1.8 2000/02/19 13:25:57 toml Exp $

#include <shkplpup.h>

#include <appagg.h>
#include <matrixs.h>

#include <iobjsys.h>
#include <osysbase.h>
#include <objpos.h>
#include <playrobj.h>
#include <creature.h>
#include <shkcret.h>
#include <cretprop.h>
#include <mnamprop.h>
#include <propman.h>
#include <property.h>
#include <pupprop.h>
#include <rendprop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#ifdef AI_RIP
//include <aitype.h>
//include <aiext.h>
#endif

ObjID g_playerPuppetID = OBJ_NULL;
eRenderType g_playerRendType = kRenderNormally;

// Create player "puppet" object for playing motions on
// This is a basic object with a creature and puppet property
// Replace any existing player puppet model
// Hide the player object model
ObjID PlayerPuppetCreate(const char *pModelName)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);
   ObjID objID;
   ObjPos *pObjPos = ObjPosGet(PlayerObject());
   mxs_vector *pPos = &(pObjPos->loc.vec);
   mxs_angvec *pAng = &(pObjPos->fac);
   AutoAppIPtr_(PropertyManager, pPropMan);

   if (g_playerPuppetID != OBJ_NULL)
      PlayerPuppetDestroy();

   objID = pObjSys->BeginCreate(ROOT_ARCHETYPE, kObjectConcrete);
   if (objID != OBJ_NULL)
   {
      ObjPosUpdate(objID, pPos, pAng);
      ObjSetModelName(objID, (char*)pModelName);
      ObjSetCreatureType(objID, kShCRTYPE_Humanoid);

      /*
      pProp = pPropMan->GetPropertyNamed(PROP_PUPPET_NAME);
      pProp->QueryInterface(IID_IPuppetProperty, (void**)&pPuppetProp);
      pPuppetProp->Create(objID);
      */
#ifdef AI_RIP
      AICreateNewAI(kAITT_GoodGuy, FALSE, objID);
#endif

      pObjSys->NameObject(objID, "PlayerPuppet");

      pObjSys->EndCreate(objID);

      // hide the player object
      g_playerRendType = ObjRenderType(PlayerObject());
      ObjSetRenderType(PlayerObject(), kRenderNotAtAll);

      g_playerPuppetID = objID;
   }   
   return objID;
}

// Destroy the player puppet object and restore the render type of the old player object 
void PlayerPuppetDestroy(void)
{
   AutoAppIPtr_(ObjectSystem, pObjSys);

   if (g_playerPuppetID != OBJ_NULL)
   {
      pObjSys->Destroy(g_playerPuppetID);
      g_playerPuppetID = OBJ_NULL;
      // restore old player object render type
      ObjSetRenderType(PlayerObject(), g_playerRendType);
   }
}
