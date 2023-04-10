/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/vocore.cpp,v 1.3 2000/02/02 15:18:05 adurant Exp $

#include <vocore.h>
#include <appagg.h>
#include <aggmemb.h>

#include <property.h>
#include <propbase.h>
#include <propfac_.h>

#include <schprop.h>
#include <schbase.h>
#include <schema.h>

#include <questapi.h>
#include <iobjsys.h>
#include <str.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <voguid.h>

////////////////////////////////////////////////////////////
// Voice over implementation classes

//------------------------------------------------------------
// cVoiceOverSys agg member implementation
//

class cVoiceOverSys: public cCTDelegating<IVoiceOverSys>,
                     public cCTAggregateMemberControl<kCTU_Default>
{
protected:
   static sRelativeConstraint gConstraints[]; 

public:
   cVoiceOverSys(IUnknown* pOuter)
      :mpInCombat(NULL),mPlaying(SCH_HANDLE_NULL)
   {
      MI_INIT_AGGREGATION_1(pOuter,IVoiceOverSys,kPriorityNormal,gConstraints); 
   }

   ~cVoiceOverSys()
   {
   }; 

protected:
   //----------------------------------------
   // Member vars
   //

   tVOTestFunc mpInCombat; 
   int mPlaying; 

   IObjectSystem* mpObjSys; 
   IQuestData* mpQuest;

   //----------------------------------------
   // Init/Term
   //

   STDMETHOD(Init)()
   {
      mpObjSys = AppGetObj(IObjectSystem); 
      mpQuest = AppGetObj(IQuestData); 
      return S_OK; 
   }

   STDMETHOD(Term)()
   {
      SafeRelease(mpObjSys); 
      SafeRelease(mpQuest); 
      return S_OK; 
   }

   //----------------------------------------
   // Schema callback 
   //
   void OnSchema(int handle, ObjID schema)
   {
      if (handle == mPlaying)
         mPlaying = SCH_HANDLE_NULL; 
   }


   static void SchemaCB(int hSchema, ObjID schemaID, void* pdata)
   {
      cVoiceOverSys* us = (cVoiceOverSys*)pdata; 
      us->OnSchema(hSchema,schemaID); 
   }

   inline cStr SchemaVarName(ObjID schemaID)
   {
      char buf[256]; 
      const char* name = mpObjSys->GetName(schemaID); 
      if (name)
         sprintf(buf,"SCHPLAY_%s",name); 
      else
         sprintf(buf,"SCHPLAYID_%d",schemaID); 

      return buf; 
   }

   //----------------------------------------
   // IVoiceOverSys Methods
   //
   
   STDMETHOD_(int,Play)(int schema)
   {
      if (mPlaying != SCH_HANDLE_NULL)
         return SCH_HANDLE_NULL; 

      sSchemaPlayParams* params = SchemaPlayParamsGet(schema); 

      //Don't crash if it doesn't exist.  
      if (!params) return SCH_HANDLE_NULL;

      if ((params->flags & SCH_PLAY_ONCE) && AlreadyPlayed(schema))
         return SCH_HANDLE_NULL; 

      if ((params->flags & SCH_NO_COMBAT) && mpInCombat && mpInCombat())
         return SCH_HANDLE_NULL; 

      sSchemaCallParams call = g_sDefaultSchemaCallParams; 
      call.flags |= SCH_SET_CALLBACK; 
      call.callback = SchemaCB; 
      call.pData = this; 

      mPlaying = SchemaIDPlay(schema,&call);  

      if (mPlaying != SCH_HANDLE_NULL)
         SetAlreadyPlayed(schema,TRUE); 

      return mPlaying; 
   }

   STDMETHOD_(BOOL,AlreadyPlayed)(int schema)
   {
      return mpQuest->Get(SchemaVarName(schema)); 
   }

   STDMETHOD(SetAlreadyPlayed)(int schema, BOOL played)
   {
      cStr var = SchemaVarName(schema); 
      if (played)
      {
         if (mpQuest->Exists(var))
            mpQuest->Set(var,TRUE); 
         else
            mpQuest->Create(var,TRUE,kQuestDataMission); 
      }
      else
         mpQuest->Delete(var);

      return S_OK; 
   }

   STDMETHOD(SetCombatTest)(tVOTestFunc func)
   {
      mpInCombat = func; 
      return S_OK; 
   }

}; 

F_DECLARE_INTERFACE(IPropertyManager); 


sRelativeConstraint cVoiceOverSys::gConstraints[] =
{
   { kConstrainAfter, &IID_IPropertyManager }, 
   { kNullConstraint },
};

void VoiceOverCreate()
{
   AutoAppIPtr(Unknown); 
   cAutoIPtr<IVoiceOverSys> sys(new cVoiceOverSys(pUnknown)); 
}



