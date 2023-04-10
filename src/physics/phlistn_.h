/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/phlistn_.h,v 1.3 2000/01/29 13:32:48 adurant Exp $
#pragma once

#ifndef __PHLISTEN_H
#define __PHLISTEN_H

#include <dynarray.h>

#include <phlistyp.h>
#include <phlisbas.h>
#include <phystyp2.h>

#include <subscrip.h>

class cPhysListener
{
public:
   cPhysListener(const char* pName, PhysListenerHandle h, PhysListenMsgSet i, PhysListenFunc f, PhysListenerData d) :
      name(pName),
      handle(h),
      interests(i),
      subscriptions(0),
      func(f),
      data(d)
   {}

   PhysListenerHandle GetHandle(void) const {return handle;}
   PhysListenMsgSet GetType(void) const {return interests;}
   const char* GetName(void) const {return (const char*)name;}

   void Subscribe(ObjID objID) {subscriptions.Subscribe(objID);}
   void Unsubscribe(ObjID objID) {subscriptions.Unsubscribe(objID);}

   BOOL IsSubscribed(ObjID objID, ePhysScriptMsgType type) {return ((type&interests) && subscriptions.IsSubscribed(objID, 0));}

   void Call(ObjID objID, const sPhysListenMsg* pMsg) {func(objID, pMsg, data);}

   void ReadSubscriptions(PhysReadWrite readFunc) {subscriptions.Read(readFunc);}
   void WriteSubscriptions(PhysReadWrite writeFunc) {subscriptions.Write(writeFunc);}
   void ResetSubscriptions(void) {subscriptions.Reset();}

private:
   cStr name;
	PhysListenMsgSet interests; 
   cSubscribeService subscriptions;
	PhysListenFunc func;
	PhysListenerData data; 
	PhysListenerHandle handle; 
}; 

//
// LISTENERS
//

class cPhysListeners
{
public:
   cPhysListeners(): m_handle(0) {};
   ~cPhysListeners();

   PhysListenerHandle CreateListener(const char* pName, PhysListenMsgSet set, PhysListenFunc func, PhysListenerData data);
   void DestroyListener(PhysListenerHandle handle);

   void Listen(PhysListenerHandle, ObjID);
   void Unlisten(PhysListenerHandle, ObjID);

   BOOL HasListener(ObjID objID, ePhysScriptMsgType_ type);

   void CallCollision(ObjID objID, int submod, ePhysCollisionType type, ObjID coll_object, 
      int coll_submod, const mxs_vector &normal, mxs_real momentum, const mxs_vector &pt);

   void Read(PhysReadWrite func);
   void Write(PhysReadWrite func);
   void Reset(void);

private:
   int m_handle;
   cDynArray<cPhysListener*> m_listeners; // listener set

   cPhysListener* Find(PhysListenerHandle);
   int FindIndex(PhysListenerHandle handle);
   cPhysListener* FindByName(const char* pName);
};

#endif