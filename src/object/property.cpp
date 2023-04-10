/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/property.cpp,v 1.65 1999/11/05 14:26:37 Justin Exp $
#include <mprintf.h>

#include <propert_.h>
#include <propstat.h>
#include <propnet.h>
#include <propguid.h>
#include <objnotif.h>
#include <trait.h>
#include <traitman.h>
#include <traitbas.h>
#include <propmix.h>
#include <osysbase.h>
#include <dlistsim.h>
#include <dlisttem.h>
#include <iobjed.h>
#include <objquery.h>
#include <dataops_.h>
#include <playrobj.h>
#include <timer.h>

#include <config.h>
#include <cfgdbg.h>
#include <netman.h>
#include <iobjnet.h>
#include <allocapi.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// CLASS: cStoredPropertyStats
//

class cStoredPropertyStats: public cCTDelegating<IPropertyStats> 
{
   cStoredProperty* mpProp; 

public:
   cStoredPropertyStats(IUnknown* outer, cStoredProperty* prop) 
      : cCTDelegating<IPropertyStats> (outer),
        mpProp(prop)
   {
   }

   STDMETHOD_(sPropTimeStats*,GetTimeStats)()
   {
      return mpProp->GetStoreTimeStats();
   }
} ; 

////////////////////////////////////////////////////////////
// CLASS: cStoredPropertyNetworking
//

#ifndef SHIP
#define NetPropSpew(msg) {if (gmbDoSpew) {mprintf msg;}}
#else
#define NetPropSpew(msg)
#endif

class cStoredPropertyNetworking: public cCTDelegating<IPropertyNetworking> 
{
   cStoredProperty* mpProp; 
   BOOL mbHandlingMessage;  // Am I responding to a network broadcast message.
   static tNetMsgHandlerID gmNetMsgHandlerID; // shared handler ID for our callback.
   // TRUE iff we should do extensive spewage:
   static BOOL gmbDoSpew;

   // Networking message structure.
   struct sPropertyNetMsg
   {
      tNetMsgHandlerID handlerID; // handler ID of the property manager component (used by NetMan).
      short propertyID;         // property sending message
      NetObjID netObjID;        // ObjID of the object on its host machine.
      ubyte type;               // the type of change happening to the property.
      char valueData[1];        // I wish this could be [0], but we get warnings if I do.
   };

   // Called when a property message is recieved.  Will go to the appropriate prop for handling.
   static void NetworkMessageCallback(const sNetMsg_Generic *pMsg, ulong size, ObjID from, void *)
   {
      sPropertyNetMsg *msg = (sPropertyNetMsg *)pMsg;
      IProperty *prop = GetProperty(msg->propertyID);
      IPropertyNetworking *propnet;
      if (prop->GetID() == PROPID_NULL)
         Warning(("Recieved invalid property ID"));
      else if (!SUCCEEDED(prop->QueryInterface(IID_IPropertyNetworking, (void **)&propnet)))
         Warning(("Received property message with an ID of a property without networking\n"));
      else
         propnet->ReceivePropertyMsg(pMsg, size, from);
      SafeRelease(propnet);
      SafeRelease(prop);
   }

   // Networking depends on in-place changes being made by calling
   // Touch instead of Set, since Touch can't be requested from a
   // remote object host. We detect whether Set is being called with
   // the same pointer to value that get returns.
   BOOL IsInPlaceSet(ObjID obj, sDatum value)
   {
      IDataOps* ops = mpProp->mpStore->GetOps();
      AssertMsg(ops!=NULL, "Networked properties need data_ops");

      sDatum currentVal;
      return (mpProp->mpStore->Get(obj, &currentVal) 
              && ops->BlockSize(currentVal) >= 0  //... currentVal is a pointer
              && currentVal == value);            //... which is the same as value
   }      

public:
   cStoredPropertyNetworking(cStoredProperty* prop) 
      : cCTDelegating<IPropertyNetworking> (NULL), // Outer set when it's first QI'ed from prop.
        mpProp(prop),
        mbHandlingMessage(FALSE)
   {
      if (gmNetMsgHandlerID == 0) {
         gmNetMsgHandlerID =
            mpProp->gmNetMan->RegisterMessageParser(NetworkMessageCallback,
                                                    "Prop", 0, NULL);
#ifndef SHIP
#ifdef SPEW_ON
         gmbDoSpew = config_is_defined("net_prop_spew")||config_is_defined("net_spew");
#else
         // Allow spew in opt builds *if* net_opt_spew is also set:
         gmbDoSpew = 
            ((config_is_defined("net_prop_spew") && config_is_defined("net_prop_opt_spew")) ||
             (config_is_defined("net_spew") && config_is_defined("net_opt_spew")));
#endif
#endif
      }
   }

   // Send network message regarding a change to this property.  If
   // type's kListenPropRequest bit is set, then this will send a
   // change request of to obj's host machine, otherwise it will send
   // a broadcast of a change that has happened.  Returns TRUE if it
   // sent the message.  @TODO: Send only the diff, the part that is
   // different from the current value.
   STDMETHOD_(BOOL,SendPropertyMsg)(ObjID obj, 
                                    sDatum value, 
                                    ePropertyListenMsg type)
   {
      if (mbHandlingMessage) {
         // we started this property change via a net message
         return FALSE;
      }

      ObjID toHostPlayer = OBJ_NULL; // NULL means it will be broadcast.

      // Are we requesting a change (as opposed to broadcasting one)
      if (type & kListenPropRequestFromHost)
      {
         toHostPlayer = mpProp->gmObjNet->ObjHostPlayer(obj);
         AssertMsg2(toHostPlayer != OBJ_NULL,
                    "Requesting change to unhosted property %d.%s!",
                    obj, mpProp->mDesc.name);
         // Check to make sure the value wasn't already changed
         // in-place by the caller.
         // We usually forbid in-place changes by clients, because it
         // violates the basic principle of how we work: property changes
         // are made on the host, *then* propagated to the client. We
         // do permit it as a temporary workaround, though, if you set
         // the net_permit_in_place_set config flag.
         AssertMsg2(!IsInPlaceSet(obj, value)
                    || config_is_defined("net_permit_in_place_set"),
                    "Networking error: must use Touch for %d.%s"
                    "when changing its value in-place",
                    obj, mpProp->mDesc.name);
         if (!(mpProp->mDesc.net_flags & kPropertyProxyChangable))
         {
            Warning(("Trying to change %d.%s from a proxy machine\n",
                     obj, mpProp->mDesc.name));
            return FALSE;
         }
      }

      AssertMsg2((type & kListenPropRequestFromHost) ||
                 mpProp->gmObjNet->ObjHostedHere(obj),
                 "Proxy machine is trying to change %d.%s!",
                 obj, mpProp->mDesc.name);

      int valSize;  // size of 'value'
      cDataOpsMemFile memFile;
      IDataOps* ops = mpProp->mpStore->GetOps();
      AssertMsg(ops!=NULL, "Networked properties need data_ops");

      if (type == kListenPropUnset)
         valSize = 0;  // Don't send the vale for Unset messages.
      else
      {
         // Reset the memFile buffer.
         memFile.Seek(kDataOpSeekFromStart, 0);
         // Cause value to be written to our memFile buffer, this
         // flattens it out.
         ops->Write(value, &memFile);
         // Determine the value's size, and create an appropriately
         // sized network message. Note that, if the value is a NULL
         // pointer, valSize should come out as 0:
         valSize = memFile.Tell();
      }
      int msgSize = valSize + sizeof(sPropertyNetMsg);
      sPropertyNetMsg *msg = (sPropertyNetMsg *)malloc(msgSize);
      // Subtract out the compile-time known size of valueData, which
      // the C compiler forced me to add in, since it wouldn't allow
      // valueData to be defined with 0 size.
      msgSize -= sizeof(msg->valueData);
      // Fill the valueData field with the flattened out value.
      memcpy(msg->valueData, memFile.GetBuffer(), valSize);
      // Fill in the header fields.
      msg->handlerID = gmNetMsgHandlerID;
      msg->propertyID = mpProp->mID;
      // the field has fewer bits, make sure no lost data:
      Assert_(msg->propertyID == mpProp->mID);
      msg->netObjID = mpProp->gmObjNet->ObjHostObjID(obj);
      msg->type = type;
      // the field has fewer bits, make sure no lost data:
      Assert_(msg->type == type);
      NetPropSpew(("SEND: property %d:%s type: 0x%x size: %d first word: %d\n",
                   msg->netObjID, mpProp->mDesc.name, msg->type,
                   valSize, *(int *)msg->valueData));
      if (toHostPlayer != OBJ_NULL)
         mpProp->gmNetMan->Send(toHostPlayer, msg, msgSize, TRUE);
      else
         mpProp->gmNetMan->Broadcast(msg, msgSize, TRUE);
      free(msg);
      return TRUE;

      // @TODO: Diffing.  To send the diff we would:
      //   - flatten out the current value into memFile
      //   - make a copy of the memFile buffer
      //   - flatten the new value
      //   - Send the difference.
      // To receive a diff we would need to:
      //   - flatten out the current value into memFile
      //   - apply the diff.
      //   - read from the memFile with the diff applied.
   }        

   STDMETHOD_(void,ReceivePropertyMsg)(const sNetMsg_Generic *pMsg, 
                                       ulong size, 
                                       ObjID fromPlayer)
   {
      sPropertyNetMsg *msg = (sPropertyNetMsg *)pMsg;

      if (fromPlayer == OBJ_NULL)
      {
         // We don't allow anonymous property sets, since we can't properly
         // deproxify them. It probably comes from a pre-Reset anyway.
         NetPropSpew(("Got property %d:%s message from unknown player.\n",
                      msg->netObjID, mpProp->mDesc.name));
         return;
      }

      Assert_(msg->type & (kListenPropSet | 
                           kListenPropModify | 
                           kListenPropUnset));
      Assert_(!(msg->type & (kListenPropLoad | kListenPropRebuildConcrete)));

      // Don't broadcast changes made to proxies, that are done in
      // response to a broadcast.
      mbHandlingMessage = !(msg->type & kListenPropRequestFromHost);

      // We intentionally do *not* assert that we own the object if this
      // was a request to us; it's possible that the object was rehosted
      // while the request was in transit. In this case, the Set() below
      // should result in a new request, to the new host...

      // Messages sent to the host arrive with the objID of the host.
      ObjID obj = (msg->type & kListenPropRequestFromHost) 
                  ? (ObjID)msg->netObjID 
                  : mpProp->gmObjNet->ObjGetProxy(fromPlayer, msg->netObjID);

      AutoAppIPtr(ObjectSystem);
      if (!pObjectSystem->Exists(obj))
      {
         // Presumably, this object has been deleted, and this message
         // represents a raceway condition:
         NetPropSpew(("Got property %s for non-existent object %d.\n",
                      mpProp->mDesc.name, obj));
         return;
      }

      if (msg->type & kListenPropUnset)
      {
         NetPropSpew(("RECEIVE: property %d:%s delete proxy: %d\n",
                      msg->netObjID, mpProp->mDesc.name, obj));
         mpProp->Delete(obj);  // Unset the property from obj.
      }
      else
      {
         IDataOps* ops = mpProp->mpStore->GetOps();
         AssertMsg(ops!=NULL, "Networked properties need data_ops");

         // Determine the size of valueData.  Involves subtracting out
         // the compile-time known size of valueData, which I wish I
         // could have made 0, but C++ won't let me.
         ulong headerSize = sizeof(sPropertyNetMsg) - sizeof(msg->valueData); 
         ulong valueSize = size - headerSize;
         if (valueSize > 0) {
            // Unflatten the data from msg->valueData into value, by
            // treating valueData as if it were a memory file.
            cDataOpsMemFile memFile(msg->valueData, valueSize);
            sDatum value = ops->New();
            ops->Read(&value,&memFile,ops->Version());

            NetPropSpew(("RECEIVE: property %d:%s type: 0x%x"
                         "size: %d first word: %d\n",
                         msg->netObjID, mpProp->mDesc.name,
                         msg->type, valueSize,
                         *(int *)msg->valueData));
            // Now call set
            mpProp->Set(obj, value);
            // Free up the space for the unflatten value.
            ops->Delete(value);
         } else {
            // valueSize is 0, which indicates that they set the value
            // to the NULL ptr. What *do* we do in response to this?
            NetPropSpew(("RECEIVE: property %d:%s type: 0x%x NULL VALUE\n",
                         msg->netObjID, mpProp->mDesc.name, msg->type));
            // Can't do this -- the underlying Set mechanism can't deal
            // with NULL pointers...
            // mpProp->Set(obj, NULL);
         }
      }
      mbHandlingMessage = FALSE;
   }
}; // end of class cStoredPropertyNetworking

tNetMsgHandlerID cStoredPropertyNetworking::gmNetMsgHandlerID = 0;
BOOL cStoredPropertyNetworking::gmbDoSpew = FALSE;

////////////////////////////////////////////////////////////
//
// cPropertyBase and cStored property were once the same class, which 
// is why their functions are intermingled here. 
//
////////////////////////////////////////////////////////////

INetManager *cStoredProperty::gmNetMan = NULL;
IObjectNetworking *cStoredProperty::gmObjNet = NULL;

//
// QueryInterface
//

HRESULT cStoredProperty::QI(IUnknown* me, REFIID id, void ** ppI)
{

   // @BUG: this violates COM rules; the interface isn't delegated. 
   if (id == IID_ITrait)
   {
      *ppI = mpDonors;
      mpDonors->AddRef();
      return S_OK;
   }

   if (id == IID_IPropertyStats)
   {
      if (!mpStats)
         mpStats = new cStoredPropertyStats(me,this); 
      *ppI = mpStats; 
      mpStats->AddRef(); 
      return S_OK; 
   }
   
#ifdef NEW_NETWORK_ENABLED
   if (id == IID_IPropertyNetworking && mpNetProp != NULL)
   {
      mpNetProp->InitDelegation(me); // Created without delgate. Initialize on (every) QI.
      *ppI = mpNetProp; 
      mpNetProp->AddRef(); 
      return S_OK; 
   }
#endif
   
   BOOL match = IsEqualOrIUnknownGUID(id,IID_IProperty);
 
   if (!match)
   {
      *ppI = 0;
      return ResultFromScode(E_NOINTERFACE);
   }
   *ppI = me;
   me->AddRef();
   return S_OK;   
}


//////////////////////////////
//
// Constructor; pass in name, and preconstructed implementation
//


IPropertyStore* InitBasePropertyStore(IPropertyStore* store, ulong flags);

cPropertyBase::cPropertyBase(const sPropertyDesc* desc) :
   mDesc(*desc), 
   mpImplied(NULL),
   mpRequired(NULL)
{
   CompileConstraints();
}

cStoredProperty::cStoredProperty(const sPropertyDesc* desc, IPropertyStore *store) 
   : cPropertyBase(desc),
     mpStore(InitBasePropertyStore(store,desc->flags)), 
     mpDonors(NULL),
     mpExemplars(NULL),
     mFlags(0),
     mpStats(NULL),
     mpNetProp(NULL)
{
#ifdef NEW_NETWORK_ENABLED
   if (gmNetMan == NULL)
      gmNetMan = AppGetObj(INetManager);
   Assert_(gmNetMan);
   if (gmObjNet == NULL)
      gmObjNet = AppGetObj(IObjectNetworking);
   Assert_(gmObjNet);
   if (!(mDesc.net_flags & kPropertyChangeLocally))
      mpNetProp = new cStoredPropertyNetworking(this);
#endif

   // Clear the statistic accumulation totals.
   memset(&mStoreStats,0,sizeof(mStoreStats)); 
   InitTraits();

   mAllocName = PROP_BLAME_NAME(desc->name); 
}

//////////////////////////////
//
// Destructor
//

cPropertyBase::~cPropertyBase ()
{
   delete mpImplied;
   delete mpRequired;
}

cStoredProperty::~cStoredProperty ()
{
   STOREDPROP_AUTO_BLAME(); 
   if (mpStore)
   {
      // Give allocation credit to the particular property.
      mpStore->Reset(); 
      int refs = mpStore->Release();
      if (refs > 0)
      {
         ConfigSpew("propstore_ref_spew",("Property store for %s has %d refs on exit\n",mDesc.name,refs)); 

         while (mpStore->Release() > 0)
            ; 
      }
   }  

   if (mpNetProp)
   {
      delete mpNetProp;
      mpNetProp = NULL;
   }

   mpStore = NULL; 
   SafeRelease(mpDonors);
   SafeRelease(mpExemplars); 
   delete mpStats; 
}


////////////////////////////////////////

void cPropertyBase::Notify (ePropertyNotifyMsg msg, PropNotifyData data)
{
   uObjNotifyData info;
   info.raw = data;
   switch(NOTIFY_MSG(msg))
   {
      case kObjNotifyDefault:
      case kObjNotifyPostLoad:
         // Recompile constraints
         CompileConstraints();
         break;
         
   }
}


// This is here to serve as a profiler entry point 
ObjID prop_inst_donor(ITrait* trait, ObjID obj)
{
   return trait->GetDonor(obj); 
}

////////////////////////////////////////

void cStoredProperty::Notify (ePropertyNotifyMsg msg, PropNotifyData data)
{
   STOREDPROP_AUTO_BLAME(); 
   cPropertyBase::Notify(msg,data); 


   uObjNotifyData info;
   info.raw = data;
   switch(NOTIFY_MSG(msg))
   {
      case kObjNotifyDelete:
         Delete(info.obj);
         break;

      case kObjNotifyBeginCreate:
         // Instantiate the property if necessary 
         if ((mDesc.flags & kPropertyInstantiate) && OBJ_IS_CONCRETE(info.obj))
         {
            ObjID example = prop_inst_donor(mpExemplars,info.obj); 
            if (example != OBJ_NULL)
            {
               Copy(info.obj,example);
               break;
            }
         }

         break;

      case kObjNotifyDefault:
      case kObjNotifyPostLoad:
      {
         sPropertyObjIter iter;
         // Give allocation credit to the particular property.
         mpStore->IterStart(&iter);

         ObjID obj;
         sDatum  value;
         // Give allocation credit to the particular property.
         while(mpStore->IterNext(&iter,&obj,&value))
         {
            EnforceRequirements(obj);
            PROP_POP_BLAME();
            CallListeners(kListenPropSet|kListenPropModify|kListenPropLoad,obj,value); 
            PROP_PUSH_BLAME(mAllocName); 
            EnforceImplications(obj);
         }
         mpStore->IterStop(&iter);

         break;
      }
         
      case kObjNotifyReset:
         // Give allocation credit to the particular property.
         mpStore->Reset();
         break;

      case kObjNotifySave:
         Save(info.db.save, msg);
         break;

      case kObjNotifyLoad:
         Load(info.db.load, msg);
         break;
   }

}

////////////////////////////////////////

PropListenerHandle cPropertyBase::Listener::gNextHandle = 0 ; 

PropListenerHandle cPropertyBase::Listen(PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data)
{
   int i = mListeners.Append(Listener(interests,func,data));
   return mListeners[i].handle; 
}

void cPropertyBase::Unlisten(PropListenerHandle handle)
{
   for (int i = 0; i < mListeners.Size(); i++)
   {
      if (mListeners[i].handle == handle)
      {
         mListeners.DeleteItem(i); 
         return;
      }
   }
   Warning(("Can't unlisten to handle %X, no such listener\n",handle)); 
}

PropListenerHandle cStoredProperty::Listen(PropertyListenMsgSet interests, PropertyListenFunc func, PropListenerData data)
{
   if (interests & (kListenPropRebuildConcrete|kListenPropRebuildConcreteRelevant))
      SetRebuildConcretes(TRUE); 
   return cPropertyBase::Listen(interests,func,data); 
}

void cPropertyBase::CallListeners(ePropertyListenMsg msg, ObjID obj, sDatum dat, ObjID donor)
{
   uPropListenerValue cbval;
   cbval.ptrval = dat; 
   OnListenMsg(msg,obj,cbval); 

   for (int i = 0; i < mListeners.Size(); i++)
   {
      Listener& l = mListeners[i];
      PropertyListenMsgSet mask = msg & l.interests;
      if (mask != 0)
      {
         sPropertyListenMsg message = { mask, mID, obj, dat.value, donor};
         
         l.func(&message,l.data);
      }
   }
}

////////////////////////////////////////

void cStoredProperty::CallListeners(ePropertyListenMsg msg, ObjID obj, sDatum value, ObjID donor)
{
   STOREDPROP_TIMER(Listener); 
   // invalidate trait
   if ((msg & (kListenPropSet|kListenPropUnset)) && !(msg & kListenPropLoad))
   {
      mpDonors->Touch(obj);
      if (mpDonors != mpExemplars)
         mpExemplars->Touch(obj);
   }

   cPropertyBase::CallListeners(msg,obj,value,donor); 
}

////////////////////////////////////////

HRESULT cStoredProperty::Create(ObjID obj)
{ 
   STOREDPROP_AUTO_BLAME(); 

   // call my method to get the property timing stuff 
   if (cStoredProperty::IsSimplyRelevant(obj)) 
      return S_FALSE;

   int msg = kListenPropSet; 
   //
   // Enforce "requires" constraints"
   //
   EnforceRequirements(obj);

   ObjID example = mpExemplars->GetDonor(obj);

   STOREDPROP_TIMER(Create); 
   sDatum value; 
   if (example != OBJ_NULL)
   {
      // Give allocation credit to the particular property.
      value = mpStore->Copy(obj,example); 
      msg |= kListenPropModify; 
   }
   else {
   // Give allocation credit to the particular property.
      value = mpStore->Create(obj);
   }

   PROP_TIMER_STOP(); 

   PROP_POP_BLAME(); 
   CallListeners(msg,obj,value); 
   PROP_PUSH_BLAME(mAllocName);

   // 
   // Enforce "autocreate" constraints
   //
   EnforceImplications(obj);

   return S_OK;
}

////////////////////////////////////////


HRESULT cStoredProperty::Copy(ObjID targ, ObjID src)
{
   if (src != OBJ_NULL)  // if by example, copy from the example
   {
      sDatum srcval; 

      // call our own relevance functions for timing 
      if (!cStoredProperty::IsSimplyRelevant(src))
         src = cStoredProperty::GetDonor(src); 

      if (src != OBJ_NULL)
      {
         PROP_PUSH_BLAME(mAllocName); 

         sDatum dat;
         ePropertyListenMsg msg = kListenPropModify; 
         if (!cStoredProperty::IsSimplyRelevant(targ))
         {
            EnforceRequirements(targ); 

            STOREDPROP_TIMER(Copy);

            dat = mpStore->Copy(targ,src); 

            PROP_TIMER_STOP(); 

            EnforceImplications(targ); 
            msg |= kListenPropSet; 
         }
         else 
         {
            STOREDPROP_TIMER(Copy);
            dat = mpStore->Copy(targ,src); 
            PROP_TIMER_STOP(); 
         }

         PROP_POP_BLAME(); 

         CallListeners(msg,targ,dat); 
         return S_OK;
      }
   }
   return S_FALSE; 
}

HRESULT cStoredProperty::Set(ObjID obj, sDatum value)
{
   ePropertyListenMsg msg = kListenPropModify;
#ifdef NEW_NETWORK_ENABLED
   // See if we need to synchronize changes to this property.
   if (gmNetMan->Networking() &&
       !gmNetMan->Suspended() &&
       mpNetProp != NULL && 
       gmObjNet->ObjIsProxy(obj))
   {
      if (mpNetProp->SendPropertyMsg(obj, value, msg | kListenPropRequestFromHost))
         return S_FALSE;
      // Otherwise, we still have to change the property.
   }
#endif
   STOREDPROP_TIMER(Set);
   PROP_PUSH_BLAME(mAllocName); 
   HRESULT result = mpStore->Set(obj,value); 
   if (result == S_OK)
      msg |= kListenPropSet; 

   PROP_POP_BLAME(); 
   PROP_TIMER_STOP(); 
   CallListeners(msg,obj,value); 
   return result; 
}   

////////////////////////////////////////

BOOL cStoredProperty::Touch(ObjID obj,sDatum* value)
{
   sDatum val; 
   if (!value) // got passed in NULL
   {
      STOREDPROP_TIMER(Touch); 
      value = &val; 
      // Give allocation credit to the particular property.
      if (!mpStore->Get(obj, value))
         return FALSE; 
   }
#ifdef NEW_NETWORK_ENABLED
   // Note that we allow any property to be changed locally *IF* you
   // suspend messaging around the Set(). We consider that to be a notice
   // that you think you know what you're doing.
   AssertMsg2(!(gmNetMan->Networking()
                && !gmNetMan->Suspended()
                && !(mDesc.net_flags & kPropertyChangeLocally)
                && gmObjNet->ObjIsProxy(obj)),
                "Can't change property %d.%s in-place (i.e. Touch) on a proxy machine",
                obj, mDesc.name);
#endif
   CallListeners(kListenPropModify,obj,*value);
   return TRUE;
}

////////////////////////////////////////

HRESULT cStoredProperty::Delete(ObjID obj)
{


   sDatum value; 
   STOREDPROP_TIMER(Get); 
   BOOL relevant = mpStore->Get(obj,&value);
   PROP_TIMER_STOP(); 

   if (relevant)
   {
      CallListeners(kListenPropUnset,obj,value);
      
      PROP_PUSH_BLAME(mAllocName); 
      STOREDPROP_TIMER(Delete); 

      HRESULT result = mpStore->Delete (obj);

      PROP_TIMER_STOP(); 
      PROP_POP_BLAME(); 
      
      if (mFlags & kRebuildConcretes)
         RebuildConcretes(obj);

      return result; 
   }
   return S_FALSE; 
}

////////////////////////////////////////

ObjID cStoredProperty::GetDonor(ObjID obj) const
{
   STOREDPROP_TIMER(GetDonor); 
	return mpDonors->GetDonor(obj);

}

ObjID cStoredProperty::GetExemplar(ObjID obj) const
{
   STOREDPROP_TIMER(GetExemplar); 
	return mpExemplars->GetDonor(obj);
}

////////////////////////////////////////

void cStoredProperty::SetStore(IPropertyStore* store)
{
   if (store) 
   {
      store->AddRef(); 
      SafeRelease(mpStore);
      mpStore = InitBasePropertyStore(store,mDesc.flags);
      mpStore->SetID(mID); 
      store->Release(); 
   }
   else 
   {
      SafeRelease(mpStore);
   }
}

////////////////////////////////////////////////////////////
// cStoredProperty Implementation
//
//

//------------------------------------------------------------
// CreateEditor
//
  
void cPropertyBase::CreateEditor(IProperty* prop)
{
   IObjEditors* edit = AppGetObj(IObjEditors);
   if (edit != NULL)
   {
      IUnknown* trait = (IUnknown*)edit->AddProperty(prop); 
      // I don't want the trait.  Just get rid of it. 
      SafeRelease(trait); 
      SafeRelease(edit);   
   }
}



//------------------------------------------------------------
// InitTraits()
//
// Create the traits for a property based on it's desc
//

//
// Trait predicate
//
BOOL PropTraitPred(ObjID obj, TraitPredicateData data)
{
   cStoredProperty* base = (cStoredProperty*)data;
   return base->IsSimplyRelevant(obj);
}

void cStoredProperty::InitTraits(void)
{
   sTraitDesc tdesc;
   memset(&tdesc,0,sizeof(tdesc));
   Assert_(sizeof(tdesc.name) >= sizeof(mDesc.name));
   strncpy(tdesc.name,mDesc.name,sizeof(mDesc.name));
   tdesc.name[sizeof(tdesc.name)-1] = '\0';

   sTraitPredicate pred = { PropTraitPred, (TraitPredicateData)this};

   AutoAppIPtr_(TraitManager,pTraitMan);

   if (mDesc.flags & kPropertyNoCache)
      tdesc.flags |= kTraitUncached;

   if (mDesc.flags & kPropertyNoInherit)
   {
#define CACHE_INSTANTIATION
#ifndef CACHE_INSTANTIATION
      tdesc.flags |= kTraitUncached; 
#endif // CACHE_INSTANTIATION
      mpExemplars = pTraitMan->CreateTrait(&tdesc,&pred);
      tdesc.flags |= kTraitUninherited;
      mpDonors = pTraitMan->CreateTrait(&tdesc,&pred); 
   }
   else
   {
      mpExemplars = pTraitMan->CreateTrait(&tdesc,&pred);
      mpDonors = mpExemplars;
      mpDonors->AddRef();
   }
}

//------------------------------------------------------------
// CompileConstraints
//
// Build constraint lists from descriptor
//

void cPropertyBase::CompileConstraints(void)
{
   delete mpImplied;
   delete mpRequired; 

   mpImplied = mpRequired = NULL;

   if (mDesc.constraints != NULL)
   {
      const sPropertyConstraint* c;
      for (c = mDesc.constraints; c->kind != kPropertyNullConstraint; c++)
      {
         // look up the property referred to by the constriant
         cAutoIPtr<IProperty> against(GetManager()->GetPropertyNamed(c->against));

         switch(c->kind)
         {
            case kPropertyAutoCreate:
               if (!mpImplied) mpImplied = new PropIDList; 
               mpImplied->Append(against->GetID());
               break;

            case kPropertyRequires:
               if (!mpRequired) mpRequired = new PropIDList; 
               mpRequired->Append(against->GetID());
               break;
         }

      }
   }
}

////////////////////////////////////////

void cPropertyBase::EnforceRequirements(ObjID obj)
{
   if (!mpRequired) return; 

   // check prerequisites
   PropIDList::cIter iter;
   for (iter = mpRequired->Iter(); !iter.Done(); iter.Next())
   {
      cAutoIPtr<IProperty> prop(GetManager()->GetProperty(iter.Value()));
      if (!prop->IsRelevant(obj))
         prop->Create(obj);
   }   
}

void cPropertyBase::EnforceImplications(ObjID obj)
{
   if (!mpImplied) return; 

   // check prerequisites
   PropIDList::cIter iter;
   for (iter = mpImplied->Iter(); !iter.Done(); iter.Next())
   {
      cAutoIPtr<IProperty> prop(GetManager()->GetProperty(iter.Value()));
      if (!prop->IsSimplyRelevant(obj))
         prop->Create(obj);
   }   
}


//------------------------------------------------------------
// SetRebuildConcretes
//
  
void cStoredProperty::SetRebuildConcretes(BOOL fRebuild) 
{ 
   if (fRebuild) 
   {
      mFlags |= kRebuildConcretes; 
      if (!(mFlags & kListeningHierarchy))
      {
         AutoAppIPtr(TraitManager);
         pTraitManager->Listen(RebuildHierarchyListener, this);
         mFlags |= kListeningHierarchy;
      }
   }
   else 
      mFlags &= ~kRebuildConcretes; 
   
}

//------------------------------------------------------------
// RebuildConcretes
//

void cStoredProperty::RebuildOneConcrete(ObjID obj)
{
   sDatum             value;
   uPropListenerValue cbval;

   // Give allocation credit to the particular property.
   BOOL relevant = mpStore->Get(obj, &value);
   ObjID donor = obj; 
   if (!relevant)
   {
      donor = mpDonors->GetDonor(obj); 
      // Give allocation credit to the particular property.
      relevant = mpStore->Get(donor, &value);
   }
   cbval.ptrval = value; 
   ePropertyListenMsg msg = kListenPropRebuildConcrete; 
   if (relevant)
      msg |= kListenPropRebuildConcreteRelevant; 
   CallListeners(msg,obj,value,donor); 
   RebuildConcrete(obj, relevant, cbval,donor);
}

void cStoredProperty::RebuildConcretes(ObjID obj)
{
   STOREDPROP_TIMER(RebuildConcretes); 

   if (OBJ_IS_CONCRETE(obj))
      RebuildOneConcrete(obj); 
   
   // This is no longer an "else" to deal with concrete archetypes...
   cAutoIPtr<IObjectQuery> pQuery(mpDonors->GetAllHeirs(obj, kObjectConcrete));
   for (; !pQuery->Done(); pQuery->Next())
      RebuildOneConcrete(pQuery->Object()); 
}

//------------------------------------------------------------
// RebuildHierarchyListener
//
  
void LGAPI cStoredProperty::RebuildHierarchyListener(const sHierarchyMsg* msg, HierarchyListenerData data)
{
   if ((((cStoredProperty *)data)->mFlags & kRebuildConcretes) &&
       ((cStoredProperty *)data)->mpExemplars->PossessedBy(msg->donor))
      ((cStoredProperty *)data)->RebuildConcretes(msg->obj);
}

//------------------------------------------------------------
// OnListenMsg
//
  
void cStoredProperty::OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue cbVal)
{
   if ((mFlags & kRebuildConcretes) &&
       (type & (kListenPropModify | kListenPropSet)))
      RebuildConcretes(obj);
#ifdef NEW_NETWORK_ENABLED
   // Is this a network game & are we the host changing the property in a way that
   // needs to be broadcast.
   if (!(type & (kListenPropLoad | kListenPropRebuildConcrete | kListenPropRequestFromHost)) 
       && gmNetMan->Networking() 
       && !gmNetMan->Suspended()
       && mpNetProp != NULL 
       && gmObjNet->ObjHostedHere(obj))
   {
      mpNetProp->SendPropertyMsg(obj, cbVal.ptrval, type);
   }
#endif
}


//------------------------------------------------------------
// CreateEditor
//
   //
   // Overridables
   // 


//------------------------------------------------------------
// InitBasePropertyStore

static IPropertyStore* InitBasePropertyStore(IPropertyStore* impl, ulong flags)
{

   if (impl) 
   {
      if (flags & kPropertyConcrete)
      {
         IPropertyStore* abstract = CreateGenericPropertyStore(kPropertyImplVerySparse);
         IPropertyStore* retval = new cMixedPropertyStore(abstract,impl);
         SafeRelease(abstract); 
         return retval; 
      }
      else
         impl->AddRef(); 
   }

   return impl;
}


////////////////////////////////////////////////////////////
// GetProperty()
//

//
// Hey look, here's where we use cPropertyManagerKnower to gratuitous advantage
//


class LookerUpper : public cPropertyManagerKnower
{
public:
   static IProperty* ByID(PropertyID id)
   {
      if (GetManager() == NULL)
         return NULL;
      return GetManager()->GetProperty(id);
   }

   static IProperty* ByName(const char* name)
   {
      if (GetManager() == NULL)
         return NULL;
      return GetManager()->GetPropertyNamed(name);      
   }
};

IProperty* GetProperty(PropertyID id)
{
   IProperty* prop = LookerUpper::ByID(id);
   return prop;
}

IProperty* GetPropertyNamed(const char* name)
{
   IProperty* prop = LookerUpper::ByName(name);
   return prop;
}

IProperty* _GetProperty(PropertyID id)
{
   IProperty* prop = LookerUpper::ByID(id);  
   // we know the property manager is holding onto it, so we can release it. 
   prop->Release(); 
   return prop; 
}

IProperty* _GetPropertyNamed(const char* name)
{
   IProperty* prop = LookerUpper::ByName(name);  
    // we know the property manager is holding onto it, so we can release it. 
   prop->Release(); 
   return prop; 
}

////////////////////////////////////////////////////////////
// INSTRUMENTATION STUFF
//

BOOL gBlameProperties = FALSE; 

#ifdef PROPERTY_BLAME

static char gPropNameBuf[16*1024]; 
static char* gPropNextName = gPropNameBuf; 

const char* PropBlameName(const char* name)
{
   static BOOL inited = FALSE; 
   
   if (!inited)
   {
      // @TODO: do this someplace more sensible
      gBlameProperties = config_is_defined("blame_properties"); 
      inited = TRUE;
   }
   
   char* s = gPropNextName; 
   sprintf(s,"Property '%s'",name); 
   gPropNextName += strlen(s) + 1; 
   AssertMsg(gPropNextName < gPropNameBuf + sizeof(gPropNameBuf),"Not enough memory for property blame names\n");
   return s; 
}

// Push and pop allocation credit 
void PropPushBlame(const char* PropName)
{
#ifdef DEBUG
   if (gBlameProperties)
      g_pMalloc->PushCredit(PropName,0);
#endif 
};

void PropPopBlame(void)
{
#ifdef DEBUG
   if (gBlameProperties)
      g_pMalloc->PopCredit();
#endif 
};

#endif // PROPERTY_BLAME


#ifdef PROPERTY_TIME

// Get the current time in milliseconds
long cPropTimer::GetTime() 
{
   return tm_get_millisec_unrecorded(); 
};

ulong cPropTimer::gDummyStat; 

#endif // PROPERTY_TIME


