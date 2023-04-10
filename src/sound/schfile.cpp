/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schfile.cpp,v 1.4 1998/10/14 03:14:17 dc Exp $

#include <schfile.h>
#include <findhack.h>  // all the abstracted lg_find stuff
#include <hashset.h>
#include <hshsttem.h>
#include <str.h>
#include <datapath.h>
#include <config.h>
#include <cfgdbg.h>

// Must be last header 
#include <dbmem.h>

typedef struct sSchemaFile
{
   cStr fileName;
   cStr pathName;
} sSchemaFile;

class cFileNameHash: public cStrHashSet<sSchemaFile *>
{
private:
   tHashSetKey GetKey(tHashSetNode node) const;
};

tHashSetKey cFileNameHash::GetKey(tHashSetNode node) const
{
   return (tHashSetKey)((const char*)(((sSchemaFile*)node)->fileName));
}

static cFileNameHash fileNameHash;


void SchemaFilesGet(const char *pszDataPath, const char *pszWhat)
{
   Datapath sDatapath;
   DatapathDir *pDatapathDir;
   char *pszFileName;
   sSchemaFile *pSchemaFile;

   DatapathClear(&sDatapath);
   DatapathAdd(&sDatapath, (char*)pszDataPath);
   pDatapathDir = DatapathOpenDir(&sDatapath, (char*)pszWhat, DP_SCREEN_DOT);
   while (NULL!=(pszFileName = DatapathReadDir(pDatapathDir)))
   {
      pSchemaFile = new sSchemaFile;
      pSchemaFile->fileName = pszFileName;
      (pSchemaFile->fileName).MakeLower();
      if (!fileNameHash.Search((const char*)(pSchemaFile->fileName)))
      {
         pSchemaFile->pathName = pszDataPath;
         fileNameHash.Insert(pSchemaFile);
      }
      else
         delete pSchemaFile;
   }
   DatapathCloseDir(pDatapathDir);
   DatapathFree(&sDatapath);
}

void SchemaFilesRead(fSchemaRead readFunc)
{
   cStr fullName;
   sSchemaFile *pSchemaFile;
   tHashSetHandle handle;

   pSchemaFile = (sSchemaFile*)(fileNameHash.GetFirst(handle));
   while (pSchemaFile != NULL)
   {
      fullName.FmtStr("%s\\%s", (const char*)(pSchemaFile->pathName), 
                      (const char*)(pSchemaFile->fileName));
      fileNameHash.Remove(pSchemaFile);
      delete pSchemaFile;
      ConfigSpew("SchemaFiles", ("Loading file %s\n", (const char*)fullName));
      (*readFunc)((char*)((const char*)fullName));
      pSchemaFile = (sSchemaFile*)fileNameHash.GetFirst(handle);
   }
}


// read all files from res path and local dir that match "what" string
void SchemaFilesLoadFromDir(const char *where, const char *what, fSchemaRead readFunc)
{
   static char szDataPath[PATH_MAX];
   cStr resSchemaPath;

   // open stuff in your current dir
   resSchemaPath.FmtStr(".\\%s",where);
   SchemaFilesGet((const char*)resSchemaPath, what);

   // get stuff from res path
   if (config_get_raw("schema_base_path", szDataPath, sizeof(szDataPath)))
   {
      resSchemaPath.FmtStr("%s\\%s", szDataPath, where);
      SchemaFilesGet((const char*)resSchemaPath, what);
   }
   // read it all
   SchemaFilesRead(readFunc);
}
