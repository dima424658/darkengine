/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/gidprop.cpp,v 1.13 1998/03/26 13:50:26 mahk Exp $
#include <propert_.h>
#include <prophash.h>
#include <dataops.h>
#include <globalid.h>
#ifdef NETWORK_ENABLED
//include <apppid.h>
#endif
#include <gidprop.h>
#include <objowner.h>
#include <propid.h>

#include <sdesbase.h>
#include <sdesc.h>

// Must be last header
#include <dbmem.h>


//
// PROPERTY CLASSES
//

class cGIDDataOps : public cClassDataOps<GlobalID>
{
}; 

class cGIDStore : public cHashPropertyStore<cGIDDataOps>
{
   
}; 

class cGIDProp : public cSpecificProperty<IGlobalIDProperty,&IID_IGlobalIDProperty,GlobalID*,cGIDStore> 
{
   typedef cSpecificProperty<IGlobalIDProperty,&IID_IGlobalIDProperty,GlobalID*,cGIDStore> cParent;

public:
   cGIDProp(const sPropertyDesc* desc) : cParent(desc) {}; 

   STANDARD_DESCRIBE_TYPE(GlobalID); 
};

//
// Structure Description
//



static sFieldDesc GIDFields[] =
{
   { "creator", kFieldTypeInt, FieldLocation(GlobalID, creator),
     kFieldFlagUnsigned },
   { "local_objid", kFieldTypeInt, FieldLocation(GlobalID, local_objid) }
};

static sStructDesc GIDStructDesc = StructDescBuild(GlobalID, kStructFlagNone, GIDFields);


////////////////////////////////////////////////////////////
// GID PROP API 
//


////////////////////////////////////////////////////////////
// GID PROPERTY CREATION 
//


static PropertyID gidprop_id = PROPID_NULL;

static sPropertyDesc gidprop_desc  = 
{
   PROP_GID_NAME,
   kPropertyTransient
   |kPropertyInstantiate
   |kPropertyNoInherit
   |kPropertyNoClone
   |kPropertyNoEdit,
};

void GlobalIDPropInit(void)
{
   IGlobalIDProperty* prop;
   prop = new cGIDProp(&gidprop_desc);
   gidprop_id = prop->GetID();
   SafeRelease(prop);
   StructDescRegister(&GIDStructDesc);
}

#define get_gid_prop(prop) GetPropertyInterface(gidprop_id,IGlobalIDProperty,prop)

BOOL ObjGetGlobalID(ObjID obj, GlobalID* id)
{
   IGlobalIDProperty* prop;
   if (get_gid_prop(&prop))
   {
      GlobalID *gid;
      if (prop->GetSimple(obj,&gid))
      {
         *id = *gid;
         SafeRelease(prop);
         return TRUE;
      }
   }
   return FALSE;
}

BOOL ObjSetGlobalID(ObjID obj, GlobalID* id)
{
   IGlobalIDProperty* prop;
   if (get_gid_prop(&prop))
   {
      prop->Set(obj,id);
      SafeRelease(prop);
      return TRUE;
   }
   return FALSE;
}


BOOL ObjIsLocal(ObjID obj)
{
   return !PropID_IsSimplyRelevant(gidprop_id,obj);
}

void ObjMakeLocal(ObjID obj)
{
   PropID_Delete(gidprop_id,obj);
}


void ObjMakeGlobal(ObjID obj)
{
   if (!ObjIsLocal(obj)) return;
#ifdef NETWORK_ENABLED
   GlobalID gid = {appPID, obj}; 
#else
   GlobalID gid = {0, obj}; 
#endif
   ObjSetGlobalID(obj,&gid);
#ifdef NETWORK_ENABLED
   ObjSetOwner(obj,appPID);
#else
   ObjSetOwner(obj,0);
#endif
}

// @OPTIMIZE:  This should use a hashtable or something.

ObjID GlobalID2Local(GlobalID* gid)
{
   IGlobalIDProperty* prop;

   ObjID retval = OBJ_NULL;

   if (!get_gid_prop(&prop))
      return retval;

   sPropertyObjIter s;
   ObjID obj;
   GlobalID* objgid;

   prop->IterStart(&s);
   while (prop->IterNextValue(&s,&obj,&objgid))
      if (GlobalIDEqual(gid,objgid))
      {
         retval = obj;
         break;
      }
   prop->IterStop(&s);
   SafeRelease(prop);
   return retval;
}

BOOL GlobalIDCreate(int creator, ObjID objid, GlobalID* pgid)
{
   if (pgid)
   {
      pgid->creator = creator;
      pgid->local_objid = objid;
      return TRUE;
   }
   else
      return FALSE;
}


