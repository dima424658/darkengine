/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/motdb.cpp,v 1.14 1999/12/27 15:24:35 BODISAFA Exp $

#include <motdb.h>
#include <osystype.h>
#include <tagdbkey.h>
#include <tagdbkst.h>
#include <motmngr.h>
#include <mschyacc.h>
#include <cfgdbg.h>
#include <config.h>
#include <schfile.h>
#include <motdb_.h>
#include <motdbase.h>

#include <dbmem.h> // must be last included header


///////////////////////////////////////////////////////
//
// cMotionDatabase implementation
//
///////////////////////////////////////////////////////

cMotionDatabase::cMotionDatabase()
   : m_Remap(&m_TagNameMap, NULL)
{
   m_nActors=0;
   m_pDatabase=NULL;
}

cMotionDatabase::~cMotionDatabase()
{
   if(m_pDatabase)
      delete m_pDatabase;
}

void cMotionDatabase::Clear()
{
   if(m_pDatabase)
   {
      delete m_pDatabase;
      m_pDatabase=NULL;
   }
   m_nActors=0;
   m_RawResult.Clear();
   m_NameMap.Clear();
   m_TagNameMap.Clear();
   m_Remap.Clear();
   m_SchemaSet.SetSize(0);
}


void cMotionDatabase::TokenLocalToGlobal(cTagDBKey *pKey)
{
   sTagRemap Remap(pKey->m_KeyType, 0, 0, 0);
   m_Remap.LocalToGlobal(&Remap, TRUE);
   pKey->m_KeyType = Remap.m_GlobalType;
}


void cMotionDatabase::TokenGlobalToLocal(cTagDBKey *pKey)
{
   sTagRemap Remap(0, 0, pKey->m_KeyType, 0);
   m_Remap.GlobalToLocal(&Remap, TRUE);
   pKey->m_KeyType = Remap.m_LocalType;
}

BOOL cMotionDatabase::GetSchema(int index, cMotionSchema **ppSchema)
{
   if ((index < 0) || (index >= m_SchemaSet.Size()))
   {
      *ppSchema = NULL;
      return FALSE;
   }

   *ppSchema = &m_SchemaSet[index];
   return TRUE;
}

// database query
BOOL cMotionDatabase::GetBestMatch(int actor, cTagDBInput *pInput, cMotionSchema **ppSchema, eMotDBMatchType matchType)
{
   cMotionSchema *pSchema=NULL;
   int size;

   if(!m_pDatabase)
      return FALSE;

   m_RawResult.Clear();
   if(!m_pDatabase->GetMatches(actor,pInput,&m_RawResult)) // this not ordered
      return FALSE;

   if((size=m_RawResult.Size())==0)
      return FALSE;

   if(size==1) // return the only match
   {
      *ppSchema=&m_SchemaSet[m_RawResult[0].m_iData];
      return TRUE;
   }

   // choose schema depending on match type
   switch(matchType)
   {
      case kMMT_AbsoluteBest:
      {
         // select schema with highest weight
         // @TODO: figure out what is best thing to do with
         // equally weighted results.  Do we always want to choose
         // same one?  It's weird to have which one gets chosen depend
         // on vagaries of tagdb implementation
         int i,best=-1,count=0;
         float bestWeight=-1.0;

         for(i=0;i<size;i++)
         {
#ifdef DBG_ON
            if(m_RawResult[i].m_fWeight==bestWeight)
            {
               count++;
            }
#endif
            if(m_RawResult[i].m_fWeight<=bestWeight)
               continue;
            best=i;
            count=1;
            bestWeight=m_RawResult[i].m_fWeight;
         }
         Assert_(best>=0);
#ifdef DBG_ON
         if(count!=1)
            ConfigSpew("MotDBTrace",("GetBestMatch: No absolute best - %d schemas have same weight\n",count));
#endif
         pSchema=&m_SchemaSet[m_RawResult[best].m_iData];
         break;
      }
      case kMMT_WeightedDither:
      {
         // sort output by weight, choose a random number between 
         // zero and sum of weights of all output, 
         // and iterate over sorted output
         // until sum of weights = chosen number
         // @TODO: implement this if it's really what we want
         // NOTE: this implementation chooses equally between
         // equally weighted schemas.
         AssertMsg(FALSE,"Motion Database weighted dither match not implemented yet");
         return FALSE;
      }
   }

   // convert raw result to schemas
   *ppSchema=pSchema;
   return TRUE;
}

// These ridiculous wrappers are the entry point for our swizzling,
// since we can't get at our 
static cMotionDatabase *g_pSwizzleDatabase;

static void GlobalToLocal(cTagDBKey *pKey)
{
   g_pSwizzleDatabase->TokenGlobalToLocal(pKey);
}

static void LocalToGlobal(cTagDBKey *pKey)
{
   g_pSwizzleDatabase->TokenLocalToGlobal(pKey);
}

////////////////////////////////////////////////////////////

#ifdef EDITOR
// construct database
BOOL cMotionDatabase::BuildFromAscii()
{
   Clear();

   // need to setup motionset for build
   g_pMotionSet->StartBuild();

   SchemaFilesLoadFromDir("mschema","*.def",MotSchemaYaccParse);   
   SchemaFilesLoadFromDir("mschema","*.mtg",MotSchemaYaccParse);   

   // retokenize w/global mapping
   g_pSwizzleDatabase = this;
   m_pDatabase->Swizzle(LocalToGlobal);
   m_pDatabase->SwizzleRequiredFlags(&m_Remap);

   // let motion set know we are done building
   g_pMotionSet->EndBuild();

   // setup runtime data now that motion set is built
   int nSchemas=m_SchemaSet.Size(),i;
   cMotionSchema *pSch;

   for(i=0;i<nSchemas;i++)
   {
      pSch=&m_SchemaSet[i];
      Assert_(pSch);
      pSch->SetupRunTimeData(&m_NameMap);
   }

   return FALSE;
}
#endif

BOOL cMotionDatabase::Load(ITagFile *pFile)
{
   ulong num;
   int i;

   // load name maps
   m_NameMap.Load(pFile);
   m_TagNameMap.Load(pFile);

   // load tag database
   ITagFile_Read(pFile, (char *)&num, sizeof(num));
   m_nActors=num;
   m_pDatabase=NewIFancyTagDatabase(m_nActors);
   m_pDatabase->Load(pFile);
   g_pSwizzleDatabase = this;
   m_pDatabase->Swizzle(LocalToGlobal);
   m_pDatabase->SwizzleRequiredFlags(&m_Remap);

   // load schemas
   ITagFile_Read(pFile, (char *)&num, sizeof(num));
   m_SchemaSet.SetSize(num);
   for(i=0;i<num;i++)
   {
      m_SchemaSet[i].Load(pFile,&m_NameMap);      
   }

   return TRUE;
}

#ifdef EDITOR
BOOL cMotionDatabase::Save(ITagFile *pFile)
{
   ulong num;
   int i;

   // Save name maps
   m_NameMap.Save(pFile);
   m_TagNameMap.Save(pFile);

   // Save tag database
   num=m_nActors;
   ITagFile_Write(pFile, (char *)&num, sizeof(num));
   g_pSwizzleDatabase = this;
   m_pDatabase->Swizzle(GlobalToLocal);
   m_pDatabase->Save(pFile);
   m_pDatabase->Swizzle(LocalToGlobal);

   // Save schemas
   num=m_SchemaSet.Size();
   ITagFile_Write(pFile, (char *)&num, sizeof(num));
   for(i=0;i<num;i++)
   {
      m_SchemaSet[i].Save(pFile);      
   }

   return TRUE;
}
#endif

//////////////////////////////////////////////////////////////////
//
// DATABASE BUILDING UTILITIES
//
//////////////////////////////////////////////////////////////////

void cMotionDatabase::CreateTagDatabase(int nActors)
{
   m_nActors=nActors;
   if(m_pDatabase)
      delete m_pDatabase;
   m_pDatabase=NewIFancyTagDatabase(m_nActors);
   Assert_(m_pDatabase);
}

void cMotionDatabase::RegisterTag(const Label *pName, const sTagInfo *pInfo)
{
   // check that tag not already registered. 
   int index=m_TagNameMap.IDFromName(pName);
   if(index==kNameMapIndexNotFound)
   {
      // register tag
      Assert_(m_pDatabase);
      int h=m_pDatabase->RegisterTag(pInfo);
      m_TagNameMap.AddNameAtIndex(pName,h);
   }
   else
   {
      // check for conflict.
      const sTagInfo *pOldInfo=m_pDatabase->GetTagInfo(index);
      Assert_(pOldInfo&&pInfo);
      if(pOldInfo->isMandatory!=pInfo->isMandatory || pOldInfo->weight!=pInfo->weight)
      {
         Warning(("tag %s already registered with conficting info\n",pName->text));
      }
   }
}

void cMotionDatabase::AddSchema(const sMotSchemaDesc *pDesc)
{
   ITagDBKeySet *pKeySet=NewITagDBKeySet();
   cMotionSchema sch;
   int index,j;

   Assert_(pDesc);

   int actor=pDesc->actor;
   
   if(actor<0||actor>=m_nActors)
   {
      Warning(("Actor %d out of bounds\n",pDesc->actor));
      return;
   }

   if(pDesc->archIsString)
   {
      index=m_NameMap.IDFromName(&pDesc->archName);
      if(index==kNameMapIndexNotFound)
      {
         index=m_NameMap.AddName(&pDesc->archName);
      }
      sch.SetArchetype(index);
      ulong flags=sch.GetFlags();
      flags|=kMSchFlag_ArchObjSwizzle;
      sch.SetFlags(flags);
   } else
   {
      sch.SetArchetype(pDesc->archInt);
   }

   // set duration/timewarp stuff
   if(pDesc->duration)
   {
      sch.SetDuration(pDesc->duration);
   } else if(pDesc->timeWarp)
   {
      sch.SetTimeWarp(pDesc->timeWarp);
   }
   // set distance/stretch stuff
   if(pDesc->distance)
   {
      sch.SetDistance(pDesc->distance);
   } else if(pDesc->stretch)
   {
      sch.SetStretch(pDesc->stretch);
   }

   for(j=0;j<pDesc->nMots;j++)
   {
      sch.AddMotion(&pDesc->pMots[j].name,&pDesc->pMots[j].stuff);
   }

   index=m_SchemaSet.Size();
   sch.SetSchemaID(index);

   ConfigSpew("yakspew",("SchemaID %d: %s\n",index,pDesc->name.text));

   m_SchemaSet.SetSize(index+1);
   m_SchemaSet.SetItem(&sch,index);

   // add to tag database
   int type;

   sTagDBData data;
   data.m_iData=index;
   data.m_fWeight=0;
   pKeySet->Clear();
   for(j=0;j<pDesc->nTags;j++)
   {
      type=m_TagNameMap.IDFromName(&pDesc->pTags[j].type);

      if(type==kNameMapIndexNotFound)
      {
         Warning(("tag %s not registered! cannot add to schema\n",pDesc->pTags[j].type.text));
         continue;
      }

      // NOTE: could add support for ranged tags if needed
      cTagDBKey key(type,pDesc->pTags[j].value,pDesc->pTags[j].value); 
      pKeySet->AddKey(&key);
   }
   m_pDatabase->InsertData(actor,pKeySet,data);

   delete pKeySet;
}

void MotDBCreate(int nActors)
{
   g_pMotionDatabase->CreateTagDatabase(nActors);
}

void MotDBRegisterTag(Label *pName, sTagInfo *pInfo)
{
   g_pMotionDatabase->RegisterTag(pName,pInfo);
}

void MotDBAddSchema(sMotSchemaDesc *pSchema)
{
   g_pMotionDatabase->AddSchema(pSchema);
}
