/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/arcmd.cpp,v 1.17 2000/02/19 12:27:38 toml Exp $

#include <wtypes.h>

#include <comtools.h>
#include <appagg.h>

#include <stimuli.h>
#include <stimsens.h>
#include <stimbase.h>

#include <receptro.h>
#include <reaction.h>
#include <stimul8r.h>

#include <ssrctype.h>
#include <ssrcbase.h>

#include <stimsrc.h>

#include <pg8rtype.h>
#include <pg8rbase.h>
#include <propag8n.h>
#include <propag8r.h>


#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>
#include <objedit.h>

#include <string.h>
#include <ctype.h>
#include <mprintf.h>

#include <command.h>
#include <status.h>

#include <aredit.h>
#include <iobjed.h>
#include <lnktrai_.h>
#include <linkman.h>
#include <relation.h>
#include <linkbase.h>

////////////////////////////////////////////////////////////
// ACT/REACT COMMANDS
//
// These are intended for debugging & to tide over until 
// real act/react editing happens
//


//------------------------------------------------------------
// Query Receptrons
//

struct sQueryTron 
{
   char object[32];
   char stimulus[32];
}; 

static sFieldDesc query_tron_fields[] = 
{
   { "Object", kFieldTypeString, FieldLocation(sQueryTron,object), },
   { "Stimulus", kFieldTypeString, FieldLocation(sQueryTron,stimulus), },
};

static sStructDesc query_tron_desc = StructDescBuild(sQueryTron,kStructFlagNone,query_tron_fields); 


static void query_tron()
{
   AutoAppIPtr_(Stimuli,pStimuli);

   // Set up initial values
   sQueryTron tron = { "", "Stimulus"};

   sStructEditorDesc editdesc = 
   {  
      "Query Receptron", 
   };

   IStructEditor* ed = CreateStructEditor(&editdesc,&query_tron_desc,&tron);

   if (ed->Go(kStructEdModal))
   {
      AutoAppIPtr_(StimSensors,pSensors);
      AutoAppIPtr_(Reactions,pReactions);
      sQueryTron* querytron = &tron;

      ObjID obj = EditGetObjNamed(querytron->object);
      ObjID stim = EditGetObjNamed(querytron->stimulus);

      IReceptronQuery* query = pSensors->QueryInheritedReceptrons(obj,stim);

      for (query->Start(); !query->Done(); query->Next())
      {
         const sReceptron& tron = *query->Receptron();
         sObjStimPair elems = query->Elems();
         mprintf("[ ");
         mprintf("Obj: %s ", ObjEditName(elems.obj));
         mprintf("Stim: %s ", ObjEditName(elems.stim));
         // mprintf does yucky floats
         char buf[80]; 
         sprintf(buf, "Min: %.2f Max: %.2f ", tron.trigger.min,tron.trigger.max);
         mprintf(buf);
         char* effectstr = "None";
         if (tron.effect.kind != REACTION_NULL)
         {
            effectstr = (char*)pReactions->DescribeReaction(tron.effect.kind)->name;
         }
         mprintf("Effect: %s ", effectstr);
         mprintf("]\n");
      }
      SafeRelease(query);
   }
   SafeRelease(ed);

}

//------------------------------------------------------------
// Stimulate
//

struct sStimulate
{
   char object[32];
   char stimulus[32];
   tStimLevel level;
}; 

static sFieldDesc stimulate_fields[] = 
{
   { "Object", kFieldTypeString, FieldLocation(sStimulate,object), },
   { "Stimulus", kFieldTypeString, FieldLocation(sStimulate,stimulus), },
   { "Intensity", kFieldTypeFloat, FieldLocation(sStimulate,level), },
};

static sStructDesc stimulate_desc = StructDescBuild(sStimulate,kStructFlagNone,stimulate_fields); 



static void stimulate()
{
   AutoAppIPtr_(Stimuli,pStimuli);

   // Set up initial values
   sStimulate _late = { "", "Stimulus", 1.0};


   sStructEditorDesc editdesc = 
   {  
      "Stimulate Object", 
   };

   IStructEditor* ed = CreateStructEditor(&editdesc,&stimulate_desc,&_late);

   if (ed->Go(kStructEdModal))
   {
      AutoAppIPtr_(Stimulator,pStimulate);
      AutoAppIPtr_(StimSensors,pSensors);
      
      sStimulate* late = &_late;

      ObjID obj = EditGetObjNamed(late->object);
      ObjID stim = EditGetObjNamed(late->stimulus);

      StimSensorID id = pSensors->LookupSensor(obj,stim); 
      if (id != SENSORID_NULL)
      {
         sStimEventData data = { stim, late->level, 0.0, id, 0, 0, kStimEventNoDefer };
         sStimEvent event(&data); 
         pStimulate->StimulateSensor(id,&event); 
         Status ("Stimulating...");
      }
      else
         Status ("No such sensor");
   }

   SafeRelease(ed);
}

//------------------------------------------------------------
// IMPORTED ACT/REACT EDITORS


#include <dynfunc.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static IActReactEditors* LGAPI import_woe(void)
{
   CriticalMsg("Could not load dialog!");
   return NULL;
}

DeclDynFunc_(IActReactEditors*, LGAPI, ExportActReactEditors, (void));
ImplDynFunc(ExportActReactEditors, "darkdlgs.dll", "_ExportActReactEditors@0", import_woe);

#define ImportEditors (DynFunc(ExportActReactEditors).GetProcAddress())

//------------------------------------------------------------
// Fancy add-source dialog
//


static void add_source(char* arg)
{
   sObjStimPair pair = { EditGetObjNamed(arg), OBJ_NULL};
   sStimSourceDesc desc;
   cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
   if (editors == NULL) 
      return;
   

   memset(&desc,0,sizeof(desc));
   if (editors->SourceDialog(kARDlgModal,&pair,&desc) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL 
          || &desc.propagator == PGATOR_NULL)
         return; 
      
      AutoAppIPtr_(StimSources,pSources);
      

      if (SUCCEEDED(pSources->AddSource(pair.obj,pair.stim,&desc)))
         Status ("Added Source");
      else
         Status ("Source Add Failed");      
   }
}

static void view_source(char* arg)
{
   int srcid;
   sscanf(arg,"%x",&srcid);

   cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
   if (editors == NULL) 
      return;



   AutoAppIPtr_(StimSources,pSources);

   sObjStimPair pair = pSources->GetSourceElems(srcid); 
   sStimSourceDesc desc;
   pSources->DescribeSource(srcid,&desc); 
   
   if (editors->SourceDialog(kARDlgModal,&pair,&desc) == S_OK)
   {

      

   }
}



//------------------------------------------------------------
// Fancy add-receptron dialog
//

static void add_tron(char* arg)
{
   sObjStimPair pair = { EditGetObjNamed(arg), OBJ_NULL};
   sReceptron tron;

   cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
   if (editors == NULL) 
      return;

   memset(&tron,0,sizeof(tron));   

   if (editors->ReceptronDialog(kARDlgModal,&pair,&tron) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL) 
         return; 
      
      AutoAppIPtr_(StimSensors,pSensors);
      

      if (SUCCEEDED(pSensors->AddReceptron(pair.obj,pair.stim,&tron)))
         Status ("Added Receptron");
      else
         Status ("Receptron Add Failed");      
   }
}

////////////////////////////////////////

static void edit_tron(char* arg)
{
   int tronid;
   sscanf(arg,"%x",&tronid);

   cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
   if (editors == NULL) 
      return;


      AutoAppIPtr_(StimSensors,pSensors);
   sObjStimPair pair = pSensors->GetReceptronElems(tronid);
   sReceptron tron;
   pSensors->GetReceptron(tronid,&tron); 

   if (editors->ReceptronDialog(kARDlgModal,&pair,&tron) == S_OK)
   {
      if (pair.obj == OBJ_NULL || pair.stim == OBJ_NULL) 
         return; 
      
      if (SUCCEEDED(pSensors->SetReceptron(tronid,&tron)))
         Status ("Added Receptron");
      else
         Status ("Receptron Add Failed");      
   }
}



////////////////////////////////////////

static void list_trons(char* arg)
{
   // blast trailing whitespace
   for (char* s= arg +strlen(arg); s >= arg && isspace(*s); s--)
      *s = '\0';

   sObjStimPair pair = { EditGetObjNamed(arg), OBJ_NULL};

   cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
   if (editors == NULL) 
      return;

   if (editors->ReceptronList(kARDlgModal,&pair) == S_OK)
   {

      

   }
}

////////////////////////////////////////

static void list_sources(char* arg)
{
   // blast trailing whitespace
   for (char* s= arg +strlen(arg); s >= arg && isspace(*s); s--)
      *s = '\0';

   sObjStimPair pair = { EditGetObjNamed(arg), OBJ_NULL};

   cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
   if (editors == NULL) 
      return;

   if (editors->SourceList(kARDlgModal,&pair) == S_OK)
   {

      

   }
}

////////////////////////////////////////////////////////////
// EDIT TRAITS FOR A/R
//

static sEditTraitDesc tron_tdesc = 
{
   kRelationTrait,
   {
      "Receptrons",
      "Receptrons",
      "Act/React",
   }
}; 

class cTronEditTrait : public cLinkEditTrait
{
   static RelationID GetRel()
   {
      AutoAppIPtr_(LinkManager,pLinkMan); 
      cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("Receptron") ); 
      return pRel->GetID(); 
   }

   void LinkDialog(ObjID obj)
   {
      sObjStimPair pair = { obj, OBJ_NULL }; 
      cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
      if (editors == NULL) 
         return;

      editors->ReceptronList(kARDlgModal,&pair); 
   }

public:
   cTronEditTrait() : cLinkEditTrait(GetRel(),&tron_tdesc)
   {
      strcpy(mDesc.strings.category,"Act/React"); 
      mCaps.flags |= kTraitCanAdd; 
   }

};

static sEditTraitDesc source_tdesc = 
{
   kRelationTrait,
   {
      "arSrcDesc",
      "Sources",
      "Act/React",
   }
}; 

class cSourceEditTrait : public cLinkEditTrait
{
   static RelationID GetRel()
   {
      AutoAppIPtr_(LinkManager,pLinkMan); 
      cAutoIPtr<IRelation> pRel ( pLinkMan->GetRelationNamed("arSrcDesc") ); 
      return pRel->GetID(); 
   }



   void LinkDialog(ObjID obj)
   {
      sObjStimPair pair = { obj, OBJ_NULL }; 
      cAutoIPtr<IActReactEditors> editors ( ImportEditors() );
      if (editors == NULL) 
         return;

      editors->SourceList(kARDlgModal,&pair); 

   }

public:
   cSourceEditTrait() : cLinkEditTrait(GetRel(),&source_tdesc)
   {
      strcpy(mDesc.strings.category,"Act/React"); 
      mCaps.flags |= kTraitCanAdd; 
   }
};


////////////////////////////////////////////////////////////
// COMMAND INSTALLATION
//

static Command commands[] = 
{
   { "ar_receptron_add", FUNC_STRING, add_tron, "Add an act/react receptron", HK_EDITOR},
   { "ar_receptron_query", FUNC_VOID, query_tron, "Query a set of act/react receptron", HK_EDITOR},
   { "ar_source_add", FUNC_STRING, add_source, "Add an act/react source", HK_EDITOR },
   //   { "ar_source_view", FUNC_STRING, view_source, "View an act/react source", HK_EDITOR },
   //   { "ar_receptron_edit", FUNC_STRING, edit_tron, "Edit an act/react receptron", HK_EDITOR },

   { "ar_list_receptrons", FUNC_STRING, list_trons, "Edit an object's receptrons", HK_EDITOR}, 
   { "ar_list_sources", FUNC_STRING, list_sources, "Edit an object's sources", HK_EDITOR}, 


   { "ar_stimulate", FUNC_VOID, stimulate, "Test-stimulate an object", HK_EDITOR},

};

typedef cAutoIPtr<IEditTrait> ETP;

EXTERN void add_ar_commands(void)
{
   COMMANDS(commands,HK_ALL);

   AutoAppIPtr_(ObjEditors,pEdit);
   
   pEdit->AddTrait(ETP(new cTronEditTrait)); 
   pEdit->AddTrait(ETP(new cSourceEditTrait)); 

}


