/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/speech.cpp,v 1.61 2000/02/08 23:10:40 patmac Exp $

// This is the procedural core of spch.  No classes are defined here.
//
// Some of this is exposed in spchglue.h, which is a C interface for
// the YACC/LEX schema parser.  The rest is strictly C++.

#include <lgassert.h>
#include <dynarray.h>
#include <mprintf.h>
#include <comtools.h>

#include <timer.h>
#include <stdlib.h>
#include <cfgdbg.h>
#include <matrix.h>

#include <recapi.h>

#include <linkbase.h>
#include <relation.h>
#include <traitman.h>
#include <appagg.h>
#include <iobjsys.h>
#include <linkman.h>
#include <lnkquery.h>
#include <propman.h>
#include <objquery.h>
#include <objremap.h>
#include <traitbas.h>
#include <edittool.h>
#include <wrtype.h>
#include <objpos.h>

#include <command.h>
#include <playrobj.h>

#include <speech.h>
#include <spchglue.h>
#include <spchprop.h>
#include <spchbase.h>

#include <schbase.h>
#include <schema.h>
#include <schprop.h>
#include <schsamps.h>

#include <tag.h>

#include <dbasemsg.h>
#include <dispbase.h>

#include <hashfast.h>
#include <dlist.h>
#include <dlisttem.h>

#include <ctag.h>
#include <namemap.h>
#include <spchdom.h>
#include <spchvoc.h>
#include <tagdbin.h>
#include <tagdbout.h>
#include <tagdbt.h>
#include <playrobj.h>

#include <config.h>
#include <cfgdbg.h>

#include <aidebug.h>
#include <aiprutil.h>
#include <aisndtyp.h>

#include <netman.h>

#include <hashpp.h>
#include <hshpptem.h>

// Must be last header
#include <dbmem.h>

// This is for tracking our load/save status...
BOOL g_bSpeechDatabaseLoaded = FALSE;

static IObjectSystem *pObjSys = NULL;
static ILinkManager *pLinkMan = NULL;
static IPropertyManager *pPropMan = NULL;
static ISpeechProperty *pSpeechProperty = NULL;
static ITraitManager *pTraitMan = NULL;
static IRecorder *pRecorder = NULL;

static RelationID voiceRelationID = RELID_NULL;
static IRelation *pVoiceRelation = NULL;


static int schemasNum;
#define SPEECH_SCHEMAS_MAX 10 // max schemas/concept-voice pair
static float schemaScores[SPEECH_SCHEMAS_MAX];

// @TBD (toml 08-03-98): if ais are to communicate through sound propogation, this test will
// have to move deeper and only be used for schemas with no associated semantic value

// min distance at which an AI will speak, squared - this will need
// tweaking
static float kAISpeechMinDist2 = 100.0 * 100.0;

cSpeechDomain g_Domain;
static int g_iNewestTag;
static Label g_NewestTagName;
static cSpeechVoice *g_pCurrentVoice;


// Each voice archetype has an index into this structure.
static cDynArray<cSpeechVoice *> g_VoiceList;

// NOTE: if kCallbackEntryMax is not 1, a crash occurs at the end of a conversation
//   inside SpeechEndCallback, because the first callback causes the speechEndCallbackHash
//   to delete the callback function array, so the second callback ptr is a bullshit ptr - patmc
//#define kCallbackEntryMax 4
#define kCallbackEntryMax 1

struct sCallbackHashEntry : public sHashFastEntry
{
   sCallbackHashEntry(ObjID obj)
      : m_obj(obj)        
   {
      memset(m_aCallbackList, 0, sizeof(m_aCallbackList));
   }

   inline ObjID GetKey()
   {
      return m_obj;
   }

   void AddCB(SpeechCallbackFn pFN)
   {
      int i;

      // only allow each callback to be registered once
      for (i = 0; i < kCallbackEntryMax; ++i)
         if (m_aCallbackList[i] == pFN)
            return;

      for (i = 0; i < kCallbackEntryMax; ++i)
         if (!m_aCallbackList[i]) {
            m_aCallbackList[i] = pFN;
            break;
         }

      Assert_(i < kCallbackEntryMax);
   }

   // This returns FALSE if this entry should be deleted (no callbacks
   // left).  The order of the callbacks may change.
   BOOL RemoveCB(SpeechCallbackFn pFN)
   {
      int i, j;
      for (i = 0; i < kCallbackEntryMax; ++i) {
         if (m_aCallbackList[i] == pFN) {
            if (i == kCallbackEntryMax - 1) {
               m_aCallbackList[kCallbackEntryMax - 1] = 0;
               return TRUE;
            } else {
               // Is the list empty now?
               if (i == 0 && m_aCallbackList[1] == 0)
                  return FALSE;

               // We swap down the last callback in the list.
               for (j = i + 1; j < kCallbackEntryMax; ++j)
                  if (!m_aCallbackList[j])
                     break;
               m_aCallbackList[i] = m_aCallbackList[j - 1];
               m_aCallbackList[j - 1] = 0;
               return TRUE;
            }
         }
      }

      // not found (considered harmless)
      return TRUE;
   }

   // data
   ObjID m_obj;
   SpeechCallbackFn m_aCallbackList[kCallbackEntryMax];
};


// Callback hash tables
static cHashFast<64, ObjID, sCallbackHashEntry> g_speechStartCallbackHash;
static cHashFast<64, ObjID, sCallbackHashEntry> g_speechEndCallbackHash;


//////////////////////////////////////////

void SpeechInstallStartCallback(ObjID speakerID, SpeechCallbackFn callback)
{
   sCallbackHashEntry *pEntry = g_speechStartCallbackHash.Search(speakerID);
   if (!pEntry) {
      pEntry = new sCallbackHashEntry(speakerID);
      g_speechStartCallbackHash.Insert(pEntry);
   }

   pEntry->AddCB(callback);
}

//////////////////////////////////////////

void SpeechUninstallStartCallback(ObjID speakerID, SpeechCallbackFn callback)
{
   sCallbackHashEntry *pEntry = g_speechStartCallbackHash.Search(speakerID);
   if (pEntry)
      if (!pEntry->RemoveCB(callback)) {
         g_speechStartCallbackHash.Remove(speakerID);
         delete pEntry;
      }
}

//////////////////////////////////////////

void SpeechInstallEndCallback(ObjID speakerID, SpeechCallbackFn callback)
{
   sCallbackHashEntry *pEntry = g_speechEndCallbackHash.Search(speakerID);
   if (!pEntry) {
      pEntry = new sCallbackHashEntry(speakerID);
      g_speechEndCallbackHash.Insert(pEntry);
   }

   pEntry->AddCB(callback);
}

//////////////////////////////////////////

void SpeechUninstallEndCallback(ObjID speakerID, SpeechCallbackFn callback)
{
   sCallbackHashEntry *pEntry = g_speechEndCallbackHash.Search(speakerID);
   if (pEntry)
      if (!pEntry->RemoveCB(callback)) {
         g_speechEndCallbackHash.Remove(speakerID);
         delete pEntry;
      }
}

//////////////////////////////////////////

int SpeechVoiceIndexFromName(const Label *pVoiceName)
{
   // get our object...
   ObjID obj = pObjSys->GetObjectNamed(pVoiceName->text);
   int iIndex;

   AssertMsg1(obj != OBJ_NULL,
              "Request for nonexistent voice: %s\n", pVoiceName);

   bool bHasIndex = ObjGetSpeechVoiceIndex(obj, &iIndex);

   AssertMsg1(bHasIndex == TRUE,
              "voice %s has no VoiceIndex.\n", pVoiceName);

   return iIndex;
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   These functions handle connections to the schema system, including
   playing the actual sounds.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

// somewhere, a schema has ended
void SpeechEndCallback(int hSchema, ObjID schemaID, void *data)
{
   sSpeech *pSpeech;

   if (pSpeechProperty->Get((ObjID)data, &pSpeech))
      pSpeech->flags &= ~SPEECH_SPEAKING;

   sCallbackHashEntry *pEntry = g_speechEndCallbackHash.Search((ObjID)data);
   if (pEntry)
      for (int i = 0; i < kCallbackEntryMax; ++i)
         if (pEntry->m_aCallbackList[i])
            pEntry->m_aCallbackList[i]((ObjID)data, hSchema, schemaID);
         else
            break;
}


void SpeechHalt(ObjID AIObjID)
{
   sSpeech *pSpeech;

   if (pSpeechProperty->Get(AIObjID, &pSpeech))
   {
      if (pSpeech->flags & SPEECH_SPEAKING)
         SchemaPlayHalt(pSpeech->hSchemaPlay);
      pSpeechProperty->Delete(AIObjID);
   }
}

// return TRUE if AI is close enough to some player to be heard,
// FALSE otherwise
BOOL SpeechInRange(ObjID AIObjID)
{
#ifdef NEW_NETWORK_ENABLED
   // Check whether *any* player is in range of this AI:
   AutoAppIPtr(NetManager);
   ObjID player;
   Position *pAIPos = ObjPosGet(AIObjID);
   FOR_ALL_PLAYERS(pNetManager, &player) {
      Position *pPlayerPos = ObjPosGet(player);

      if (mx_dist2_vec(&pAIPos->loc.vec, &pPlayerPos->loc.vec)
          < kAISpeechMinDist2)
         return TRUE;
   }
   return FALSE;
#else
   // Non-networked version just worries about the single player:
   Position *pPlayerPos = ObjPosGet(PlayerObject());
   Position *pAIPos = ObjPosGet(AIObjID);

   if (mx_dist2_vec(&pAIPos->loc.vec, &pPlayerPos->loc.vec)
     < kAISpeechMinDist2)
      return TRUE;
   else
      return FALSE;
#endif
}


// find and return the minimum distance from any object linked to the
// given voice to the AI.  FLT_MAX if no object so linked
float SpeakerMinDist(ObjID voiceID, ObjID AIObjID)
{
   ILinkQuery *pQuery = pVoiceRelation->Query(LINKOBJ_WILDCARD, voiceID);
   float minDist = 999999999.0;
   float dist;
   sLink link;

   Position *pObjPos;
   Position *pAIPos = ObjPosGet(AIObjID);

   while (!pQuery->Done())
   {
      pQuery->Link(&link);
      if (!(pTraitMan->IsArchetype(link.source))) {
         pObjPos = ObjPosGet(link.source);
         dist = mx_dist2_vec(&pAIPos->loc.vec, &pObjPos->loc.vec);
         if (dist < minDist)
            minDist = dist;
      }
      pQuery->Next();
   }
   SafeRelease(pQuery);
   return minDist;
}


ObjID SpeechChooseDescendent(ObjID objID, ObjID AIObjID)
{
   ObjID voiceID;
   IObjectQuery *pQuery = pTraitMan->Query(objID, kTraitQueryAllDescendents);
   float maxDist = 0;
   float voiceMinDist;
   ObjID curID;

   if (pQuery->Done())
      // no descendents, just return the current ID
      voiceID = objID;
   else {
      // choose descentdent by finding the voice that is not in use near
      // the AI. If this is not the terminal level, choice will be random
      while (!pQuery->Done()) {
         curID = pQuery->Object();
         voiceMinDist = SpeakerMinDist(curID, AIObjID);
         if (voiceMinDist > maxDist) {
            maxDist = voiceMinDist;
            voiceID = curID;
         }
         pQuery->Next();
      }
      // recurse to find best voice from current choice
      SpeechChooseDescendent(voiceID, AIObjID);
   }
   SafeRelease(pQuery);
   return voiceID;
}


// Get an object's voice, OBJ_NULL if none, first voice if multiple
// create a link to voice if none exists
ObjID SpeechGetVoice(ObjID AIObjID)
{
   ILinkQuery *pQuery;
   ObjID voiceID = OBJ_NULL;
   sLink link;

   pQuery = pLinkMan->Query(AIObjID, LINKOBJ_WILDCARD, voiceRelationID);
   if (!pQuery->Done()) {

      // we've got a link to a voice
      pQuery->Link(&link);
      voiceID = link.dest;
#ifdef DBG_ON
      pQuery->Next();
      if (!pQuery->Done())
         ConfigSpew("SpeechSpew", ("SpeechGetVoice: object %d has more"
                                   " than one voice\n"));
#endif
   }
   SafeRelease(pQuery);

   if (voiceID == OBJ_NULL) {
      Label *pVoiceLabel;

      // check for voice name property
      if (NULL!=(pVoiceLabel = ObjGetSpeechVoice(AIObjID))) {
         voiceID = pObjSys->GetObjectNamed(pVoiceLabel->text);
         if (voiceID != OBJ_NULL) {
            voiceID = SpeechChooseDescendent(voiceID, AIObjID);

            // add a link for next time
            pVoiceRelation->Add(AIObjID, voiceID);
         } else {
            ConfigSpew("SpeechSpew", 
                       ("SpeechGetVoice: voice %s not found for obj %d\n",
                        &(pVoiceLabel->text[0]), AIObjID));
            voiceID = OBJ_NULL;
         }
      } else {
         ConfigSpew("SpeechSpew", 
                    ("SpeechGetVoice: obj %d has no voice property\n",
                     AIObjID));
         voiceID = OBJ_NULL;
      }
   }
   return voiceID;
}


int SpeechSpeak(ObjID SpeakerObjID, const Label *pConcept,
                cTagDBInput *pInput, void *pData)
{
   if (!g_bSpeechDatabaseLoaded)
   {
      AIWatch(Sound, SpeakerObjID, "Not speaking: No speech database");
      return SCH_HANDLE_NULL;
   }

   ObjID VoiceObjID = SpeechGetVoice(SpeakerObjID);
   int iConceptIndex = g_Domain.ConceptIndex(pConcept);

   if (iConceptIndex == kNameMapIndexNotFound)
   {
      AIWatch(Sound, SpeakerObjID, "Not speaking: Unknown voice");
      return SCH_HANDLE_NULL;
   }

   int iConceptPriority = g_Domain.FindConceptPriority(iConceptIndex);

   ConfigSpew("SpeechSpew", ("Object %d is saying %s\n", SpeakerObjID,
                             pConcept->text));

   int iVoiceIndex;
   if (!ObjGetSpeechVoiceIndex(VoiceObjID, &iVoiceIndex))
   {
      AIWatch(Sound, SpeakerObjID, "Not speaking: Unknown voice");
      return SCH_HANDLE_NULL;
   }

   sSpeech *pSpeech;
   sSchemaCallParams sCallParams;
   int iSchemaHandle;

   int iSpeechTime = tm_get_millisec_unrecorded();

   // This is bugger already saying something?
   BOOL bStart = !(pSpeechProperty->Get(SpeakerObjID, &pSpeech)
                && (pSpeech->flags & SPEECH_SPEAKING)
                && pSpeech->m_iPriority >= iConceptPriority);

   RecStreamAddOrExtract(pRecorder, &bStart, sizeof(BOOL), "speech start");

   if (!bStart)
   {
      AIWatch(Sound, SpeakerObjID, "Not speaking: Current sound has priority");
      return SCH_HANDLE_NULL;
   }

   cTagDBOutput Output;
   g_VoiceList[iVoiceIndex]->SelectSchemas(pInput, iConceptIndex, &Output);
   ObjID SchemaID = Output.Choose();

   if (SchemaID == OBJ_NULL) 
   {
      ConfigSpew("SpeechSpew", ("Speech: No schemas found!\n"));
      AIWatch(Sound, SpeakerObjID, "Not speaking: No schemas found");
      return SCH_HANDLE_NULL;
   }

   sAISoundType * pSoundType = AIGetSoundType(SpeakerObjID);
   
   if ((!pSoundType || (!pSoundType->type && !pSoundType->szSignal[0])) &&
       !SpeechInRange(SpeakerObjID))
   {
      AIWatch(Sound, SpeakerObjID, "Not speaking: Too far from player and sound has no meaning");
      return SCH_HANDLE_NULL;
   }

   // Kill the old one, make a new one
   SpeechHalt(SpeakerObjID);

   pSpeech = SpeechPropCreateAndGet(SpeakerObjID);
   if (!pSpeech)
   {
      AIWatch(Sound, SpeakerObjID, "Not speaking: Failed to create speech property");
      return SCH_HANDLE_NULL;
   }

#ifndef SHIP
   AutoAppIPtr_(EditTools, pTools);
   ConfigSpew("SpeechSpew", ("SpeechSpeakExecute: %s\n", 
                             pTools->ObjName(SchemaID)));
#endif // ~SHIP

   // start our schema
   sCallParams.flags = SCH_SET_SAMPLE | SCH_SET_OBJ | SCH_SET_CALLBACK;
   sCallParams.sampleNum = -1;
   sCallParams.sourceID = SpeakerObjID;
   sCallParams.callback = SpeechEndCallback;

   sCallParams.pData = (void*)SpeakerObjID;

   iSchemaHandle = SchemaIDPlay(SchemaID, &sCallParams, pData);
   pSpeech->hSchemaPlay = iSchemaHandle;

   if (iSchemaHandle != SCH_HANDLE_NULL) 
   {
      pSpeech->flags |= SPEECH_SPEAKING;
      pSpeech->time = iSpeechTime;
      pSpeech->schemaID = SchemaID;
      pSpeech->m_iConceptIndex = iConceptIndex;
      pSpeech->m_iPriority = iConceptPriority;
   }

   pSpeechProperty->Set(SpeakerObjID, pSpeech);

   sCallbackHashEntry *pEntry = g_speechStartCallbackHash.Search(SpeakerObjID);
   if (pEntry)
      for (int i = 0; i < kCallbackEntryMax; ++i)
         if (pEntry->m_aCallbackList[i])
            pEntry->m_aCallbackList[i](SpeakerObjID, iSchemaHandle, SchemaID);
         else
            break;

   return iSchemaHandle;
}


BOOL SpeechIsSpeaking(ObjID SpeakerObjID)
{
   sSpeech *pSpeech;
   return (pSpeechProperty->Get(SpeakerObjID, &pSpeech)
        && (pSpeech->flags & SPEECH_SPEAKING));
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   This section is our loading and saving.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */

static TagFileTag g_TagFileTag = { "Speech_DB" };
static TagVersion g_TagVersion = { 1, 3 };


// After we load each voice, we have it apply this to all of its data
// to swizzle it to the current ObjIDs.
static void SwizzleTag(sTagDBData *pData)
{
   pData->m_iData = ObjRemapOnLoad(pData->m_iData);
}


// After we've loaded in our binary data we swizzle it to cTag
// indices, generating the necessary tags as we go.
void SpeechLocalToGlobal(cTagDBKey *pKey)
{
   sTagRemap Remap(pKey->m_KeyType, 0, 0, 0);

   if (g_Domain.FindTagFlags(pKey->m_KeyType) & kTagFlagInt)
      g_Domain.TokensLocalToGlobal(&Remap, TRUE);
   else
      for (int i = 0; i < 8 && pKey->m_aEnum[i] != kTagDBKeyEnumUnused; ++i) {
         Remap.m_LocalValue = pKey->m_aEnum[i];
         g_Domain.TokensLocalToGlobal(&Remap, FALSE);
         pKey->m_aEnum[i] = Remap.m_GlobalValue;
      }

   ConfigSpew(("SpeechSpew"),
              ("Swizzled tag %s from %d to %d\n",
               g_Domain.TagName(pKey->m_KeyType)->text,
               Remap.m_LocalType, Remap.m_GlobalType));

   pKey->m_KeyType = Remap.m_GlobalType;
}



// This is the companion function to SpeechNameToTag.  Before we save
// off our database we put all its data in terms of the local mapping
// we'll be saving.
void SpeechGlobalToLocal(cTagDBKey *pKey)
{
   sTagRemap Remap(0, 0, pKey->m_KeyType, 0);
   g_Domain.TokensGlobalToLocal(&Remap, TRUE);

   if (g_Domain.FindTagFlags(Remap.m_LocalType) & kTagFlagInt)
      g_Domain.TokensGlobalToLocal(&Remap, TRUE);
   else
      for (int i = 0; i < 8 && pKey->m_aEnum[i] != kTagDBKeyEnumUnused; ++i) {
         Remap.m_GlobalValue = pKey->m_aEnum[i];
         g_Domain.TokensGlobalToLocal(&Remap, FALSE);
         pKey->m_aEnum[i] = Remap.m_LocalValue;
      }

   pKey->m_KeyType = Remap.m_LocalType;

   ConfigSpew(("SpeechSpew"),
              ("Key %s from %d to %d\n",
               g_Domain.TagName(pKey->m_KeyType)->text,
               Remap.m_GlobalType, Remap.m_LocalType));
}


static void SwizzleDatabases()
{
   int iSize = g_VoiceList.Size();

   for (int i = 0; i < iSize; ++i) {
      g_VoiceList[i]->TransformData(SwizzleTag);
      g_VoiceList[i]->TransformKeys(SpeechLocalToGlobal);
   }
}


static void UnswizzleDatabases()
{
   int iSize = g_VoiceList.Size();

   for (int i = 0; i < iSize; ++i)
      g_VoiceList[i]->TransformKeys(SpeechGlobalToLocal);
}


// We do not save the request registry.  The requests have to be
// rebuilt every time we initialize our app.
static void SpeechLoad(ITagFile *pFile)
{
   TagVersion found_version = g_TagVersion;
   HRESULT result = ITagFile_OpenBlock(pFile, &g_TagFileTag, &found_version);

   if (result == S_OK
    && found_version.major == g_TagVersion.major
    && found_version.minor == g_TagVersion.minor) {
      int i, iTemp;

      g_Domain.Load(pFile);

      ITagFile_Read(pFile, (char *) &iTemp, sizeof(iTemp));
      g_VoiceList.SetSize(iTemp);
      for (i = 0; i < iTemp; ++i) {
         g_VoiceList[i] = new cSpeechVoice();
         g_VoiceList[i]->Load(pFile);
      }
      g_bSpeechDatabaseLoaded = TRUE;
   }

   ITagFile_CloseBlock(pFile);
}


static void SpeechSave(ITagFile *pFile)
{
   if (!g_bSpeechDatabaseLoaded || g_VoiceList.Size() == 0)
      return;

   HRESULT result = ITagFile_OpenBlock(pFile, &g_TagFileTag, &g_TagVersion);

   if (result == S_OK) {
      int i, iTemp;

      g_Domain.Save(pFile);

      iTemp = g_VoiceList.Size();
      ITagFile_Write(pFile, (char *) &iTemp, sizeof(iTemp));
      for (i = 0; i < iTemp; ++i)
         g_VoiceList[i]->Save(pFile);
   }

   ITagFile_CloseBlock(pFile);
}


static void MultiplyWeight(sTagDBData *pData)
{
   pData->m_fWeight *= SchemaSamplesNum(pData->m_iData);
}


void SpeechNotifyTextParseEnd()
{
   for (int i = 0; i < g_VoiceList.Size(); ++i) {
      g_VoiceList[i]->TransformData(MultiplyWeight);
      g_VoiceList[i]->TransformKeys(SpeechLocalToGlobal);
   }

   g_bSpeechDatabaseLoaded = TRUE;
}


// Voice relation
struct sRelationDesc sVoiceDesc =
{
   VOICE_RELATION, kRelationNetworkLocalOnly,
};

struct sRelationDataDesc sVoiceDataDesc = LINK_NO_DATA;


void SpeechBuildDefault(void)
{
   ITraitManager *pTraitMan = AppGetObj(ITraitManager);
   ObjID baseVoiceID;

   // create the base voice
   pTraitMan->CreateBaseArchetype(BASE_VOICE_OBJ, &baseVoiceID);

   SafeRelease(pTraitMan);
}

extern "C" void SpeechInit(void)
{
   SpeechPropsInit();

   if (!pObjSys)
      pObjSys = AppGetObj(IObjectSystem);
   if (!pLinkMan)
      pLinkMan = AppGetObj(ILinkManager);
   if (!pPropMan)
      pPropMan = AppGetObj(IPropertyManager);
   if (!pTraitMan)
      pTraitMan = AppGetObj(ITraitManager);
   if (!pRecorder)
      pRecorder = AppGetObj(IRecorder);
   if (!pSpeechProperty)
      pSpeechProperty
         = (ISpeechProperty*)pPropMan->GetPropertyNamed(PROP_SPEECH);

   // create relations
   pVoiceRelation = CreateStandardRelation(&sVoiceDesc,
                                           &sVoiceDataDesc,
                                           kQCaseSetSourceKnown);
   voiceRelationID = pVoiceRelation->GetID();
}


extern "C" void SpeechTerm(void)
{
   SafeRelease(pObjSys);
   SafeRelease(pLinkMan);
   SafeRelease(pPropMan);
   SafeRelease(pTraitMan);
   SafeRelease(pRecorder);
   SafeRelease(pSpeechProperty);
}


/* Deal with database events--load and save here refer to tag files,
   not the text files we read in to build our databases.

*/
EXTERN void SpeechDatabaseNotify(DispatchData *msg)
{
   msgDatabaseData data;

   data.raw = msg->data;
   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         SpeechDestroy();
         break;

      case kDatabaseDefault:
         SpeechBuildDefault();
         break;

      case kDatabaseSave:
         if (msg->subtype & kObjPartAbstract) {
            UnswizzleDatabases();
            SpeechSave(data.save);
            SwizzleDatabases();
         }
         break;

      case kDatabaseLoad:
         if (msg->subtype & kObjPartAbstract) {
            SpeechLoad(data.load);
         }
         break;

      case kDatabasePostLoad:
         if (msg->subtype & kObjPartAbstract) {
            SpeechBuildDefault();
            SwizzleDatabases();
         }
         break;
   }
}


// Destroy all voice relations
void SpeechVoiceRelationsDestroy(void)
{
   ILinkQuery *pQuery;

   // destroy voice links (AI<->voice)
   pQuery
      = pLinkMan->Query(LINKOBJ_WILDCARD, LINKOBJ_WILDCARD, voiceRelationID);
   while (!pQuery->Done())
   {
      pLinkMan->Remove(pQuery->ID());
      pQuery->Next();
   }
   SafeRelease(pQuery);
}

// Destroy all expression and voice relations
void SpeechRelationsDestroy(void)
{
   SpeechVoiceRelationsDestroy();
}


// clear out all this tag and concept stuff
static void SpeechDatabasesDestroy(void)
{
   int i;

   for (i = 0; i < g_VoiceList.Size(); ++i)
      delete g_VoiceList[i];
   g_VoiceList.SetSize(0);
   g_Domain.Clear();

   g_bSpeechDatabaseLoaded = FALSE;
}


// Destroy all voice data structures and objects (except base archetypes)
void SpeechDestroy(void)
{
   ITraitManager *pTraitMan = AppGetObj(ITraitManager);
   IObjectSystem *pObjSys = AppGetObj(IObjectSystem);
   ObjID baseVoiceID;

   // clean up all relations involving voices or concepts
   SpeechRelationsDestroy();
   SpeechDatabasesDestroy();

   if ((baseVoiceID = pObjSys->GetObjectNamed(BASE_VOICE_OBJ))
    != OBJ_NULL)
   {
      IObjectQuery *pQuery;

      pQuery = pTraitMan->Query(baseVoiceID, kTraitQueryAllDescendents);
      while (!pQuery->Done())
      {
         pObjSys->Destroy(pQuery->Object());
         pQuery->Next();
      }
      SafeRelease(pQuery);
   }
   SafeRelease(pTraitMan);
   SafeRelease(pObjSys);
}


/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\

   This batch of functions and data structures handles the work exposed
   in Speechglue.h: creation and cross-indexing of tags and concepts,
   and applying them to schemas.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */


extern "C" void SpeechAddConcept(const Label *pConceptName, int iPriority)
{
   ConfigSpew("SpeechSpew", ("SpeechAddConcept: %s %d\n",
                             &pConceptName->text, iPriority));
   g_Domain.AddConcept((Label *) pConceptName, iPriority);
}


extern "C" void SpeechAddTag(const Label *pTagName)
{
   ConfigSpew("SpeechSpew", ("SpeechAddTag: %s\n", &pTagName->text));
   g_Domain.AddTag((Label *) pTagName, kTagFlagEnum);
}


extern "C" void SpeechAddTagValue(const Label *pTagValueName)
{
   ConfigSpew("SpeechSpew", ("SpeechAddTagValue: %s\n", &pTagValueName->text));
   g_Domain.AddValue((Label *) pTagValueName);
}


extern "C" void SpeechAddIntTag(const Label *pTagName)
{
   ConfigSpew("SpeechSpew", ("SpeechAddIntTag: %s\n", &pTagName->text));
   g_iNewestTag = g_Domain.AddTag((Label *) pTagName, kTagFlagInt);
}


extern "C" void SpeechVoiceNew(ObjID VoiceObjID)
{
   ConfigSpew("SpeechSpew", ("SpeechVoiceNew: %d\n", VoiceObjID));

   ObjSetSpeechVoiceIndex(VoiceObjID, g_VoiceList.Size());
   cSpeechVoice *pNewVoice = new cSpeechVoice();
   g_VoiceList.Append(pNewVoice);
}


// By the time we're reading in schemas we have our tags, domains,
// and concepts laid out, and can identify them by name or ID.
extern "C" void SpeechSchemaNewStart(const Label *pSchemaName,
                                     ObjID SchemaObjID,
                                     const Label *pVoiceName,
                                     int iWeight,
                                     const Label *pConceptName)
{
   g_pCurrentVoice = g_VoiceList[SpeechVoiceIndexFromName(pVoiceName)];

   ConfigSpew("SpeechSpew", ("SpeechSchemaNewStart: %s %s %d %s, voice %d\n",
                             &pSchemaName->text, &pVoiceName->text,
                             iWeight, &pConceptName->text,
                             SpeechVoiceIndexFromName(pVoiceName)));

   sTagDBData Data(SchemaObjID, iWeight);
   g_pCurrentVoice->AddDataStart(Data, pConceptName);
}


extern "C" void SpeechSchemaNewAddIntTag(const Label *pTagName,
                                         int iMin, int iMax)
{
   ConfigSpew("SpeechSpew", ("SpeechSchemaNewAddIntTag: %s [%d %d] %d\n",
                             &pTagName->text, iMin, iMax));
   g_pCurrentVoice->AddDataAddTag(pTagName, iMin, iMax);
}


// pEnum is expected to be an array of eight const labels.
extern "C" void SpeechSchemaNewAddEnumTag(const Label *pTagName,
                                          const Label *pEnum)
{
   ConfigSpew("SpeechSpew", ("SpeechSchemaNewAddEnumTag: %s\n",
                             &pTagName->text));
   g_pCurrentVoice->AddDataAddTag(pTagName, pEnum);
}


extern "C" void SpeechSchemaNewFinish()
{
   ConfigSpew("SpeechSpew", ("SpeechSchemaNewFinish\n"));
   g_pCurrentVoice->AddDataFinish();
}
