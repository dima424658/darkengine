/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/keysys.cpp,v 1.1 1998/06/25 22:50:45 XEMU Exp $

// COM fun
#include <appagg.h>
#include <aggmemb.h>

#include <objtype.h>
#include <objdef.h>

#include <keysys.h>
#include <keyprop.h>
#include <lockprop.h>

// must be last header
#include <dbmem.h>
#include <initguid.h>
#include <keyiid.h>

class cKeySys : public cCTDelegating<IKeySys>,
                public cCTAggregateMemberControl<kCTU_Default>
{

public:
   cKeySys(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, IKeySys, kPriorityNormal, NULL);
   }

   STDMETHOD_(BOOL, TryToUseKey)(ObjID key_obj, ObjID lock_obj, eKeyUse how)
   {
      AssertMsg(KeySrcProp && KeyDstProp, "No Key Property, cant call key services");
      sKeyInfo *key, *lock;
      if (!KeySrcProp->Get(key_obj,&key))
         return FALSE;
      if (!KeyDstProp->Get(lock_obj,&lock))
         return FALSE;
      if (!KeyWorksOnLock(key,lock))
         return FALSE;        // no matter what, it didnt do anything
      if (how==kKeyUseCheck)  // if we are just checking, the key matches
         return TRUE;         //   so we are done, go home, so on
      if ( (how==kKeyUseDefault)||
          ((how==kKeyUseOpen)&&(ObjSelfLocked(lock_obj)))||
          ((how==kKeyUseClose)&&(!ObjSelfLocked(lock_obj))) )
         ObjSetSelfLocked(lock_obj,!ObjSelfLocked(lock_obj));
      else       // toggle the selflocked, since we need to do something
         return FALSE;   // this is a case where the lock state doesnt match
      return TRUE;
   }
};

// creation func
void KeySysCreate()
{
   AutoAppIPtr(Unknown); 
   new cKeySys(pUnknown); 
}

