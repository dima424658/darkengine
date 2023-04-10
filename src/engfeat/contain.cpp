/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/contain.cpp,v 1.26 2000/01/13 13:17:37 adurant Exp $

// COM fun
#include <appagg.h>
#include <aggmemb.h>

#include <hashpp.h>
#include <hshpptem.h>
#include <mprintf.h>
#include <dlistsim.h>
#include <dlisttem.h>

// link stuff
#include <lnkbyobj.h>
#include <linkbase.h>
#include <linkint.h>
#include <relation.h>
#include <linkguid.h>
#include <bintrait.h>

// sdesc
#include <sdesc.h>
#include <sdesbase.h>
#include <propbase.h>

// object handling
#include <iobjsys.h>
#include <objdef.h>

#include <contain.h>
#include <combprop.h>

#include <config.h>
#include <cfgdbg.h>

// script messages
#include <contscrm.h>
#include <scrptapi.h>

// database messages
#include <dbasemsg.h>

#include <allocapi.h>

// Last, please
#include <dbmem.h>
#include <initguid.h>
#include <contiid.h>

//////////////////////////////
// internals

// creation of the relation itself

// query database - sorts links by data field field
class OrderedLinksByIntData : public LinksByObj
{
protected:
   int CompareLinks(LinkID l1, LinkID l2)
   {
      const int val1 = *(int*)LinkMan()->GetData(l1);
      const int val2 = *(int*)LinkMan()->GetData(l2);
      return val1-val2;
   }
};

#define _ContainGetQuery(iter) ((ILinkQuery *)(iter->dont_use))
#define _ContainSetQuery(iter,val) iter->dont_use=(void *)val

//////////////////////////////
// system initialization

static sRelationDesc     contRelDesc     = { "Contains", };
static sRelationDataDesc contRelDataDesc = { CONTAIN_SDESC_NAME , sizeof(int), kRelationDataAutoCreate };

static sFieldDesc contain_raw_fields = 
   { "ordinal", kFieldTypeInt, sizeof(int), 0, 0 };
static sStructDesc contain_raw_sdesc = 
   { CONTAIN_SDESC_NAME, sizeof(int), kStructFlagNone, 1, &contain_raw_fields };

static IRelation *pRelContains;
static ILinkQueryDatabase* pDBContains; 


ILabelProperty *gCombineTypeProp;
IIntProperty *gStackCountProp;

//////////////////////////////
// Our actual COM implementation!

static sRelativeConstraint ContainConstraints[] =
{
   { kConstrainAfter, &IID_ILinkManager},
   { kNullConstraint, }
};

class cContainSys : public cCTDelegating<IContainSys>,
                   public cCTAggregateMemberControl<kCTU_Default>
{

protected:

   struct sCBElem {
      pContainCallback cb;
      ContainCBData data;
   
      sCBElem(pContainCallback foo = NULL, ContainCBData d = NULL) : cb(foo),data(d) {};
      operator pContainCallback() { return cb; }
   };

   typedef cHashTableFunctions<ObjID> ObjIDHashFunctions;
   typedef cSimpleDList<sCBElem> cCBList; 
   typedef cHashTable<ObjID, cCBList*, ObjIDHashFunctions> cCBTable;

   cCBTable m_CBTable;

   STDMETHOD(Init)();
   STDMETHOD(End)(); 

   //
   // Helpers
   // 

   // Send a "container" and a "contained" script message
   void SendScriptMessage(ObjID container, ObjID containee, int event)
   {
      AutoAppIPtr(ScriptMan); 
      sContainerScrMsg msg(container,event,containee);
      pScriptMan->SendMessage(&msg); 

      sContainedScrMsg msg2(containee,event,container); 
      pScriptMan->SendMessage(&msg2); 

   }


   // get me the single contains link between two objects
   LinkID contains_link(ObjID container, ObjID containee)
   {
      return pRelContains->GetSingleLink(container,containee); 
   }

   // Obj Sys Listener

   void OnObjSysMsg(ObjID obj, eObjNotifyMsg msg)
   {
      switch (msg)
      {
         case kObjNotifyDelete:
         {
            cCBList* list = m_CBTable.Search(obj);
            if (list)
            {
               delete list;
               m_CBTable.Delete(obj); 
            }
         }
         break; 

      }
   }

   static void ObjSysListener(ObjID obj, eObjNotifyMsg msg, void* data)
   {
      cContainSys* us = (cContainSys*)data;
      us->OnObjSysMsg(obj,msg); 
   }

public:
   cContainSys(IUnknown* pOuter)
   {
      MI_INIT_AGGREGATION_1(pOuter, IContainSys, kPriorityNormal, ContainConstraints);
   }

   STDMETHOD(Listen)(ObjID obj, pContainCallback ContListen, ContainCBData data)
   {
      cCBList* list = m_CBTable.Search(obj); 
      if (list == NULL)
      {
         list = new cCBList; 
         m_CBTable.Set(obj,list);
      }

      list->Append(sCBElem(ContListen,data));

      return(S_OK);
   }

   // if we become cool and allow multiple listeners per obj, this should deal with all of them
   STDMETHOD_(BOOL,CheckCallback)(ObjID cbobj, eContainsEvent event, ObjID container, ObjID containee, eContainType type)
   {

      cCBList* list = m_CBTable.Search(cbobj);
      if (list != NULL)
         for (cCBList::cIter iter = list->Iter(); !iter.Done(); iter.Next())
         {
            const sCBElem& elem = iter.Value(); 
            if (!elem.cb(event,container,containee,type,elem.data))
               return FALSE; 
         }

      return(TRUE);
   }
   //////////////////////////////
   // Basic Add/Remove sort of stuff

   // the add to container call, if combine false, will not try to
   STDMETHOD(Add)(ObjID container, ObjID containee, eContainType type, uint flags)
   {
      ConfigSpew("contain_spew",("containsys->add %d to %d (type %d)\n",container,containee,type));

      // check for cycles!

      if (Contains(containee,container))
      {
         AssertMsg(FALSE,"about to create container link cycle!"); 
         return S_FALSE; 
      }

      // First things first, give our listener a chance to object
      if (!CheckCallback(container,kContainQueryAdd,container,containee,type))
         return(S_FALSE);
      if (!CheckCallback(OBJ_NULL,kContainQueryAdd,container,containee,type))
         return(S_FALSE);

      // remove the containee from whereever it may be 
      Remove(LINKOBJ_WILDCARD,containee); 

      // now, if checking combine then do it to it, baby
      if (flags & CTF_COMBINE)
      {
         // is there a combineable object in the inventory?
         // iterate through all of our links and try to combine on each   
         sContainIter *scip;
         // should get different iter based on flags for contain type matching?
         scip = IterStart(container);
         while (!scip->finished)
         {
            if (CombineTry(scip->containee,containee,type) == S_OK)
            {
               // we have succesfully combined, so stop the presses
               IterEnd(scip);
               return(S_OK);
            }
            IterNext(scip);
         }
         IterEnd(scip);
      }

      // check for existing containment, just to be defensive
      // now actually add the damn link
      pRelContains->AddFull(container,containee,&type);

      // listener will send message

      return(S_OK);
   }

   // Remove containee from container
   // returns whether successful (ie whether containee actually was in container)
   // note that doesn't care about stack count -- if you want to remove just one of a stack use CombineAdd
   // Note callbacks get called in the code that listens for the link destruction
   STDMETHOD(Remove)(ObjID container, ObjID containee)
   {
      HRESULT retval = S_FALSE; 
      // now go blast all links 
      ILinkQuery* query=pRelContains->Query(container,containee);
      for (; !query->Done(); query->Next())
      {

         pRelContains->Remove(query->ID()); 
#ifdef DBG_ON
         if (config_is_defined("contain_spew"))
         {
            sLink link;
            query->Link(&link); 
            mprintf("removing obj %d from container %d\n",link.dest,link.source); 
         }
#endif DBG_ON
         retval = S_OK; 
      }
      SafeRelease(query); 

      return retval; 
   }

   STDMETHOD(SetContainType)(ObjID container, ObjID containee, eContainType newctype)
   {
      LinkID id = contains_link(container,containee); 
      if (id != LINKID_NULL)
      {
         sLink thelink; 
         pRelContains->Get(id,&thelink); 

         // Changing contain type changes the sort order of our custom QDB, so we 
         // need to pull it out of the DB and re-insert it in its new place.
         pDBContains->Remove(id,&thelink); 
         pRelContains->SetData(id,&newctype);
         pDBContains->Add(id,&thelink); 
         return S_OK; 
      }
      return S_FALSE; 
  
   }
      
   // is containee in container 
   // returns type of containment or ECONTAIN_NULL if not contained
   STDMETHOD_(eContainType,IsHeld)(ObjID container, ObjID containee)
   {
      LinkID id = contains_link(container,containee); 
      if (id != LINKID_NULL)
         return *(eContainType*)pRelContains->GetData(id); 
      return ECONTAIN_NULL; 
   }

   // Is containee in container (recursive)
   STDMETHOD_(BOOL,Contains)(ObjID container, ObjID containee)
   {
      if (container == containee)
         return TRUE;

      ObjID bag = containee; 
      while (bag != OBJ_NULL)
      {
         bag = GetContainer(bag); 
         if (bag == OBJ_NULL) 
            return FALSE; 
         AssertMsg(bag != containee,"Container link cycle detected!"); 
         if (bag == container)   
            return TRUE; 
      }
      return FALSE; 
   }

   STDMETHOD_(ObjID,GetContainer)(ObjID obj)
   {
      LinkID id = contains_link(LINKOBJ_WILDCARD,obj);
      if (id == LINKID_NULL)
         return OBJ_NULL;
      
      sLink link;
      pRelContains->Get(id,&link); 
      return link.source; 
   }

   struct sContainData
   {
      eContainType type; 
      ObjID obj;

      sContainData(ObjID o = OBJ_NULL, eContainType t = 0) :type(t),obj(o) {};  
   };


   typedef cSimpleDList<sContainData> cContents; 

   STDMETHOD(MoveAllContents)(ObjID targ, ObjID src, int addflags) 
   {
      cContents contents; 

      // copy the contents into a linked list, so that deletions happen before adds,
      // so that listeners get called in the right order 
      
      ILinkQuery* query = pRelContains->Query(src,LINKOBJ_WILDCARD);
      for (; !query->Done(); query->Next())
      {
         LinkID id = query->ID(); 
         eContainType ctype = *(eContainType*)pRelContains->GetData(id);
         sLink link;
         query->Link(&link); 
         contents.Append(sContainData(link.dest,ctype)); 
         pRelContains->Remove(id); 
      }
      SafeRelease(query); 

      // at this point, removed links are GONE, we can add new ones safely
      for (cContents::cIter iter = contents.Iter(); !iter.Done(); iter.Next())
      {
         Add(targ,iter.Value().obj,iter.Value().type, addflags); 
      }
      
      return S_OK; 
   }

   //////////////////////////////
   // Containment-related functions

   STDMETHOD_(BOOL, CanCombine)(ObjID combinee, ObjID new_obj, eContainType type)
   {
      // Okay, do we have a matching CombineType property?
      if (gCombineTypeProp->IsRelevant(new_obj) 
          && gCombineTypeProp->IsRelevant(combinee))
      {
         // at least we both HAVE combine types, now do they match?
         Label *l1,*l2;
         gCombineTypeProp->Get(new_obj,&l1);
         gCombineTypeProp->Get(combinee,&l2);
         if (strcmp(l1->text,l2->text) == 0)
         {
            // Ask for additional permission from the base obj
            if (!CheckCallback(combinee,kContainQueryCombine,combinee,new_obj,type))
               return(FALSE);
            // ... and from the universe at large
            if (!CheckCallback(OBJ_NULL,kContainQueryCombine,combinee,new_obj,type))
               return(FALSE);

            // yay, we can do it!
            return(TRUE);
         }
      }
      return(FALSE);
   }

   STDMETHOD_(BOOL, CanCombineContainer)(ObjID container, ObjID new_obj, eContainType type)
   {
      sContainIter *scip;
      // should get different iter based on flags for contain type matching?
      scip = IterStart(container);
      while (!scip->finished)
      {
         if (CanCombine(scip->containee,new_obj, type))
         {
            // we have succesfully combined, so stop the presses
            IterEnd(scip);
            return(TRUE);
         }
         IterNext(scip);
      }
      IterEnd(scip);
      return(FALSE);
   }

   // for attempting to merge an object only (and going and doing it)
   STDMETHOD(CombineTry)(ObjID combinee, ObjID new_obj, int type)
   {
      if (CanCombine(combinee,new_obj, type ))
      {
         // a match!
         // if we have a count property, bump up the stack count on the original (combinee)
         int v1,v2;
         if (gStackCountProp->IsRelevant(new_obj) && gStackCountProp->IsRelevant(combinee))
         {
            gStackCountProp->Get(new_obj,&v1);
            gStackCountProp->Get(combinee,&v2);
            gStackCountProp->Set(combinee, v1 + v2);
         }

         // now inform the listeners while they still have a chance to 
         // look at both objs
         CheckCallback(combinee,kContainCombine,combinee,new_obj,type);
         CheckCallback(OBJ_NULL,kContainCombine,combinee,new_obj,type);
         
         AutoAppIPtr(ScriptMan); 

         // check for a container, and send a message
         LinkID id = contains_link(LINKOBJ_WILDCARD,combinee); 
         if (id != LINKID_NULL)
         {
            sLink link; 
            pRelContains->Get(id,&link); 
            SendScriptMessage(link.source,combinee,kContainCombine); 
         }

         // Send the combine message anyway 
         sCombineScrMsg msg(combinee,new_obj); 
         pScriptMan->SendMessage(&msg); 

         // now blow up the leftovers (new_obj)
         AutoAppIPtr_(ObjectSystem,pObjSys); 
         pObjSys->Destroy(new_obj); 
         return(S_OK);
      }
      else
         return(S_FALSE);
   }

   // How many items of combine type foo do I contain, in total?
   // ctype is the archetype of the obj you want to check into
   // (maybe should be string name of the CombineType?)
   // does this want a flags option for specifying a contain type value to search within?
   STDMETHOD_(int,CombineCount)(ObjID container, ObjID ctype)
   {
      int total,quan;
      Label *ctypename, *itername;
      sContainIter *scip;

      // if the type in question doesn't combine, return 0
      if (!gCombineTypeProp->IsRelevant(ctype))
         return(0);

      total = 0;
      gCombineTypeProp->Get(ctype,&ctypename);
      // now iterate over all of our contents and start adding up
      scip = IterStart(container);
      while (!scip->finished)
      {
         if (gCombineTypeProp->IsRelevant(scip->containee))
         {
            gCombineTypeProp->Get(scip->containee, &itername);
            if (strcmp(ctypename->text, itername->text) == 0)
            {
               gStackCountProp->Get(scip->containee, &quan);
               total = total + quan;
            }
         }
         IterNext(scip);
      }
      IterEnd(scip);
      return(total);
   }

   // accumulate / deduct to combineable
   // ctype is archetype of combineable obj
   // use negative quantity to deduct
   // returns whether or not it succeeded -- failure cases include nonexistant object of
   // that type (hmm, maybe should just instantiate one in inv? ick) or insufficient amount
   // to remove
   STDMETHOD(CombineAdd)(ObjID container, ObjID ctype, int quantity, ulong flags)
   {

      // if there is no containtype on the archetype, abort out
      if (!gCombineTypeProp->IsRelevant(ctype))
         return(S_FALSE);

      Label *ctypename;
      gCombineTypeProp->Get(ctype,&ctypename);

      // now iterate through all the objs, trying to get our balance to zero by adding or
      // subtracting from each thing that we get to (destroying if we subtract down to 0)
      // until we either get our balance to zero, or run out of things to iterate over
      int balance = quantity;
      sContainIter* scip = IterStart(container);
      for (; !scip->finished && balance != 0; IterNext(scip))
      {
         Label *itername;

         if (gCombineTypeProp->Get(scip->containee, &itername))
         {
            // figure out if we are of a matching containtype

            if (strcmp(itername->text, ctypename->text) == 0)
               balance -= StackAdd(scip->containee,balance,flags); 
         }
      }
      IterEnd(scip);
      if (balance == 0)
         return(S_OK);
      else
         return(S_FALSE); // maybe E_FAIL?
   }


   STDMETHOD_(ObjID,RemoveFromStack)(THIS_ ObjID stack, ulong flags, ulong how_many)
   {
      if (how_many <= 0)
      {  
         Warning(("Trying to remove %d objects from a stack\n",how_many)); 
         return OBJ_NULL; 
      }

      AutoAppIPtr_(ObjectSystem,pObjSys); 

      int count = 0; 
      if (gStackCountProp->Get(stack,&count))
      {

         // decrement the count 
         if (!(flags & kStackRemoveNoDecrement))
         {
            if (how_many >= count) 
            {
               if (flags & kStackRemoveLeaveEmpty)
                  // Don't allow them to take more than are there
                  how_many = count;
               else
                  // We're taking all of them.  Just return the whole stack
                  return stack; 
            }

            count -= how_many; 
            gStackCountProp->Set(stack,count); 
         }
         else
         {
            if (how_many >= count)
               how_many = count; 
         }

         // clone the damn object 
         ObjID clone = pObjSys->BeginCreate(stack,kObjectConcrete); 
         gStackCountProp->Set(clone,how_many); 
         pObjSys->EndCreate(clone); 
         return clone; 
      }
      else if (flags & kStackRemoveAllowInfinite) // clone the object 
         return pObjSys->Create(stack,kObjectConcrete); 
      
      return stack; 
   }

   STDMETHOD_(int,StackAdd)(ObjID stack, int how_many, ulong flags)
   {
      int count = 0; 
      if (gStackCountProp->Get(stack,&count))
      {
         if (count < 0)
            Warning(("Object %d has a stack count of %d\n",stack,count)); 

         int retval = how_many;

         // subtracting more than we have case
         if (-how_many >= count)
         {
            retval = -count; 
            count = 0; 
         }
         else
            count += how_many;

         if ((flags & kStackAddDestroyIfZero) && count == 0)
         {
            AutoAppIPtr_(ObjectSystem,pObjSys); 
            pObjSys->Destroy(stack); 
         }
         else
            gStackCountProp->Set(stack,count); 

         return retval; 
               
      }

      return (flags & kStackAddAllowInfinite) ? how_many : 0; 
   }

   // Return the "stack count" property
   STDMETHOD_(IIntProperty*,StackCountProp)()
   {
      gStackCountProp->AddRef(); 
      return gStackCountProp; 
   }

   // contain iter functions
   STDMETHOD_(sContainIter *,IterStart)(ObjID container)
   {
      return(IterStartType(container,IGNORE_TYPES,IGNORE_TYPES));
   }

   // setup an iteration from min->max on container
   STDMETHOD_(sContainIter *,IterStartType)(ObjID container, eContainType min_type, eContainType max_type)
   {
      sContainIter *pIter=new sContainIter; 
      ILinkQuery *painful=pRelContains->Query(container,LINKOBJ_WILDCARD);
      pIter->min=min_type;
      pIter->max=max_type;
      pIter->index=0;
      eContainType ect;
      for (; !painful->Done(); painful->Next())
      {
         ect = *(eContainType *)painful->Data();
         if ((pIter->min == IGNORE_TYPES) || (ect>=pIter->min))
            break;
      }
      if (painful->Done() || ((pIter->max != IGNORE_TYPES) && (ect>pIter->max)))
         pIter->finished=TRUE; // never found any valid candidates
      else
      {
         sLink link;
         painful->Link(&link);
         pIter->type=*(eContainType *)painful->Data();
         pIter->link_id=painful->ID();      
         pIter->containee=link.dest;
         pIter->index++;
         pIter->finished=FALSE;
      }
      _ContainSetQuery(pIter,painful);
      return pIter;
   }
 
   STDMETHOD_(sContainIter *,IterStartInherits)(ObjID container)
     {
       return(IterStartInheritsType(container,IGNORE_TYPES,IGNORE_TYPES));
     }

   STDMETHOD_(sContainIter *,IterStartInheritsType)(ObjID container, eContainType min_type, eContainType max_type)
     {
       sContainIter *pIter=new sContainIter;
       ILinkQuery *painful=QueryInheritedLinksSingleUncached(pRelContains,container,LINKOBJ_WILDCARD);
       pIter->min=min_type;
       pIter->max=max_type;
       pIter->index=0;
       eContainType ect;
       for (; !painful->Done(); painful->Next())
	 {
	   ect = *(eContainType *)painful->Data();
	   if ((pIter->min == IGNORE_TYPES) || (ect>=pIter->min))
	     break;
	 }
       if (painful->Done() || ((pIter->max != IGNORE_TYPES) && (ect>pIter->max)))
	 pIter->finished=TRUE; //never found any valid candidates
       else
	 {
	   sLink link;
	   painful->Link(&link);
	   pIter->type=*(eContainType *)painful->Data();
	   pIter->link_id=painful->ID();
	   pIter->containee=link.dest;
	   pIter->index++;
	   pIter->finished=FALSE;
	 }
       _ContainSetQuery(pIter,painful);
       return pIter;
     }

   // get next iter element
   STDMETHOD_(BOOL,IterNext)(sContainIter *pIter)
   {
      ILinkQuery *painful=_ContainGetQuery(pIter);
      if (!pIter->finished)
      {
         painful->Next();
         if (painful->Done() || ((pIter->max != IGNORE_TYPES) && ((eContainType)painful->Data()>pIter->max)))
            pIter->finished=TRUE;   // we are done
         else
         {
            sLink link;
            painful->Link(&link);
            pIter->type=*(eContainType*)painful->Data();
            pIter->link_id=painful->ID();      
            pIter->containee=link.dest;         
            pIter->index++;
            return TRUE;            // still more data to go
         } 
      }
      return FALSE;
   }

   // finish query process
   STDMETHOD_(void,IterEnd)(sContainIter *pIter) 
   {
      ILinkQuery *painful=_ContainGetQuery(pIter);
      //if (!pIter->finished) Warning(("Called ContainIterEnd before finished\n"));
      SafeRelease(painful);
      delete pIter; 
   }

   STDMETHOD(DatabaseMessage)(ulong msg, IUnknown* file)
   {
      switch(DB_MSG(msg))
      {
         case kDatabaseReset:
         {
            // delete all specific-object callbacks 
            for (cCBTable::cIter iter = m_CBTable.Iter(); !iter.Done(); iter.Next())
               if (iter.Key() != OBJ_NULL)
               {  
                  cCBList* list = iter.Value(); 
                  delete list;
                  m_CBTable.Delete(iter.Key()); 
               }     
         }
         break; 
      }

      return S_OK; 
   }

   void SendLinkMsg(eContainsEvent ev, const sLink& link, eContainType ctype)
   {
      CheckCallback(link.source,ev,link.source,link.dest,ctype);
      CheckCallback(OBJ_NULL,ev,link.source,link.dest,ctype);
      SendScriptMessage(link.source,link.dest,ev); 
   }

   void OnRelationMsg(const sRelationListenMsg* msg)
   {
      eContainType ctype = *(eContainType*)pRelContains->GetData(msg->id); 

      switch(msg->type)
      {
         case kListenLinkPostMortem:
            SendLinkMsg(kContainRemove,msg->link,ctype); 
            break;

         case kListenLinkBirth:
            SendLinkMsg(kContainAdd,msg->link,ctype); 
            break;
      }
   }


   static void LGAPI LinkListener(sRelationListenMsg* msg, RelationListenerData data)
   {
      ((cContainSys*)data)->OnRelationMsg(msg); 
   }

};

// creation func
void ContainSysCreate()
{
   AutoAppIPtr(Unknown); 
   new cContainSys(pUnknown); 
}

////////////////////////////////////////////////////////////
// AGGREGATE PROTOCOL
//

static sPropertyDesc CombineTypeDesc =
{
   "CombineType", 0, 
   NULL, 0, 0, // constraints, versions
   { "Engine Features", "Combine Type" },
};

static sPropertyDesc StackCountDesc =
{
   "StackCount", 0, 
   NULL, 0, 0, // constraints, versions
   { "Engine Features", "Stack Count" },
};


HRESULT cContainSys::Init()
{
   LGALLOC_AUTO_CREDIT(); 

   sCustomRelationDesc desc;
   memset(&desc,0,sizeof(desc));
   OrderedLinksByIntData *table = new OrderedLinksByIntData;
   ILinkQueryDatabase* fromdb = new cFromObjQueryDatabase(table,TRUE);
   desc.DBs[kRelationSourceKnown] = fromdb; 
   StructDescRegister(&contain_raw_sdesc);
   pRelContains = CreateCustomRelation(&contRelDesc,&contRelDataDesc,&desc);

   pDBContains = fromdb; 

   // build our properties
   gCombineTypeProp = CreateLabelProperty(&CombineTypeDesc, kPropertyImplDense);
   gStackCountProp = CreateIntProperty(&StackCountDesc, kPropertyImplDense);

   // set up the link-destruction listener
   pRelContains->Listen(kListenLinkBirth|kListenLinkPostMortem, LinkListener, this);

   // listen to obj sys changes 
   AutoAppIPtr_(ObjectSystem,pObjSys);
   sObjListenerDesc listener = { ObjSysListener, this}; 
   pObjSys->Listen(&listener); 

   return(S_OK);
}

HRESULT cContainSys::End()
{

   for (cCBTable::cIter iter = m_CBTable.Iter(); !iter.Done(); iter.Next())
   {  
      cCBList* list = iter.Value(); 
      delete list;
      m_CBTable.Delete(iter.Key()); 
   }     

   SafeRelease(pRelContains);
   SafeRelease(pDBContains); 
   return(S_OK);
}


