/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drklinks.cpp,v 1.17 2000/02/19 12:27:20 toml Exp $

// needed for links
#include <drklinks.h>
#include <linktype.h>
#include <linkbase.h>
#include <relation.h>
#include <label.h>

struct sLinkTableEntry 
{
   sRelationDesc desc;
   sRelationDataDesc ddesc;
   RelationQueryCaseSet cases; 
}; 

static sLinkTableEntry dark_links[] = 
{
   {   
      { "SoundDescription" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   {   
      { "ControlDevice" },  
      LINK_NO_DATA,
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   { 
      { "Route" },
      LINK_NO_DATA, 
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   { 
      { "ScriptParams" },
      LINK_DATA_DESC(Label), 
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   { 
      { "Owns" },
      LINK_NO_DATA, 
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   { 
      { "Population" },
      LINK_NO_DATA, 
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   { 
      { "Transmute" },
      LINK_NO_DATA, 
      kQCaseSetSourceKnown|kQCaseSetDestKnown, 
   },

   // Temporary hack while we port conversation stuff
   { 
      { "OldConversation" }, 
      LINK_NO_DATA, 
      kQCaseSetSourceKnown
   },




};

#define NUM_DARK_LINKS (sizeof(dark_links)/sizeof(dark_links[0]))

void DarkInitLinks(void)
{
   for (int i = 0; i < NUM_DARK_LINKS; i++)
   {
      sLinkTableEntry& entry = dark_links[i]; 

      IRelation* relation = CreateStandardRelation(&entry.desc,&entry.ddesc,entry.cases);
      SafeRelease(relation); 
   }
}

//////////////////
// properties

// needed for props
#include <propface.h>
#include <propbase.h>
#include <engfeat.h>  // PROP GROUP
#include <memall.h>
#include <dbmem.h>   // must be last header! 

///////////////////
// replace texture property for scripts

#define TERR_REPL_PROP_IMPL kPropertyImplSparseHash
#define TERR_REPL_PROP_DESC(str) \
   static sPropertyDesc str##PropDesc = { "TerrRep"#str, 0, NULL, 0, 0, { "Script", "TerrReplace"#str }}
#define TERR_REPL_PROP_CREATE(str) \
   IStringProperty *##str##Prop = CreateStringProperty(&##str##PropDesc,TERR_REPL_PROP_IMPL); \
   SafeRelease(##str##Prop)

TERR_REPL_PROP_DESC(Off);
TERR_REPL_PROP_DESC(On);
TERR_REPL_PROP_DESC(Destroy);

//
// another for script property, this one is less gruesome, though
//
#define BOOK_PROP_IMPL kPropertyImplSparseHash
static sPropertyDesc BookPropDesc = { "Book", 0, NULL, 0, 0, { "Book", "Text" }};
static sPropertyDesc BookArtDesc =  { "BookArt", 0, NULL, 0, 0, { "Book", "Art" }};

//
// Trap Qvar property 
// 

static sPropertyDesc TrapQvarDesc =  { "TrapQVar", 0, NULL, 0, 0, { "Trap", "Quest Var" }};


//
// sticky note property 
//

static sPropertyDesc StickyNoteDesc = 
{
   "DesignNote", 0, NULL, 0, 0,
   { "Editor", "Design Note" },
};

//
// Script/Timing property
//
static sPropertyDesc ScriptTimingDesc = 
{
   "ScriptTiming", 0, NULL, 0, 0,
   { "Script", "Timing" },
};

typedef cAutoIPtr<IStringProperty> cISP; 
typedef cAutoIPtr<IIntProperty> cIIP;

void DarkInitProps(void)
{
   cISP(CreateStringProperty(&BookPropDesc,BOOK_PROP_IMPL)); 
   cISP(CreateStringProperty(&BookArtDesc,BOOK_PROP_IMPL)); 

   TERR_REPL_PROP_CREATE(Off);
   TERR_REPL_PROP_CREATE(On);
   TERR_REPL_PROP_CREATE(Destroy);

   cISP(CreateStringProperty(&StickyNoteDesc,kPropertyImplSparseHash)); 
   cISP(CreateStringProperty(&TrapQvarDesc,kPropertyImplSparseHash)); 

   cIIP(CreateIntProperty(&ScriptTimingDesc,kPropertyImplSparseHash));
}
