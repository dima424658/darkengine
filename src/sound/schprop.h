/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schprop.h,v 1.13 2000/01/29 13:41:55 adurant Exp $
// Properties that go on schema objects
#pragma once

#ifndef __SCHPROP_H
#define __SCHPROP_H

#include <propface.h>
#include <schtype.h>
#include <tagtype.h>

#undef INTERFACE
#define INTERFACE ISchemaPlayParamsProperty
DECLARE_PROPERTY_INTERFACE(ISchemaPlayParamsProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sSchemaPlayParams*); 
}; 

#define PROP_SCHEMA_PLAY_PARAMS "SchPlayParams"

EXTERN ISchemaPlayParamsProperty *g_schemaPlayParamsProperty;

EXTERN void SchemaPlayParamsPropertyInit();
EXTERN sSchemaPlayParams *SchemaPlayParamsGet(ObjID objID);
#define SCHEMA_PLAY_PARAMS_SET(objID, pParams) \
   PROPERTY_SET(g_schemaPlayParamsProperty, objID, (sSchemaPlayParams*)pParams)
#define SCHEMA_PLAY_PARAMS_DELETE(objID) \
   IProperty_Delete(g_schemaPlayParamsProperty, objID)

#undef INTERFACE
#define INTERFACE ISchemaLoopParamsProperty
DECLARE_PROPERTY_INTERFACE(ISchemaLoopParamsProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sSchemaLoopParams*); 
}; 

#define PROP_SCHEMA_LOOP_PARAMS "SchLoopParams"

EXTERN ISchemaLoopParamsProperty *g_schemaLoopParamsProperty;

EXTERN void SchemaLoopParamsPropertyInit();
EXTERN sSchemaLoopParams *SchemaLoopParamsGet(ObjID objID);
#define SCHEMA_LOOP_PARAMS_SET(objID, pParams) \
   PROPERTY_SET(g_schemaLoopParamsProperty, objID, (sSchemaLoopParams*)pParams)
#define SCHEMA_LOOP_PARAMS_DELETE(objID) \
   IProperty_Delete(g_schemaLoopParamsProperty, objID)

// Priority
EXTERN IIntProperty *g_pPropSchemaPriority;

#define SCH_PRIORITY_MIN 0
#define SCH_PRIORITY_NORMAL 128
#define SCH_PRIORITY_MAX 255

#define PROP_SCHEMA_PRIORITY "SchPriority"
EXTERN void SchemaPriorityPropInit(void);
EXTERN int SchemaPriorityGet(ObjID objID);
#define SCHEMA_PRIORITY_SET(objID, priority) \
   PROPERTY_SET(g_pPropSchemaPriority, objID, priority)
#define SCHEMA_PRIORITY_DELETE(objID) \
   IProperty_Delete(g_pPropSchemaPriority, objID)

// Sound messages
#define PROP_SCHEMA_MESSAGE "SchMsg"

// Schema message property
EXTERN ILabelProperty *g_pPropSchemaMessage;

EXTERN void SchemaMessagePropertyInit(void);
EXTERN Label *SchemaMessageGet(ObjID objID);
#define SCHEMA_MESSAGE_SET(objID, msg) \
   PROPERTY_SET(g_pPropSchemaMessage, objID, msg)
#define SCHEMA_MESSAGE_DELETE(objID)  \
   IProperty_Delete(g_pPropSchemaMessage, objID)

// "Action" sounds
#define PROP_SCHEMA_ACTION_SND "SchActionSnd"
EXTERN ILabelProperty *g_pPropSchemaActionSnd;
//EXTERN ILabelProperty *gPropSchemaActionSnd;
EXTERN void SchemaActionSndPropertyInit(void);
EXTERN Label *SchemaActionSndGet(ObjID objID);
#define SCHEMA_ACTION_SND_SET(objID, msg) \
   PROPERTY_SET(g_pPropSchemaActionSnd, objID, msg)
#define SCHEMA_ACTION_SND_DELETE(objID)  \
   IProperty_Delete(g_pPropSchemaActionSnd, objID)

// Last sample property
EXTERN IIntProperty *g_pPropSchemaLastSample;

#define PROP_SCHEMA_LAST_SAMPLE "SchLastSample"
EXTERN void SchemaLastSamplePropInit(void);
EXTERN int SchemaLastSampleGet(ObjID objID);
#define SCHEMA_LAST_SAMPLE_SET(objID, LastSample) \
   PROPERTY_SET(g_pPropSchemaLastSample, objID, LastSample)
#define SCHEMA_LAST_SAMPLE_DELETE(objID) \
   IProperty_Delete(g_pPropSchemaLastSample, objID)

// attenuation factor property
#define PROP_SCHEMA_ATTFAC "SchAttFac"
EXTERN IFloatProperty *g_pPropSchemaAttFac;
EXTERN float SchemaAttFacGet(ObjID objID);

// Initialize all schema properties
EXTERN void SchemaPropsInit(void);
EXTERN void SchemaPropsTerm(void);  // hey, lets clean up too, odd, i realize

#endif


