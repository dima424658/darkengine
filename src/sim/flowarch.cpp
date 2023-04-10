/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/flowarch.cpp,v 1.6 1998/09/23 15:43:24 mahk Exp $
#include <flowarch.h>
#include <port.h>

#include <propbase.h>
#include <propert_.h>
#include <dataops_.h>
#include <propsprs.h>

#include <dynarray.h>

#include <iobjsys.h>
#include <objnotif.h>
#include <objdef.h>
#include <editobj.h>
#include <dbasemsg.h>

#include <appagg.h>
#include <traitman.h>

#include <rendprop.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <flowprid.h>

////////////////////////////////////////////////////////////
// Flow Group Property 
//

class cFlowStore : public cSparseHashPropertyStore<cSimpleDataOps>
{
}; 


typedef cSpecificProperty<IFlowGroupProperty,&IID_IFlowGroupProperty,int,cFlowStore> cBaseFlowProp; 


class cFlowProp : public cBaseFlowProp
{
   static sPropertyDesc gDesc; 
public:
   cFlowProp() : cBaseFlowProp(&gDesc),
      mpObjSys(AppGetObj(IObjectSystem))
   {
   };

   ~cFlowProp()
   {
      SafeRelease(mpObjSys); 
   }

   STANDARD_DESCRIBE_TYPE(int); 

protected:

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj, uPropListenerValue val)
   {
      int idx = val.intval; 
      Assert_(idx < MAX_CELL_MOTION); 
      if (msg & kListenPropUnset)
      {
         Assert_(mByIdx[idx] == obj); 
         mByIdx[idx] = OBJ_NULL; 
      }

      if (msg & kListenPropModify)
      {
         if (mByIdx[idx] != OBJ_NULL &&
             mByIdx[idx] != obj)
            Delete(mByIdx[idx]);  // get rid of the old one

         mByIdx[idx] = obj; 
      }
   }

   STDMETHOD_(void,Notify)(ePropertyNotifyMsg msg, PropNotifyData data)
   {
      cBaseFlowProp::Notify(msg,data); 

      ulong nmsg = NOTIFY_MSG(msg); 

      switch (nmsg)
      {
         case kObjNotifyReset:
         {
            mByIdx.SetSize(MAX_CELL_MOTION); 
            for (int i = 0; i < MAX_CELL_MOTION; i++)
               mByIdx[i] = OBJ_NULL; 
         }
         break; 

         case kObjNotifyPostLoad:
            if (!(msg & kFiletypeAll))  // the postest post-load we can get 
            {
#ifdef EDITOR
               // Create all flow archetypes
               // We don't do this in game because it's slow...
               for (int i = 0; i < wr_num_cells; i++)
                  if (wr_cell[i]->motion_index > 0)
                     GetObj(wr_cell[i]->motion_index); 
#endif 
            }
            if (!(msg & kObjPartAbstract))
               break; 
            // else fall through 

         case kObjNotifyDefault:
         {
            AutoAppIPtr_(TraitManager,pTraitMan); 
            pTraitMan->CreateBaseArchetype("Flow Group",&mFlowRoot); 
            // detach from tree 
            pTraitMan->SetArchetype(mFlowRoot,OBJ_NULL); 
            ObjSetHasRefs(mFlowRoot,FALSE); 
#ifdef EDITOR
            ObjSetCanHaveBrush(mFlowRoot,FALSE); 
#endif
            static const char* water_name = "Water Flow Group"; 
            mWaterRoot = mpObjSys->GetObjectNamed(water_name); 
            if (mWaterRoot == OBJ_NULL)
               mWaterRoot = pTraitMan->CreateArchetype(water_name,mFlowRoot);
         }
         break; 
            
      }
   }

   STDMETHOD_(ObjID, GetObj)(THIS_ int idx) 
   {
      Assert_(idx < MAX_CELL_MOTION); 

      if (idx <= 0)
         return OBJ_NULL; 

      ObjID obj = mByIdx[idx]; 

      // create one if necessary
      if (obj == OBJ_NULL)
      {
         // Most flow groups are water, so assume that...
         obj = mpObjSys->BeginCreate(mWaterRoot,kObjectConcrete); 
         char buf[20]; 
         sprintf(buf,"Flow Group %d",idx); 
         mpObjSys->NameObject(obj,buf); 
         Set(obj,idx);  // implicitly updates table 
         mpObjSys->EndCreate(obj); 
      }

      return obj; 
   }


   // Inverse map 
   cDynArray<ObjID> mByIdx; 
   // Flow group root archetype
   ObjID mFlowRoot; 
   ObjID mWaterRoot;

   // Objsys Interface
   IObjectSystem* mpObjSys; 
};

//
// Descriptor
//

sPropertyDesc cFlowProp::gDesc = 
{
   PROP_FLOW_GROUP_NAME,
   kPropertyNoInherit|kPropertyNoEdit,
   NULL, // constraints
   0, 0, // version 
   {
      "Renderer",
      "Flow Group", 
   }
}; 


////////////////////////////////////////
// Init/Term
//

void FlowGroupPropInit(void)
{
   cAutoIPtr<IFlowGroupProperty>(new cFlowProp);
}

void FlowGroupPropTerm(void)
{
}



