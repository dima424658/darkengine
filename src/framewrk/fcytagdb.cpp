/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/fcytagdb.cpp,v 1.7 1998/06/08 17:05:21 TOML Exp $

#include <fcytagdb.h>
#include <tagdbdat.h>
#include <tagdbin.h>
#include <tagdbkst.h>
#include <dynarray.h>
#include <packflag.h>
#include <tagremap.h>

#include <dbmem.h> // must be last included header


class cFancyTagDatabase : public IFancyTagDatabase
{
public:
   cFancyTagDatabase(int nCategories=0);
   virtual ~cFancyTagDatabase();

   // associate a piece of data with an unordered set of keys
   virtual void InsertData(int category, ITagDBKeySet *pSet, const sTagDBData Data);

   // returns handle for tag
   virtual ulong RegisterTag(const sTagInfo *pTagInfo);
   // returns tag info for handle

   virtual const sTagInfo *GetTagInfo(ulong handle) const;

   virtual BOOL Load(ITagFile *pFile);
   virtual BOOL Save(ITagFile *pFile);

   virtual void Swizzle(void Func(cTagDBKey *));
   virtual void SwizzleRequiredFlags(cTagRemapper *pRemapper);

   // find all data which matches the input set.  Tag input should only use
   // previously registered tag keys.
   // Only data of the specified category is considered.
   // Output sorted by tag weight.
   virtual BOOL GetMatches(int category, cTagDBInput *pInput,
                           cTagDBOutput *pOutput);

private:
   const int m_nCategories;
   ITagDBDatabase **m_pTagDatabases;
   cDynArray<sTagInfo> m_TagSet;
   cPackedBoolSet *m_pTagRequired;
};

cFancyTagDatabase::cFancyTagDatabase(int nCategories)
   : m_nCategories(nCategories),
     m_pTagRequired(0)
{
   if(m_nCategories==0)
   {
      m_pTagDatabases=NULL;
   } else
   {
      m_pTagDatabases=new ITagDBDatabase *[m_nCategories];

      int i;
      for(i=0;i<m_nCategories;i++)
      {
         m_pTagDatabases[i]=NewITagDBDatabase();
      }
   }
}

cFancyTagDatabase::~cFancyTagDatabase() 
{
   int i;

   for(i=0;i<m_nCategories;i++)
   {
      delete m_pTagDatabases[i];
   }
   if(m_pTagDatabases)
      delete m_pTagDatabases;
   if (m_pTagRequired)
      delete m_pTagRequired;
}

// associate a piece of data with an unordered set of keys
void cFancyTagDatabase::InsertData(int category, ITagDBKeySet *pSet, const sTagDBData Data)
{
   Assert_(category < m_nCategories && category >= 0);

   // The weight of this datum equals the sum of the weights of its tags.
   float fWeight = 0;
   tTagDBKeySetIter i = pSet->IterStart();
   cTagDBKey Key;
   while (pSet->IterNext(&i, &Key))
      fWeight += m_TagSet[Key.m_KeyType].weight;

   sTagDBData NewData(Data.m_iData, fWeight);

   m_pTagDatabases[category]->Insert(pSet, NewData);
}

// returns handle for tag
ulong cFancyTagDatabase::RegisterTag(const sTagInfo *pTagInfo)
{
   return m_TagSet.Append(*pTagInfo);
}

const sTagInfo *cFancyTagDatabase::GetTagInfo(ulong handle) const
{
   Assert_(handle>=0&&handle<m_TagSet.Size());
   return &m_TagSet[handle];
}

BOOL cFancyTagDatabase::Load(ITagFile *pFile)
{
   int i;

   // load the tag declarations
   ulong size;
   sTagInfo info;

   ITagFile_Read(pFile, (char *)&size, sizeof(size));

   m_TagSet.SetSize(size);
   for(i=0;i<size;i++)
   {
      ITagFile_Read(pFile,(char *)&info,sizeof(info));
      m_TagSet.SetItem(&info,i);
   }

   // load the tag databases
   // m_nCategories is set permanently in the constructor, so 
   // if m_nCategories changes the motion database will be
   // out of date.
   // check for this and assert
   int ncat;
   ITagFile_Read(pFile, (char *)&ncat, sizeof(ncat));
   AssertMsg(ncat==m_nCategories,"number of tagdb categories not save as saved file.  Must up version number");

   for(i=0;i<m_nCategories;i++)
   {
      m_pTagDatabases[i]->Load(pFile);
   }
   return TRUE;
}


BOOL cFancyTagDatabase::Save(ITagFile *pFile)
{
   int i;
    
   // save tag declarations
   ulong size=m_TagSet.Size();
   ITagFile_Write(pFile, (char *)&size, sizeof(size));
   for(i=0;i<size;i++)
   {
      ITagFile_Write(pFile,(char *)&m_TagSet[i],sizeof(m_TagSet[i]));
   }

   // save tag databases
   int ncat=m_nCategories;
   ITagFile_Write(pFile, (char *)&ncat, sizeof(ncat));

   for (i = 0; i < m_nCategories; ++i)
   {
      m_pTagDatabases[i]->Save(pFile);
   }
   return TRUE;
}

#ifndef SHIP
static BOOL g_DumpDatabase=FALSE;
#endif

void cFancyTagDatabase::Swizzle(void Func(cTagDBKey *))
{
   ulong size = m_nCategories;

   for (int i = 0; i < size; ++i)
   {
      ITagDBDatabase *pNewDB = m_pTagDatabases[i]->CopyWithFilter(Func);
      delete m_pTagDatabases[i];
      m_pTagDatabases[i] = pNewDB;

#ifndef SHIP
      if(g_DumpDatabase)
      {
         mprintf("Tag database for category %d\n",i); 
         m_pTagDatabases[i]->Dump();
      }
#endif
   }
}


void cFancyTagDatabase::SwizzleRequiredFlags(cTagRemapper *pRemapper)
{
   int i;
   int iHighestGlobalToken = 0;

   int iSize = m_TagSet.Size();

   // Our first pass simply figures out how large to make our bit
   // vector.
   for (i = 0; i < iSize; ++i) {
      if (m_TagSet[i].isMandatory) {
         sTagRemap Remap(i, 0, 0, 0);
         pRemapper->LocalToGlobal(&Remap, TRUE);
         if (Remap.m_GlobalType > iHighestGlobalToken)
            iHighestGlobalToken = Remap.m_GlobalType;
      }
   }

   if (m_pTagRequired)
      delete m_pTagRequired;

   m_pTagRequired  = new cPackedBoolSet(iHighestGlobalToken);

   // Our second pass sets the actual flags.
   for (i = 0; i < iSize; ++i) {
      if (m_TagSet[i].isMandatory) {
         sTagRemap Remap(i, 0, 0, 0);
         pRemapper->LocalToGlobal(&Remap, TRUE);
         m_pTagRequired->Set(Remap.m_GlobalType);
      }
   }
}


// just return all matches for category 
// that has anything that matches search pattern,
// sorting by/interpreting the weights is up to client. 
BOOL cFancyTagDatabase::GetMatches(int category, cTagDBInput *pInput,cTagDBOutput *pOutput)
{
   if(!m_pTagDatabases)
      return FALSE;

   if(category<0||category>=m_nCategories)
   {
      Warning(("GetMatches: category %d out of bounds\n",category));
      return FALSE;
   }
   int j,size;
   // set whether tags mandatory or not
   size=pInput->Size();
   for(j=0;j<size;j++)
   {
      cTagDBInputTag *ptr=&((*pInput)[j]);
      if (m_pTagRequired->IsSet(ptr->m_KeyType))
      {
         ptr->m_iFlags|=kTagDBInputFlagRequired;
      }
   }
   return m_pTagDatabases[category]->MatchPartialSet(pInput,pOutput);
}


// factory function
IFancyTagDatabase *NewIFancyTagDatabase(int nCategories)
{
   return new cFancyTagDatabase(nCategories);
}

