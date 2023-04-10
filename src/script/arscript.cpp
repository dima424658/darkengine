/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/arscript.cpp,v 1.10 1999/03/03 17:38:00 mahk Exp $

#include <comtools.h>
#include <appagg.h>

#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <arscrs.h>
#include <arscrm.h>
#include <objremap.h>

#include <sdestool.h>
#include <sdesbase.h>
#include <sdesparm.h>

#include <stimtype.h>
#include <stimbase.h>
#include <stimuli.h>

#include <reaction.h>
#include <reacbase.h>
#include <simtime.h>

#include <stimul8r.h>
#include <stimsens.h>
#include <sensbase.h>
#include <receptro.h>

#include <stimsrc.h>
#include <ssrcbase.h>

#include <periog8r.h>
#include <pg8rbase.h>


#include <osysbase.h>

#include <lazyagg.h>

#include <string.h>

// must be last header
#include <dbmem.h>


//
// STIMULUS MESSAGE
//

#define kStimMsgVer 1 

IMPLEMENT_SCRMSG_PERSISTENT(sStimMsg)
{
   PersistenceHeader(sScrMsg,kStimMsgVer);

   PersistentObject(stimulus); 
   Persistent(intensity); 
   Persistent(sensor);
   Persistent(source); 
   return TRUE; 
}

LazyAggMember(IStimuli) pStimuli; 

const char* sStimMsg::message_name(stimulus_kind kind)
{
   static char buf[64];
   sprintf(buf,"%sStimulus",pStimuli->GetStimulusName(kind)); 
   return buf; 
}

sStimMsg::sStimMsg(ObjID toWhom, stimulus_kind stim, real i, integer sen, integer src)
   : sScrMsg(toWhom,message_name(stim)),
     stimulus(stim),
     intensity(i),
     sensor(sen),
     source(src)
{
   
}

sStimMsg::sStimMsg()
   : stimulus(0), intensity(0.0), sensor(0), source(0) 
{
}

////////////////////////////////////////////////////////////
// Act/React Service
//

LazyAggMember(IReactions) pReactions; 
LazyAggMember(IStructDescTools) pSdescTools; 
LazyAggMember(IStimulator) pStimulator; 
LazyAggMember(IStimSensors) pStimSensors; 
LazyAggMember(IStimSources) pStimSources; 



//
// Forward decls of 
// 

static void add_stim_message_receptron(ObjID obj, StimID stim);
static void rem_stim_message_receptron(ObjID obj, StimID stim);



DECLARE_SCRIPT_SERVICE_IMPL(cActReactSrv, ActReact)
{
protected:
   IPeriodicPropagator* mpGator; 

public: 

   STDMETHOD_(void,Init)()
      {
         sPropagatorDesc scriptdesc = { "Script", kPGatorAllStimuli }; 
         mpGator = CreatePeriodicPropagator(&scriptdesc,"ScriptContact"); 
      }

   STDMETHOD_(void,End)()
      {
         SafeRelease(mpGator); 
      }
   
   STDMETHOD(React)(reaction_kind what, real stim_intensity, 
                    object target, object agent,
                    const cMultiParm& parm1 = 0, 
                    const cMultiParm& parm2 = 0, 
                    const cMultiParm& parm3 = 0, 
                    const cMultiParm& parm4 = 0, 
                    const cMultiParm& parm5 = 0, 
                    const cMultiParm& parm6 = 0, 
                    const cMultiParm& parm7 = 0, 
                    const cMultiParm& parm8 = 0)
      {
         // Lookup the reaction 
         const sReactionDesc* desc = pReactions->DescribeReaction(what); 

         if (desc == NULL)
            return E_FAIL; 

         // Build the reaction event
         sStimEventData evdata = { what, stim_intensity, 0.0, 0, 0, GetSimTime() }; 
         sStimEvent stimevent(&evdata); 
         sStimTrigger trigger = { 0.0, 0.0, kStimTrigNoMin|kStimTrigNoMax }; 
         sReactionEvent event = { &stimevent, &trigger, target }; 

         sReactionParam param = { { target, agent}, }; 
         memset(&param.data,0,sizeof(param.data));

         const sStructDesc* sdesc = pSdescTools->Lookup(desc->param_type.name); 
         if (sdesc)
         {
            const cMultiParm* parms[] = { &parm1, &parm2, &parm3, &parm4, &parm5, &parm6, &parm7, &parm8 }; 

            FillStructFromParams(sdesc,&param.data,(cMultiParm**)parms,sizeof(parms)/sizeof(parms[0]));
         }

         pReactions->React(what,&event,&param); 

         return S_OK; 
         
      }

   STDMETHOD(Stimulate)(object who, stimulus_kind what, real how_much, object srcobj)
      {
         StimSensorID sensid = pStimSensors->LookupSensor(who,what); 

         if (sensid != SENSORID_NULL)
         {
            sStimEventData data = { what, how_much, 0.0, sensid, 0, GetSimTime() }; 
            sStimEvent event(&data); 

            if (srcobj != object(0))
            {
               sStimSourceDesc srcdesc;
               memset(&srcdesc,0,sizeof(srcdesc));

               srcdesc.propagator = PGATOR_NULL;
               srcdesc.level = event.intensity;

               event.source = pStimSources->AddSource(srcobj,event.kind,&srcdesc);
            }

            pStimulator->StimulateSensor(sensid,&event); 
            
            return S_OK;
         }

         return S_FALSE; 
      }

   STDMETHOD_(integer,GetReactionNamed)(const char* name) 
      {
         return pReactions->GetReactionNamed(name); 
      }

   STDMETHOD_(string,GetReactionName)(long id) 
      {
         return pReactions->DescribeReaction(id)->name; 
      }

   STDMETHOD(SubscribeToStimulus)(object obj, stimulus_kind what) 
      {
         BOOL valid = OBJ_IS_CONCRETE(obj) && OBJ_IS_ABSTRACT(what); 
         if (!valid)
            return E_FAIL; 
         add_stim_message_receptron(obj,what); 
         return S_OK; 
      }

   STDMETHOD(UnsubscribeToStimulus)(object obj, stimulus_kind what) 
      {
         BOOL valid = OBJ_IS_CONCRETE(obj) && OBJ_IS_ABSTRACT(what); 
         if (!valid)
            return E_FAIL; 
         rem_stim_message_receptron(obj,what); 
         return S_OK; 
      }

   STDMETHOD(BeginContact)(object source, object sensor)
      {  
         Assert_((int)source && (int)sensor); 
         return mpGator->BeginContact(source,sensor); 
      }

   STDMETHOD(EndContact)(object source, object sensor)
      {  
         Assert_((int)source && (int)sensor); 
         return mpGator->EndContact(source,sensor); 
      }

   STDMETHOD(SetSingleSensorContact)(object source, object sensor)
      {  
         Assert_((int)source && (int)sensor); 
         return mpGator->SetSingleSensorContact(source,sensor); 
      }

}; 


IMPLEMENT_SCRIPT_SERVICE_IMPL(cActReactSrv, ActReact);

////////////////////////////////////////////////////////////
// Script Message Reaction 
//

struct sStimMsgParam
{
   int count; // reference count, not actually used by reaction 
   ulong flags; 
   char pad[sizeof(sReactParamData) - sizeof(int) -sizeof(ulong)];
};

enum eStimMsgFlags
{
   kRefCount = 1 << 0,  
};

static eReactionResult LGAPI stim_msg_func(sReactionEvent* event, 
                                           const sReactionParam* , 
                                           tReactionFuncData data)
{
   IScriptMan* pScriptMan = (IScriptMan*)data; 
   sStimMsg msg(event->sensor_obj,event->stim->kind,event->stim->intensity,event->stim->sensor,event->stim->source);

   pScriptMan->SendMessage(&msg); 

   return kReactionNormal;
}

static sReactionDesc stim_msg_desc =
{
   "stim_script_msg",
   "Send to Scripts",
   REACTION_PARAM_TYPE(sStimMsgParam), 
};


static const char* flag_names[] = 
{
   "(Created By Script)",
};

#define NUM_FLAGS (sizeof(flag_names)/sizeof(flag_names[0]))

static sFieldDesc stim_msg_fields[] = 
{
   { "Ref Count", kFieldTypeInt, FieldLocation(sStimMsgParam,count), kFieldFlagNotEdit },
   { "Flags", kFieldTypeBits, FieldLocation(sStimMsgParam,flags), kFieldFlagNone, 0, NUM_FLAGS, NUM_FLAGS, flag_names },
};

static sStructDesc stim_msg_sdesc = StructDescBuild(sStimMsgParam,kStructFlagNone,stim_msg_fields);


static ReactionID create_stim_msg(IReactions* pReactions)
{
   pSdescTools->Register(&stim_msg_sdesc); 

   return pReactions->Add(&stim_msg_desc,stim_msg_func,AppGetObj(IScriptMan));
}

static ReactionID stim_msg_reaction = REACTION_NULL; 

EXTERN void InitScriptReactions()
{
   AutoAppIPtr(Reactions);
   stim_msg_reaction = create_stim_msg(pReactions); 
}

//------------------------------------------------------------
// Stim message receptron maintenance tools 
//

static void add_stim_message_receptron(ObjID obj, StimID stim)
{
   if (stim_msg_reaction == REACTION_NULL)
      return ;

   // check for an existing one. 
   cAutoIPtr<IReceptronQuery> query ( pStimSensors->QueryInheritedReceptrons(obj,stim));  
   for (query->Start(); !query->Done(); query->Next())
   {
      const sReceptron* tron = query->Receptron(); 
      if (tron->effect.kind == stim_msg_reaction)
      {
         // We found one, bump its refcount and go 
         sReceptron copytron = *tron; 
         sStimMsgParam* param = (sStimMsgParam*)&copytron.effect.param.data;
         if (param->flags & kRefCount)
         {
            param->count ++; // bump ref count
            pStimSensors->SetReceptron(query->ID(),&copytron); 
         }

         return; 
      }
      
   }

   sReceptron tron;
   memset(&tron,0,sizeof(tron)); 
   sStimMsgParam* param = (sStimMsgParam*)&tron.effect.param.data;
   tron.trigger.flags = kStimSensNoMin|kStimSensNoMax; 
   tron.effect.kind = stim_msg_reaction;
   param->count = 1; // initial refcount 
   param->flags = kRefCount; 

   pStimSensors->AddReceptron(obj,stim,&tron); 

}


static void rem_stim_message_receptron(ObjID obj, StimID stim)
{
   if (stim_msg_reaction == REACTION_NULL)
      return ;

   // check for an existing one. 
   cAutoIPtr<IReceptronQuery> query ( pStimSensors->QueryReceptrons(obj,stim) ); 
   for (query->Start(); !query->Done(); query->Next())
   {
      const sReceptron* tron = query->Receptron(); 
      if (tron->effect.kind == stim_msg_reaction)
      {
         // We found one, bump its refcount and go 
         sReceptron copytron = *tron; 
         sStimMsgParam* param = (sStimMsgParam*)&copytron.effect.param.data;
         if (param->flags & kRefCount)
         {
            param->count --; // decrement refcount
            if (param->count == 0)
               pStimSensors->RemoveReceptron(query->ID()); 
            else
               pStimSensors->SetReceptron(query->ID(),&copytron); 
         }
         // get out of here
         break; 
      }
   }

}




