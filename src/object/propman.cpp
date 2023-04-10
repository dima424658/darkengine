/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/propman.cpp,v 1.26 1999/06/16 18:35:17 mahk Exp $

#include <propbase.h>
#include <property.h>
#include <propman.h>
#include <propman_.h>
#include <hshsttem.h>
#include <propdb.h>
#include <config.h>
#include <cfgdbg.h>
#include <propnull.h>
#include <iobjed.h>
#include <propknow.h>
#include <tminit.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
//
// cPropertyManager
//
////////////////////////////////////////////////////////////

IMPLEMENT_AGGREGATION_SELF_DELETE(cPropertyManager);

//////////////////////////////
//
// Constructor
//

static sRelativeConstraint Constraints[] = 
{
   { kConstrainBefore, &IID_IObjEditors }, 
   { kNullConstraint }
};

cPropertyManager::cPropertyManager (IUnknown* pOuter) 
   : props_sorted_(FALSE),
     nullprop_(new cNullProperty)
{
   // get past index zero.
   props_.Append(nullprop_); 
   // Add ourselves to the app aggregate.
   INIT_AGGREGATION_1(pOuter,IID_IPropertyManager,this,
                   kPriorityNormal, Constraints);

}

//////////////////////////////
//
// Destructor
//
cPropertyManager::~cPropertyManager ()
{


}

////////////////////////////////////////////////////////////
// AGGREGATE PROTOCOL
//

HRESULT cPropertyManager::Init()
{
   tm_init();
   sPropertyDatabases::DB = new sPropertyDatabases;
   return S_OK;
}


////////////////////////////////////////

HRESULT cPropertyManager::End()
{
   for (int i = 1; i < props_.Size(); i++)
   {
      if (props_[i] != NULL)
      {
         IProperty* prop = props_[i]; 
         if (prop == nullprop_)
            continue;

         prop->AddRef();
         DelProperty(prop);

         int refs = prop->Release();
         if (refs > 0)
         {
            ConfigSpew("propref_spew", ("Property %s has %d refs on exit\n",prop->Describe()->name,refs));

            while(prop->Release() > 0)
               ; 
         }
      }
   }

   while (nullprop_->Release() > 0)
      ;

   delete sPropertyDatabases::DB;

   tm_close();
   return S_OK;
}

//////////////////////////////
//
// Common code for AddProperty and AddMetaProperty
//
BOOL cPropertyManager::AddPropertyBase (IProperty * prop)
{
   // Is a system with this name already being managed?
   const IProperty *there = name_hash_.Search (prop->Describe()->name);

   if (there)
   {
      char buf[80]; 
      sprintf(buf,"Duplicate Property %s\n", prop->Describe()->name);
      CriticalMsg(buf); 
      return FALSE;
   }

   name_hash_.Insert (prop);
   ((IProperty*)prop)->AddRef();

   return TRUE;
}

//////////////////////////////
//
// Add a PropertySystem to the list being managed, return success
// Stuffs a unique ID into *id
//

void sort_props(const PropArray& in, PropIDArray& out);

BOOL cPropertyManager::AddProperty (IProperty * prop,  PropertyID *id)
{  
   if (!AddPropertyBase (prop)) return FALSE;
   *id = (PropertyID) props_.Append(prop);

   props_sorted_ = FALSE;

   return TRUE;
}


//////////////////////////////
//
// Remove a Property from the list being managed, return success
//
BOOL cPropertyManager::DelProperty (IProperty *prop)
{
   const IProperty *deleted = name_hash_.Remove (prop);

   if (!deleted)
   {
      ConfigSpew ("propdel",("Tried to delete nonexistent IProperty %s\n", prop->Describe()->name));
      return FALSE;
   }
   props_[prop->GetID()] = nullprop_;
   nullprop_->AddRef();


   prop->Release();
   return TRUE;
}

//////////////////////////////
//
// Find the Property with a given name
//
IProperty*  cPropertyManager::ByName (const char *name) const
{
   IProperty *result_ptr = name_hash_.Search (name);

   if (result_ptr) 
      return result_ptr;
   else 
      return (IProperty*)nullprop_;
}

////////////////////////////////////////
// Get a Property by id
//

IProperty*  cPropertyManager::ByID (PropertyID id) const
{
   if (id >= 0 && id < props_.Size())
      return props_[id];
   else
      return (IProperty*)nullprop_;
}


////////////////////////////////////////////////////////////

STDMETHODIMP_(IProperty*)  cPropertyManager::GetPropertyNamed (const char *name) const
{
   IProperty* prop = ByName(name);
   prop->AddRef();
   return prop;
}


STDMETHODIMP_(IProperty*)  cPropertyManager::GetProperty (PropertyID id) const
{
   IProperty* prop = ByID(id);
   prop->AddRef();
   return prop;
}

////////////////////////////////////////////////////////////
// ITERATION
//

STDMETHODIMP cPropertyManager::BeginIter(sPropertyIter* iter)
{
   if (!props_sorted_)
      sort_props(props_,sort_order_);
   props_sorted_ = TRUE;

   iter->id = 0;
   return S_OK;
}

////////////////////////////////////////

STDMETHODIMP_(IProperty*) cPropertyManager::NextIter(sPropertyIter* iter)
{
   if (iter->id >= 0)
   {
      if (iter->id < sort_order_.Size())
      {
         IProperty* prop = props_[sort_order_[iter->id++]];
         prop->AddRef();
         return prop;
      }
      else
         iter->id = -1;
   }
   return NULL;
}

////////////////////////////////////////




STDMETHODIMP cPropertyManager::Notify(ePropertyNotifyMsg msg, PropNotifyData data)
{
   if (!props_sorted_)
      sort_props(props_,sort_order_);
   props_sorted_ = TRUE; 

   uint nmsg = NOTIFY_MSG(msg); 

   // On reset, recompute arrays
   if (nmsg == kObjNotifyReset)
   {
      begin_create_.SetSize(0); 
      end_create_.SetSize(0); 
      load_obj_.SetSize(0); 

      int size = sort_order_.Size(); 
      for (int i = 0; i < size; i++)
      {
         PropertyID id = sort_order_[i]; 
         IProperty* prop = props_[id]; 
         const sPropertyDesc* desc = prop->Describe(); 
         
         if (desc->flags & (kPropertyInstantiate|kPropertySendBeginCreate))
            begin_create_.Append(id); 
         if (desc->flags & kPropertySendEndCreate)
            end_create_.Append(id); 
         if (desc->flags & kPropertySendLoadObj)
            load_obj_.Append(id); 

      }
   }

   // select an array based on message type 
   // soon this should be a switch...
   PropIDArray& propids = (nmsg == kObjNotifyCreate)      ? end_create_     : 
                          (nmsg == kObjNotifyBeginCreate) ? begin_create_   :
                          (nmsg == kObjNotifyLoadObj)     ? load_obj_       :
                                                           sort_order_; 
   
   for (int i = 0; i < propids.Size(); i++)
   {
      props_[propids[i]]->Notify(msg,data); 
   }
   return S_OK;
}

////////////////////////////////////////

//------------------------------------------------------------
// Sorting by constraints
//

//
// What I want is lexical scoping, but a class will have to do
//

class ConstrainedPropertySorter : public cPropertyManagerKnower
{
   ubyte* Seen;  // array determining whether we have seen this property
   PropIDArray& Out;
   const PropArray& In;

   void ExpandNode(PropertyID id);
   void DoSort();

   ConstrainedPropertySorter(const PropArray& in, PropIDArray& out)
      : Seen(new ubyte[in.Size()]),
        Out(out),
        In(in)
   {
      Out.SetSize(0);
      memset(Seen,0,in.Size());
   }

   ~ConstrainedPropertySorter()
   {
      delete Seen;
   }

public:

   static void Sort(const PropArray& in, PropIDArray& out)
   {
      ConstrainedPropertySorter sorter(in,out);
      sorter.DoSort();
   }
   
   
   
};



void ConstrainedPropertySorter::ExpandNode(PropertyID id)
{
   if (Seen[id]) return;

   Seen[id] = TRUE;

   IPropertyManager* PropMan = GetManager();
   cAutoIPtr<IProperty> prop(PropMan->GetProperty(id));
   const sPropertyConstraint* constraints = prop->Describe()->constraints;

   // Make sure I come after the things I'm constrained to come after

   if (constraints)
   {
      const sPropertyConstraint* c;
      for (c = constraints; c->kind != kPropertyNullConstraint; c++)
         switch (c->kind)
         {
            case kPropertyRequires:
            {
               cAutoIPtr<IProperty> against(PropMan->GetPropertyNamed(c->against));
               PropertyID NextID = against->GetID();

               if (NextID == PROPID_NULL)
               {
                  ConfigSpew("prop_sort_spew",("Couldn't sort against %s\n",c->against)); 
                  continue; 
               }
               ExpandNode(against->GetID());
            }
            break;
         }
   }

   ConfigSpew("propsort",("Adding %s to property list\n",prop->Describe()->name));
   Out.Append(id);
}

void ConstrainedPropertySorter::DoSort()
{
   int i;
   for (i = 0; i < In.Size(); i++)
   {
      PropertyID id = In[i]->GetID();
      if (id != PROPID_NULL)
         ExpandNode(id);
   }
}

static void sort_props(const PropArray& in, PropIDArray& out)
{
   ConstrainedPropertySorter::Sort(in,out);
}

////////////////////////////////////////////////////////////
//
// cPropertyManagerKnower
//
////////////////////////////////////////////////////////////

// Here is the one true property manager.  However, it is bad karma to
// have everything referring to the correct global instance by hand,
// so if you want to get to it, inherit from cPropertyManagerKnower
// and then you'll get it through your property_manager_ member.

LazyAggMember(IPropertyManager) cPropertyManagerKnower::pPropMan;

//
// Creation function for the one true property manager
//

tResult LGAPI PropertyManagerCreate(void)
{
   IUnknown* outer = AppGetObj(IUnknown); 
   cPropertyManager* man = new cPropertyManager(outer);
   return (man != NULL) ? NOERROR : E_FAIL;
}
