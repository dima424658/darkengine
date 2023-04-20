/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/ambprop.cpp,v 1.13 1999/05/26 12:18:55 JON Exp $
// ambient sound wackiness for Dark
// initial hacked version, sorry

#include <propert_.h>
#include <propbase.h>
#include <propcary.h>

#include <sdesc.h>
#include <sdesbase.h>

#include <schema.h>
#include <ambbase.h>
#include <ambient.h>
#include <ambprop.h>
#include <ambient_.h>

#include <objedit.h>
#include <mprintf.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// Ambient Sound Object Property Declaration
//

// this is SO hacked.  keep this in sync with the declaration in ambient.c

EXTERN AmbientRunTime ambRTData[MAX_AMB_OBJS];

//
// My data ops. 
// If you wanted to change how the data gets manipulated (e.g. read and written)
// overload the operations of the base class. 

class cAmbientDataOps : public cClassDataOps<AmbientSound>
{

}; 

static cAmbientDataOps ambient_ops; 


class cAmbientSoundProperty : public cGenericProperty<IAmbientSoundProperty,&IID_IAmbientSoundProperty,AmbientSound*>
{
  typedef cGenericProperty<IAmbientSoundProperty,&IID_IAmbientSoundProperty,AmbientSound*> cParent; 

public:
   cAmbientSoundProperty(const sPropertyDesc* desc, IPropertyStore* store)
      : cParent(desc,store, &ambient_ops)
   {
   }; 

   void OnListenMsg(ePropertyListenMsg msg, ObjID objID, uPropListenerValue value)
   {
      if (msg & kListenPropUnset)
      { 
         for (int i=0; i<ambMax(); i++)
         {
            // shouldnt we call some abstracted version of the kill code in ambient
            // !!!bug!!! what if this is a env sound
            if (ambObjID(i) == objID)
               if (ambRTData[i].schemaHandle!=SCH_HANDLE_NULL)
                  SchemaPlayHalt(ambRTData[i].schemaHandle);
         }
      }
      
      cParent::OnListenMsg(msg, objID, value);
   }

   // our type descriptor
   STANDARD_DESCRIBE_TYPE(AmbientSound); 

};

static const char *flags_names[] =
 { "Environmental", "NoSharpCurve", "TurnedOff", "RemoveProp (OneShot)", "Music", "Synch", "NoFade", "DestroyObj", "DoAutoOff" };

// structure descriptor fun
static sFieldDesc amb_fields [] =
{
   { "Radius",           kFieldTypeInt,    FieldLocation(AmbientSound,rad) },
   { "Override Volume",  kFieldTypeInt,    FieldLocation(AmbientSound,over_volume) },
   { "Flags",            kFieldTypeBits,   FieldLocation(AmbientSound,flags), FullFieldNames(flags_names) },
   { "Schema Name",      kFieldTypeString, FieldLocation(AmbientSound,schema_name) },
   { "Aux Schema 1",     kFieldTypeString, FieldLocation(AmbientSound,aux_schema_1) },
   { "Aux Schema 2",     kFieldTypeString, FieldLocation(AmbientSound,aux_schema_2) },
};

static sStructDesc amb_struct = StructDescBuild(AmbientSound,kStructFlagNone,amb_fields);

// the property descriptor
static sPropertyDesc ambpropdesc =
{
   PROP_AMBIENTSOUND_NAME,
   kPropertyNoInherit|kPropertyInstantiate|kPropertyConcrete,
   NULL
};

static cGenericCompactArrayPropertyStore* ambstore = NULL;

IAmbientSoundProperty *g_pAmbientProp = NULL;

// let us attempt to initialize ourselves
BOOL AmbSoundPropInit(void)
{
   AmbientRunTimeInit();
   StructDescRegister(&amb_struct);
   ambstore = new cGenericCompactArrayPropertyStore(); 
   g_pAmbientProp = new cAmbientSoundProperty(&ambpropdesc,ambstore);
   return TRUE;
}

void AmbSoundPropTerm(void)
{
   SafeRelease(g_pAmbientProp);
}

#ifdef PLAYTEST
EXTERN void AmbSoundDump(void)
{
   for (int i=0; i<MAX_AMB_OBJS && i<ambstore->MaxIdx(); i++)
   {
      ObjID objID = ambObjID(i);
      mprintf("Amb [%d]: %s\n", i, (objID != OBJ_NULL) ? ObjWarnName(ambObjID(i)) : "OBJ_NULL");
   }
}
#endif

int ambMax()
{
#ifdef PLAYTEST
   if (ambstore->MaxIdx() >= MAX_AMB_OBJS)
      AmbSoundDump();
#endif
   AssertMsg(ambstore->MaxIdx() < MAX_AMB_OBJS, "Out of ambient objects!");
   return ambstore->MaxIdx(); 
}

ObjID ambObjID(int idx)
{
#ifdef PLAYTEST
   if (idx >= MAX_AMB_OBJS)
      AmbSoundDump();
#endif
   AssertMsg1(idx < MAX_AMB_OBJS, "Invalid ambient index %d requested!", idx);
   return ambstore->Idx2Obj(idx); 
}

AmbientSound* ambState(int idx)
{
   return (AmbientSound*)(*ambstore)[idx].value; 
}

void ambRemove(int idx)
{
   ObjID obj=ambObjID(idx);
   if (obj==OBJ_NULL)
   {
      Warning(("Trying to Remove NULL\n"));
      return;
   }
   g_pAmbientProp->Delete(obj);
}

void ambSetFlags(int idx, int flags)
{
   ObjID obj=ambObjID(idx);
   AmbientSound *snd;
   if (obj==OBJ_NULL)
   {
      Warning(("Trying to SetFlags on NULL\n"));
      return;
   }
   if (g_pAmbientProp->Get(obj,&snd))
   {
      snd->flags=flags;
      g_pAmbientProp->Set(obj,snd);
   }
}
