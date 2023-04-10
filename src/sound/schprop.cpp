/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schprop.cpp,v 1.19 1999/04/24 15:47:56 Justin Exp $

#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>
#include <sdesc.h>
#include <sdestype.h>
#include <sdesbase.h>

#include <schbase.h>
#include <schprop.h>

#include <tag.h>
#include <tagbase.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////
// Schema play parameters property

// This listener is only being used to try to catch a particular
// error, in which the base schema's volume is set to 0x10040.
#ifndef SHIP
static void LGAPI PlayParamsListener(sPropertyListenMsg * msg,
                                     PropListenerData data)
{
   if (!msg->value.ptrval)
      return;

   sSchemaPlayParams *pParams = (sSchemaPlayParams *) msg->value.ptrval;

   // creation
   if (msg->type & kListenPropSet) {
      AssertMsg1(pParams->volume <= 0,
                 "Schema volume should be negative, but we have one at %d",
                 pParams->volume);
   }
}
#endif // ~SHIP

ISchemaPlayParamsProperty *g_schemaPlayParamsProperty;

static const char* flag_names[] = 
{
   "Retrigger",
   "Pan position",
   "Pan Range",
   "No Repeate",  
   "No Cache",
   "Stream",
   "Play Once",
   "No Combat", 
   "Net Ambient",
   "Local Spatial",
   "","",  // 6 unused bits
   "","","","",
   "Noise",
   "Speech",
   "Ambient",
   "Music",
   "MetaUI",
}; 

#define NUM_FLAGS (sizeof(flag_names)/sizeof(flag_names[0]))

static sFieldDesc SchemaPlayParamsFields[] = 
{
   {"Flags", kFieldTypeBits, FieldLocation(sSchemaPlayParams, flags), kFieldFlagNone, 0, NUM_FLAGS, NUM_FLAGS, flag_names},
   {"Volume", kFieldTypeInt, FieldLocation(sSchemaPlayParams, volume),},
   {"Initial Delay", kFieldTypeShort, FieldLocation(sSchemaPlayParams, initialDelay),},
   {"Pan", kFieldTypeInt, FieldLocation(sSchemaPlayParams, pan),},
   {"Fade", kFieldTypeInt, FieldLocation(sSchemaPlayParams, fade),},
};

static sStructDesc SchemaPlayParamsStructDesc = 
   StructDescBuild(sSchemaPlayParams, kStructFlagNone, SchemaPlayParamsFields);

class cSchemaPlayOps : public cClassDataOps<sSchemaPlayParams>
{
   
}; 

class cSchemaPlayStore: public cHashPropertyStore<cSchemaPlayOps>
{
}; 

class cSchemaPlayProp : public cSpecificProperty<ISchemaPlayParamsProperty,&IID_ISchemaPlayParamsProperty,sSchemaPlayParams*, cSchemaPlayStore>
{
   typedef cSpecificProperty<ISchemaPlayParamsProperty,&IID_ISchemaPlayParamsProperty,sSchemaPlayParams*, cSchemaPlayStore> cParent; 

public:
   cSchemaPlayProp(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sSchemaPlayParams); 

};


static sPropertyDesc schemaPlayParamsDesc = 
{
   PROP_SCHEMA_PLAY_PARAMS, 0, NULL, 0, 0, { "Schema", "Play Params" },
};

void SchemaPlayParamsPropertyInit()
{
   StructDescRegister(&SchemaPlayParamsStructDesc);

   g_schemaPlayParamsProperty
      = new cSchemaPlayProp(&schemaPlayParamsDesc);
#ifndef SHIP
   g_schemaPlayParamsProperty->Listen(kListenPropSet, 
                                      PlayParamsListener, NULL);
#endif // ~SHIP
}

sSchemaPlayParams *SchemaPlayParamsGet(ObjID objID)
{
   sSchemaPlayParams *pParams;

   if (g_schemaPlayParamsProperty->Get(objID, &pParams))
      return pParams;
   return NULL;
}

////////////////////////////////
// Schema loop parameters property

ISchemaLoopParamsProperty *g_schemaLoopParamsProperty;

static sFieldDesc SchemaLoopParamsFields[] = 
{
   {"Flags", kFieldTypeBits, FieldLocation(sSchemaLoopParams, flags),},
   {"Max Samples", kFieldTypeInt, FieldLocation(sSchemaLoopParams, maxSamples),},
   {"Interval Min", kFieldTypeInt, FieldLocation(sSchemaLoopParams, intervalMin),},
   {"Interval Max", kFieldTypeInt, FieldLocation(sSchemaLoopParams, intervalMax),},
};

static sStructDesc SchemaLoopParamsStructDesc = 
   StructDescBuild(sSchemaLoopParams, kStructFlagNone, SchemaLoopParamsFields);

class cSchLoopOps : public cClassDataOps<sSchemaLoopParams>
{
}; 

class cSchLoopStore : public cHashPropertyStore<cSchLoopOps>
{
   
};

class cSchemaLoopProp : public cSpecificProperty<ISchemaLoopParamsProperty,&IID_ISchemaLoopParamsProperty, sSchemaLoopParams*, cSchLoopStore>
{
   typedef cSpecificProperty<ISchemaLoopParamsProperty,&IID_ISchemaLoopParamsProperty, sSchemaLoopParams*, cSchLoopStore> cParent; 

public:
   cSchemaLoopProp(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }; 

   STANDARD_DESCRIBE_TYPE(sSchemaLoopParams); 

}; 

static sPropertyDesc schemaLoopParamsDesc = 
{
   PROP_SCHEMA_LOOP_PARAMS, 0, NULL, 0, 0, { "Schema", "Loop Params" },
};

void SchemaLoopParamsPropertyInit()
{
   StructDescRegister(&SchemaLoopParamsStructDesc);

   g_schemaLoopParamsProperty = new cSchemaLoopProp(&schemaLoopParamsDesc);
}

sSchemaLoopParams *SchemaLoopParamsGet(ObjID objID)
{
   sSchemaLoopParams *pParams;

   if (g_schemaLoopParamsProperty->Get(objID, &pParams))
      return pParams;
   return NULL;
}

////////////////////////////////
// Schema priority property
IIntProperty *g_pPropSchemaPriority;

static sPropertyDesc SchemaPriorityDesc = 
{
   PROP_SCHEMA_PRIORITY, 0, NULL, 0, 0, { "Schema", "Priority" },
};

#define SCHEMA_PRIORITY_IMPL kPropertyImplDense

void SchemaPriorityPropertyInit(void)
{
   g_pPropSchemaPriority = CreateIntProperty(&SchemaPriorityDesc, SCHEMA_PRIORITY_IMPL);
}

int SchemaPriorityGet(ObjID objID)
{
   int pri = SCH_PRIORITY_NORMAL;
   g_pPropSchemaPriority->Get(objID, &pri);
   return pri;
}

////////////////////////////////
// Schema messages 

ILabelProperty *g_pPropSchemaMessage;

static sPropertyDesc SchemaMessageDesc = 
{
   PROP_SCHEMA_MESSAGE, 0, NULL, 0, 0, { "Schema", "Message" },
};

void SchemaMessagePropertyInit(void)
{
   g_pPropSchemaMessage = CreateLabelProperty(&SchemaMessageDesc, kPropertyImplDense);
}

// get voice
Label *SchemaMessageGet(ObjID objID)
{
   Label *pMessage;

   if (g_pPropSchemaMessage->Get(objID, &pMessage))
      return pMessage;
   else
      return NULL;
}

////////////////////////////////
// Schema "action" sounds 

ILabelProperty *g_pPropSchemaActionSnd;

static sPropertyDesc SchemaActionSndDesc = 
{
   PROP_SCHEMA_ACTION_SND, 0, NULL, 0, 0, { "Schema", "Action" },
};

void SchemaActionSndPropertyInit(void)
{
   g_pPropSchemaActionSnd = CreateLabelProperty(&SchemaActionSndDesc, kPropertyImplDense);
}

Label *SchemaActionSndGet(ObjID objID)
{
   Label *pActionSnd;

   if (g_pPropSchemaActionSnd->Get(objID, &pActionSnd))
      return pActionSnd;
   else
      return NULL;
}

////////////////////////////////
// Schema attenuation factor
IFloatProperty *g_pPropSchemaAttFac;

static sPropertyDesc SchemaAttenuationDesc = 
{
   PROP_SCHEMA_ATTFAC, 0, NULL, 0, 0, { "Schema", "Attenuation Factor" },
};

#define SCHEMA_ATTFAC_IMPL kPropertyImplVerySparse

void SchemaAttFacPropertyInit(void)
{
   g_pPropSchemaAttFac = CreateFloatProperty(&SchemaAttenuationDesc, SCHEMA_PRIORITY_IMPL);
}

float SchemaAttFacGet(ObjID objID)
{
   float fac = 1.0;
   g_pPropSchemaAttFac->Get(objID, &fac);
   return fac;
}

// Schema LastSample property
IIntProperty *g_pPropSchemaLastSample;

static sPropertyDesc SchemaLastSampleDesc = 
{
   PROP_SCHEMA_LAST_SAMPLE, kPropertyTransient,
   NULL, 0, 0, { "Schema", "Last Sample" },
   kPropertyChangeLocally,
};

#define SCHEMA_LAST_SAMPLE_IMPL kPropertyImplDense

void SchemaLastSamplePropertyInit(void)
{
   g_pPropSchemaLastSample = CreateIntProperty(&SchemaLastSampleDesc, SCHEMA_LAST_SAMPLE_IMPL);
}

int SchemaLastSampleGet(ObjID objID)
{
   int lastSample = -1;
   g_pPropSchemaLastSample->Get(objID, &lastSample);
   return lastSample;
}

// Initialize all schema properties
void SchemaPropsInit(void)
{
   SchemaLoopParamsPropertyInit();
   SchemaPlayParamsPropertyInit();
   SchemaPriorityPropertyInit();
   SchemaMessagePropertyInit();
   SchemaActionSndPropertyInit();
   SchemaLastSamplePropertyInit();
   SchemaAttFacPropertyInit();
}

void SchemaPropsTerm(void)
{
   SafeRelease(g_pPropSchemaAttFac);
   SafeRelease(g_pPropSchemaLastSample);
   SafeRelease(g_schemaPlayParamsProperty);
   SafeRelease(g_schemaLoopParamsProperty);
   SafeRelease(g_pPropSchemaPriority);
   SafeRelease(g_pPropSchemaMessage);
   SafeRelease(g_pPropSchemaActionSnd);
}
