/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drknetap.cpp,v 1.1 1999/10/20 16:34:28 Justin Exp $
//
// Dark implementation of required network services
//
// This implements the functions defined in netapp.h, which are required
// by netman.cpp.
//

#include <appagg.h>
#include <aggmemb.h>

#include <netapp.h>
#include <drknetap.h>

#include <config.h>
#include <iobjsys.h>
#include <netman.h>

// Include these absolutely last
#include <dbmem.h>

#define DEFAULT_AVATAR_NAME "Default Avatar"

// This code structure was modeled roughly on quest.cpp, which seemed
// closest to what I was trying to accomplish:
class cDarkNetServices: public cCTDelegating<INetAppServices>,
                        public cCTAggregateMemberControl<kCTU_Default>
{
   static sRelativeConstraint gConstraints[]; 

public:
   cDarkNetServices(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, INetAppServices,
                            kPriorityNormal, gConstraints);
   }

   STDMETHOD(Init)() {return S_OK;}
   STDMETHOD(End)() {return S_OK;}

   // Get this player's avatar. This code is directly ripped off from
   // Shock, and assumes that the gamesys has objects "Default Avatar 1",
   // "2", "3", and so on. It's pretty utterly stupid, and future
   // generations should feel more than welcome to do something cleverer
   // and more dynamic here.
   STDMETHOD_(ObjID, MyAvatarArchetype)()
   {
      char abstractName[128];
      ObjID abstractPlayer = OBJ_NULL;
      AutoAppIPtr_(ObjectSystem, pObjSys);
      AutoAppIPtr(NetManager);
      ulong myPlayerNum = pNetManager->MyPlayerNum();

      // The net_abstract_player is the archetype to be created for
      // our player on other player's machines.
      if (!config_get_raw("net_avatar", 
                          abstractName, 
                          sizeof abstractName))
      {
         // Use the default player
         sprintf(abstractName, "%s %d", DEFAULT_AVATAR_NAME, myPlayerNum);
         abstractPlayer = pObjSys->GetObjectNamed(abstractName);
         AssertMsg1(abstractPlayer,
                    "Default avatar %s not in gamesys!",
                    abstractName);
      } else {
         abstractPlayer = pObjSys->GetObjectNamed(abstractName);
         if (abstractPlayer == OBJ_NULL) {
            Warning(("Unknown abstract player object %s",abstractName));
            // Use the default player
            sprintf(abstractName, "%s %d", DEFAULT_AVATAR_NAME, 
                    myPlayerNum);
            abstractPlayer = pObjSys->GetObjectNamed(abstractName);
            AssertMsg1(abstractPlayer,
                       "Default avatar %s not in gamesys!",
                       abstractName);
         }
      }

      return abstractPlayer;
   }
};

sRelativeConstraint cDarkNetServices::gConstraints[] = 
{
   { kNullConstraint }, 
}; 

void DarkNetServicesCreate()
{
   AutoAppIPtr(Unknown); 
   new cDarkNetServices(pUnknown); 
};
