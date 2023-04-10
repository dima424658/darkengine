/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phppscpt.cpp,v 1.5 2000/02/19 12:32:21 toml Exp $
//
#include <lg.h>

#include <objtype.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <netman.h>
#include <iobjnet.h>

#include <phppscpt.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

IMPLEMENT_SCRMSG_PERSISTENT(sPressurePlateMsg)
{
   PersistenceHeader(sPressurePlateMsg, kPressurePlateMsgVer);
   return TRUE;
}

void PressurePlateMessage(ObjID objID, int message)
{
#ifdef NEW_NETWORK_ENABLED
   // If the object is just a proxy, don't send the messages -- the
   // host machine for the object should deal with it.
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking()) {
      AutoAppIPtr(ObjectNetworking);
      if (pObjectNetworking->ObjIsProxy(objID)) {
         return;
      }
   }
#endif

   AutoAppIPtr(ScriptMan);

   sPressurePlateMsg pp_msg(objID, message);
   pScriptMan->SendMessage(&pp_msg);
}
