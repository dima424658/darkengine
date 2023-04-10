/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schdb.cpp,v 1.31 1998/10/14 03:14:07 dc Exp $

#include <cfgdbg.h>

#include <objsys.h>
#include <traitman.h>
#include <propman.h>
#include <iobjsys.h>
#include <objquery.h>
#include <traitbas.h>

#include <dbasemsg.h>
#include <dispbase.h>

#include <datapath.h>
#include <config.h>

#include <schema.h>
#include <schprop.h>
#include <schbase.h>
#include <schdb.h>
#include <schsamps.h>
#include <schyacc.h>
#include <speech.h>
#include <esnd.h>
#include <schema_.h>
#include <schfile.h>

#include <texprop.h>

// Must be last header
#include <dbmem.h>

#define SCHEMA_DIR "schema" // where we expect to find schemas off base art path

// number of schemas/samples to start with
#define SCHEMAS_START_NUM       100
#define SAMPLES_START_NUM       200
#define SAMPLE_CHARS_START_NUM  2000

// Hey, I copied this from lresname.c
// It should undoubtedly be changed/centralised somewhere.
#ifndef __WATCOMC__
# if defined(__OS2__) || defined(__NT__)
#   define NAME_MAX	255		/* maximum filename for HPFS or NTFS */
#   define PATH_MAX	259 /* maximum length of full pathname excl. '\0' */
# else
#   define NAME_MAX	12		/* 8 chars + '.' +  3 chars */
#   define PATH_MAX	143 /* maximum length of full pathname excl. '\0' */
# endif
#endif

static struct sSchemaPlayParams baseSchemaPlayParams = {0, -1, 0, 0, 0};

// Is the object a schema?
BOOL ObjIsSchema(ObjID objID)
{
   ObjID baseSchemaID;

   if ((baseSchemaID = pObjSys->GetObjectNamed(BASE_SCHEMA_OBJ)) != OBJ_NULL)
      return pTraitMan->ObjHasDonor(objID, baseSchemaID);
   return FALSE;
}

// destroy all schemas
void SchemasDestroy(void)
{
   ObjID baseSchemaID;

   if ((baseSchemaID = pObjSys->GetObjectNamed(BASE_SCHEMA_OBJ)) != OBJ_NULL)
   {
      IObjectQuery *pQuery;

      pQuery = pTraitMan->Query(baseSchemaID, kTraitQueryAllDescendents);
      while (!pQuery->Done())
      {
         pObjSys->Destroy(pQuery->Object());
         pQuery->Next();
      }
      SafeRelease(pQuery);
   }
   SchemaSamplesClear();
}

static BOOL obj_is_missing(ObjID obj)
{
   ObjID missing = pObjSys->GetObjectNamed("Missing"); 
   return  missing != OBJ_NULL && pTraitMan->ObjHasDonor(obj,missing); 
}


// Create a new schema, return Obj ID of new object, 
// OBJ_NULL if can't create or object exists already
ObjID SchemaCreate(Label *pSchemaLabel, ObjID arch)
{

   ConfigSpew("SchemaCreate", ("SchemaCreate: %s, arch: %d\n", &pSchemaLabel[0], arch));

   ObjID obj = pObjSys->GetObjectNamed(pSchemaLabel->text);

   if (obj != OBJ_NULL)
   {
      if (ObjIsSchema(obj) || obj_is_missing(obj))
      {
         SchemaSamplesDestroy(obj);
         SCHEMA_PLAY_PARAMS_DELETE(obj);
         SCHEMA_LOOP_PARAMS_DELETE(obj);
         SCHEMA_PRIORITY_DELETE(obj);
         SCHEMA_MESSAGE_DELETE(obj);
         if (!ObjIsSchema(arch))
         {
            arch = pObjSys->GetObjectNamed(BASE_SCHEMA_OBJ);
            Warning(("Archetype %d is not a schema archetype\n",arch)); 
         }
         pTraitMan->SetArchetype(obj,arch); 
      }  
      else
      {
         Label newname = { "s$" }; 
         strncat(newname.text,pSchemaLabel->text,sizeof(newname.text)-strlen(newname.text)-1);
         Warning(("Schema name conflict! Renaming %s to %s\n",pSchemaLabel->text,newname.text)); 
         return SchemaCreate(&newname,arch); 
      }
   }
   else
   {
      obj = pTraitMan->CreateArchetype(pSchemaLabel->text, arch); 
      if (obj == OBJ_NULL)
         Warning(("SchemaCreate: can't create schema %s\n", &pSchemaLabel[0]));
   }
   return obj;
}

void SchemasBuildDefault(void)
{
   ISchemaPlayParamsProperty *playParamsProp;
   ITraitManager *pTraitMan = AppGetObj(ITraitManager);
   ObjID baseSchemaID;

   if (SUCCEEDED(pTraitMan->CreateBaseArchetype(BASE_SCHEMA_OBJ, &baseSchemaID)))
   {
      GetPropertyInterfaceNamed(PROP_SCHEMA_PLAY_PARAMS, ISchemaPlayParamsProperty, &playParamsProp);
      playParamsProp->Create(baseSchemaID);
      playParamsProp->Set(baseSchemaID, &baseSchemaPlayParams);
   }
   SafeRelease(pTraitMan);
}

#ifdef SCHEMA_LOADING
void SchemaFilesLoad(const char *what)
{
   SchemaFilesLoadFromDir("schema", what, SchemaYaccParse);
}

void SchemaFilesLoadAll()
{
   SchemaFilesLoad("*.spc");
   SchemaFilesLoad("*.arc");
   SchemaFilesLoad("*.sch");

   SpeechNotifyTextParseEnd();
   ESndNotifyTextParseEnd();
}
#endif

/*
  Deal with database events.
*/
EXTERN void SchemaDatabaseNotify(DispatchData *msg)
{
   msgDatabaseData data;

   data.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         SpeechDestroy();
         SchemaSamplesClear();
         break;

      case kDatabaseDefault:
         SchemasBuildDefault();
         SpeechDestroy();
         SpeechBuildDefault();
         terrainprop_load();         
         break;

      case kDatabaseSave:
         if (msg->subtype & kObjPartAbstract)
         {
            SchemaSamplesWrite(data.save);
         }
         break;

      case kDatabaseLoad:
         if (msg->subtype & kObjPartAbstract)
         {
            SchemaSamplesRead(data.load);
         }
         break;

      case kDatabasePostLoad:
         terrainprop_load();
         break;
   }
}
