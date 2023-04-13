/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/relation.cpp,v 1.45 1999/08/31 14:40:15 Justin Exp $

#include <relatio_.h>
#include <relinver.h>
#include <linkint.h>
#include <objnotif.h>
#include <reldeleg.h>

#include <linkbase.h>

#include <iobjed.h>
#include <lnktrait.h>

#include <objedit.h>

// Link implementations
#include <linkarry.h>

#include <string.h>

#include <cfgdbg.h>
#include <config.h>

#include <iobjnet.h>
#include <netman.h>

#ifndef SHIP
#include <mprintf.h>
#endif

// Must be last header
#include <dbmem.h>


//------------------------------------------------------------
// Link network message structure (private)
//
struct sLinkNetMsg
{
   tNetMsgHandlerID handlerID; // handler ID of the property manager component (used by NetMan).
   RelationID relationID;      // relation sending message
   eRelationListenMsg ops;     // The operation(s) being done on this link.   
   sGlobalObjID sourceGID;     // GlobalObjID of the source object
   sGlobalObjID destGID;       // GlobalObjID of the dest object (unknown host machine).
   char linkData[1];  // Data for the link. I wish this could be [0], but we get warnings if I do.
};

////////////////////////////////////////////////////////////
//
// cBaseRelation
//
////////////////////////////////////////////////////////////

INetManager *cBaseRelation::gmNetMan = NULL;
IObjectNetworking *cBaseRelation::gmObjNet = NULL;
ObjID cBaseRelation::gmRespondSourceObj = OBJ_NULL;
ObjID cBaseRelation::gmRespondDestObj = OBJ_NULL;

IMPLEMENT_UNAGGREGATABLE_SELF_DELETE(cUnknownRelation,IRelation);

STDMETHODIMP_(const sRelationDesc*) cBaseRelation::Describe() const
{
   return &Desc;
}

STDMETHODIMP_(const sRelationDataDesc*) cBaseRelation::DescribeData() const
{
   return &DataDesc;
}

STDMETHODIMP_(RelationID) cBaseRelation::GetID() const 
{
   return ID;
}

STDMETHODIMP_(IRelation*) cBaseRelation::Inverse()
{
   if (Desc.flags & kRelationDirectionless)
   {
      AddRef();   
      return this;
   }
   else
      return new cInverseRelation(this);
}

STDMETHODIMP cBaseRelation::Listen(RelationListenMsgSet interests, RelationListenFunc func, RelationListenerData data)
{
   Listeners.Append(Listener(interests,func,data));
   return S_OK;
}

void cBaseRelation::CallListeners(eRelationListenMsg msg, LinkID id, sLink* linkdata)
{
   sRelationListenMsg message = { msg, id};

   if (linkdata == NULL)
   {
      Get(id,&message.link);
   }
   else
      message.link = *linkdata;

   for (int i = 0; i < Listeners.Size(); i++)
   {
      Listener& l = Listeners[i];
      if (l.interests & msg)
      {
         message.type = msg & l.interests;
         l.func(&message,l.data);
      }
   }
#ifdef NEW_NETWORK_ENABLED
   // If this a network game & are we the host of link->source,
   // broadcast the change, unless one of the objects is local-only.
   if (gmNetMan->Networking()
       && !gmNetMan->Suspended()
       && message.link.flavor != RELID_NULL
       && !(Desc.flags & kRelationNetworkLocalOnly)
       && !(msg & kListenLinkPostMortem)
       && !(msg & kListenLinkDeath))
   {
      SendLinkMsg(msg, id, &message.link);
   }
#endif
}

STDMETHODIMP_(LinkID) cBaseRelation::GetSingleLink(ObjID source, ObjID dest)
{
   LinkID result = LINKID_NULL;
   ILinkQuery * pQuery;

   pQuery = Query(source, dest);
   
   if (!pQuery->Done())
   {
      result = pQuery->ID();
#ifndef SHIP
      pQuery->Next();
      if (!pQuery->Done())
      {
         pQuery->Release();
         
         sLink link;
         int   count = 0;
         
         pQuery = Query(source, dest);
         
         mprintf("ERROR: Caller expecting a singleton link on (%s [%d --> %d])\n", Desc.name, source, dest);
         mprintf("   Actual links were:\n");

         while (!pQuery->Done())
         {
            count++;
            pQuery->Link(&link);
            mprintf("      %s --> %s [%08X]\n", ObjWarnName(link.source),ObjWarnName(link.dest),pQuery->ID());
            pQuery->Next();
         }
         CriticalMsg4("Singleton link expected (%s [%d --> %d]) [%d]", Desc.name, source, dest, count);
      }
#endif
   }
   
   pQuery->Release();
   return result;
}

STDMETHODIMP_(BOOL) cBaseRelation::AnyLinks(ObjID source, ObjID dest)
{
   BOOL result;
   ILinkQuery * pQuery;

   pQuery = Query(source, dest);
   result = !pQuery->Done();
   pQuery->Release();
   
   return result;
}

////////////////////////////////////////////////////////////
//
// cLinkQueryDatabaseSet
//
////////////////////////////////////////////////////////////

typedef ILinkQueryDatabase ILinkQDB;

void cLinkQueryDatabaseSet::Init(void)
{
   for (int i = 0; i < Size(); i++)
      DB[i]->AddRef();
}

cLinkQueryDatabaseSet::~cLinkQueryDatabaseSet()
{
   for (int i = 0; i < Size(); i++)
      DB[i]->Release();
}

cLinkQueryDatabaseSet::cLinkQueryDatabaseSet(ILinkQDB* none, ILinkQDB* src, ILinkQDB* dst, ILinkQDB* both)
{
   Assrt(none != NULL);
   DB[kRelationNoneKnown]     = none;
   DB[kRelationSourceKnown]   = src  = (src  != NULL) ? src  : none;
   DB[kRelationDestKnown]     = dst  = (dst  != NULL) ? dst  : none;
   DB[kRelationBothKnown]     = both = (both != NULL) ? both : src ; 
   Init();
}

cLinkQueryDatabaseSet::cLinkQueryDatabaseSet(const cLinkQueryDatabaseSet& set)
{
   for (int i = 0; i < Size(); i++)
      DB[i] = set[i];
   Init();
}

cLinkQueryDatabaseSet::cLinkQueryDatabaseSet(IUnknown* db)
{
   ILinkQueryDatabase* tempdb;
   Verify(SUCCEEDED(COMQueryInterface(db,IID_ILinkQueryDatabase,(void**)&tempdb)));
   for (int i = 0; i < Size(); i++)
      DB[i] = tempdb;
   Init();
   SafeRelease(tempdb);
}

////////////////////////////////////////////////////////////
//
// cStandardRelation
//
////////////////////////////////////////////////////////////

tNetMsgHandlerID cStandardRelation::gmNetMsgHandlerID = 0;

void cStandardRelation::Init()
{
   Store->AddRelation(ID);
   DataStore->AddRelation(ID); 
   for (int i = 0; i < DB.Size(); i++)
   {
      DB[i]->AddRelation(ID);
      LinkMan()->AddQueryDB(DB[i],SingletonQCaseSet(i));
   }
#ifdef NEW_NETWORK_ENABLED
   if (gmObjNet == NULL)
      gmObjNet = AppGetObj(IObjectNetworking);
   Assert_(gmObjNet);
   if (gmNetMan == NULL)
      gmNetMan = AppGetObj(INetManager);
   Assert_(gmNetMan);
   if (gmNetMsgHandlerID == 0)
      gmNetMsgHandlerID = gmNetMan->RegisterMessageParser(ReceiveLinkMsg, "Link", 0, NULL);
#endif

   // @TODO: Do this somewhere else? 
#ifdef EDITOR
   IObjEditors* pEdit = AppGetObj(IObjEditors);
   if (pEdit)
   {
      IEditTrait* trait;
      trait = CreateLinkEditTrait(ID,FALSE); 
      pEdit->AddTrait(trait); 
      SafeRelease(trait);

      trait = CreateLinkEditTrait(RELID_INVERT(ID),FALSE); 
      pEdit->AddTrait(trait); 
      SafeRelease(trait);

      SafeRelease(pEdit); 
   }
#endif // EDITOR
}

cStandardRelation::cStandardRelation(const sRelationDesc& desc, const sRelationDataDesc& ddesc, IUnknown* store, IUnknown* db, IUnknown* datastor)
   : cBaseRelation(desc,ddesc), DB(db)
{
   Verify(SUCCEEDED(COMQueryInterface(store,IID_ILinkStore,(void**)&Store)));
   if (datastor == NULL)
      DataStore = CreateManagedLinkDataStore(ddesc.size);
   else
      Verify(SUCCEEDED(COMQueryInterface(store,IID_ILinkDataStore,(void**)&DataStore)));

   Init();
}

cStandardRelation::cStandardRelation(const sRelationDesc& desc, const sRelationDataDesc& ddesc, IUnknown* store, const cLinkQueryDatabaseSet& db, IUnknown* datastor)
   : DB(db), cBaseRelation(desc,ddesc)
{
   Verify(SUCCEEDED(COMQueryInterface(store,IID_ILinkStore,(void**)&Store)));
   if (datastor == NULL)
      DataStore = CreateManagedLinkDataStore(ddesc.size);
   else
      Verify(SUCCEEDED(COMQueryInterface(store,IID_ILinkDataStore,(void**)&DataStore)));

   Init();
}

cStandardRelation::~cStandardRelation()
{
   if (LinkMan() != NULL)
      for (int i = 0; i < DB.Size(); i++)
      {
         LinkMan()->RemoveQueryDB(DB[i],SingletonQCaseSet(i));
      }
   Store->Release();
   DataStore->Release();
}

//------------------------------------------------------------
// IRelation methods
//

STDMETHODIMP cStandardRelation::SetID(RelationID newid)
{
   Store->RemoveRelation(ID);
   DataStore->RemoveRelation(ID);
   for (int i = 0; i < DB.Size(); i++)
      DB[i]->RemoveRelation(ID);

   ID = newid; 

   Store->AddRelation(ID);
   DataStore->AddRelation(ID);
   for (int i = 0; i < DB.Size(); i++)
      DB[i]->AddRelation(ID);

   return S_OK; 
}

//
// Add/Remove/Get Links
// 

STDMETHODIMP_(LinkID) cStandardRelation::Add(ObjID source, ObjID dest)
{
   return AddFull(source,dest,NULL);
}

STDMETHODIMP_(LinkID) cStandardRelation::AddFull(ObjID source, ObjID dest, void* data)
{
   Assert_(source != OBJ_NULL && dest != OBJ_NULL); 

   sLink link = { source, dest, ID}; 
   LinkID id = Store->Add(&link);
   eRelationListenMsg msg = kListenLinkBirth;

   DataStore->Add(id);

   if (data != NULL)
   {
      DataStore->Set(id,data);
      msg |= kListenLinkModify;
   }

   for (int i = 0; i < DB.Size(); i++)
   {
      DB[i]->Add(id,&link);
   }

   LinkMan()->LinkBirth(id);
   

   CallListeners(msg,id,&link);
   return id;
}

STDMETHODIMP cStandardRelation::Remove(LinkID id)
{
#ifdef NEW_NETWORK_ENABLED
   // We need to transmit the link's demise now, not at LinkDeath,
   // because order of events matters more in the networked case.
   if (gmNetMan->Networking()
       && !gmNetMan->Suspended()
       && !(Desc.flags & kRelationNetworkLocalOnly))
   {
      sLink link;
      if (Get(id, &link) && (link.flavor != RELID_NULL))
      {
         SendLinkMsg(kListenLinkDeath, id, &link);
      }
   }
#endif

   if (LinkMan()->LinkDeath(id))
      return S_OK;

   sLink link;
   if (!Get(id,&link)) return S_FALSE;
   CallListeners(kListenLinkDeath,id,&link);
   for (int i = 0; i < DB.Size(); i++)
   {
      DB[i]->Remove(id,&link);
   }
   DataStore->Remove(id);
   HRESULT retval = Store->Remove(id);
   CallListeners(kListenLinkPostMortem,id,&link); 
   return retval; 
}

STDMETHODIMP_(BOOL) cStandardRelation::Get(LinkID id, sLink* out) const
{  
   return Store->Get(id,out);
}


STDMETHODIMP cStandardRelation::SetData(LinkID id, void* data)
{
   DataStore->Set(id,data);
   CallListeners(kListenLinkModify,id);
   return S_OK;
}

void* cStandardRelation::GetData(LinkID id)
{
   void* data = DataStore->Get(id);
   if (data == NULL && (DataDesc.flags & kRelationDataAutoCreate))
   {
      char* buf = new char[DataDesc.size];
      memset(buf,0,DataDesc.size); 
      SetData(id,buf); 
      delete [] buf;

      data = DataStore->Get(id);
   }
   return data; 
}

STDMETHODIMP_(ILinkQuery*) cStandardRelation::Query(ObjID source, ObjID dest) const
{
   int qcase = kRelationNoneKnown;
   if (source != LINKOBJ_WILDCARD)
      qcase |= kRelationSourceKnown;
   if (dest != LINKOBJ_WILDCARD)
      qcase |= kRelationDestKnown;

   return DB[qcase]->Query(source,dest,ID);
}

STDMETHODIMP cStandardRelation::Notify(eRelationNotifyMsg msg, RelationNotifyData data)
{
   uObjNotifyData info;
   info.raw = data;

   switch(NOTIFY_MSG(msg))
   {
      case kObjNotifyDelete:
      {
         ILinkQuery* q = Query(info.obj,LINKOBJ_WILDCARD);
         for (; !q->Done(); q->Next())
         {
            Remove(q->ID());
         }
         SafeRelease(q);
         
       q = Query(LINKOBJ_WILDCARD,info.obj);
         for (; !q->Done(); q->Next())
         {
            Remove(q->ID());
         }
         SafeRelease(q);

      }
      break;

      case kObjNotifyReset:
      {

         // Do the datastore first, accounting for clunky 
         // implementations that run queries...
         DataStore->Reset(ID);

         for (int i = 0; i < DB.Size(); i++) 
            DB[i]->Reset(ID);
         Store->Reset(ID);

      } 
      break;

      case kObjNotifySave:
         if (!(Desc.flags & kRelationTransient))
         {
            Store->Save(ID, (IUnknown*)info.db.save, msg);
            DataStore->Save(ID, (IUnknown*)info.db.save, msg);
         }
         break;
         
      case kObjNotifyLoad:
         if (!(Desc.flags & kRelationTransient))
         {
            Store->Load(ID,(IUnknown*)info.db.load,msg);
            DataStore->Load(ID,(IUnknown*)info.db.load,msg);
         }
         break;

      case kObjNotifyPostLoad:
      {
         ILinkQuery* q = Store->GetAll(ID);
         for (; !q->Done(); q->Next())
         {
            sLink link;
            q->Link(&link);
            for (int i = 0; i < DB.Size(); i++)
               DB[i]->Add(q->ID(),&link);
         }
         SafeRelease(q);
      }
      break;
   }
   return S_OK;
}

// Network spew macros
#ifndef SHIP
#ifdef SPEW_ON
// Normal dbg case
#define BaseSpew()        (config_is_defined("net_spew"))
#define RelNetSpew(msg)   do {if (BaseSpew()||(config_is_defined("net_link_spew"))) {mprintf msg;} } while (0)
#else
// Normal opt case
#define BaseSpew()        (config_is_defined("net_opt_spew")&&config_is_defined("net_spew"))
#define RelNetSpew(msg)   do {if (BaseSpew()||(config_is_defined("net_link_spew") && config_is_defined("net_link_opt_spew"))) {mprintf msg;}} while (0)
#endif
#else
// SHIP -- just cut it off
#define RelNetSpew(msg)
#endif

// Send network message regarding this link.
void cStandardRelation::SendLinkMsg(eRelationListenMsg ops, 
                                    LinkID id, 
                                    sLink* linkdata)
{
   // Prevent any loops
   if ((linkdata->source == gmRespondSourceObj) &&
       (linkdata->dest == gmRespondDestObj))
   {
      // Don't do anything -- we're responding to someone else's broadcast
      // to deal with this link.
      return;
   }

   // Don't send a message if either object is local
   if (gmObjNet->ObjLocalOnly(linkdata->source) ||
       gmObjNet->ObjLocalOnly(linkdata->dest))
   {
      return;
   }

   // If this is a LinkDeath, and one of the objects doesn't exist, don't
   // bother networking it: it's just the snapping of the links after
   // the object's been destroyed, and should happen locally on each
   // machine.
   if (ops & kListenLinkDeath)
   {
      AutoAppIPtr(ObjectSystem);
      if (!pObjectSystem->Exists(linkdata->source) ||
          !pObjectSystem->Exists(linkdata->dest))
      {
         return;
      }
   }

   // In general, only the owner of the source of a link can change
   // the link. Clients can only change the link if so indicated here.
   //
   // @TBD: This really doesn't seem right; given the realities of
   // links, it seems like we ought to allow anyone to change them.
   // In practice, however, allowing the floodgates to open seems to
   // cause a rash of problems. This needs further study; for now, we
   // only allow specified links to work like this.
   if (!(Desc.flags & kRelationNetProxyChangeable) &&
       !gmObjNet->ObjHostedHere(linkdata->source))
   {
#ifdef PLAYTEST
      if (config_is_defined("net_loud_scary_filter"))
         mprintf("Scary link filter op %d rel %s id %x (%d -> %d)\n",
                  ops,Desc.name,id,linkdata->source,linkdata->dest);
#endif
      return;
   }

   LinkID actualLink = GetSingleLink(linkdata->source, linkdata->dest);
   if ((actualLink == 0) && (id != 0) && !(ops & kListenLinkDeath)) {
      // This link doesn't currently exist, so don't try to broadcast it
      return;
   }
   AssertMsg(actualLink == id || (ops & kListenLinkDeath),
             "Networked links may not have the"
             "same 2 objects paired multiple times");

   // Allocate a message with enough space for the link data, if necessary.
   ulong dataSize = (ops & kListenLinkModify) ? DataStore->DataSize() : 0;
   ulong msgSize = dataSize + sizeof sLinkNetMsg;
   sLinkNetMsg *netmsg = (sLinkNetMsg *)malloc(msgSize);
   // Subtract out the compile-time known size of linkData, which the
   // C compiler forced me to add in, since it wouldn't allow linkData
   // to be defined with 0 size.
   msgSize -= sizeof(netmsg->linkData);
   netmsg->handlerID = gmNetMsgHandlerID;
   netmsg->ops = ops;
   netmsg->relationID = linkdata->flavor;

   // No GID needed, since receivers know I am its host.
   //AssertMsg1(gmObjNet->ObjHostedHere(linkdata->source),
   //           "Sending link msg when I don't own %d!",
   //           linkdata->source);
   netmsg->sourceGID = gmNetMan->ToGlobalObjID(linkdata->source);
   netmsg->destGID = gmNetMan->ToGlobalObjID(linkdata->dest);

   if (ops & kListenLinkModify)
   {
      void *data = DataStore->Get(id);
      memcpy(&netmsg->linkData, data, dataSize);
   }

   RelNetSpew(("SEND: %s link %s->%s op %x (id %x)\n",
               Desc.name, ObjWarnName(linkdata->source), 
               ObjWarnName(linkdata->dest), ops, id));

   gmNetMan->Broadcast(netmsg, msgSize, TRUE);

   free(netmsg);
}

void cStandardRelation::ReceiveLinkMsg(const sNetMsg_Generic *pMsg, ulong size, ObjID from, void*)
{
   sLinkNetMsg *netmsg = (sLinkNetMsg *)pMsg;
   AutoAppIPtr(LinkManager);
   IRelation* rel = pLinkManager->GetRelation(netmsg->relationID);
   ObjID source = gmNetMan->FromGlobalObjID(&(netmsg->sourceGID));
   ObjID dest = gmNetMan->FromGlobalObjID(&(netmsg->destGID));
   LinkID id = LINKID_NULL;

   if (from == OBJ_NULL)
   {
      // This message is suspect, probably coming from a reset phase:
      RelNetSpew(("Got Link %s message from unknown player\n",
                  rel->Describe()->name));
      return;
   }

   RelNetSpew(("RECEIVE: %s Link %s->%s op %x ",
               rel->Describe()->name, ObjWarnName(source), ObjWarnName(dest), netmsg->ops));

   // Prevent loops
   gmRespondSourceObj = source;
   gmRespondDestObj = dest;

   AutoAppIPtr(ObjectSystem);

   if (rel->GetID() == RELID_NULL || source == OBJ_NULL || dest == OBJ_NULL)
      Warning(("Invalid relation ID or object ID in relation message\n"));
   else if (!pObjectSystem->Exists(source) ||
            !pObjectSystem->Exists(dest))
   {
      RelNetSpew(("  Message rejected due to non-existent object.\n"));
      return;
   }
   // Check for Birth & Modify combined, so the right listener gets called.
   else if (netmsg->ops == (kListenLinkBirth | kListenLinkModify))
      rel->AddFull(source, dest, &netmsg->linkData);
   else if (netmsg->ops == kListenLinkBirth)
      rel->Add(source, dest);
   else
   {
      id = rel->GetSingleLink(source, dest);
      if (id == LINKID_NULL)
      {
#ifndef SHIP
         // This is pretty normal in the case of link deaths, but not
         // modifies:
         if (!(netmsg->ops == kListenLinkDeath))
         {
            RelNetSpew(("\n")); // since we are in untermed RNS otherwise
            Warning(("NetmsgRcv No link %s from %s to %s (ops %d)\n",
                     rel->Describe()->name, ObjWarnName(source), 
                     ObjWarnName(dest), netmsg->ops));
         }
#endif
      }
      else if (netmsg->ops == kListenLinkDeath)
         rel->Remove(id);
      else if (netmsg->ops == kListenLinkModify)
         rel->SetData(id, &netmsg->linkData);
      else
         Warning(("Unexpected relation ops recieved in net message\n"));
   }
#ifdef PLAYTEST
   if (id==LINKID_NULL)
   {
      if ((source == OBJ_NULL) ||
          (dest == OBJ_NULL))
      {
         RelNetSpew(("(OBJ_NULL!!!)\n"));
      } else {
         id=rel->GetSingleLink(source, dest);
         RelNetSpew(("(id %x)\n",id));
      }
   }
#endif
   gmRespondSourceObj = OBJ_NULL;
   gmRespondDestObj = OBJ_NULL;
}

typedef ILinkQueryDatabase* (*factfunc)(void); 

static factfunc db_factories[4] = 
{
   NULL, // already handled
   CreateKnownSourceLinkQueryDatabase,
   CreateKnownDestLinkQueryDatabase,
   CreateBothKnownLinkQueryDatabase,
};

static factfunc db_shared[4] = 
{
   NULL, // already handled
   SharedKnownSourceLinkQueryDatabase,
   SharedKnownDestLinkQueryDatabase,
   SharedBothKnownLinkQueryDatabase,
};

IRelation* CreateRelationFromLinkStore(const sRelationDesc* desc, const sRelationDataDesc* datadesc, ILinkStore* store, RelationQueryCaseSet optimize)
{
#ifdef DBG_ON
   // debugging var
   if (config_is_defined("unoptimized_links"))
      optimize = 0; 
#endif 

   // Special case: if you optimize source-known but not both-known, we use source-known
   // in the both-known case.  So, if you optimize both, we need to make them separate

   ulong special_case = kQCaseSetSourceKnown|kQCaseSetBothKnown;
   if (optimize & special_case)
      optimize |= PrivateQDatabase(kQCaseSetSourceKnown);

   

   // Fill in the custom descriptor
   sCustomRelationDesc stuff;
   memset(&stuff,0,sizeof(stuff));

   stuff.store = store;
   stuff.DBs[0] = NULL;    // Always use the store's db in the none-known case
   for (int i = 1; i < kRelationNumQueryCases; i++)
   {
      ulong qcase = SingletonQCaseSet(i);
      // if we want to optimize this case, then call the factory
      if (optimize & qcase)
      {
         if (optimize & PrivateQDatabase(qcase) && db_factories[i] != NULL)
            stuff.DBs[i] = db_factories[i]();
         else if (db_shared[i] != NULL)
            stuff.DBs[i] = db_shared[i]();
         else
           stuff.DBs[i] = NULL;
      }
   }

   IRelation* result = CreateCustomRelation(desc,datadesc,&stuff);

   for (int i = 0; i < kRelationNumQueryCases; i++)
      SafeRelease(stuff.DBs[i]);

   return result;
}

IRelation* CreateStandardRelation(const sRelationDesc* desc, const sRelationDataDesc* ddesc, RelationQueryCaseSet optimize)
{
   return CreateRelationFromLinkStore(desc,ddesc,NULL,optimize);
}


IRelation* CreateCustomRelation(const sRelationDesc* desc, const sRelationDataDesc* datadesc, const sCustomRelationDesc* custom)
{
   sCustomRelationDesc me = *custom;

   // set up a default store
   if (me.store != NULL)
      me.store->AddRef();
   else
      me.store = new cLinkArrayDelegate;

   // set up a default query database 
   if (me.DBs[0] != NULL)
      me.DBs[0]->AddRef();
   else
      Verify(SUCCEEDED(COMQueryInterface(me.store,IID_ILinkQueryDatabase,(void**)&me.DBs[0])));

   cLinkQueryDatabaseSet DB(me.DBs[0],me.DBs[1],me.DBs[2],me.DBs[3]);

   IRelation* result = new cStandardRelation(*desc,*datadesc,me.store,DB,me.datastore);
   SafeRelease(me.store);
   SafeRelease(me.DBs[0]);

   return result;
}


IMPLEMENT_DELEGATION(cDelegatedRelation);
