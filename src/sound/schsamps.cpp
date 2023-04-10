/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schsamps.cpp,v 1.14 1999/06/22 21:17:45 ccarollo Exp $
// Schema sample name and frequency storage

#include <lg.h>
#include <cfgdbg.h>

// data structures
#include <dynarray.h>
#include <str.h>
#include <hashpp.h>
#include <hshpptem.h>

#include <vernum.h>

#include <schsamps.h>

// for destroying things when we load bad data
#include <speech.h>
#include <schdb.h>

// Must be last header
#include <dbmem.h>

// Schema sample class
// contains n samples and frequencies for a given schema
// Must initialize with number of samples

// TODO: Should replace dynarrays below with a static array
typedef cDynArray<cStr*> cStringArray;
typedef cDynArray<uchar> cFreqArray;

class cSchemaSamples
{
public:
   cSchemaSamples(int n);
   ~cSchemaSamples();
   const cStr &GetSample(int n) const;
   int GetSampleIndex(const char *name) const;
   const uchar *GetFreqs() const;
   int GetNum() const;
   void Set(int n, const char *pszSample, uchar freq);
   void Write(ITagFile *pFile); // write to an already open tag file
private:
   int num;
   cStringArray sampleArray;
   cFreqArray freqArray;
};

cSchemaSamples::cSchemaSamples(int n)
{
   int i;

   num = n;
   sampleArray.SetSize(n);
   freqArray.SetSize(n);
   for (i=0; i<n; i++)
   {
      sampleArray[i] = new cStr;
      freqArray[i] = 0;
   }
}

cSchemaSamples::~cSchemaSamples()
{
   int i;

   for (i=0; i<num; i++)
      delete sampleArray[i];
}

inline const cStr &cSchemaSamples::GetSample(int n) const
{
   return *(sampleArray[n]);
}

inline int cSchemaSamples::GetSampleIndex(const char *name) const
{
   for (int i=0; i<num; i++)
   {
      if (strcmp(((const char *)(*(sampleArray[i]))), name) == 0)
         return i;
   }
   return -1;
}

inline const uchar *cSchemaSamples::GetFreqs() const
{
   return &(freqArray[0]);
}

inline int cSchemaSamples::GetNum() const
{
   return num;
}

inline void cSchemaSamples::Set(int n, const char *pszSample, uchar freq)
{
   *(sampleArray[n]) = pszSample;
   freqArray[n] = freq;
}

void cSchemaSamples::Write(ITagFile *pFile)
{
   int i;
   cStr *pStr;

   pFile->Write((char*)&num, sizeof(int));
   for (i=0; i<num; i++)
   {
      int strLen;

      pStr = sampleArray[i];
      strLen = pStr->GetLength();
      pFile->Write((char*)(&strLen), sizeof(int));
      pFile->Write(*pStr, sizeof(char)*pStr->GetLength());
      pFile->Write((char*)&(freqArray[i]), sizeof(uchar));
   }
}

// Hash table for storing ObjID to schema sample structure mapping
typedef cHashTableFunctions<ObjID> sampleHashFunctions;
typedef cHashTable<ObjID, cSchemaSamples*, sampleHashFunctions> SchemaSampleHash;

static SchemaSampleHash *pSchemaSampleHash = NULL;

// Get a schema sample name, NULL if doesn't exist
const char *SchemaSampleGet(ObjID objID, int num)
{
   cSchemaSamples *pSchemaSamples;

   if (pSchemaSampleHash->Lookup(objID, &pSchemaSamples))
      return (const char *)(pSchemaSamples->GetSample(num));
   return NULL;
}

int SchemaSampleGetNamed(ObjID objID, const char *name)
{
   cSchemaSamples *pSchemaSamples;

   if (pSchemaSampleHash->Lookup(objID, &pSchemaSamples))
      return pSchemaSamples->GetSampleIndex(name);
   return -1;
}

// Get frequency array for schema, NULL if none
const uchar *SchemaFreqsGet(ObjID objID)
{
   cSchemaSamples *pSchemaSamples;

   if (pSchemaSampleHash->Lookup(objID, &pSchemaSamples))
      return pSchemaSamples->GetFreqs();
   return NULL;
}

// Get number of samples, 0 if schema not existant
int SchemaSamplesNum(ObjID objID)
{
   cSchemaSamples *pSchemaSamples;

   if (pSchemaSampleHash->Lookup(objID, &pSchemaSamples))
      return pSchemaSamples->GetNum();
   return 0;
}

// Create schema samples
BOOL SchemaSamplesCreate(ObjID objID, int num, const char **ppSampleName,
                         const uchar *pFreq)
{
   int i;
   cSchemaSamples *pSchemaSamples;

   ConfigSpew("SchemaSamples", ("Creating %d samples for schema %d\n", num, objID));
   if ((pSchemaSamples = new cSchemaSamples(num)) == NULL)
   {
      Warning(("SchemaSamplesCreate: can't create schema samples\n"));
      return FALSE;
   }
   pSchemaSampleHash->Set(objID, pSchemaSamples);
   for (i=0; i<num; i++)
      pSchemaSamples->Set(i, ppSampleName[i], pFreq[i]);
   return TRUE;
}

// Destroy samples for schema
void SchemaSamplesDestroy(ObjID objID)
{
   cSchemaSamples *pSchemaSamples;

   if (pSchemaSampleHash->Lookup(objID, &pSchemaSamples))
   {
      pSchemaSampleHash->Delete(objID);
      delete pSchemaSamples;
   }
}

// Clear all schema samples
void SchemaSamplesClear(void)
{
   SchemaSampleHash::cIter iter;

   iter = pSchemaSampleHash->Iter();
   while (!iter.Done())
   {
      pSchemaSampleHash->Delete(iter.Key());
      delete (cSchemaSamples*)(iter.Value());
      iter.Next();
   }
}

void SchemaSamplesInit(void)
{
   if (pSchemaSampleHash == NULL)
      pSchemaSampleHash = new SchemaSampleHash;
   else
      SchemaSamplesClear();
   Assert_(pSchemaSampleHash != NULL);
}

void SchemaSamplesShutdown(void)
{
   SchemaSamplesClear();
   delete pSchemaSampleHash;
}

static TagFileTag sSchemaSamplesTag = {"SchSamp"};
static TagVersion sSchemaSamplesVersion = {1, 0};

static BOOL SchemaSamplesOpenForReading(ITagFile *pFile)
{
   TagVersion sVersion = sSchemaSamplesVersion;

   if (SUCCEEDED(pFile->OpenBlock(&sSchemaSamplesTag, &sVersion)))
   {
      if ((sVersion.major != sSchemaSamplesVersion.major) ||
          (sVersion.minor != sSchemaSamplesVersion.minor))
      {
         pFile->CloseBlock();
         Warning(("SchemaSamplesOpenFile: file contains old data, not loaded\n"));
         Warning(("You should destroy all schemas with destroy_schemas\n"));
         Warning(("Then reload with load_schemas\n"));
         return FALSE;
      }
      return TRUE;
   }
   else
      return FALSE;
}

// Load sample info from tag file
void SchemaSamplesRead(ITagFile *pFile)
{
   int i;

   if (SchemaSamplesOpenForReading(pFile))
   {
      ObjID objID;
      cSchemaSamples *pSchemaSamples;
      int num;
      uchar freq;
      int strLen;
      cStr strBuffer;

      SchemaSamplesClear();
      while (pFile->Read((char*)&objID, sizeof(objID)))
      {
         pFile->Read((char*)&num, sizeof(int));
         pSchemaSamples = new cSchemaSamples(num);
         for (i=0; i<num; i++)
         {
            pFile->Read((char*)&strLen, sizeof(int));
            pFile->Read(strBuffer.GetBufferSetLength(strLen+1),
                        strLen*sizeof(char));
            strBuffer[strLen] = '\0';
            pFile->Read((char*)&freq, sizeof(uchar));
            pSchemaSamples->Set(i, (const char*)strBuffer, freq);
         }
         pSchemaSampleHash->Set(objID, pSchemaSamples);
      }
   }
}

// Save sample info to tag file
void SchemaSamplesWrite(ITagFile *pFile)
{
   SchemaSampleHash::cIter iter;
   TagVersion sVersion = sSchemaSamplesVersion;

   if (SUCCEEDED(pFile->OpenBlock(&sSchemaSamplesTag, &sVersion)))
   {
      iter = pSchemaSampleHash->Iter();
      while (!iter.Done())
      {
         pFile->Write((char*)&(iter.Key()), sizeof(iter.Key()));
         ((cSchemaSamples*)(iter.Value()))->Write(pFile);
         iter.Next();
      }
      pFile->CloseBlock();
   }
}



