/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/periog8r.cpp,v 1.6 1998/11/04 16:54:53 mahk Exp $
#include <periog8r.h>
#include <pg8rbase.h>
#include <dlistsim.h>
#include <linkbase.h>
#include <autolink.h>
#include <relation.h>
#include <stimtype.h>
#include <stimbase.h>
#include <string.h>
#include <propag8n.h>
#include <stimul8r.h>
#include <stimsrc.h>
#include <stimsens.h>
#include <ssrcbase.h>
#include <sensbase.h>
#include <simtime.h>
#include <listset.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <lststtem.h>
#include <dlisttem.h>
#include <appagg.h>
#include <sdestool.h>
#include <sdesbase.h>
#include <ssrclife.h>
#include <iobjsys.h>

#include <config.h>
#include <cfgdbg.h>

// Include these last 
#include <dbmem.h>
#include <initguid.h>
#include <perig8id.h>

enum { kEndOfTime = 0xFFFFFFFF }; 

class cPeriodicPropagator : public cCTUnaggregated<IPeriodicPropagator,&IID_IPeriodicPropagator, kCTU_Default>
{

protected:
   //------------------------------------------------------------
   // HELPER TYPES
   // 

   //
   // Here's the data we hang on our contact links 
   //

   enum eContactFlags
   {
      kInContact     = 1 << 0, // we're actually in contact 
      kFirstFiring   = 1 << 1, // we haven't ever fired 
      kExpired       = 1 << 2, // we have no interested sources any more
   }; 
   
   struct sContact 
   {
      tStimTimeStamp start; 
      tStimTimeStamp next; // the next time we should fire.
      tStimTimeStamp last; // last time we fired 
      ulong flags;         // flags, see above
   }; 


   //
   // That, plus a link id, for the queue
   //

   struct sContactElem
   {
      LinkID id;  // contact link 
      tStimTimeStamp time; 

      sContactElem() {}; 
      sContactElem(const sContact& c, LinkID i = LINKID_NULL)
         : time(c.next),id(i)
      {
      }
   }; 


   //
   // "contact queue" abstraction, keeps all contacts in sorted order
   // of when they need to be fired.   
   //

   class cContactList: public  cSimpleDList<sContactElem> {};

   class cContactQueue: public cContactList
   {

   public:
      void AddNode(cNode* elem)
      {
         // really, we want to iterate backwards, because we expect our 
         // new node will be at the back. 
         for (cNode* node = GetLast(); node != NULL; node = node->GetPrevious())
            if (elem->Value().time >= node->Value().time)
            {
               cParent::InsertAfter(node,elem); 
               return; 
            }
         PrependNode(elem); 
      }

      void AddElem(const sContactElem& elem) { AddNode(new cNode(elem)); }

      void RemoveID(LinkID id)
      {
         for (cIter iter = Iter(); !iter.Done(); iter.Next())
         {
            if (iter.Value().id == id)
            {
               Delete(iter.Node());
               break;
            }
         }
      }
   }; 

   //
   // Source-tracking data structures
   //

   // List of sources
   class cSourceList : public cSimpleListSet<StimSourceID>
   {
      
   };

   // Hash functions
   typedef cScalarHashFunctions<ObjID> cSourceTableHashFuncs; 

   // Table of lists of sources
   class cSourceTable : public cHashTable<ObjID,cSourceList*,cSourceTableHashFuncs>
   {
   public:
      void RemoveAll()
      {
         for (cIter iter = Iter(); !iter.Done(); iter.Next())
         {
            delete iter.Value(); 
            Delete(iter.Key()); 
         }
      }
      
   };

   //
   // Source Life Cycle 
   //
   
public:

   typedef sPeriodicLifeCycle sLifeCycle; 

   enum eLifeCycleFlags
   {
      kNoMaxFirings  = sLifeCycle::kNoMaxFirings,
      kDestroy       = sLifeCycle::kDestroy,
   }; 



   //------------------------------------------------------------
   // Internals
   //
protected:

   void OnLink(const sRelationListenMsg* msg)
   {
      switch(msg->type)
      {
         case kListenLinkDeath:
            if (mInSim && msg->id != mRemoving)
               mQueue.RemoveID(msg->id); 
            break; 
      }
   }

   static void LGAPI LinkCB(sRelationListenMsg* msg, RelationListenerData data)
   {
      cPeriodicPropagator* us = (cPeriodicPropagator*)data; 
      us->OnLink(msg); 
   }
     

   //------------------------------------------------------------
   // DATA MEMBERS
   //
protected:
   IRelation* mpContacts; 
   cContactQueue mQueue; 
   sPropagatorDesc mDesc;
   PropagatorID mID; 
   cSourceTable mSourceTable; 
   LinkID mRemoving;  // the link we're in the middle of removing 
   BOOL mInSim; // Are we running the sim

   IStimulator* mpStimulator; 
   IPropagation* mpPropagation;
   IStimSources* mpSources; 
   IStimSensors* mpSensors; 
   IObjectSystem* mpObjSys; 


   //------------------------------------------------------------
   // CONSTRUCTION/ DECONSTRUCTION
   //

public:

   cPeriodicPropagator(const sPropagatorDesc* desc, const char* relname)
      : mpStimulator(NULL),
        mDesc(*desc),
        mpPropagation(AppGetObj(IPropagation)),
        mpSources(AppGetObj(IStimSources)),
        mpSensors(AppGetObj(IStimSensors)),
        mpObjSys(AppGetObj(IObjectSystem)), 
        mRemoving(LINKID_NULL),
        mInSim(FALSE)
   {
      sRelationDesc rdesc = { "", kRelationNoEdit | kRelationNetworkLocalOnly, 1, 1};
      strncpy(rdesc.name,relname,sizeof(rdesc.name)); 
      rdesc.name[sizeof(rdesc.name)-1] = '\0';

      sRelationDataDesc ddesc = LINK_DATA_DESC_FLAGS(sContact,kRelationDataAutoCreate); 

      mpContacts = CreateStandardRelation(&rdesc,&ddesc,kQCaseSetBothKnown|kQCaseSetDestKnown); 
      mID = mpPropagation->AddPropagator(this); 
      mDesc.flags |= kPGatorAllStimuli; 

      mpContacts->Listen(kListenLinkDeath,LinkCB,this); 

   }

   ~cPeriodicPropagator()
   {
      SafeRelease(mpContacts); 
      SafeRelease(mpSources); 
      SafeRelease(mpSensors);
      SafeRelease(mpObjSys); 
      SafeRelease(mpStimulator); 
   }

   //////////////////////////////////////////////////////////////
   // IPropagator Methods
   //

   //----------------------------------------
   // Administrivia
   //

   STDMETHOD_(const sPropagatorDesc*,Describe)()
   {
      return &mDesc; 
   }

   STDMETHOD_(PropagatorID,GetID)()
   {
      return mID; 
   }

   STDMETHOD(Connect)(IUnknown* stimulator)
   {
      SafeRelease(mpStimulator); 
      return stimulator->QueryInterface(IID_IStimulator,(void**)&mpStimulator); 
   }


   //----------------------------------------
   // Stimulus tracking
   //

   STDMETHOD(AddStimulus)(THIS_ StimID stim)
   {
      if (!SupportsStimulus(stim))
         mpPropagation->AddPropagatorStimulus(mID,stim);
      return S_OK; 
   }

   STDMETHOD(RemoveStimulus)(THIS_ StimID stim)
   {
      if (SupportsStimulus(stim))
         mpPropagation->RemovePropagatorStimulus(mID,stim);
      return S_OK; 
   }

   STDMETHOD_(BOOL,SupportsStimulus)(THIS_ StimID stim)
   {
      return mpPropagation->SupportsStimulus(mID,stim); 
   }

   //----------------------------------------
   // Source Description
   //

   STDMETHOD_(const struct sStructDesc*,DescribeShapes)()
   {
      //      CriticalMessage("DescribeShapes isn't implemented"); 
      return NULL; 
   }


 
   STDMETHOD_(const struct sStructDesc*,DescribeLifeCycles)() 
   {
      return sLifeCycle::gpDesc; 
   }


   //------------------------------------------------------------
   // Events
   //

   STDMETHOD(SourceEvent)(sStimSourceEvent* event) 
   {

      AssertMsg(mpSources->GetSourceElems(event->id).obj == event->elems.obj,"Act/React Source event is out of synch"); 

      switch(event->type)
      {
         case kStimSourceCreate:
         {
            cSourceList* src = mSourceTable.Search(event->elems.obj);
            if (src == NULL)
            {
               src = new cSourceList;
               mSourceTable.Insert(event->elems.obj,src);
            }
            src->AddElem(event->id);

            // Refresh all expired contacts with this object 
            if (mInSim)
            {
               cAutoLinkQuery query(mpContacts,event->elems.obj); 
               for (; !query->Done(); query->Next())
               {
                  sContact* contact = (sContact*)query->Data(); 
                  if (contact->flags & kExpired)
                  {
                     LinkID id = query->ID(); 
                     contact->flags &= ~kExpired; 
                     mpContacts->SetData(id,contact); 

                     mQueue.AddElem(sContactElem(*contact,id)); 
                  }
               }
            }
         }
         break;

         case kStimSourceDestroy:
         {
            cSourceList* src = mSourceTable.Search(event->elems.obj);
            if (src)
            {
               Verify(src->RemoveElem(event->id)); 
               if (src->Size() == 0)
               {
                  delete src;
                  mSourceTable.Delete(event->elems.obj); 
               }
            }
         }
         break;
      }
      return S_OK;      
   }

   STDMETHOD(SensorEvent)(sStimSensorEvent* event) 
   {
      // I get my sensors fresh each day
      return S_OK; 
   }

   //------------------------------------------------------------
   // Sources
   //

   STDMETHOD_(tStimLevel,GetSourceLevel)(StimSourceID id) 
   {
      // @TODO: evolve over time? 
      sStimSourceDesc desc; 
      mpSources->DescribeSource(id,&desc); 
      return desc.level; 
   }


   STDMETHOD(DescribeSource)(StimSourceID id, sStimSourceDesc* desc) 
   {
      return mpSources->DescribeSource(id,desc); 
   }

   STDMETHOD(InitSource)(sStimSourceDesc* desc)
   {
      static sLifeCycle def_life = { kNoMaxFirings, 1000, -1, 0.0 }; 
      memset(desc,0,sizeof(*desc));
      desc->propagator = mID;
      desc->valid_fields = kStimSrcLifeValid;
      *(sLifeCycle*)&desc->life = def_life; 
      return S_OK;
   }


   //------------------------------------------------------------
   // Propagation
   //

   //
   // Fire a single source on a single sensor
   // Return whether the source is not expired, and the next
   // time it neets to be updated 
   //

   BOOL GenerateEvents(tStimTimeStamp time, const sContact& contact, StimSourceID srcid, const sObjStimPair& srcpair, StimSensorID sensid, tStimTimeStamp* pnext_time)
   {
      sStimSourceDesc desc;   
      DescribeSource(srcid,&desc); 
            
      // look at the life cycle 
      sLifeCycle& life = *(sLifeCycle*)&desc.life; 

      // figure out start time 
      tStimTimeStamp start = contact.start; 
      // @TODO: cache this value? 
      tStimTimeStamp birth = mpSources->GetSourceBirthDate(srcid); 
      if (birth > start) start = birth; 
     
      BOOL first_fire = contact.flags & kFirstFiring; 
 
      // When did we last fire?
      tStimTimeStamp last = contact.last; 
      if (last < start)
      {
         last = start; 
         first_fire = TRUE; 
      }
      

      sLifeCycle::sFire* fire = life.BeginFiring(first_fire,desc.level,start,last,time);

      tStimTimeStamp t; 
      tStimLevel level; 
      ulong flags; 

      while(life.FireNext(fire,&level,&t,&flags))
      {
         sStimEventData evdata = { srcpair.stim, level , 0.0, sensid, srcid, t, flags }; 
         sStimEvent event(&evdata); 
         mpStimulator->StimulateSensor(sensid,&event); 
      }
      BOOL retval = life.EndFiring(fire); 
      *pnext_time = t; 

      return retval; 
   }

   //
   // Per-frame propagation logic
   //


   STDMETHOD(Propagate)(tStimTimeStamp curtime, tStimDuration duration) 
   {
      cContactList requeue; 

      mpObjSys->Lock(); 

      cContactQueue::cIter iter; 
      for (iter = mQueue.Iter(); !iter.Done(); iter.Next())
      {
         sContactElem elem = iter.Value(); 

         // Everything after this one is in the future. 
         if (curtime < elem.time)
            break; 

         // get the contact info 
         sContact* contact = (sContact*)mpContacts->GetData(elem.id); 

         // is this ready to be deleted
         if (!(contact->flags & kInContact))
         {
            ConfigSpew("periog8r_spew",("Discarding contact %X\n",elem.id));

            // dequeue 
            mQueue.Delete(iter.Node()); 
            mRemoving = elem.id; 
            mpContacts->Remove(elem.id); 
            mRemoving = LINKID_NULL; 
            
            continue; 
         }

         BOOL expired = TRUE; 
         tStimTimeStamp next_time = kEndOfTime; 
         sLink link; 
         mpContacts->Get(elem.id,&link); 
         

         ConfigSpew("periog8r_spew",("Propagating from %d to %d\n",link.source,link.dest)); 

         // get the sources 
         cSourceList* srclist = mSourceTable.Search(link.source); 
         cSourceList::cIter srciter; 
         if (srclist)
            for (srciter = srclist->Iter(); !srciter.Done(); srciter.Next())
            {
               StimSourceID srcid = srciter.Value(); 

               sObjStimPair pair = mpSources->GetSourceElems(srcid); 

               AssertMsg(link.source == pair.obj,"Act/React source does not match object\n"); 
               StimSensorID sensid = mpSensors->LookupSensor(link.dest,pair.stim);
               if (sensid != SENSORID_NULL)
               {
                  tStimTimeStamp next;
                  if (GenerateEvents(curtime, *contact,srcid,pair,sensid,&next))
                     expired = FALSE; 
                  if (next < next_time)
                     next_time = next; 
               }
            
            }

         if (expired || next_time == kEndOfTime) // no source has any firings left to do. 
         {
            contact->flags |= kExpired; 
            mQueue.Delete(iter.Node()); 
            ConfigSpew("periog8r_spew",("Contact %X has expired\n",elem.id)); 
            next_time = GetSimTime(); 
         }
         else // re-queue 
         {
            ConfigSpew("periog8r_spew",("Contact %X was re-queued for time %d.%03d\n",elem.id,next_time/1000,next_time%1000)); 
            cContactList::cNode& node = iter.Node(); 
            mQueue.Remove(&node); 
            node.Value().time = next_time; 
            requeue.AppendNode(&node); 

         }
         contact->last = curtime;
         contact->flags &= ~kFirstFiring; 
         contact->next = next_time;
         mpContacts->SetData(elem.id,contact); 

      }

      // Now re-queue the events that need to be 
      for (iter = requeue.Iter(); !iter.Done(); iter.Next())
      {
         cContactList::cNode& node = iter.Node(); 
         requeue.Remove(&node); 
         mQueue.AddNode(&node); 
      }
      
      mpObjSys->Unlock(); 

      return S_OK; 
   }

   //------------------------------------------------------------
   // Contacts
   //

   void RefreshContactID(LinkID id)
   {
      sContact* ctact = (sContact*)mpContacts->GetData(id); 
      ConfigSpew("periog8r_spew",("Refreshing contact %X\n",id)); 

      ctact->flags |= kInContact; 
      mpContacts->SetData(id,ctact); 
   }
   
   //
   // Notify the propagator that a contact has begun or ended
   // 
   STDMETHOD(BeginContact)(ObjID src, ObjID sensor)
   {
      // Look for a contact link 
      LinkID id = mpContacts->GetSingleLink(src,sensor); 

      // If we find an existing link, just mark it as in contact.  
      if (id != LINKID_NULL)
      {
         RefreshContactID(id); 
         return S_OK; 
      }
      
      // Make the link
      tStimTimeStamp time = GetSimTime(); 
      sContact ctact = { time, time, time, kInContact|kFirstFiring }; 
      id = mpContacts->AddFull(src,sensor,&ctact); 

      ConfigSpew("periog8r_spew",("Beginning contact %X between %d and %d\n",id,src,sensor)); 

      // queue it 
      mQueue.AddElem(sContactElem(ctact,id)); 

      return S_OK; 
   }

   void EndContactID(LinkID id)
   {
      // We don't actually destroy the link here.  We just mark it as
      // not in contact, and we'll destroy it when its time comes up
      // in the queue.  This is to filter out oscillation cases.
      // where contact gets formed and broken at a faster rate than
      // the frequency of the sources.

      ConfigSpew("periog8r_spew",("Ending contact %X\n",id)); 

      sContact* ctact = (sContact*)mpContacts->GetData(id); 
      ctact->flags &= ~kInContact; 
      mpContacts->SetData(id,ctact); 

      // re-queue it, so it can have a chance to be destroyed
      if (ctact->flags & kExpired)
         mQueue.AddElem(sContactElem(*ctact,id)); 

   }

   STDMETHOD(EndContact)(ObjID src, ObjID sensor)
   {
      if (src == OBJ_NULL) src = LINKOBJ_WILDCARD; 
      if (sensor == OBJ_NULL) sensor = LINKOBJ_WILDCARD; 
      
      HRESULT retval = S_FALSE; 

      ConfigSpew("periog8r_spew",("Ending contacts between %d and %d.\n",src,sensor)); 
      for (cAutoLinkQuery query(mpContacts,src,sensor); !query->Done(); query->Next())
      {
         retval = S_OK; 
         EndContactID(query->ID()); 
      }

      return retval; 
   }


   STDMETHOD(SetSingleSensorContact)(ObjID src, ObjID sensor) 
   {
      LinkID found = LINKID_NULL; 

      ConfigSpew("periog8r_spew",("Setting single contact between %d and %d.\n",src,sensor)); 


      cAutoLinkQuery query(mpContacts,OBJ_NULL,sensor); 
      for (;!query->Done(); query->Next())
      {
         LinkID id = query->ID(); 
         sLink link; 
         query->Link(&link); 
         if (link.source == src)
            found = id; 
         else 
            EndContactID(id); 
      }

      if (found != LINKID_NULL)
         RefreshContactID(found); 
      else
         BeginContact(src,sensor); 
      
      return S_OK; 
   }


   //------------------------------------------------------------
   // MAJOR DATABASE CHANGES
   //

   STDMETHOD(Reset)() 
   {
      mSourceTable.RemoveAll(); 
      mQueue.DestroyAll(); 
      return S_OK; 
   }

   STDMETHOD(Start)() // SimStart
   {
      // re-build the queue based on the links 
      mQueue.DestroyAll(); 
      for (cAutoLinkQuery query(mpContacts); !query->Done(); query->Next())
      {
         sContact* c = (sContact*)query->Data();
         mQueue.AddElem(sContactElem(*c,query->ID())); 
      }
      mInSim = TRUE; 

      return S_OK; 
   }

   STDMETHOD(Stop)() // Sim stop
   {
      mQueue.DestroyAll(); 
      mInSim = FALSE; 
      return S_OK; 
   }

   

}; 

////////////////////////////////////////

//
// life cycle sdesc 
//


////////////////////////////////////////

IPeriodicPropagator* CreatePeriodicPropagator(const sPropagatorDesc* desc, const char* relname)
{
   return new cPeriodicPropagator(desc,relname); 
}





