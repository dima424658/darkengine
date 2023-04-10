/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/propscpt.cpp,v 1.20 1999/06/10 20:21:58 Justin Exp $

#include <string.h>

#include <lg.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <propscpt.h>

#include <property.h>
#include <propman.h>
#include <propbase.h>
#include <propstor.h>
#include <dataops.h>

#include <sdesbase.h>
#include <sdestool.h>
#include <sdesparm.h>

#include <propface.h>

#include <netmsg.h>

#include <mprintf.h>
#include <trait.h>

// must be last header
#include <dbmem.h>

//////////
//
// Networking code
//

// This is actually implemented at the bottom of this file:
void doSetField(ObjID obj, 
                const char *propname,
                const char *field,
                const cMultiParm &val,
                void *pPropService);

// The "Set Field" message, which is the heart of Set():
static sNetMsgDesc g_SetFieldDesc =
{
   kNMF_SendToObjOwner,
   "SetField",
   "Set Property Field",
   NULL,
   doSetField,
   {{kNMPT_ReceiverObjID, kNMPF_None, "Obj"},
    {kNMPT_String, kNMPF_None, "PropName"},
    {kNMPT_String, kNMPF_None, "Field"},
    {kNMPT_MultiParm, kNMPF_None, "Val"},
    {kNMPT_End}}
};

////////////////////////////////////////////////////////////
// PROPERTY SCRIPT SERVICE
//

#define HEY_OBJ_NULL(obj,retval) \
do if (obj == OBJ_NULL) \
{ \
   Warning(("Property script service was passed OBJ_NULL, line %d\n",__LINE__)); \
   return retval; \
} while (0)

#define NOTHING /**/

DECLARE_SCRIPT_SERVICE_IMPL(cPropertySrv, Property)
{
public:
   cPropertySrv()
   : m_pPropMan(NULL),m_pSdescTools(NULL),m_pSetFieldMsg(NULL)
   {
   }

   virtual ~cPropertySrv()
   {
   }

   STDMETHOD_(void, Init)()
   {
      m_pSetFieldMsg = new cNetMsg(&g_SetFieldDesc, this);
   }

   STDMETHOD_(void, End)()
   {
      delete m_pSetFieldMsg;
   }

   IPropertyManager* PropMan()
   {
      if (m_pPropMan == NULL)
      {
         m_pPropMan = AppGetObj(IPropertyManager);
      }
      return m_pPropMan;
   }

   IStructDescTools* SdescTools()
   {
      if (m_pSdescTools == NULL)
      {
         m_pSdescTools = AppGetObj(IStructDescTools);
      }
      return m_pSdescTools;
   }

   ObjID get_prop_donor(IProperty* prop, ObjID obj)
   {
      if (prop->IsSimplyRelevant(obj))
         return obj; 
      ITrait* trait; 
      if (SUCCEEDED(prop->QueryInterface(IID_ITrait,(void**)&trait)))
         return trait->GetDonor(obj); 
      return OBJ_NULL; 
   }

   cMultiParm GetPropField(IProperty* prop, ObjID obj, const sFieldDesc* field)
   {
      HEY_OBJ_NULL(obj,0);

      IPropertyStore* store;
      Verify(SUCCEEDED(prop->QueryInterface(IID_IPropertyStore,(void**)&store)));

      cMultiParm retval = 0;
      
      IDataOps* ops = store->GetOps();
      if (!ops) 
      {
         SafeRelease(store); 
         return 0; 
      }

      ObjID donor = get_prop_donor(prop,obj); 

      if (donor == OBJ_NULL)
         return 0; 

      sDatum dat; 
      if (store->GetCopy(donor,&dat))  // @TODO: break out an sdesc2multi ?
      {
         if (ops->BlockSize(dat) >= 0) // it's a pointer
            retval = GetParmFromField(field,dat.value); 
         else
            retval = GetParmFromField(field,&dat); 

         store->ReleaseCopy(obj,dat); 
      }

      SafeRelease(ops);
      SafeRelease(store); 

      return retval;

   }


   void SetPropField(IProperty* prop, ObjID obj, const sFieldDesc* field, const cMultiParm& val )
   {
      HEY_OBJ_NULL(obj,NOTHING);

      IPropertyStore* store;
      Verify(SUCCEEDED(prop->QueryInterface(IID_IPropertyStore,(void**)&store)));

      IDataOps* ops = store->GetOps();
      if (!ops) 
      {
         SafeRelease(store); 
         return;
      }
      
      sDatum dat; 


      BOOL got = store->GetCopy(obj,&dat);
      if (!got)
      {
         // Create the property
         prop->Create(obj); 
         got = store->GetCopy(obj,&dat); 
         if (!got) return; 
      }

      // figure out whether it's a pointer or not
      void* data = (ops->BlockSize(dat) >= 0) ? dat.value : &dat; 
      SetFieldFromParm(field,data,val);
      store->Set(obj,dat); 
      prop->Touch(obj); 

      if (got)
         store->ReleaseCopy(obj,dat);
      else
         ops->Delete(dat); 

      SafeRelease(ops);
      SafeRelease(store);
   }


   STDMETHOD_(cMultiParm, Get)(object sobj, const char * propname, const char* field)
   {
      ObjID obj = ScriptObjID(sobj); 
      HEY_OBJ_NULL(obj,0); 

      cAutoIPtr<IProperty> prop ( PropMan()->GetPropertyNamed(propname)); 

      if (prop->GetID() == PROPID_NULL)
      {
         Warning(("'%s' is not a property name\n", propname));
         return 0;
      }

      // Look up its sdesc
      const sPropertyTypeDesc* tdesc = prop->DescribeType();
      const sStructDesc* sdesc = SdescTools()->Lookup(tdesc->type);

      if (sdesc != NULL)
      {
         AssertMsg1(field != NULL || sdesc->nfields == 1,"Property %s has multiple fields",propname);

         if (field == NULL)
            return GetPropField(prop,obj,&sdesc->fields[0]);

         for (int i = 0; i < sdesc->nfields; i++)
         {
            if (stricmp(sdesc->fields[i].name,field) == 0)
               return GetPropField(prop,obj,&sdesc->fields[i]);
         }
      }
      else // @HACK: maybe it's a string property? 
      {
         IStringProperty* strprop; 
         if (SUCCEEDED(prop->QueryInterface(IID_IStringProperty,(void**)&strprop)))
         {
            const char* retval = NULL; 
            strprop->Get(obj,&retval); 
            SafeRelease(strprop);
            return retval; 
         }
      }


      return 0;
   }

   // @NOTE: Property.Set() *will* work across the network -- a script
   // on one object can change a property on another, even if that other
   // object resides on a different machine. However, beware raceway
   // conditions! If, for example, two machines Set() the property at
   // the same time, the end result isn't guaranteed. Similarly, you may
   // *not* count upon the Set() taking place immediately! If the object
   // is on another machine, it will take a long time (on the order of
   // hundreds of milliseconds) before it takes effect.
   STDMETHOD(Set)(object sobj, const char * prop, const cMultiParm & val)
   {
      ObjID obj = ScriptObjID(sobj); 
      Set(obj,prop,NULL,val);
      return S_OK;
   }

   STDMETHOD(Set)(object sobj, const char * propname, const char * field, const cMultiParm & val)
   {
      ObjID obj = ScriptObjID(sobj); 
      HEY_OBJ_NULL(obj,S_FALSE); 

      // This will call SetField on whichever machine owns the object:
      m_pSetFieldMsg->Send(OBJ_NULL, obj, propname, field, &val);

      return S_OK;
   }

   // SetLocal() says to set the property locally, even if it's on a
   // proxy object. It should usually only be used for ChangeLocally
   // properties.
   STDMETHOD(SetLocal)(object sobj, const char * propname, const char * field, const cMultiParm & val)
   {
      ObjID obj = ScriptObjID(sobj); 
      HEY_OBJ_NULL(obj,S_FALSE);

      SetField(obj, propname, field, val);
      return S_OK;
   }

   void SetField(ObjID obj, const char *propname, const char *field, const cMultiParm &val)
   {
      IProperty* prop = PropMan()->GetPropertyNamed(propname);

      if (prop->GetID() == PROPID_NULL)
      {
         Warning(("'%s' is not a property name\n", propname));
         return;
      }

      // Look up its sdesc
      const sPropertyTypeDesc* tdesc = prop->DescribeType();
      const sStructDesc* sdesc = SdescTools()->Lookup(tdesc->type);

      if (sdesc != NULL)
      {
         AssertMsg1(field != NULL || sdesc->nfields == 1,"Property %s has multiple fields",propname);

         if (field == NULL)
            SetPropField(prop,obj,&sdesc->fields[0],val);
         else
            for (int i = 0; i < sdesc->nfields; i++)
               if (stricmp(sdesc->fields[i].name,field) == 0)
               {
                  SetPropField(prop,obj,&sdesc->fields[i],val);
                  break;
               }

      }
      else if (field == NULL) // @HACK: maybe it's a string property? 
      {
         IStringProperty* strprop; 
         if (SUCCEEDED(prop->QueryInterface(IID_IStringProperty,(void**)&strprop)))
         {
            strprop->Set(obj,(const char*)val);
            SafeRelease(strprop);
         }
      }
   }

   STDMETHOD(Add)(object sobj, const char* propname)
   {
      ObjID obj = ScriptObjID(sobj); 
      HEY_OBJ_NULL(obj,S_FALSE); 

      IProperty* prop = PropMan()->GetPropertyNamed(propname);

      if (prop->GetID() == PROPID_NULL)
      {
         Warning(("'%s' is not a property name\n", propname));
         return E_FAIL;
      }

      prop->Create(obj);

      return S_OK;
   }

   STDMETHOD(Remove)(object sobj, const char* propname)
   {
      ObjID obj = ScriptObjID(sobj); 
      HEY_OBJ_NULL(obj,S_FALSE); 

      IProperty* prop = PropMan()->GetPropertyNamed(propname);
      if (prop->GetID() == PROPID_NULL)
      {
         Warning(("'%s' is not a property name\n", propname));
         return E_FAIL;
      }

      prop->Delete(obj);
      return S_OK;
   }

   STDMETHOD_(BOOL,Possessed)(object sobj, const char* propname)
   {
      ObjID obj = ScriptObjID(sobj); 
      HEY_OBJ_NULL(obj,FALSE); 

      IProperty* prop = PropMan()->GetPropertyNamed(propname);
      if (prop->GetID() == PROPID_NULL)
      {
         Warning(("'%s' is not a property name\n", propname));
         return FALSE;
      }

      return prop->IsRelevant(obj);
   }


   STDMETHOD(CopyFrom)(object starg, const char* propname, object ssrc)
   {
      ObjID targ = ScriptObjID(starg); 
      ObjID src = ScriptObjID(ssrc); 

      if (src == OBJ_NULL || targ == OBJ_NULL)
         return S_FALSE; 

      IProperty* prop = PropMan()->GetPropertyNamed(propname);
      if (prop->GetID() == PROPID_NULL)
      {
         Warning(("'%s' is not a property name\n", propname));
         return E_FAIL;
      }

      prop->Copy(targ,src);
      return S_OK;
   }

protected:
   IPropertyManager* m_pPropMan;
   IStructDescTools* m_pSdescTools;
   cNetMsg *m_pSetFieldMsg;

};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cPropertySrv, Property);

// A little more networking code:
static void doSetField(ObjID obj, 
                       const char *propname,
                       const char *field,
                       const cMultiParm &val,
                       void *pPropService)
{
   ((cPropertySrv *) pPropService)->SetField(obj, propname, field, val);
}
