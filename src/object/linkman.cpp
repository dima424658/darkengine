/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/linkman.cpp,v 1.20 1998/10/18 02:32:41 mahk Exp $
#include <appagg.h>
#include <linktype.h>
#include <linkbase.h>
#include <linkman_.h>
#include <string.h>
#include <hshpptem.h>
#include <hshsttem.h>
#include <lnkquer_.h>
#include <linkdb_.h>
#include <linkid.h>
#include <lnkremap.h>

#include <tagfile.h>
#include <objnotif.h>
#include <vernum.h>

#include <config.h>
#include <cfgdbg.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// 
// cLinkManager
//
////////////////////////////////////////////////////////////

static ulong filetype_masks[] = { kObjPartAbstract, kObjPartMission, kObjPartTerrain }; 

IMPLEMENT_AGGREGATION_SELF_DELETE(cLinkManager);

//----------------------------------------
// Construction/Deconstruction
//

cLinkManager::cLinkManager(IUnknown* pOuter)
   : LockCount(0),
     QueryTime(0),
     MidPurge(FALSE),
     EarliestDeath(QUERY_DATE_NONE),
     LatestBirth(QUERY_DATE_NONE),
     NullRel(new cNullRelation)
{
   // set up null relation
   byID.Append((IRelation*)NullRel);
   // Add ourselves to the aggregate
   INIT_AGGREGATION_2(pOuter,IID_ILinkManager,this,
                             IID_ILinkManagerInternal,this, 
                      kPriorityNormal, NULL);

}

cLinkManager::~cLinkManager()
{

}

////////////////////////////////////////////////////////////
// NON-INTERFACE METHODS
// 

STDMETHODIMP_(RelationID) cLinkManager::AddRelation(IRelation* rel)
{
   const char* name = rel->Describe()->name;
   const IRelation* found = byName.Search(name);

   if (found != NULL)
   {
      Warning(("Attempted to add duplicate relation %s\n",name));
      return RELID_NULL;
   }
   byName.Insert(rel);
   RelationID id = (RelationID)byID.Append(rel);
   rel->AddRef();
   return id;
}

////////////////////////////////////////

STDMETHODIMP cLinkManager::ReplaceRelation(RelationID id, IRelation* rel)
{
   Assert_(id < byID.Size());
   IRelation* old = byID[id];
   byName.Remove(old);
   byName.Insert(rel);
   byID[id] = rel;

   rel->AddRef();
   SafeRelease(old);

   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cLinkManager::RemoveRelation(RelationID id)
{
   IRelation* rel = byID[id];
   if (rel != NullRel)
   {
      byName.Remove(rel);
      byID[id] = NullRel;
      NullRel->AddRef();
      rel->Release();
   }
   else
   {
      ConfigSpew("linkdel",("No relation %d to remove\n",id));
      return E_FAIL;
   }
   return S_OK;
}

////////////////////////////////////////

static BOOL AddQDBToList(ILinkQueryDatabase* db, LinkQDBList& QueryDBs)
{
   int i;
   for (i = 0; i < QueryDBs.Size(); i++)
   {
      LinkQDBEntry& e = QueryDBs[i];
      if (e.db == db)
      {
         e.count++;
         return FALSE;
      }
   }
   QueryDBs.Append(LinkQDBEntry(db)); 
   return TRUE;   
} 

STDMETHODIMP cLinkManager::AddQueryDB(ILinkQueryDatabase* db, RelationQueryCaseSet cases)
{
   BOOL accum = FALSE;
   // @OPTIMIZE: Faster algorithm! 
   for (int i = 0; i < kRelationNumQueryCases; i++)
      if (cases & SingletonQCaseSet(i))
      {
         accum = AddQDBToList(db,QueryDBs[i]) || accum ;
      }

   return (accum) ? S_OK : S_FALSE;
}

////////////////////////////////////////


static BOOL RemoveQDBFromList(ILinkQueryDatabase* db, LinkQDBList& QueryDBs)
{
   int i;
   for (i = 0; i < QueryDBs.Size(); i++)
   {
      LinkQDBEntry& e = QueryDBs[i];
      if (e.db == db)
      {
         BOOL remove = --e.count == 0;
         if (remove)
            QueryDBs.DeleteItem(i);
         return remove;
      }
   }
   return FALSE;
}

STDMETHODIMP cLinkManager::RemoveQueryDB(ILinkQueryDatabase* db, RelationQueryCaseSet cases)
{
   BOOL accum = FALSE;
   for (int i = 0; i < kRelationNumQueryCases; i++)
      if (cases & SingletonQCaseSet(i))
      {
         accum = RemoveQDBFromList(db,QueryDBs[i]) || accum;
      }
   return (accum) ? S_OK : S_FALSE;
}

////////////////////////////////////////

STDMETHODIMP_(tQueryDate) cLinkManager::Lock(void)
{
   ++LockCount; 
   AssertMsg1(LockCount < 64, "Link Manager Lock Count is %d\n",LockCount);
   return ++QueryTime;
}

STDMETHODIMP cLinkManager::Unlock(tQueryDate )
{
      // remove any deferred links
   if (LockCount == 1)
   {
      MidPurge = TRUE;

      while (DeathDates.nElems() > 0)
      {
         LinkTimeTable::cIter iter;
         for (iter = DeathDates.Iter(); !iter.Done(); iter.Next())
         {
            Remove(iter.Key());
            DeathDates.Delete(iter.Key());
         }

         //         mprintf("DeathDates.nElems() is %d\n",DeathDates.nElems()); 
      }

      DeathDates.Clear(); 

      if (BirthDates.nElems() > 0)
         BirthDates.Clear(); 

      QueryTime = 0;
      EarliestDeath = QUERY_DATE_NONE;
      LatestBirth = QUERY_DATE_NONE;
      MidPurge = FALSE;
   }
   LockCount--;
   return S_OK;
}

STDMETHODIMP_(long) cLinkManager::GetLockCount()
{
   return (long)LockCount;
} 

STDMETHODIMP_(BOOL) cLinkManager::LinkDeath(LinkID link)
{
   if (LockCount == 0 || LockCount == 1 && MidPurge)
      return FALSE;
   if (EarliestDeath == QUERY_DATE_NONE ||
       QueryTime < EarliestDeath)
      EarliestDeath = QueryTime;
   DeathDates.Insert(link,QueryTime);
   return TRUE;
}

STDMETHODIMP cLinkManager::LinkBirth(LinkID link)
{
   if (LockCount != 0)
   {
      BirthDates.Insert(link,QueryTime);
   if (LatestBirth == QUERY_DATE_NONE ||
       QueryTime > LatestBirth)
      LatestBirth = QueryTime;
   }
   return S_OK;
}

STDMETHODIMP_(BOOL) cLinkManager::LinkValid(LinkID link, tQueryDate age)
{
   tQueryDate date;
   // Did the link die before the query started? 
   if (EarliestDeath != QUERY_DATE_NONE && age > EarliestDeath) 
      if (DeathDates.Lookup(link,&date))
      {
         if (date < age) // strictly less than.  date == age means died after
            return FALSE;
      }
   // was it born after?
   if (LatestBirth != QUERY_DATE_NONE && age <= LatestBirth)
      if (BirthDates.Lookup(link,&date))
      {
         if (date >= age) // strictly greater.  date == age means born after
            return FALSE;
      }
   return TRUE;
}

//----------------------------------------
// Relation Look-up
//

STDMETHODIMP_(IRelation*) cLinkManager::GetRelation(RelationID id)
{
   if (RELID_IS_INVERTED(id))
   {
      id = RELID_INVERT(id);
      if (id < byID.Size())
         return byID[id]->Inverse(); // Inverse should addref
   }
   else if (id < byID.Size())
   {
      IRelation* rel = byID[id];
      rel->AddRef();
      return rel;
   }
   NullRel->AddRef();
   return NullRel;
}

STDMETHODIMP_(IRelation*) cLinkManager::GetRelationNamed(const char* name)
{
   static const char* prefix = RELNAME_INVERSE_PREFIX;
   BOOL inverted = FALSE;
   while (strnicmp(prefix,name,strlen(prefix)) == 0)
   {
      name += strlen(prefix);
      inverted = !inverted;
   }
   IRelation* rel = byName.Search(name);
   if (rel == NULL) rel = NullRel;

   if (inverted)
      rel = rel->Inverse();
   else
      rel->AddRef();
   return rel;
}


//------------------------------------------------------------
// Iteration
// 

STDMETHODIMP cLinkManager::IterStart(sRelationIter* iter)
{
   if (iter == NULL) return E_FAIL;
   iter->relid = RELID_NULL;
   return S_OK;
}

STDMETHODIMP_(BOOL) cLinkManager::IterNext(sRelationIter* iter, RelationID* rel)
{
   for (iter->relid++; iter->relid < byID.Size(); iter->relid++)
   {
      if (byID[iter->relid] != NullRel)
      {
         if (rel != NULL) *rel = iter->relid;
         return TRUE;
      }
   }
   return FALSE;
}

STDMETHODIMP cLinkManager::IterStop(sRelationIter* )
{
   return S_OK;   
}

//------------------------------------------------------------
// Notification
//

STDMETHODIMP cLinkManager::Notify(eRelationNotifyMsg msg, RelationNotifyData data)
{
   int i;   
   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifySave:
         save_relation_ids((ITagFile*)data,NOTIFY_PARTITION(msg));
         break;
      case kObjNotifyLoad:
         load_relation_ids((ITagFile*)data,NOTIFY_PARTITION(msg));
         break;
      case kObjNotifyReset:
      {
         for (int i = 0; i < kNumLinkPartitions; i++)
            idMap[i].SetSize(0); 
         ClearLinkMappingTable(); 
      }
      break; 
   }


   for (i = RELID_NULL+1; i < byID.Size(); i++)
   {
      byID[i]->Notify(msg,data);
   }
   return S_OK;
}

//------------------------------------------------------------
// LINK OPS
//

STDMETHODIMP_(LinkID) cLinkManager::Add(ObjID source, ObjID dest, RelationID id)
{
   IRelation* rel = GetRelation(id);
   LinkID link = rel->Add(source,dest);
   SafeRelease(rel); 
   return link;
}

STDMETHODIMP_(LinkID) cLinkManager::AddFull(ObjID source, ObjID dest, RelationID id, void* data)
{
   IRelation* rel = GetRelation(id);
   LinkID link = rel->AddFull(source,dest,data);
   SafeRelease(rel); 
   return link;
}

STDMETHODIMP cLinkManager::Remove(LinkID link)
{
   IRelation* rel = GetRelation(LINKID_RELATION(link));
   HRESULT result = rel->Remove(link);
   SafeRelease(rel); 
   return result;

}

STDMETHODIMP_(BOOL) cLinkManager::Get(LinkID link, sLink* out) const
{
   IRelation* rel = ((ILinkManager*)this)->GetRelation(LINKID_RELATION(link));
   BOOL result = rel->Get(link,out);
   SafeRelease(rel); 
   return result;   
}

STDMETHODIMP cLinkManager::SetData(LinkID link,void* data)
{
   IRelation* rel = GetRelation(LINKID_RELATION(link));
   HRESULT result = rel->SetData(link,data);
   SafeRelease(rel); 
   return result;
}

void* cLinkManager::GetData(LinkID link)
{
   IRelation* rel = GetRelation(LINKID_RELATION(link));
   void* result = rel->GetData(link);
   SafeRelease(rel); 
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(LinkID) cLinkManager::GetSingleLink(RelationID id, ObjID source, ObjID dest)
{
   IRelation* rel = GetRelation(id);
   LinkID result = rel->GetSingleLink(source, dest);
   SafeRelease(rel); 
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cLinkManager::AnyLinks(RelationID id, ObjID source, ObjID dest)
{
   IRelation* rel = GetRelation(id);
   BOOL result = rel->AnyLinks(source, dest);
   SafeRelease(rel); 
   return result;
}

//------------------------------------------------------------
// QUERIES 
//

////////////////////////////////////////////////////////////
// 
// cAllQuery
//
// Traverses a set of query databases
//
////////////////////////////////////////////////////////////

typedef cDynArray<ILinkQuery*> QueryList;

class cAllQuery : public cBaseLinkQuery
{
   int CurQ;
   QueryList Queries;
   ObjID Source,Dest;

   void Skip();

   cAllQuery(ObjID source, ObjID dest)
      : CurQ(0), Source(source), Dest(dest)
   {
   }

public:
   cAllQuery(LinkQDBList& forward, LinkQDBList& back, ObjID source, ObjID dest)
      : CurQ(0), 
        Source(source), 
        Dest(dest)
   {
      int i;
      for (i = 0; i < forward.Size(); i++)
      {
         ILinkQuery* query = forward[i].db->QueryComplex(source,dest,RELID_WILDCARD,Birthday);
         Queries.Append(query);
      }

      for (i = 0; i < back.Size(); i++)
      {
         ILinkQuery* query = back[i].db->QueryComplex(dest,source,RELID_WILDCARD,Birthday);
         Queries.Append(query->Inverse());
         SafeRelease(query);
      }
      Skip();
   }

   ~cAllQuery()
   {
      for (int i = 0; i < Queries.Size(); i++)
         SafeRelease(Queries[i]);
   }

   STDMETHOD_(BOOL,Done)() const; 
   STDMETHOD(Link)(sLink* link) const;
   STDMETHOD(Next)();   

   STDMETHOD_(LinkID,ID)() const { return Queries[CurQ]->ID();};
   STDMETHOD_(void*,Data)() const { return Queries[CurQ]->Data();};
   STDMETHOD_(ILinkQuery*,Inverse)();

                      
};


STDMETHODIMP_(BOOL) cAllQuery::Done() const
{
   return CurQ >= Queries.Size();
}

STDMETHODIMP cAllQuery::Link(sLink* link) const
{
   return Queries[CurQ]->Link(link);
}

void cAllQuery::Skip()
{
   if (CurQ >= Queries.Size() || !Queries[CurQ]->Done())
      return;
      
   for(CurQ++; CurQ < Queries.Size(); CurQ++)
   {
      if (!Queries[CurQ]->Done()) 
         return;
   }
}

STDMETHODIMP cAllQuery::Next() 
{
   if (Done()) return E_FAIL;

   HRESULT result = Queries[CurQ]->Next();

   if (!Queries[CurQ]->Done()) return S_OK;
   
   Skip();

   return S_OK;
}

STDMETHODIMP_(ILinkQuery*) cAllQuery::Inverse()
{
   cAllQuery* out = new cAllQuery(Dest,Source);
   // copy just the unspent queries
   for (int i = CurQ; i < Queries.Size(); i++)
   {
      out->Queries.Append(Queries[i]->Inverse());
   }
   return out;
}

////////////////////////////////////////



STDMETHODIMP_(ILinkQuery*) cLinkManager::Query(ObjID source, ObjID dest, RelationID flavor)
{
   const int inverse_cases[] = { kRelationNoneKnown, kRelationDestKnown, kRelationSourceKnown, kRelationBothKnown };
   if (flavor == RELID_WILDCARD)
   {
      int qcase = kRelationNoneKnown;
      if (source != LINKOBJ_WILDCARD)
         qcase |= kRelationSourceKnown;
      if (dest != LINKOBJ_WILDCARD)
         qcase |= kRelationDestKnown;
      return new cAllQuery(QueryDBs[qcase],
                           QueryDBs[inverse_cases[qcase]],
                           source,dest);
   }
   else
   {
      IRelation* rel = GetRelation(flavor);
      ILinkQuery* result = rel->Query(source,dest);
      SafeRelease(rel); 
      return result;
   }
}

////////////////////////////////////////

STDMETHODIMP_(LinkID) cLinkManager::RemapOnLoad(LinkID id)
{
   cIDMap& map = idMap[LINKID_PARTITION(id)]; 
   RelationID rel = LINKID_RELATION(id); 
   BOOL inverted = RELID_IS_INVERTED(rel); 
   if (inverted) rel = RELID_INVERT(rel); 

   if (rel > map.Size())
      return LINKID_NULL; 

   rel = map[rel]; 
   if (rel == RELID_NULL)
      return LINKID_NULL; 

   if (inverted)
      rel = RELID_INVERT(rel); 

   return LINKID_MAKE2(rel,LINKID_NON_RELATION(id)); 
}

//------------------------------------------------------------
// INTERNAL HELPERS
//

//
// This is code to preserve relation ID's across saves.
// It can't possibly work, because different database files can have different opinions.
// We need an objid-style remapper.
// 

#define SAVE_RELATIONS
#ifdef SAVE_RELATIONS

static const TagFileTag relation_tag = { "Relations"}; 
static const TagVersion relation_ver = { 1, 0 };

void cLinkManager::save_relation_ids(ITagFile* file, ulong filetype)
{
   TagVersion v = relation_ver;

   if (SUCCEEDED(file->OpenBlock(&relation_tag,&v)))
   {
      for (int i = RELID_NULL + 1; i < byID.Size(); i++)
      {
         const sRelationDesc* desc = byID[i]->Describe();
         Verify(file->Write(desc->name,sizeof(desc->name)) == sizeof(desc->name)); 
      }
      file->CloseBlock(); 
   }
}

void cLinkManager::load_relation_ids(ITagFile* file, ulong filetype)
{
   TagVersion v = relation_ver;

   if (SUCCEEDED(file->OpenBlock(&relation_tag,&v)))
   {
      for (int i = RELID_NULL + 1; file->TellFromEnd() > 0; i++)
      {
         sRelationDesc desc;
         Verify(file->Read(desc.name,sizeof(desc.name)) == sizeof(desc.name)); 

         IRelation* rel = byName.Search(desc.name); 

         if (rel != NULL && rel != NullRel)
         {
            RelationID newid = rel->GetID(); 

            for (int p = 0; p < kNumLinkPartitions; p++)
               if (filetype & filetype_masks[p])
               {
                  cIDMap& map = idMap[p];

                  while(i >= map.Size())
                     map.Append(LINKID_NULL); 

                  map[i] = newid; 
               }
         }

      }
      file->CloseBlock(); 
   }
}

#endif 

//------------------------------------------------------------
// AGGREGATE PROTOCOL 
//

HRESULT cLinkManager::Init()
{
   // Put app init stuff here
   SetManager(this);
   
   LinkDatabases::SetDBs(MakeLinkDatabases());
   return S_OK;
}


HRESULT cLinkManager::End()
{
   for(RelationID i = RELID_NULL+1; i < byID.Size(); i++)
   {
      IRelation* rel = byID[i];
      if (rel == NullRel) 
         continue;
      rel->AddRef();

      RemoveRelation(i);

      int refs = rel->Release();
      if (refs > 0)
      {
         ConfigSpew("linkref_spew",("Relation %s has %d refs on exit\n",rel->Describe()->name,refs));
         while (rel->Release() > 0)
            ;
      }
      
   } 
   
   while (NullRel->Release() > 0)
      ;

   SetManager(NULL);

   // Put app term stuff here
   delete LinkDatabases::DBs();
   LinkDatabases::SetDBs(NULL);

   return S_OK;
}


////////////////////////////////////////////////////////////

ILinkManagerInternal* cLinkManagerKnower::linkMan = NULL;

tResult LGAPI LinkManagerCreate(void)
{
   IUnknown* outer = AppGetObj(IUnknown); 
   cLinkManager* man = new cLinkManager(outer);
   return (man != NULL) ? NOERROR : E_FAIL;
}
