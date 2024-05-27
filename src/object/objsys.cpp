/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/objsys.cpp,v 1.91 2000/03/07 19:57:45 toml Exp $
#include <appagg.h>
#include <constrid.h>
#include <aggmemb.h>

#include <iobjsys_.h>
#include <objsys.h>
#include <osysbase.h>
#include <objnotif.h>
#include <objremap.h>

#include <propman.h>
#include <propbase.h>

#include <linkman.h>
#include <linkbase.h>

#include <traittyp.h>
#include <traitbas.h>
#include <traitman.h>
#include <traitprp.h>
#include <donorq.h>

#include <objquer_.H>

#include <tagfile.h>
#include <vernum.h>

#include <config.h>
#include <cfgdbg.h>

#include <dlisttem.h>

#include <netman.h>
#include <simtime.h>
#include <playrobj.h>


// For playtest object creation timing
#include <timer.h>
#include <edittool.h>
#include <mprintf.h>

#include <allocapi.h>

// Must be last header
#include <dbmem.h>

#define min(x,y) ((x<y)?(x):(y))
#define max(x,y) ((x>y)?(x):(y))

F_DECLARE_INTERFACE(IObjectSystem);

////////////////////////////////////////////////////////////
// OBJECT SYSTEM  IMPLEMENTATION
//

//------------------------------------------------------------
// EOS VARS
//

#define OLD_MIN_OBJ (-128)
#define OLD_MAX_OBJ (128)

ObjID gMaxObjID = OLD_MAX_OBJ;
ObjID gMinObjID = OLD_MIN_OBJ;

int gObjCount = 0;

// EOS globals
ubyte*  BaseActiveArray = NULL;

#define ARRAY_SIZE() ((gMaxObjID - gMinObjID + 7) / 8)

ObjID BottomObj = ROOT_ARCHETYPE; // Lowest active ObjID there has been
ObjID TopObj = 0;  // highest active objid


#ifdef PLAYTEST
//
// Create Statistics
//
static ulong gTotalCreateTime = 0;
static ulong gNumCreates = 0;

struct sObjCreateTime
{
   ulong stamp;
   ulong accum;
};

#define TIME_STACK_DEPTH 32
static sObjCreateTime gTimeStack[TIME_STACK_DEPTH];
static int gTimeStackTop = 0;

EXTERN void ResetObjTimerStats()
{
   if (gTotalCreateTime > 0)
   {
      float avg = gNumCreates ? 1.0*gTotalCreateTime/gNumCreates : 0.0;
      ConfigSpew("obj_stats",("Created %d objects, Total time %d msec avg %.02g\n",gNumCreates,gTotalCreateTime,avg));
   }

   gNumCreates = gTotalCreateTime = 0;
   gTimeStackTop = 0;
}

static void PushTimer()
{
   Assert_(gTimeStackTop < TIME_STACK_DEPTH);

   sObjCreateTime& time = gTimeStack[gTimeStackTop++];
   time.stamp = 0;
   time.accum = 0;
}

static void PopTimer()
{
   gTimeStackTop--;
}

static void StartTimer()
{
   gTimeStack[gTimeStackTop-1].stamp = tm_get_millisec_unrecorded();
}

// Convert gCreateTime from a timestamp to an interval
// Creepy that this is the same thing
static void StopTimer()
{
   sObjCreateTime& time = gTimeStack[gTimeStackTop-1];

   ulong delta = tm_get_millisec_unrecorded() - time.stamp;
   time.accum += delta;
}

// Add the timer to our stats
// Warn if this is unusual
static void AccumTimerStats(ObjID obj)
{
   sObjCreateTime& time = gTimeStack[gTimeStackTop-1];

   ulong t = time.accum;
   // If we're greater than twice the average, warn
   if (t*gNumCreates > 2*gTotalCreateTime)
   {
      AutoAppIPtr_(TraitManager,pTraitMan);
      AutoAppIPtr_(ObjectSystem,pObjSys);  // oh bitter irony

      ObjID arch = pTraitMan->GetArchetype(obj);
      ConfigSpew("obj_stats",("WOE: Took %d msec to create %s %d\n",t,pObjSys->GetName(arch),obj));
   }

   gNumCreates++;
   gTotalCreateTime += t;
}


#else

#define ResetObjTimerStats()
#define ResetTimer()
#define StartTimer()
#define StopTimer()
#define PushTimer()
#define PopTimer()
#define AccumTimerStats(x)

#endif


////////////////////////////////////////////////////////////
// cObjectSystem
//

//------------------------------------------------------------
// Construction/Deconstruction
//

cObjectSystem* cObjectSystem::TheObjSys = NULL;

static sRelativeConstraint Constraints[] =
{
   { kConstrainAfter, &IID_IPropertyManager },
   { kNullConstraint},
};

cObjectSystem::cObjectSystem(IUnknown* pOuter)
   :LinkMan(NULL),
    PropMan(NULL),
    TraitMan(NULL),
    SymName(NULL),
    Transient(NULL),
    Immobile(NULL),
    NetMan(NULL),
    ObjNet(NULL),
    mLockCount(0)
{
   MI_INIT_AGGREGATION_1(pOuter,IObjectSystem,kPriorityNormal,Constraints);
   TheObjSys = this;
}

cObjectSystem::~cObjectSystem()
{
   TheObjSys = NULL;
}

//------------------------------------------------------------
// BUILT IN PROPERTIES
//

//
// Transience property (do we save the object to disk)
//

static sPropertyDesc transdesc =
{
   "Transient",
   kPropertyNoInherit,
   NULL, 0,0,
   { "Object System", "Transient" },

};

static IBoolProperty* CreateTransience()
{
   return CreateBoolProperty(&transdesc,kPropertyImplSparseHash); // Boolean);
}

//
// Immobile property (is this object essentially terrain)
//

static sPropertyDesc immobiledesc =
{
   "Immobile",
   0,
   NULL, 0, 0,
   { "Object System", "Immobile" },
};

static IBoolProperty* CreateImmobility()
{
   return CreateBoolProperty(&immobiledesc, kPropertyImplSparseHash);
}

static BOOL LGAPI mission_subpart_filter_func(ObjID , const sObjPartitionFilter* )
{
   return TRUE; // all concrete objects are in the mission
}

//------------------------------------------------------------
// Init/Shutdown
//

void dispatchObjSysMsg(const sNetMsg_Generic *pMsg,
                       ulong size,
                       ObjID from,
                       void *pClientData);

STDMETHODIMP cObjectSystem::Init()
{
   gMinObjID = 0; 
   gMaxObjID = 0; 

   // count of actives
   gObjCount = 0;
   BaseActiveArray = NULL;

   PropMan = AppGetObj(IPropertyManager);
   LinkMan = AppGetObj(ILinkManager);
   TraitMan = AppGetObj(ITraitManager);

   Transient = CreateTransience();
   Immobile = CreateImmobility();

#ifdef NEW_NETWORK_ENABLED
   NetMan = AppGetObj(INetManager);
   ObjNet = AppGetObj(IObjectNetworking);
#endif
   
   // set the default sub partition filter
   sObjPartitionFilter filter = { mission_subpart_filter_func };
   SetSubPartitionFilter(kObjPartMission,&filter);

   return S_OK;
}

STDMETHODIMP cObjectSystem::End()
{
   for (int i = 0; i < m_Sinks.Size(); i++)
      SafeRelease(m_Sinks[i]); 

   delete [] BaseActiveArray;
   BaseActiveArray = nullptr;


   SafeRelease(PropMan);
   SafeRelease(LinkMan);
   SafeRelease(TraitMan);

   SafeRelease(Transient);
   SafeRelease(Immobile);
   SafeRelease(SymName);

#ifdef NEW_NETWORK_ENABLED
   SafeRelease(NetMan);
   SafeRelease(ObjNet);
#endif

   return S_OK;
}



////////////////////////////////////////

STDMETHODIMP cObjectSystem::Reset()
{
   DatabaseNotify(kObjNotifyReset,NULL);
   return S_OK;
}


//------------------------------------------------------------
// Create/Destroy
//

void cObjectSystem::build_obj_lists(void)
{
   mActiveObjs.DestroyAll();
   mFreeObjs.DestroyAll();
   TopObj = 0; 

   // @PORTABILITY: if we start using lots of objects (i.e. > 64k)
   // the "active vector" will become impractical.
   for (int i = gMaxObjID - 1; i >= 0; i--)
      if (ObjExists(i) && i > TopObj)
      {
         TopObj = i;
         break;
      }

   gObjCount = 0;
   LGALLOC_AUTO_CREDIT();
   for (int i = 1; i <= TopObj; i++)
      if (ObjExists(i))
      {
         mActiveObjs.Append(i);
         gObjCount++;
      }
      else
         mFreeObjs.Append(i);
   ConfigSpew("objsys_count",("Object count after rebuild is %d\n",gObjCount));
}

ObjID cObjectSystem::get_next_obj(eObjConcreteness concrete)
{
   ObjID retval = OBJ_NULL;
   if (concrete == kObjectAbstract)
   {
      ObjID i;
      for (i = -1; i >= gMinObjID; i--)
         if (!ObjExists(i))
            break;
      if (i < gMinObjID)
      {
         CriticalMsg("Ran out of abstract obj ID's\nRe-run with a larger obj_min.");
         return OBJ_NULL;
      }
      if (i < BottomObj) BottomObj = i;
      retval= i;
   }
   else
   {
      cObjList::cNode* next = mFreeObjs.GetFirst();
      if (next)
      {
         retval = next->Value();
         mFreeObjs.Delete(*next);
      }
      else
      {
         TopObj++;
         retval = TopObj;
      }

#ifdef EDITOR
	  if (retval >= (gMaxObjID - 100))
	    mprintf("Object count dangerously high(%d of %d).  Be very very careful.\n",retval,gMaxObjID);
#endif


      if (retval >= gMaxObjID)
      {
         CriticalMsg("Ran out of concrete obj ID's\nRe-run with a larger obj_max.");

         if (TopObj >= gMaxObjID)
            TopObj = gMaxObjID - 1;

         return OBJ_NULL;
      }
      LGALLOC_AUTO_CREDIT();
      mActiveObjs.Append(retval);
   }
   BaseActiveElem(retval) |= BaseActiveMask(retval);

   ConfigSpew("objsys_start_create",("Creating object %d\n",retval));
   return retval;
}

////////////////////////////////////////


STDMETHODIMP_(ObjID) cObjectSystem::BeginCreate(ObjID exemplar, eObjConcreteness concrete)
{
   PushTimer();
   StartTimer();

   Lock();
   ObjID archetype = exemplar;
   Assert_(concrete == kObjectConcrete || OBJ_IS_ABSTRACT(archetype) || archetype == OBJ_NULL);

   if (!ObjExists(archetype))
   {
      Warning(("BeginCreate: Archetype %d does not exist\n",archetype));
      archetype = OBJ_NULL;
   }

   BOOL clone = archetype != OBJ_NULL && OBJ_IS_CONCRETE(archetype) && !TraitMan->ObjIsDonor(archetype);

   ObjID obj = get_next_obj(concrete);

   if (obj == OBJ_NULL)
   {
      StopTimer();
      return obj;
   }

#ifdef NEW_NETWORK_ENABLED
   // Net broadcast the creation if it's called for.
   ObjNet->StartBeginCreate(exemplar, obj);
#endif

   // Make sure the archetype is inheritable
   if (clone)
      archetype = TraitMan->GetArchetype(exemplar);

#ifndef SHIP
   if (concrete == kObjectConcrete) {
      ConfigSpew("objcreate", ("Creating object %d (%s)\n",
                               obj,
                               GetName(archetype)));
   }
#endif

   TraitMan->AddObject(obj,archetype);

#ifdef NEW_NETWORK_ENABLED
   ObjNet->FinishBeginCreate(obj);
#endif

   notify_obj(obj,kObjNotifyBeginCreate);

   if (clone)
      CloneObject(obj,exemplar);

   StopTimer();

   return obj;
}

////////////////////////////////////////

STDMETHODIMP cObjectSystem::EndCreate(ObjID obj)
{
   HRESULT retval = S_FALSE;

   StartTimer();

#ifdef NEW_NETWORK_ENABLED
   ObjNet->StartEndCreate(obj);
#endif

   if (obj != OBJ_NULL)
   {
      notify_obj(obj,kObjNotifyCreate);
      ConfigSpew("objsys",("Done creating %d\n",obj));
      retval = S_OK;
      gObjCount++;
      ConfigSpew("objsys_count",("Object count now %d\n",gObjCount));
   }
   Unlock();

#ifdef NEW_NETWORK_ENABLED
   ObjNet->FinishEndCreate(obj);
#endif

   StopTimer();
   AccumTimerStats(obj);
   PopTimer();

   return(retval);
}

////////////////////////////////////////


STDMETHODIMP_(ObjID) cObjectSystem::Create(ObjID archetype, eObjConcreteness concrete)
{
   ObjID obj = BeginCreate(archetype,concrete);
   EndCreate(obj);
   return obj;
}

////////////////////////////////////////

STDMETHODIMP cObjectSystem::Destroy(ObjID id)
{
   // Destroying OBJ_NULL is bad for you...
   if (id == OBJ_NULL)
      return S_FALSE;

#ifdef NEW_NETWORK_ENABLED
   // If we don't own the object, request that the owner destroy it:
   if (NetMan->Networking() &&
       ObjNet->ObjIsProxy(id) &&
       !ObjNet->HandlingProxy())
   {
      ObjNet->RequestDestroy(id);
      return S_FALSE;
   }
#endif

   ConfigSpew("objsys",("Destroying object %d\n",id));
   if (!ObjExists(id))
   {
      Warning(("Deleting non-existent object %d\n",id));
      return E_FAIL;
   }

   if (mLockCount > 0)
   {
      cObjList::cIter iter ;
      for (iter = mDeletedObjs.Iter(); !iter.Done(); iter.Next())
         if (id == iter.Value())
         {
            Warning(("Object %d has already been deleted\n",id));
            return S_FALSE;
         }

      mDeletedObjs.Prepend(id);

      return S_FALSE;
   }
   else
   {
      Lock();
      remove_obj(id);
      Unlock();
      return S_OK;
   }
}

////////////////////////////////////////

void cObjectSystem::remove_obj(ObjID id)
{
#ifdef NEW_NETWORK_ENABLED
   ObjNet->StartDestroy(id);
#endif

   BaseActiveElem(id) &= ~BaseActiveMask(id);

   notify_obj(id,kObjNotifyDelete);
   if (OBJ_IS_CONCRETE(id))
   {
      gObjCount--;
      ConfigSpew("objsys_count",("Object count now %d\n",gObjCount));

      // I guess we grovel through the list...
      // @OPTIMIZE: Perhaps a binary tree or hashtable is in order.
      //            this would help scalability as well as speed.
      cObjList::cIter iter;
      for (iter = mActiveObjs.Iter(); !iter.Done(); iter.Next())
         if (iter.Value() == id)
         {
            mActiveObjs.Delete(iter.Node());

            break;
         }

      LGALLOC_AUTO_CREDIT();
      if (id == TopObj)
         TopObj--;
      else
         mFreeObjs.Append(id);
   }

#ifdef NEW_NETWORK_ENABLED
   ObjNet->FinishDestroy(id);
#endif
}

////////////////////////////////////////

STDMETHODIMP cObjectSystem::Lock()
{
   mLockCount++;
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP cObjectSystem::Unlock()
{
   HRESULT retval = S_FALSE;
   mLockCount++;  // avoid race conditions
   if (mLockCount == 2)
   {
      while (mDeletedObjs.GetFirst() != NULL)
      {
         cObjList::cIter iter ;
         for (iter = mDeletedObjs.Iter(); !iter.Done(); iter.Next())
         {
            remove_obj(iter.Value());
            mDeletedObjs.Delete(iter.Node());
         }
      }
      retval = S_OK;
   }
   // decrement lock count twice
   mLockCount -= 2;
   Assert_(mLockCount >= 0);
   return retval;
}

////////////////////////////////////////

STDMETHODIMP cObjectSystem::CloneObject(ObjID to, ObjID from)
{
   // copy metaproperties
   {
      cAutoIPtr<IObjectQuery> oquery = TraitMan->Query(from,kTraitQueryDonors);
      cAutoIPtr<IDonorQuery> query(IID_IDonorQuery,oquery);

      for (; !query->Done(); query->Next())
      {
         tDonorPriority pri = query->Priority();
         // skip archetype
         if (pri == 0)
            continue;

         TraitMan->AddObjMetaPropertyPrioritized(to,query->Object(),pri);
      }
   }

   // copy non-cloneable properties.
   sPropertyIter iter;
   IProperty* prop;

   PropMan->BeginIter(&iter);
   for (prop = PropMan->NextIter(&iter); prop != NULL; prop = PropMan->NextIter(&iter))
   {
      const sPropertyDesc* desc = prop->Describe();
      if (!(desc->flags & kPropertyNoClone) && prop->IsSimplyRelevant(from))
         prop->Copy(to,from);
      SafeRelease(prop);
   }
   return S_OK;
}

//------------------------------------------------------------
// save/load stuff
//

static const TagFileTag VecTag = { "ObjVec" };
static const TagVersion VecVersion = { 0, 2};
static const TagVersion MinMaxAddedVersion = { 0, 2};
//
// Write or read the objactivearray
//

void cObjectSystem::SaveActiveArray(ITagFile* file, eObjPartition partition)
{
   TagVersion v = VecVersion;
   if (SUCCEEDED(file->OpenBlock(&VecTag,&v)))
   {
      partition = NOTIFY_PARTITION(partition);

      // Copy relevant bits of array to temporary array.

      uchar* tempArray  = new ubyte[ARRAY_SIZE()];
      memset(tempArray, 0, ARRAY_SIZE());

      int minobj = 0;
      int maxobj = 0;


      if (partition & kObjPartAbstract)
      {
         minobj = gMinObjID;
      }
      if (partition & kObjPartConcrete)
      {
         maxobj = gMaxObjID;
      }
      int start = ObjActiveIdx(minobj - gMinObjID);
      int end = ObjActiveIdx(maxobj - gMinObjID);
      int size = &tempArray[end] - &tempArray[start];

      memcpy(&tempArray[start], &BaseActiveArray[start], size);

      // if we're in a sub-partition, we need to clear out stuff
      if (partition & ~kObjParts)
         for (int i = minobj; i < maxobj; i++)
         if (!IsObjSavePartition(i,partition))
            tempArray[ObjActiveIdx(i-gMinObjID)] &= ~ObjActiveMask(i-gMinObjID);

      file->Move((char*)&gMinObjID,sizeof(gMinObjID));
      file->Move((char*)&gMaxObjID,sizeof(gMaxObjID));

      file->Move((char*)tempArray,ARRAY_SIZE());

      delete [] tempArray;
      file->CloseBlock();
   }

}

HRESULT cObjectSystem::LoadActiveArray(ITagFile* file, eObjPartition partition)
{
   TagVersion v = VecVersion;
   if (SUCCEEDED(file->OpenBlock(&VecTag,&v)))
   {
      partition = NOTIFY_PARTITION(partition);
      ObjID maxobj = OLD_MAX_OBJ, minobj = OLD_MIN_OBJ;
      if (v.major >= MinMaxAddedVersion.major
          && v.minor >= MinMaxAddedVersion.minor)
      {
         file->Move((char*)&minobj,sizeof(gMinObjID));
         file->Move((char*)&maxobj,sizeof(gMaxObjID));
      }

      if (minobj < gMinObjID || maxobj > gMaxObjID)
      {
         sObjBounds newBounds = { gMinObjID, gMaxObjID }; 

         if (partition & kObjPartAbstract)
            newBounds.min = min(gMinObjID,minobj);
            
         if (partition & kObjPartConcrete)
            newBounds.max = max(gMaxObjID,maxobj);
            
         Warning(("Resizing object id bounds to (%d, %d)\n",newBounds.min,newBounds.max)); 

         ResizeObjIDSpace(newBounds); 
      }

      int sz = (maxobj - minobj + 7) / 8;
      ubyte* tempArray = new uchar[sz];

      // Read in the array into tempArray, then OR the bits into ObjActiveArray.
      file->Move((char*)tempArray,sz);

      int offset = (minobj - gMinObjID)/8;

      // @TODO: actually store of vector of new objects
      // so that we can send the minimal set of create messages
      if (partition & kObjPartAbstract)
      {
         // load in the abstract objects;
         for (int i=0; i < ObjActiveIdx(-minobj); i++)
            BaseActiveArray[i + offset] |= tempArray[i];
      }

      if (partition & kObjPartConcrete)
      {

         // @OPTIMIZE: This is not fast!
         for (int i = 1; i < gMaxObjID; i++)
         {
            int idx = ObjActiveIdx(i-minobj);
            int mask = ObjActiveMask(i-minobj);

            if (tempArray[idx] & mask) // ObjID i is in the file
            {
               if (ObjExists(i)) // it's already in the level!
               {
                  // we have no free list, so we're going to have to scan the
                  // array for one.
                  int free;
                  for (free = 1; free < gMaxObjID; free++)
                  {
                     int idx = ObjActiveIdx(free-minobj);
                     int mask = ObjActiveMask(free-minobj);

                     if (!(tempArray[idx] & mask) && !ObjExists(free))
                        break;
                  }
                  if (free < gMaxObjID)
                  {
                     // make the object exist.
                     BaseActiveElem(free) |= BaseActiveMask(free);
                  }
                  else
                  {
                     Warning(("Cannot load obj %d from partition %x, no room!\n",i,partition));
                     // we're going to leave free exactly where it is; it will show up as not existing.
                  }

                  ConfigSpew("obj_remap_spew",("Remapping %d to %d\n",i,free));
                  // map the old objID to the new one
                  LGALLOC_AUTO_CREDIT();
                  AddObjMappingToTable(free,i);
                  mLoadedObjs.Append(free);
               }
               else
               {
                  LGALLOC_AUTO_CREDIT();
                  BaseActiveElem(i) |= BaseActiveMask(i);
                  mLoadedObjs.Append(i);
               }
            }
         }
      }

      delete [] tempArray;

      file->CloseBlock();
   }
   return S_OK;
}

////////////////////////////////////////

//
// Pretend all transient objects don't exist
//

static void hide_transients(IBoolProperty* Transient)
{
   sPropertyObjIter iter;
   ObjID obj;
   BOOL trans;

   Transient->IterStart(&iter);
   while (Transient->IterNextValue(&iter,&obj,&trans))
      if (trans)
      {
         BaseActiveElem(obj) &= ~BaseActiveMask(obj);
      }
   Transient->IterStop(&iter);

}

static void unhide_transients(IBoolProperty* Transient)
{
   sPropertyObjIter iter;
   ObjID obj;
   BOOL trans;

   Transient->IterStart(&iter);
   while (Transient->IterNextValue(&iter,&obj,&trans))
      if (trans)
      {
         BaseActiveElem(obj) |= BaseActiveMask(obj);
      }
   Transient->IterStop(&iter);

}

////////////////////////////////////////

static ITagFile* tagfile = NULL;
static BOOL movefunc(void* buf, int len)
{
   return ITagFile_Move(tagfile,(char*)buf,len) == len;
}

STDMETHODIMP cObjectSystem::DatabaseNotify(eObjNotifyMsg msg, ObjNotifyData raw)
{
   uObjNotifyData data;
   data.raw = raw;

   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifyReset:
      {
         mActiveObjs.DestroyAll();
         mFreeObjs.DestroyAll();
         ClearObjMappingTable();
         memset(BaseActiveArray,0,ARRAY_SIZE());
         gObjCount = 0;
         ResetObjTimerStats();

   
      }
      break;

      case kObjNotifySave:
         if (msg & kObjPartConcrete)
            SaveObjMappingTable(data.db.save);

         if (!config_is_defined("save_transients"))
            hide_transients(Transient);

         SaveActiveArray(data.db.save, msg);
         break;

      case kObjNotifyLoad:
            if (msg & kObjPartConcrete)
               LoadObjMappingTable(data.db.load);
            LoadActiveArray(data.db.load, msg);
            // scan for Bottom Obj
            if (msg & kObjPartAbstract)
            {
               BaseActiveElem(ROOT_ARCHETYPE) |= BaseActiveMask(ROOT_ARCHETYPE);
               for(ObjID o = ROOT_ARCHETYPE; o >= gMinObjID; o--)
                  if (ObjExists(o))
                     BottomObj = o;
            }
            build_obj_lists();

            // Have the object list ready now
         break;

      case kObjNotifyDefault:
         BottomObj = ROOT_ARCHETYPE;
         TopObj = 0;
         assert(BaseActiveArray != nullptr);
         BaseActiveElem(ROOT_ARCHETYPE) |= BaseActiveMask(ROOT_ARCHETYPE);
         BaseActiveElem(OBJ_NULL) &= ~BaseActiveMask(OBJ_NULL);
         build_obj_lists();
         break;

      case kObjNotifyPostLoad:
         BaseActiveElem(OBJ_NULL) &= ~BaseActiveMask(OBJ_NULL);
         break;
   }

   // Notify the trait manager NOW, so that
   // the root archetype is correctly set up.
   if (NOTIFY_MSG(msg) == kObjNotifyDefault)
      TraitMan->Notify(msg,raw);
   LinkMan->Notify(msg,raw);
   PropMan->Notify(msg,raw);
   if (NOTIFY_MSG(msg) != kObjNotifyDefault)
      TraitMan->Notify(msg,raw);

   switch (NOTIFY_MSG(msg))
   {
      case kObjNotifyPostLoad:
      {
         eObjPartition part = NOTIFY_PARTITION(msg);
         send_post_load(part);
         if (part == 0)
            ResetObjTimerStats();
      }
      break;

      case kObjNotifySave:
         if (!config_is_defined("save_transients"))
            unhide_transients(Transient);
         break;
   }
   return S_OK;
}

////////////////////////////////////////////////////////////
// Custom Object Queries
//

class cAbstractObjQuery : public cBaseObjectQuery
{
   ObjID LastObj;
   ObjID NextObj;
public:
   cAbstractObjQuery(ObjID last = BottomObj) :NextObj(-1),LastObj(last) {};

   STDMETHOD_(BOOL,Done)() { return NextObj < LastObj;}
   STDMETHOD_(ObjID,Object)() { return NextObj;};
   STDMETHOD(Next)()
   {
      for(NextObj--; !Done(); NextObj--)
         if (ObjExists(NextObj))
            break;
      return S_OK;
   }
};

////////////////////////////////////////

class cConcreteObjQuery : public cBaseObjectQuery
{
   cObjList::cIter Iter;
public:
   cConcreteObjQuery(const cObjList::cIter& iter)
      : Iter(iter)
   {};

   STDMETHOD_(BOOL,Done)() { return Iter.Done();}
   STDMETHOD_(ObjID,Object)() { return Iter.Value();};
   STDMETHOD(Next)()
   {
      Iter.Next();
      return S_OK;
   }
};

////////////////////////////////////////

class cAllObjQuery : public cBaseObjectQuery
{
   IObjectQuery* Query;
   BOOL do_abstract;
   void Skip()
   {
      if (Query->Done() && do_abstract)
      {
         // Next query
         SafeRelease(Query);
         do_abstract = FALSE;
         Query = new cAbstractObjQuery();
      }
   }

public:
   cAllObjQuery(const cObjList::cIter& iter)
      : do_abstract(TRUE), Query(new cConcreteObjQuery(iter))
   {
      Skip();
   }

   virtual ~cAllObjQuery() { SafeRelease(Query); };

   STDMETHOD_(BOOL,Done)() { return !do_abstract && Query->Done();}
   STDMETHOD_(ObjID,Object)() { return Query->Object();};
   STDMETHOD(Next)()
   {
      Query->Next();
      Skip();

      return S_OK;
   }

};

////////////////////////////////////////////////////////////

STDMETHODIMP_(IObjectQuery*) cObjectSystem::Iter(eObjConcreteness which)
{
   switch (which)
   {
      case kObjectConcrete:
         return new cConcreteObjQuery(mActiveObjs.Iter());

      case kObjectAbstract:
         return new cAbstractObjQuery;

      case kObjectAll:
         return new cAllObjQuery(mActiveObjs.Iter());
   }
   return CreateEmptyObjectQuery();
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cObjectSystem::Exists(ObjID obj)
{
   if (obj < gMinObjID || obj >= gMaxObjID)
      return FALSE;

   return ObjExists(obj);

  if (!ObjExists(obj))
     return FALSE;

  return TRUE;

}

////////////////////////////////////////

STDMETHODIMP_(ObjID) cObjectSystem::MaxObjID()
{
   return gMaxObjID;
}

STDMETHODIMP_(ObjID) cObjectSystem::MinObjID()
{
   return gMinObjID;
}

STDMETHODIMP_(int) cObjectSystem::ActiveObjects()
{
   return gObjCount;
}

//------------------------------------------------------------
// INTERNALS
//


void cObjectSystem::notify_obj(ObjID obj, ulong msg)
{
   BOOL forward = msg != kObjNotifyDelete;

   if (forward)
   {
      PropMan->Notify(msg,(ObjNotifyData)obj);
      LinkMan->Notify(msg,(ObjNotifyData)obj);
      TraitMan->Notify(msg,(ObjNotifyData)obj);
   }

   for (int i = 0; i < mListeners.Size(); i++)
   {
      sObjListenerDesc& listen = mListeners[i];
      if (listen.func)
         listen.func(obj,msg,listen.data);
   }

   if (!forward)
   {
      PropMan->Notify(msg,(ObjNotifyData)obj);
      LinkMan->Notify(msg,(ObjNotifyData)obj);
      TraitMan->Notify(msg,(ObjNotifyData)obj);
   }
}

void cObjectSystem::notify_obj_all(eObjConcreteness which, ulong msg)
{
   IObjectQuery* query = Iter(which);
   for (; !query->Done(); query->Next())
   {
      notify_obj(query->Object(), msg);
   }
   SafeRelease(query);
}

void cObjectSystem::send_post_load(eObjPartition part)
{
   // @TODO: if we have abstract partitions, we need to put them in mLoadedObjs
   if (part & kObjPartAbstract)
      notify_obj_all(kObjectAbstract,kObjNotifyLoadObj);
   if (part & kObjPartConcrete)
   {
      cObjList::cIter iter;
      for (iter = mLoadedObjs.Iter(); !iter.Done(); iter.Next())
      {
         notify_obj(iter.Value(), kObjNotifyLoadObj);
         mLoadedObjs.Delete(iter.Node());
      }
   }
}


////////////////////////////////////////

tResult LGAPI ObjectSystemCreate(void)
{
   IUnknown* pOuter = AppGetObj(IUnknown);
   cObjectSystem* sys = new cObjectSystem(pOuter);
   if (!sys) return E_FAIL; 
   new cObjIDMan(pOuter,sys); 
   return S_OK; 
}

////////////////////////////////////////////////////////////
// Transience
//

STDMETHODIMP cObjectSystem::SetObjTransience(ObjID obj, BOOL tr)
{
   if (tr)
      Transient->Set(obj,tr);
   else
      Transient->Delete(obj);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cObjectSystem::ObjIsTransient(ObjID obj)
{
   BOOL result = FALSE;
   Transient->Get(obj,&result);
   return result;
}

////////////////////////////////////////////////////////////
// Immobility
//

STDMETHODIMP cObjectSystem::SetObjImmobility(ObjID obj, BOOL im)
{
   if (im)
      Immobile->Set(obj, im);
   else
      Immobile->Delete(obj);
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(BOOL) cObjectSystem::ObjIsImmobile(ObjID obj)
{
   BOOL result = FALSE;
   Immobile->Get(obj,&result);
   return result;
}

////////////////////////////////////////////////////////////
// SYMNAME STUFF
//

static IInvStringProperty* SymNameProp(IPropertyManager* PropMan)
{
   IInvStringProperty* symname;
   IProperty* prop = PropMan->GetPropertyNamed(PROP_SYMNAME_NAME);
   Verify(SUCCEEDED(prop->QueryInterface(IID_IInvStringProperty,(void**)&symname)));
   SafeRelease(prop);
   return symname;
}

STDMETHODIMP cObjectSystem::NameObject(ObjID obj, const char* name)
{
   if (SymName == NULL) SymName = SymNameProp(PropMan);
   if (name != NULL && name[0] != '\0')
      SymName->Set(obj,name);
   else
      SymName->Delete(obj);
   return S_OK;
}

STDMETHODIMP_(const char*) cObjectSystem::GetName(ObjID obj)
{
   if (SymName == NULL) SymName = SymNameProp(PropMan);
   const char* str = NULL;
   SymName->Get(obj,&str);
   return str;
}

STDMETHODIMP_(ObjID) cObjectSystem::GetObjectNamed(const char* name)
{
   if (SymName == NULL) SymName = SymNameProp(PropMan);
   ObjID obj = OBJ_NULL;
   SymName->GetObj(name,&obj);
   return obj;
}

////////////////////////////////////////

STDMETHODIMP_(IProperty*) cObjectSystem::GetCoreProperty(eObjSysProperty which)
{
   IProperty* result = NULL;
   switch (which)
   {
      case kSymNameProperty:
         result = SymName;
         if (result == NULL)
            SymName = SymNameProp(PropMan);
         break;
      case kTransienceProperty:
         result = Transient;
         break;
      case kImmobilityProperty:
         result = Immobile;
         break;
   }
   if (result)
      result->AddRef();
   else
      result = PropMan->GetProperty(PROPID_NULL);
   return result;
}

////////////////////////////////////////

STDMETHODIMP cObjectSystem::PurgeObjects()
{
   return S_OK;
}


////////////////////////////////////////////////////////////
// LISTENERS
//

STDMETHODIMP_(tObjListenerHandle) cObjectSystem::Listen(sObjListenerDesc* desc)
{
   Assert_(desc->func);
   return mListeners.Append(*desc);
}

STDMETHODIMP cObjectSystem::Unlisten(tObjListenerHandle handle)
{
   mListeners[handle].func = NULL;

   // shrink listeners if possible
   int newsize = mListeners.Size();
   while (newsize-1 >= 0 && mListeners[newsize-1].func == NULL)
      newsize--;
   mListeners.SetSize(newsize);

   return S_OK;
}

////////////////////////////////////////////////////////////
// PARTITIONING/REMAPPING
//

STDMETHODIMP_(BOOL) cObjectSystem::IsObjSavePartition(ObjID obj, eObjPartition part)
{
   if (!Exists(obj) || ObjIsTransient(obj))
      return FALSE;

   eObjPartition subpart_mask = 0;
   if (OBJ_IS_ABSTRACT(obj))
   {
      if (!(part & kObjPartAbstract))
         return FALSE;
      subpart_mask = kObjAbstractSubparts;
   }
   else if (OBJ_IS_CONCRETE(obj))
   {
      if (!(part & kObjPartConcrete))
         return FALSE;
      subpart_mask = kObjConcreteSubparts;
   }

   // sub-partition logic

   eObjPartition subpart = part & subpart_mask; // we only want sub-partitions

   // if we didn't specify a sub-partition, then we have already verified
   // that we belong to the major partition
   if (subpart == 0) return TRUE;

   for (int i = 0; i < kNumSubPartitions; i++)
   {
      sFilter& filter = mSubPartFilters[i];
      if (filter.func != NULL)
         if (filter.func(obj,&filter)) // does this subpartition think we belong.
            return (subpart & (kObjPartBase << i)) != 0;
   }


   return FALSE;
}





STDMETHODIMP_(BOOL) cObjectSystem::IsObjLoadPartition(ObjID obj, eObjPartition part)
{
   if (!Exists(obj))
      return FALSE;

   if (OBJ_IS_ABSTRACT(obj) && !(part & kObjPartAbstract))
      return FALSE;

   if (OBJ_IS_CONCRETE(obj) && !(part & kObjPartConcrete))
      return FALSE;

   // (No sub-partition logic; if it's in the file, you can load it)

   return TRUE;
}

STDMETHODIMP_(eObjPartition) cObjectSystem::ObjDefaultPartition(ObjID obj)
{
   eObjPartition part = 0;
   eObjPartition subpart_mask = 0;

   if (OBJ_IS_ABSTRACT(obj))
   {
      part |= kObjPartAbstract;
      subpart_mask = kObjAbstractSubparts;
   }
   else if (OBJ_IS_CONCRETE(obj))
   {
      part |= kObjPartConcrete;
      subpart_mask = kObjConcreteSubparts;
   }

   // sub-partition logic

   if (subpart_mask == 0)
      return part;

   for (int i = 0; i < kNumSubPartitions; i++)
   {
      eObjPartition subpart = kObjPartBase << i;
      if (subpart_mask & subpart)
      {
         sFilter& filter = mSubPartFilters[i];
         if (filter.func != NULL && filter.func(obj,&filter)) // does this subpartition think we belong.
         {
            part |= subpart;
            break;
         }
      }
   }

   return part;
}


STDMETHODIMP_(ObjID) cObjectSystem::RemapOnLoad(ObjID obj)
{
   return ObjRemapOnLoad(obj);
}

STDMETHODIMP cObjectSystem::SetSubPartitionFilter(eObjPartition subpart, const sObjPartitionFilter* filter)
{
   // mask out the major partitions
   subpart &= ~kObjParts;
   if (subpart == 0) return S_FALSE;
   for (int i = 0; i < kNumSubPartitions; i++)
      if (subpart & (kObjPartBase << i))
      {
         mSubPartFilters[i] = *filter;
      }
   return S_OK;
}

////////////////////////////////////////////////////////////
//
// IObjIDManager API
//

sObjBounds cObjectSystem::GetObjIDBounds()
{
   sObjBounds bounds = { gMinObjID, gMaxObjID}; 
   return bounds; 
}

tResult cObjectSystem::ResizeObjIDSpace(const sObjBounds& bounds)
{
   //
   // We can only shrink if there are no active objects the lost space
   //
   int i; 
   for (i = bounds.max; i < gMaxObjID; i++)
      if (ObjExists(i))
         return E_FAIL; 

   for (i = gMinObjID; i < bounds.min; i++)
      if (ObjExists(i))
         return E_FAIL; 

   if (bounds.min == gMinObjID && bounds.max == gMaxObjID)
      return S_FALSE; 

   Assert_(bounds.max <= HACK_MAX_OBJ); // enforce HACK_MAX_OBJ until the cavalry comes and drives it away. 

   //
   // Save off min and max
   //

   ObjID oldMax = gMaxObjID; 
   ObjID oldMin = gMinObjID; 
   
   gMaxObjID = bounds.max;
   gMinObjID = bounds.min;

   // round off to multiples of 8
   // max rounds up
   gMaxObjID += 7;
   gMaxObjID &= ~7;
   // min rounds down
   gMinObjID &= ~7;


   // Make a new active array
   ubyte* oldBase = BaseActiveArray; 
   BaseActiveArray = new ubyte[ARRAY_SIZE()]; 

   // clear the array
   memset(BaseActiveArray,0,ARRAY_SIZE()); 

   // copy the subrange that overlaps
   int start = (oldMin > gMinObjID) ? oldMin : gMinObjID; 
   int end   = (oldMax < gMaxObjID) ? oldMax : gMaxObjID; 

   if (start < end)
   {
      ubyte* newStart = &BaseActiveArray[(start-gMinObjID)/8];
      ubyte* oldStart = &oldBase[(start-oldMin)/8];
      int len = (end-start)/8; 

      memcpy(newStart,oldStart,len); 
   }

   delete [] oldBase; 

   // Rebuild the active/free list 
   build_obj_lists(); 

   // Now call our sinks
   sObjBounds newbounds = { gMinObjID, gMaxObjID };

   for (i = 0; i < m_Sinks.Size(); i++)
      m_Sinks[i]->OnObjIDSpaceResize(newbounds); 


   return S_OK; 


}; 

tResult cObjectSystem::Connect(IObjIDSink* pSink)
{
   if (!pSink)
      return S_FALSE; 

   for (int i = 0; i < m_Sinks.Size(); i++)
      if (m_Sinks[i] == pSink)
         return S_FALSE; 

   m_Sinks.Append(pSink); 
   pSink->AddRef(); 

   return S_OK; 

}

tResult cObjectSystem::Disconnect(IObjIDSink* pSink)
{
   if (!pSink)
      return S_FALSE; 

   for (int i = 0; i < m_Sinks.Size(); i++)
      if (m_Sinks[i] == pSink)
      {
         pSink->Release(); 
         m_Sinks.DeleteItem(i); 
         return S_OK; 
      }

   return S_FALSE; 
}; 

////////////////////////////////////////////////////////////
// CLASS: cObjIDMan
//

static sRelativeConstraint gIDManConstraints[] =
{
   { kConstrainAfter, &IID_IObjectSystem },
   { kNullConstraint},
};

cObjIDMan::cObjIDMan(IUnknown* pOuter, cObjectSystem* pObjSys)
   : m_pObjSys(pObjSys)
{
   ((IObjectSystem*)m_pObjSys)->AddRef(); 
   MI_INIT_AGGREGATION_1(pOuter,IObjIDManager,kPriorityNormal,gIDManConstraints);
}



////////////////////////////////////////////////////////////
// LEGACY API
//

ObjID BeginObjectCreate(ObjID archetype, eObjConcreteness concrete)
{
   AutoAppIPtr_(ObjectSystem,sys);
   return sys->BeginCreate(archetype,concrete);
}

void EndObjectCreate(ObjID obj)
{
   AutoAppIPtr_(ObjectSystem,sys);
   sys->EndCreate(obj);
}

BOOL ObjectExists(ObjID obj)
{
   AutoAppIPtr_(ObjectSystem,sys);
   return sys->Exists(obj);
}





