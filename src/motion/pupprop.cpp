/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/pupprop.cpp,v 1.5 1998/04/12 16:49:02 mahk Exp $

#include <pupprop.h>

#include <property.h>
#include <propert_.h>
#include <dataops_.h>
#include <propval_.h>
#include <proplist.h>
#include <propbase.h>

#include <objtype.h>
#include <osysbase.h>

// Include this last
#include <dbmem.h>

//
// puppet value funcs 
//

class cPuppetOps : public cClassDataOps<cPuppet> 
{
public:
   // override default flags
   cPuppetOps() : cClassDataOps<cPuppet>(kNoFlags) { }; 

   STDMETHOD(Read)(sDatum* dat, IDataOpsFile* , int )
   {
      dat->value = new cPuppet; 
      return S_OK; 
   }

};

static sPropertyDesc g_PuppetDesc =
{
   PROP_PUPPET_NAME, // name
   kPropertyConcrete|kPropertyNoInherit|kPropertyInstantiate, // flags
};

//
// Property store class 
//
class cPuppetStore : public cListPropertyStore<cPuppetOps> 
{
}; 

//
// Property class.  Because it's kPropertyConcrete, I need to use cGenericProperty
//

class cPuppetProp : public cGenericProperty<IPuppetProperty,&IID_IPuppetProperty,cPuppet*>
{
   typedef cGenericProperty<IPuppetProperty,&IID_IPuppetProperty,cPuppet*> cParent; 

protected:
   cSimpleDataOps mAbstractOps; 

public:

   cPuppetProp(const sPropertyDesc* desc, cPuppetStore* store)
      : cParent(desc, store)
   {
      // Ok, I know that cPuppetStore's SetOps doesn't do anything.  
      // But, because I'm kPropertyConcrete, I know that the 
      // mpStore is a "mixed" store with a different implementation for abstract objects.
      // I want abstract objects to always have NULL values, so I use cSimpleDataOps instead 
      // of cPuppetOps
      mpStore->SetOps(&mAbstractOps); 
   }

   STANDARD_DESCRIBE_TYPE(cPuppet); 

   
   //
   // Hookup a puppet to its ObjID. 
   //

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue value)
   {
      
      if (msg & (kListenPropModify|kListenPropSet))
      {
         cPuppet* puppet = (cPuppet*)value.ptrval; 
         if (puppet && obj != puppet->GetOwner())
            puppet->SetOwner(obj); 
      }
      cParent::OnListenMsg(msg,obj,value); 
   }
   
}; 


cPuppetProp *g_pPuppetProp=NULL;
cPuppetStore* g_pPuppetStore = NULL; 

IPuppetProperty *PuppetPropertyInit(void)
{
   if(g_pPuppetProp!=NULL)
      return g_pPuppetProp;
   g_pPuppetStore = new cPuppetStore(); 
   g_pPuppetProp= new cPuppetProp(&g_PuppetDesc,g_pPuppetStore);
   return g_pPuppetProp;
}


//
// We use our special knowledge of the puppet property store to iterate across only concrete puppets
//

void PuppetsUpdate(ulong dt)
{
   sPropertyObjIter iter;
   cPuppet* puppet;
   ObjID objID;

   if(!g_pPuppetStore)
      return;
   g_pPuppetStore->IterStart(&iter);
   while(g_pPuppetStore->IterNext(&iter,&objID,(sDatum*)&puppet))
   {
      if (puppet)
         puppet->Update(dt);
   }
   g_pPuppetStore->IterStop(&iter); 
}



