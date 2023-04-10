/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/esndprop.cpp,v 1.4 1998/08/04 22:49:47 mahk Exp $

#include <esndprop.h>
#include <property.h>
#include <propbase.h>
#include <propface.h>
#include <propfac_.h>
#include <dataops_.h>
#include <proplist.h>
#include <objnotif.h>

#include <sdesbase.h>
#include <sdestool.h>

// Include these last 
#include <dbmem.h>
#include <initguid.h>
#include <esndprid.h>

/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   The structure descriptor is also used by both properties.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

static sFieldDesc g_Fields [] =
{
   { "1: Tags", kFieldTypeString, 
     FieldLocation(sESndTagList, m_TagStrings) },
};

static sStructDesc g_ListStruct
  = StructDescBuild(sESndTagList, kStructFlagNone, g_Fields);


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   The class property is for archetypes.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

static IESndTagProperty* g_pESndClassProp = NULL;

//
// This version of sESndTagList is used by the data ops
// It keeps its cTagList in synch with the string.
// 

struct sOpsList : public sESndTagList 
{
   sOpsList()
   {
      m_pTagSet = new cTagSet; 
      m_TagStrings[0] = '\0'; 
   } 

   sOpsList(const sESndTagList& dat)
   {
      strncpy(m_TagStrings,dat.m_TagStrings,sizeof(m_TagStrings)); 
      m_pTagSet = new cTagSet(m_TagStrings); 
   }

   sOpsList(const sOpsList& dat)
   {
      strncpy(m_TagStrings,dat.m_TagStrings,sizeof(m_TagStrings)); 
      m_pTagSet = new cTagSet(m_TagStrings); 
   }

   sOpsList& operator=(const sOpsList& dat)
   {
      strncpy(m_TagStrings,dat.m_TagStrings,sizeof(m_TagStrings)); 
      m_pTagSet->FromString(m_TagStrings); 
      return *this; 
   }

   ~sOpsList()
   {
      delete m_pTagSet; 
   }

} ; 

class cESndTagOps : public cClassDataOps<sOpsList>
{
   typedef cClassDataOps<sOpsList> cParent; 
public: 

   cESndTagOps() : cParent(kNoFlags) {} ; 
   STDMETHOD(Read)(THIS_ sDatum* dat, IDataOpsFile* file, int version)
   {
      if (!dat->value)
         *dat = New(); 
      sOpsList* list = (sOpsList*)dat->value; 
      sESndTagList tmp;
      memset(&tmp,0,sizeof(tmp)); 
      // we derive from cClassDataOps, thus version == size; 
      int sz = (version < sizeof(tmp)) ? version : sizeof(tmp); 
      Verify(file->Read(&tmp,sz) == sz); 
      *list = tmp; 
      return S_OK; 
   }

}; 

class cESndTagStore : public cListPropertyStore<cESndTagOps>
{
}; 

class cESndTagProperty : public cSpecificProperty<IESndTagProperty,
   &IID_IESndTagProperty, sESndTagList*, cESndTagStore>

{
   typedef cSpecificProperty<IESndTagProperty,
                             &IID_IESndTagProperty,
                             sESndTagList*,
                             cESndTagStore> cParent;

public: 
   cESndTagProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(sESndTagList); 


}; 


static sPropertyDesc g_ClassPropDesc =
{
   PROP_ESND_CLASS_NAME,
   0,
   NULL,
   0,0, // verison
   { "Schema", "Class Tags" }, 
};

EXTERN BOOL ObjGetESndClass(ObjID obj, sESndTagList **pData)
{
   Assert_(g_pESndClassProp);
   return g_pESndClassProp->Get(obj, pData);
}


EXTERN BOOL ObjSetESndClass(ObjID obj, sESndTagList *pData)
{
   Assert_(g_pESndClassProp);
   return g_pESndClassProp->Set(obj, pData);
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   The material property is for material archetypes and terrain.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

static IESndTagProperty* g_pESndMaterialProp = NULL;

static sPropertyDesc g_MaterialPropDesc =
{
   PROP_ESND_MATERIAL_NAME,
   0,
   NULL,
   0,0, // verison
   { "Schema", "Material Tags" }, 
};

EXTERN BOOL ObjGetESndMaterial(ObjID obj, sESndTagList **pData)
{
   Assert_(g_pESndMaterialProp);
   return g_pESndMaterialProp->Get(obj, pData);
}


EXTERN BOOL ObjSetESndTagList(ObjID obj, sESndTagList *pData)
{
   Assert_(g_pESndMaterialProp);
   return g_pESndMaterialProp->Set(obj, pData);
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   Initialization of both properties and listeners.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

void ESndPropsInit()
{
   AutoAppIPtr_(StructDescTools, pTools);

   pTools->Register(&g_ListStruct);
   g_pESndClassProp = new cESndTagProperty(&g_ClassPropDesc);
   g_pESndMaterialProp = new cESndTagProperty(&g_MaterialPropDesc);
}

void ESndPropsTerm()
{
   SafeRelease(g_pESndClassProp); 
   SafeRelease(g_pESndMaterialProp); 
}


