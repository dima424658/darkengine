/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/object/traitlnk.cpp,v 1.9 1999/05/19 16:05:56 mahk Exp $

#include <appagg.h>

#include <traittyp.h>
#include <traitlnk.h>
#include <traitln_.h>
#include <traitqdb.h>

#include <traitprp.h>

#include <property.h>
#include <propbase.h>
#include <propman.h>

#include <linktype.h>
#include <linkbase.h>
#include <linkstor.h>
#include <linkint.h>
#include <relation.h>
#include <linkqdb.h>

#include <lnkbyobj.h>

#include <string.h>
#include <sdesc.h>
#include <sdesbase.h>

#include <allocapi.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////////////////////////
// LINK RELATIONS FOR THE TRAIT MANAGER
//
IRelation* gpMetaPropRel = NULL; 

//
// OPTIMIZATIONS to use
//
#define OPTIMIZATIONS (kQCaseSetSourceKnown|kQCaseSetDestKnown)



////////////////////////////////////////////////////////////
// METAPROPERTY RELATION
//


//
// Metaprop relation description
//

static sRelationDesc metaprop_desc = 
{
  LINK_METAPROP_NAME,
  kRelationNoEdit,
};

static sRelationDataDesc metaprop_ddesc 
   = LINK_DATA_DESC_FLAGS(tMetaPropertyPriority,kRelationDataAutoCreate);

StructDescDefineSingleton(MetaPropStructDesc,tMetaPropertyPriority,kFieldTypeInt,kFieldFlagNone); 

static cMetaPropQDB* gpMetaQDB = NULL;

static IRelation* BuildMetaPropRelation(void)
{
   LGALLOC_AUTO_CREDIT(); 
   sCustomRelationDesc desc;
   memset(&desc,0,sizeof(desc));

   gpMetaQDB = new cMetaPropQDB; 
   // build special table
   desc.DBs[kRelationSourceKnown] = gpMetaQDB; 

   // optimize dest known case
   desc.DBs[kRelationDestKnown] = CreateKnownDestLinkQueryDatabase();

   IRelation* metaprop = CreateCustomRelation(&metaprop_desc,&metaprop_ddesc,&desc);

   SafeRelease(desc.DBs[kRelationDestKnown]);

   return metaprop;
}

MetaPropLinks* GetObjectMetaPropLinks(ObjID obj)
{
   Assert_(gpMetaQDB != NULL);
   return gpMetaQDB->Get(obj); 
}

void InitTraitRelations(void)
{
   StructDescRegister(&MetaPropStructDesc); 
   gpMetaPropRel = BuildMetaPropRelation();
}

void TermTraitRelations(void)
{
   SafeRelease(gpMetaPropRel); 
   SafeRelease(gpMetaQDB); 
}






