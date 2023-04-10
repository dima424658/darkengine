/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/esnd.cpp,v 1.23 2000/01/10 14:07:11 porges Exp $

#include <limits.h>

#include <lgassert.h>
#include <dynarray.h>
#include <mprintf.h>
#include <comtools.h>
#include <packflag.h>

#include <timer.h>
#include <stdlib.h>
#include <cfgdbg.h>

#include <recapi.h>

#include <traitman.h>
#include <appagg.h>
#include <iobjsys.h>
#include <lnkquery.h>
#include <propman.h>
#include <objquery.h>
#include <objremap.h>
#include <traitbas.h>
#include <edittool.h>
#include <objedit.h>

#include <command.h>
#include <playrobj.h>
#include <dbasemsg.h>
#include <dispbase.h>

#include <esnd.h>
#include <esndglue.h>
#include <esndprop.h>
#include <rooms.h>

#include <schbase.h>
#include <schema.h>
#include <schprop.h>
#include <schsamps.h>

#include <hashpp.h>
#include <hshpptem.h>
#include <dlist.h>
#include <dlisttem.h>

#include <namemap.h>
#include <spchdom.h>
#include <speech.h>
#include <spchvoc.h>
#include <tagdbin.h>
#include <tagdbout.h>
#include <tagdbt.h>
#include <tagdbdat.h>

#include <config.h>
#include <cfgdbg.h>

#include <esndreac.h>

// Must be last header
#include <dbmem.h>


extern cSpeechDomain g_Domain;

static IPropertyManager *pPropMan = NULL;
static ITraitManager *pTraitMan = NULL;
static IRecorder *pRecorder = NULL;

// our databases--the tags and values and all that we borrow from the
// speech system
static cDynArray_<uchar, 2> g_LocalTagRequired(64);
static cPackedBoolSet *g_pTagRequired;
static ITagDBDatabase *g_pDatabase;

// These are used to build the databases from text files.
static sTagDBData g_InputKey;
static ITagDBKeySet *g_pInputKeySet;
static BOOL g_bCurrentInputValid;


static tESndGameCallback ESndGameCallback = 0;
tESndGameCallback ESndSetGameCallback(tESndGameCallback pfFunc)
{
   tESndGameCallback pOld = ESndGameCallback;
   ESndGameCallback = pfFunc;
   return pOld;
}


#ifndef SHIP
static void DumpTagSet(cTagSet *pTagSet)
{
   if (pTagSet && pTagSet->Size()) {
      cStr TempBuff;
      pTagSet->ToString(&TempBuff);
      mprintf("%s\n", TempBuff.BufIn());
   }
}
#endif // ~SHIP


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   These functions handle connections to the schema system, including
   playing the actual sounds.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

static int ESndPlayCore(cTagSet *pTagSet, ObjID Obj1, ObjID Obj2,
                        void *pData, sSchemaCallParams *pCallParams)
{
   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("env snd: objs %d and %d\n", Obj1, Obj2);
   #endif

   // We depend on the speech system for our label management so we've
   // got no chance without it.
   if (!g_bSpeechDatabaseLoaded)
      return -1;

   // Some tags are passed in.  There are as many as four other
   // sources for tags: each of our two objects, the room obj1 is in,
   // and a callback into the game which takes the entire list of tags
   // from other sources plus our ObjIDs.
   sESndTagList *pObj1ClassList, *pObj1MaterialList, *pRoomList;
   sESndTagList *pObj2ClassList, *pObj2MaterialList;
   cTagSet *pObj1ClassTags = 0, *pObj1MaterialTags = 0, *pRoomTags = 0;
   cTagSet *pObj2ClassTags = 0, *pObj2MaterialTags = 0;

   int retval = SCH_HANDLE_NULL;        // return this if we fail

   if (Obj1 != OBJ_NULL) {
      if (ObjGetESndClass(Obj1, &pObj1ClassList))
         pObj1ClassTags = new cTagSet(*pObj1ClassList->m_pTagSet); 
      if (ObjGetESndMaterial(Obj1, &pObj1MaterialList))
         pObj1MaterialTags = new cTagSet(*pObj1MaterialList->m_pTagSet);

      cRoom *Obj1Room = g_pRooms->GetObjRoom(Obj1);
      if (Obj1Room && ObjGetESndClass(Obj1Room->GetObjID(), &pRoomList))
         pRoomTags = new cTagSet(*pRoomList->m_pTagSet);
   }

   if (Obj2 != OBJ_NULL) {
      if (ObjGetESndClass(Obj2, &pObj2ClassList))
         pObj2ClassTags = new cTagSet(*pObj2ClassList->m_pTagSet);
      if (ObjGetESndMaterial(Obj2, &pObj2MaterialList))
         pObj2MaterialTags = new cTagSet(*pObj2MaterialList->m_pTagSet);
   }

   if (ESndGameCallback)
   {
      sESndEvent ev = 
        { Obj1, Obj2, 
          {
             pObj1ClassTags, 
             pObj1MaterialTags, 
             pObj2ClassTags,
             pObj2MaterialTags,
             pRoomTags,
             pTagSet
          },
          pCallParams,
        };
                      
      ESndGameCallback(&ev); 
   }

#ifndef SHIP
   if (config_is_defined("EnvSoundSpew")) {
      DumpTagSet(pTagSet);
      DumpTagSet(pObj1ClassTags);
      DumpTagSet(pObj1MaterialTags);
      DumpTagSet(pRoomTags);
      DumpTagSet(pObj2ClassTags);
      DumpTagSet(pObj2MaterialTags);
   }
#endif // ~SHIP

   cTagDBInput Input;
   Input.AppendTagSet(pTagSet, 0, 1.0);
   if (pObj1ClassTags)
      Input.AppendTagSet(pObj1ClassTags, 0, 1.0);
   if (pObj1MaterialTags)
      Input.AppendTagSet(pObj1MaterialTags, 0, 1.0);
   if (pRoomTags)
      Input.AppendTagSet(pRoomTags, 0, 1.0);
   if (pObj2ClassTags)
      Input.AppendTagSet(pObj2ClassTags, 0, 1.0);
   if (pObj2MaterialTags)
      Input.AppendTagSet(pObj2MaterialTags, 0, 1.0);

   Input.Sort();

   int iInSize = Input.Size();
   cTagDBInputTag *pInPointer = Input.AsPointer();
   for (int i = 0; i < iInSize; ++pInPointer, ++i)
      // Must check size here because set is only as big as the largest
      // numbered tag that IS required, not the largest numbered tag
      // that we could ask IF IT IS required.
      if ((pInPointer->m_KeyType < g_pTagRequired->Size()) &&
             (g_pTagRequired->IsSet(pInPointer->m_KeyType)))
         pInPointer->m_iFlags |= kTagDBInputFlagRequired;

   ObjID SchemaID = OBJ_NULL; 
   cTagDBOutput Output;
   Output.Clear();
   if (!g_pDatabase->MatchPartialSet(&Input, &Output))
   {
      #ifndef SHIP
      if (config_is_defined("EnvSoundSpew"))
         mprintf("ESndPlayCore:No Partial Set Match\n");
      #endif

      goto out;
   }

   Output.NextByWeight(0);
   SchemaID = Output[0].m_iData;

   if (SchemaID == OBJ_NULL) {
      #ifndef SHIP
      if (config_is_defined("EnvSoundSpew"))
         mprintf("ESndPlayCore: No schemas found!\n");
      #endif

      goto out; 
   }

   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("ESndPlayCore: playing %s\n", ObjEditName(SchemaID));
   #endif

   // start our schema
   pCallParams->sampleNum = -1;

   // does this do the job?
   pCallParams->sourceID = Obj1;

   retval = SchemaIDPlay(SchemaID, pCallParams, pData);

out:
   delete pObj1ClassTags;
   delete pObj2ClassTags;
   delete pRoomTags;
   delete pObj1MaterialTags;
   delete pObj2MaterialTags;
   return retval; 
}


int ESndPlay(cTagSet *pTagSet, ObjID Obj1, ObjID Obj2, 
             sSchemaCallParams *pParams, void *pData)
{
   sSchemaCallParams Params, *pUsedParams;

   if (pParams)
      pUsedParams = pParams;
   else {
      Params = g_sDefaultSchemaCallParams;
      pUsedParams = &Params;
   }

   return ESndPlayCore(pTagSet, Obj1, Obj2, pData, pUsedParams);
}


// treat Obj1 as the source
int ESndPlayObj(cTagSet *pTagSet, ObjID Obj1, ObjID Obj2,
                sSchemaCallParams *pParams, void *pData)
{
   sSchemaCallParams Params, *pUsedParams;

   if (pParams)
      pUsedParams = pParams;
   else {
      Params = g_sDefaultSchemaCallParams;
      pUsedParams = &Params;
   }

   pUsedParams->flags |= SCH_SET_OBJ;
   pUsedParams->sourceID = Obj1;

   return ESndPlayCore(pTagSet, Obj1, Obj2, pData, pUsedParams);
}


int ESndPlayLoc(cTagSet *pTagSet,  ObjID Obj1, ObjID Obj2, 
                const mxs_vector *pLoc, sSchemaCallParams *pParams, 
                void *pData)
{
   sSchemaCallParams Params, *pUsedParams;

   if (pParams)
      pUsedParams = pParams;
   else {
      Params = g_sDefaultSchemaCallParams;
      pUsedParams = &Params;
   }

   pUsedParams->flags |= SCH_SET_LOC;
   pUsedParams->pSourceLoc = (mxs_vector *)pLoc;  // why isn't source loc a const?

   return ESndPlayCore(pTagSet, Obj1, Obj2, pData, pUsedParams);
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   This section is our loading and saving.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

static TagFileTag g_TagFileTag = { "ENV_SOUND" };
static TagVersion g_TagVersion = { 1, 1 };


static void ESndLoad(ITagFile *pFile)
{
   TagVersion found_version = g_TagVersion;
   HRESULT result = ITagFile_OpenBlock(pFile, &g_TagFileTag, &found_version);

   if (result == S_OK
    && found_version.major == g_TagVersion.major
    && found_version.minor == g_TagVersion.minor) {
      int temp;
      ITagFile_Read(pFile, (char *) &temp, sizeof(temp));
      g_LocalTagRequired.SetSize(temp);
      ITagFile_Read(pFile, (char *) g_LocalTagRequired.AsPointer(), temp);

      g_pDatabase->Load(pFile);
      ITagFile_CloseBlock(pFile);
   }
}


static void ESndSave(ITagFile *pFile)
{
   HRESULT result = ITagFile_OpenBlock(pFile, &g_TagFileTag, &g_TagVersion);

   if (result == S_OK) {
      int temp = g_LocalTagRequired.Size();
      ITagFile_Write(pFile, (const char *) &temp, sizeof(temp));
      ITagFile_Write(pFile,
                     (const char *) g_LocalTagRequired.AsPointer(), temp);

      g_pDatabase->Save(pFile);
      ITagFile_CloseBlock(pFile);
   }
}


extern "C" void ESndInit(void)
{
   if (!pPropMan)
      pPropMan = AppGetObj(IPropertyManager);
   if (!pTraitMan)
      pTraitMan = AppGetObj(ITraitManager);
   if (!pRecorder)
      pRecorder = AppGetObj(IRecorder);

   ESndPropsInit();

   g_pDatabase = NewITagDBDatabase();
   g_pInputKeySet = NewITagDBKeySet();

   EnvSoundReactionInit();
}


extern "C" void ESndTerm(void)
{
   SafeRelease(pPropMan);
   SafeRelease(pTraitMan);
   SafeRelease(pRecorder);
   if (g_pTagRequired)
      delete g_pTagRequired;

   delete g_pDatabase;
}

#ifndef SHIP
void ESndDump() {
   g_pDatabase->Dump();
}
#endif

// Our required bits--telling us which tags to require in our
// searches--do not represent a new name space.  So we only need to
// swizzle in one direction, from local to global.
static void FindRequiredBits()
{
   int i;
   int iHighestRemappedToken = 0;
   int iSize = g_LocalTagRequired.Size();
   for (i = 0; i < iSize; ++i) {
      if (g_LocalTagRequired[i]) {
         sTagRemap Remap(i, 0, 0, 0);
         g_Domain.TokensLocalToGlobal(&Remap, TRUE);
         if (Remap.m_GlobalType > iHighestRemappedToken)
            iHighestRemappedToken = Remap.m_GlobalType;
      }
   }

   if (g_pTagRequired)
      delete g_pTagRequired;

   g_pTagRequired  = new cPackedBoolSet(iHighestRemappedToken + 1);

   for (i = 0; i < iSize; ++i) {
      if (g_LocalTagRequired[i]) {
         sTagRemap Remap(i, 0, 0, 0);
         g_Domain.TokensLocalToGlobal(&Remap, TRUE);
         g_pTagRequired->Set(Remap.m_GlobalType);
      }
   }
}


static void RemapDatabase(void Func(cTagDBKey *))
{
   ITagDBDatabase *pTemp = g_pDatabase->CopyWithFilter(Func);
   delete g_pDatabase;
   g_pDatabase = pTemp;
}


EXTERN void ESndDatabaseNotify(DispatchData *msg)
{
   msgDatabaseData data;

   data.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         break;

      case kDatabaseDefault:
         ESndDestroy();
         break;

      case kDatabaseSave:
         if (msg->subtype & kObjPartAbstract) {
            RemapDatabase(SpeechGlobalToLocal);
            ESndSave(data.save);
            RemapDatabase(SpeechLocalToGlobal);
         }
         break;

      case kDatabaseLoad:
         if (msg->subtype & kObjPartAbstract) {
            ESndDestroy();
            ESndLoad(data.load);
         }
         break;

      case kDatabasePostLoad:
         if (msg->subtype & kObjPartAbstract) {
            RemapDatabase(SpeechLocalToGlobal);
            FindRequiredBits();
         }
         break;
   }
}


void ESndNotifyTextParseEnd()
{
   FindRequiredBits();
}


// clear out all this tag and concept stuff
static void ESndDatabasesDestroy(void)
{
   g_pDatabase->Clear();
   g_LocalTagRequired.SetSize(0);
}


// Destroy all voice data structures and objects (except base archetypes)
void ESndDestroy(void)
{
   ITraitManager *pTraitMan = AppGetObj(ITraitManager);

   // clean up all relations involving voices or concepts
   ESndDatabasesDestroy();

   SafeRelease(pTraitMan);
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   Here's our text file reading.  See schema.y for the other end of
   this.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

extern "C" void ESndSetTagRequired(const Label *pTagName)
{
   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("ESndSetTagRequired: %s\n", &pTagName->text);
   #endif

   int iTagIndex = g_Domain.TagIndex(pTagName);
   if (iTagIndex == kNameMapIndexNotFound) {
      Warning(("ESndSetTagRequired: not found: %s\n", pTagName->text));
      return;
   }

   if (g_LocalTagRequired.Size() <= iTagIndex) {
      int iOldSize = g_LocalTagRequired.Size();
      g_LocalTagRequired.SetSize(iTagIndex + 1);
      for (int i = iOldSize; i < iTagIndex; ++i)
         g_LocalTagRequired[i] = 0;
   }

   g_LocalTagRequired[iTagIndex] = 1;
}


extern "C" void ESndSchemaNewStart(const Label *pSchemaName, ObjID SchemaObjID)
{
   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("ESndSchemaNewStart: %s\n", &pSchemaName->text);
   #endif

   g_pInputKeySet->Clear();
   g_InputKey.m_iData = SchemaObjID;
   g_InputKey.m_fWeight = 1.0;
}


extern "C" void ESndSchemaNewAddIntTag(const Label *pTagName,
                                       int iMin, int iMax)
{
   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("ESndSchemaNewAddIntTag: %s [%d %d]\n", &pTagName->text, iMin, iMax);
   #endif

   cTagDBKey NewKey(g_Domain.TagIndex(pTagName), iMin, iMax);
   SpeechLocalToGlobal(&NewKey);
   g_pInputKeySet->AddKey(&NewKey);
}


// pEnum here is expected to be an array of eight const labels.
extern "C" void ESndSchemaNewAddEnumTag(const Label *pTagName,
                                        const Label *pEnum)
{
   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("ESndSchemaNewAddEnumTag: %s [", &pTagName->text);
   #endif

   tTagDBKeyEnum aEnum[8];

   // force type-value pairs into existance in tag system
   for (int i = 0; i < 8; ++pEnum, ++i) {
      if (pEnum->text[0]) {
         aEnum[i] = (uchar) g_Domain.ValueIndex(pEnum);
         ConfigSpew("EnvSoundSpew", ("%s (%d)", pEnum->text,aEnum[i]));
      } else {
         aEnum[i] = kTagDBKeyEnumUnused;
      }
   }

   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("]\n");
   #endif

   cTagDBKey NewKey(g_Domain.TagIndex(pTagName), aEnum);
   SpeechLocalToGlobal(&NewKey);
   g_pInputKeySet->AddKey(&NewKey);
}


extern "C" void ESndSchemaNewFinish(void)
{
   #ifndef SHIP
   if (config_is_defined("EnvSoundSpew"))
      mprintf("ESndSchemaNewFinish\n");
   #endif

   g_pDatabase->Insert(g_pInputKeySet, g_InputKey);
}







