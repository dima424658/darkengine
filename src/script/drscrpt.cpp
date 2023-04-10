/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/drscrpt.cpp,v 1.6 2000/02/24 13:46:55 patmac Exp $
//

#include <lg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <objscrpt.h>
#include <drscrpt.h>

#include <doorphys.h>

// must be last header
#include <dbmem.h>

////////////////////////////////////////
// 
// Door scripting service implementation
//

DECLARE_SCRIPT_SERVICE_IMPL(cDoorSrv, Door)
{
public:
   STDMETHOD_(BOOL, CloseDoor)(object door_obj)
   {      
      ObjID obj = ScriptObjID(door_obj);

      if (!IsDoor(obj))
         return FALSE;

      if ((GetDoorStatus(obj) == kDoorClosed) ||
          (GetDoorStatus(obj) == kDoorClosing))
         return FALSE;

      DoorPhysActivate(obj, DOORPHYS_CLOSE);
      return TRUE;
   }

   STDMETHOD_(BOOL, OpenDoor)(object door_obj)
   {
      ObjID obj = ScriptObjID(door_obj);

      if (!IsDoor(obj))
         return FALSE;

      if ((GetDoorStatus(obj) == kDoorOpen) ||
          (GetDoorStatus(obj) == kDoorOpening))
         return FALSE;

      DoorPhysActivate(obj, DOORPHYS_OPEN);
      return FALSE;
   }

   STDMETHOD_(int, GetDoorState)(object door_obj)
   {
      ObjID obj = ScriptObjID(door_obj);

      if (!IsDoor(obj))
         return kDoorNoDoor;

      return GetDoorStatus(obj);
   }

   STDMETHOD(ToggleDoor)(object door_obj)
   {
      ObjID obj = ScriptObjID(door_obj);

      if (!IsDoor(obj))
         return FALSE;

      return DoorPhysActivate(obj, DOORPHYS_TOGGLE);
   }

   STDMETHOD(SetBlocking)(object door_obj, BOOL state)
   {
      ObjID obj = ScriptObjID(door_obj);

      if (!IsDoor(obj))
         return FALSE;

      if (state)
         AddDoorBlocking(obj);
      else
         RemoveDoorBlocking(obj);

      return TRUE;
   }

   STDMETHOD_(BOOL, GetSoundBlocking)(object door_obj)
   {
      ObjID obj = ScriptObjID(door_obj);

      if (!IsDoor(obj))
         return FALSE;

      return GetDoorSoundBlocking(obj);
   }

      
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDoorSrv, Door);

////////////////////////////////////////
//
// Door scripting message implementation
//

IMPLEMENT_SCRMSG_PERSISTENT(sDoorMsg)
{
   PersistenceHeader(sScrMsg, kDoorMsgVer);
   PersistentEnum(ActionType);
   return TRUE;
}
