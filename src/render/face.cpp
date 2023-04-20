/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/face.cpp,v 1.4 2000/01/20 16:47:45 MAT Exp $

/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\
   face.cpp

   This is a subsystem of meshtex.  It controls facial expressions.

   FaceSpew                     starts/ends of speech
   FaceStringSpew               getting strings from resource file

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */


#include <math.h>

#include <mms.h>
#include <mm.h>
#include <prophash.h>
#include <dataops.h>
#include <obprpimp.h>
#include <sdesbase.h>
#include <sdesc.h>
#include <mprintf.h>
#include <config.h>
#include <cfgdbg.h>
#include <resapi.h>
#include <strrstyp.h>
#include <sndrstyp.h>
#include <resistr.h>
#include <traitman.h>
#include <imaadpcm.h>

#include <objsys.h>
#include <objquery.h>

#include <simtime.h>
#include <speech.h>
#include <schema.h>
#include <schsamps.h>
#include <appsfx.h>
#include <meshtex.h>

#include <face.h>
#include <dbmem.h>
#include <initguid.h>
#include <faceg.h>


void SpeechStartCB(ObjID speakerID, int hSchema, ObjID schemaID);
void SpeechEndCB(ObjID speakerID, int hSchema, ObjID schemaID);
extern ISearchPath *pSoundPath; // from appsfx


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   constants

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// number of samples per second
static float kFaceSamplesPerSec = 16.0;

#define kFaceFilename "facepos.str"
#define kFacePathConfigVar "face_path"

// for finding mouth positions--see FindFacePos
static float kFaceCutoffClosed = .025;
static float kFaceCutoffBig = .175;
static float kFaceCutoffShout = .35;
static float kFaceOpenFudge = 1.2;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   globals

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static IObjectSystem *g_pObjSys;
static ITraitManager *g_pTraitMan;
static IResMan *g_pResMan;

static mms_smatr *g_pMaterial;
static ulong g_nDefaultMaterial;
static IRes *g_pFaceTalkRes;
static IStringRes *g_pFaceTalk;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   El Propertano (1)

   Our main property has the names of the textur it's replacing, and
   the ones to swap in.  Some of the swapping uses machinery in
   MeshTex.  If I'd laid this out better I could reuse even more of
   MeshTex.

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

enum eMouthPos
{
   kMouthClosed,
   kMouthSmallOh,
   kMouthSmallEe,
   kMouthBigOh,
   kMouthBigEe,
   kMouthShout,

   kMouthPosCount
};


// keeping the names and faces straight...
struct sFaceNames
{
   // We do this so the cDynArrays won't try to get copied on load,
   // with their pointers left over from when they were saved off.
   // The data we're losing is derived from the data we're keeping.
   sFaceNames &operator=(const sFaceNames &from)
   {
      memcpy(&m_aszTextureToReplace, &from.m_aszTextureToReplace,
             sizeof(m_aszTextureToReplace));
      memcpy(&m_aszVisageTexture, &from.m_aszVisageTexture,
             sizeof(m_aszVisageTexture));
      m_bSet = FALSE;

      return *this;
   }

   // data

   // set in SDESC
   char m_aszTextureToReplace[16];
   char m_aszVisageTexture[kFaceVisageCount][16];

   // derived info--we compute this on load/change
   int m_iMaterialIndex;
   grs_bitmap *m_apBitmap[kFaceVisageCount][kMouthPosCount];

   // other
   BOOL m_bSet;                 // have bitmaps been set up for this struct?
};


#undef  INTERFACE
#define INTERFACE IFaceProperty

DECLARE_PROPERTY_INTERFACE(IFaceProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sFaceNames *);
};

static sPropertyConstraint g_aFaceConstraint[] =
{
   { kPropertyAutoCreate, "Face State" },
   { kPropertyNullConstraint }
};

static sPropertyDesc g_FaceDesc
= {
   "Face",
   0,           // flags
   g_aFaceConstraint,  // constraints
   1, 0,        // version
   { "Renderer", "Face Textures", "What you looking at BOY?" },
   kPropertyChangeLocally,
};

class cFaceOps : public cClassDataOps<sFaceNames>
{
};

class cFaceStore : public cHashPropertyStore<cFaceOps>
{
};

typedef cSpecificProperty<IFaceProperty,
   &IID_IFaceProperty, sFaceNames *,
   cFaceStore> cBaseFaceProperty;

class cFaceProperty : public cBaseFaceProperty
{
public:
   cFaceProperty(const sPropertyDesc *desc)
      : cBaseFaceProperty(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cFaceProperty);

protected:
   void CreateEditor();
};


static sFieldDesc g_aFaceFieldDesc[kFaceVisageCount + 1]
= {
   { "texture to replace", kFieldTypeString,
     FieldLocation(sFaceNames, m_aszTextureToReplace) },

   { "Neutral", kFieldTypeString,
     FieldLocation(sFaceNames, m_aszVisageTexture[kFaceNeutral]) },
   { "Smile", kFieldTypeString,
     FieldLocation(sFaceNames, m_aszVisageTexture[kFaceSmile]) },
   { "Wince", kFieldTypeString,
     FieldLocation(sFaceNames, m_aszVisageTexture[kFaceWince]) },
   { "Surprise", kFieldTypeString,
     FieldLocation(sFaceNames, m_aszVisageTexture[kFaceSurprise]) },
   { "Stunned", kFieldTypeString,
     FieldLocation(sFaceNames, m_aszVisageTexture[kFaceStunned]) },
};

static sStructDesc g_FaceStructDesc
= StructDescBuild(cFaceProperty, kStructFlagNone,
                  g_aFaceFieldDesc);

void cFaceProperty::CreateEditor()
{
   AutoAppIPtr(StructDescTools);
   pStructDescTools->Register(&g_FaceStructDesc);
   cPropertyBase::CreateEditor(this);
}

IFaceProperty *g_pFaceProperty;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   El Propertano (2)

   state of a particular guy

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

struct sFaceState
{
   // data
   BOOL m_bTalking;     // currently talking?
   ulong m_nTalkStartTime;
   char *m_pszFacePosString;
   int m_iFacePosStringLength;

   eFaceVisage m_Visage;
   uint m_nPriority;    // If this is 0 we're in our default state
   ulong m_nEndTime;    // milliseconds
};


#undef  INTERFACE
#define INTERFACE IFaceStateProperty

DECLARE_PROPERTY_INTERFACE(IFaceStateProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sFaceState *);
};


static sPropertyDesc g_FaceStateDesc
= {
   "Face State",
   kPropertyInstantiate|kPropertyNoInherit,     // flags
   NULL,        // constraints
   1, 0,        // version
   { "Renderer", "Face State", "State of a particular dude's face" },
   kPropertyChangeLocally,
};

class cFaceStateOps : public cClassDataOps<sFaceState>
{
};

class cFaceStateStore : public cHashPropertyStore<cFaceStateOps>
{
};

typedef cSpecificProperty<IFaceStateProperty, &IID_IFaceStateProperty,
   sFaceState *, cFaceStateStore> cBaseFaceStateProperty;

class cFaceStateProperty : public cBaseFaceStateProperty
{
public:
   cFaceStateProperty(const sPropertyDesc *desc)
      : cBaseFaceStateProperty(desc)
   {
   }

   STANDARD_DESCRIBE_TYPE(cFaceStateProperty);

   void OnListenMsg(ePropertyListenMsg type, ObjID obj, uPropListenerValue val)
   {
      cBaseFaceStateProperty::OnListenMsg(type, obj, val);

      if (!val.ptrval)
         return;

      sFaceState *pState = (sFaceState *) val.ptrval;

      if (type & kListenPropSet) {
         SpeechUninstallStartCallback(obj, SpeechStartCB);
         SpeechInstallStartCallback(obj, SpeechStartCB);
         SpeechUninstallEndCallback(obj, SpeechEndCB);
         SpeechInstallEndCallback(obj, SpeechEndCB);
      } else if (type & kListenPropUnset) {
         SpeechUninstallStartCallback(obj, SpeechStartCB);
         SpeechUninstallEndCallback(obj, SpeechEndCB);
         if (pState->m_pszFacePosString)
            free(pState->m_pszFacePosString);
      }
   }

protected:
   void CreateEditor();
};


static const char *g_apszFaceVisageName[kFaceVisageCount]
= {
   "Neutral",
   "Smile",
   "Wince",
   "Surprise",
   "Stunned",
};

static sFieldDesc g_aFaceStateFieldDesc[]
= {
   { "Talking?", kFieldTypeBool,
     FieldLocation(sFaceState, m_bTalking) },
   { "Current visage", kFieldTypeEnum,
     FieldLocation(sFaceState, m_Visage),
     FullFieldNames(g_apszFaceVisageName) },
   { "Priority of visage", kFieldTypeInt,
     FieldLocation(sFaceState, m_nPriority) },
   { "End time of visage", kFieldTypeInt,
     FieldLocation(sFaceState, m_nEndTime) },
};


static sStructDesc g_FaceStateStructDesc
= StructDescBuild(cFaceStateProperty, kStructFlagNone,
                  g_aFaceStateFieldDesc);

void cFaceStateProperty::CreateEditor()
{
   AutoAppIPtr(StructDescTools);
   pStructDescTools->Register(&g_FaceStateStructDesc);
   cPropertyBase::CreateEditor(this);
}

IFaceStateProperty *g_pFaceStateProperty;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   El Propertano (3)

   to mark schemas for which we find sequences of face textures

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

static sPropertyDesc g_FacePosDesc
= {
   "Face Pos",
   0,
   NULL,        // constraints
   1, 0,        // version
   { "Sound", "Face Motions", "Do we find mouth positions for this schema?" },
   kPropertyChangeLocally,
};

IBoolProperty *g_pFacePosProperty;


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   internal debugging functions

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifndef SHIP
static inline const char *Article(const char *pName)
{
   static char vowels[] = "AEIOUaeiou";

   if (strchr(vowels, pName[0]) != NULL)
      return "An";
   return "A";
}


// This is meant to make mono spew easier to read.  It just gives a
// short name--using the archetype if the object itself has no name.
// There's bound be a function like it out there someplace.  I can't
// find it.
static const char *ObjName(ObjID id)
{
   // Does it have a name of its own?
   const char *pName = g_pObjSys->GetName(id);
   if (pName)
      return pName;

   // No?  Get archetype name.
   ObjID arch = g_pTraitMan->GetArchetype(id);
   pName = g_pObjSys->GetName(arch);
   if (pName) {
      static char aszNameBuffer[64];
      sprintf(aszNameBuffer, "%s %s", Article(pName), pName);
      return aszNameBuffer;
   }

   // If no archetype name, give up!
   return "Unknown thingamajig";
}
#endif // ~SHIP


static inline char *FetchMouthString(const char* pSample)
{
   char *pS = g_pFaceTalk->StringLock(pSample);
   if (pS) {
      char *pS2 = strdup(pS);
      g_pFaceTalk->StringUnlock(pSample);
      ConfigSpew("FaceStringSpew", ("face: Got string for %s.\n", pSample));
      return pS2;
   } else {
      ConfigSpew("FaceStringSpew", ("face: Empty-handed for %s.\n", pSample));
      return 0;
   }
}


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   internals for finding mouth positions (editor-only)

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

#ifdef EDITOR
// make buffer of floats from ADPCM data
static inline void NormBuffFromRes4(sSndAttribs *pAttribs, void *pRawData,
                                    float *pfSamples)
{
   short *pTemp = (short *) malloc(pAttribs->numSamples * sizeof(short));
   short *pOut = pTemp;
   uint nSamplesOutNeeded = pAttribs->numSamples;
   int iInSamplesLeft = 0;
   uint skipNSamples = 0;
   uint nSamps;
   BOOL getHeader;
   IMA_ADPCM Decoder;

   // This loop is more or less copied from the sndsrc library.
   while ( nSamplesOutNeeded > 0 ) {
      if ( iInSamplesLeft == 0 ) {
         // current block is empty, get new block
         iInSamplesLeft = pAttribs->samplesPerBlock - skipNSamples;
         if ( skipNSamples != 0 ) {
            pRawData = DecompressIMABlkPartial(&Decoder, FALSE,
                                               (char *) pRawData,pOut,0,TRUE);
            getHeader = FALSE;
         } else
            getHeader = TRUE;
      } else
         getHeader = FALSE;

      // decompress a block
      nSamps = (nSamplesOutNeeded < iInSamplesLeft) ?
         nSamplesOutNeeded : iInSamplesLeft;
      pRawData = DecompressIMABlkPartial(&Decoder, FALSE, (char *) pRawData,
                                         pOut, nSamps, getHeader);
      iInSamplesLeft -= nSamps;

      pOut += nSamps;
      nSamplesOutNeeded -= nSamps;
   } // end while nSamplesOutNeeded > 0

   for (int i = 0; i < pAttribs->numSamples; ++i)
      pfSamples[i] = fabs(pTemp[i] * (1.0 / 32767));

   free(pTemp);
}


// buffer of floats from 8-bit sound file
static inline void NormBuffFromRes8(sSndAttribs *pAttribs, void *pRawData,
                                    float *pfSamples)
{
   uchar *pRawUChars = (uchar *) pRawData;

   // 8-bit WAV files are unsigned.
   for (int i = 0; i < pAttribs->numSamples; ++i)
      pfSamples[i] = fabs((pRawUChars[i] - 127.0)  * (1.0 / 127.0));
}


// buffer of floats from 16-bit sound file
static inline void NormBuffFromRes16(sSndAttribs *pAttribs, void *pRawData,
                                     float *pfSamples)
{
   short *pRawShorts = (short *) pRawData;
   for (int i = 0; i < pAttribs->numSamples; ++i)
      pfSamples[i] = fabs(pRawShorts[i] * (1.0 / 32767));
}


// find average of an array of floats
static float Average(float *pfSamples, uint nCount)
{
   float fSum = 0;
   for (int i = 0; i < nCount; ++i)
      fSum += pfSamples[i];
   return fSum / nCount;
}


// find st. dev. of an array of floats
static float StandardDeviation(float *pfSamples, uint nCount, float fAverage)
{
   float fSum = 0;
   for (int i = 0; i < nCount; ++i) {
      float fDifference = pfSamples[i] - fAverage;
      fSum += fDifference * fDifference;
   }
   return sqrt(fSum / nCount);
}


// DO NOT READ THIS.
static inline char FindFacePos(float *pfSamples, int iNum, float fStDev)
{
   float fAverage = Average(pfSamples, iNum);

   if (fAverage < kFaceCutoffClosed)
      return kMouthClosed;
   if (fAverage > kFaceCutoffShout)
      return kMouthShout;

   int iRV = kMouthSmallOh;
   if (fAverage > kFaceCutoffBig)
      iRV = kMouthBigOh;

   // Between being closed and shouting we choose a position based
   // on how noisy the sound is (in the standard deviation sense).
   float fLocalStDev = StandardDeviation(pfSamples, iNum, fAverage);
   if (fLocalStDev * kFaceOpenFudge > fStDev)
      ++iRV;
   return iRV;
}


static inline void ProcessSample(const char *pszName, FILE *pFile)
{
   mprintf("face:     sample %s.  ", pszName);

   IRes *pRes = g_pResMan->Bind(pszName, RESTYPE_SOUND, pSoundPath);
   if (!pRes) {
      Warning(("Cannot open file.\n"));
      return;
   }

   void *pResData = pRes->Lock();
   void *pRawData;

   sSndAttribs Attribs;
   ulong nLength, nSamples;
   if (SndCrackRezHeader(pResData, 256, &pRawData, &nLength,
                         &nSamples, &Attribs)) {
      pRes->Unlock();
      pRes->Release();
      return;
   }

   if (Attribs.nChannels != 1) {
      Warning(("face: Sound has %d channels.  Needs to be a mono sound!\n",
               Attribs.nChannels));
      pRes->Unlock();
      pRes->Release();
      return;
   }

   float *pfSamples = (float *) malloc(Attribs.numSamples * sizeof(float));
   mprintf("(%d bits per sample)", Attribs.bitsPerSample);

   // We want a buffer of floats, each 0-1.  We can handle three formats.
   switch(Attribs.bitsPerSample)
   {
      case 4:        // ADPCM
         NormBuffFromRes4(&Attribs, pRawData, pfSamples);
         break;
      case 8:
         NormBuffFromRes8(&Attribs, pRawData, pfSamples);
         break;
      case 16:
         NormBuffFromRes16(&Attribs, pRawData, pfSamples);
         break;
      default:
         Warning(("Cannot handle %d bits per sample.\n",
                  Attribs.bitsPerSample));
         pRes->Unlock();
         pRes->Release();
         return;
   }

   // At last, it's time for the real work: finding the mouth
   // positions.
   float fLengthInSeconds
      = (float) Attribs.numSamples / (float) Attribs.sampleRate;
   int iNumPositions = fLengthInSeconds * kFaceSamplesPerSec;
   int iSamplesPerPosition = (float) Attribs.sampleRate / kFaceSamplesPerSec;
   float fStDev = StandardDeviation(pfSamples, Attribs.numSamples,
                                    Average(pfSamples, Attribs.numSamples));

   char *pszPositions = (char *) malloc(iNumPositions + 1);

   int i;
   for (i = 0; i < iNumPositions; ++i)
      pszPositions[i] = '0' + FindFacePos(pfSamples + i * iSamplesPerPosition,
                                          iSamplesPerPosition, fStDev);

   pszPositions[i] = 0;         // null-terminate our string
   fprintf(pFile, "%s:\"%s\"\n", pszName, pszPositions);

   pRes->Unlock();
   pRes->Release();
   free(pfSamples);
   free(pszPositions);

   mprintf("\n", pszName);
}


void BuildFacePosFile()
{
   char aszPath[256];

   // build the path for our file
   if (!config_get_raw(kFacePathConfigVar, aszPath, sizeof(aszPath))) {
      Warning(("face: Please set face_path before trying this.\n"));
      return;
   }

   /* @TODO: Do we want to do this, once we're further along localization?
   int iPos = strlen(aszPath);
   if (!config_get_raw("language", aszPath + iPos, sizeof(aszPath) - iPos)) {
      Warning(("face: Please set language before trying this.\n"));
      return;
   }
   */

   strcat(aszPath, kFaceFilename);
   mprintf("face: Using path: %s.\n", aszPath);
   FILE *pFile = fopen(aszPath, "w");
   if (!pFile) {
      Warning(("face: Cannot open file %s to write.\n", aszPath));
      return;
   }

   for (IObjectQuery *pObjQuery = g_pObjSys->Iter(kObjectAbstract);
        !pObjQuery->Done();
        pObjQuery->Next()) {
      ObjID Obj = pObjQuery->Object();
      BOOL bOn;
      if (g_pFacePosProperty->Get(Obj, &bOn) && bOn) {
         int iNumSamples = SchemaSamplesNum(Obj);
         if (iNumSamples)
            mprintf("face: Processing schema %s.\n", ObjName(Obj));
         for (int i = 0; i < iNumSamples; ++i)
            ProcessSample(SchemaSampleGet(Obj, i), pFile);
      }
   }
   fclose(pFile);
}
#endif // EDITOR


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   internal game-time functions for rendering

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

// callback from speech system for start of speech
static void SpeechStartCB(ObjID speakerID, int hSchema, ObjID schemaID)
{
   ConfigSpew("FaceSpew", ("face: Start callback on %s.\n",
                           ObjName(speakerID)));

   sFaceState *pState;
   g_pFaceStateProperty->Get(speakerID, &pState);
   Assert_(pState);

   if (pState->m_bTalking && pState->m_pszFacePosString)
      free(pState->m_pszFacePosString);

   pState->m_bTalking = TRUE;
   pState->m_pszFacePosString = 0;
   pState->m_nTalkStartTime = GetSimTime();

   int iSFXHandle = SchemaGetSFXFromHandle(hSchema, 0);
   if (iSFXHandle == SFX_NO_HND) {
      Warning(("face: No SFX handle from schema handle.\n"));
      return;
   }

   const char *pszSampleName = SFX_Name(iSFXHandle);
   if (!pszSampleName) {
      Warning(("face: No name from SFX handle.\n"));
      return;
   }

   // Is this sample delayed?
   const sfx_parm *pParm = SFX_Parm(iSFXHandle);
   pState->m_nTalkStartTime += pParm->delay;

   // Is there a string for this sample?
   pState->m_pszFacePosString = FetchMouthString(pszSampleName);
   if (pState->m_pszFacePosString) {
      pState->m_iFacePosStringLength = strlen(pState->m_pszFacePosString);
      ConfigSpew("FaceSpew", ("face: Sample: %s -- string: %s.\n",
                              pszSampleName, pState->m_pszFacePosString));
   }
}


// callback from speech system for end of speech
static void SpeechEndCB(ObjID speakerID, int hSchema, ObjID schemaID)
{
   ConfigSpew("FaceSpew", ("face: End callback on %s.\n",
                           ObjName(speakerID)));

   sFaceState *pState;
   g_pFaceStateProperty->Get(speakerID, &pState);
   Assert_(pState);
   pState->m_bTalking = FALSE;
   if (pState->m_pszFacePosString) {
      free(pState->m_pszFacePosString);
      pState->m_pszFacePosString = 0;
   }
}


// This sets up the bitmap pointers in an sFaceNames structure.
static inline void SetUpFaceNames(ObjID Obj, mms_model *pModel,
                                  sFaceNames *pNames)
{
   mms_smatr *pMaterial = mm_smatr_list(pModel);
   mms_segment *pSegment = mm_segment_list(pModel);

   // find the texture we're replacing
   for (int iMat = 0; iMat < pModel->smatrs; ++iMat)
      if (!stricmp(pMaterial[iMat].name, pNames->m_aszTextureToReplace))
         pNames->m_iMaterialIndex = iMat;

#ifndef SHIP
   if (!pNames->m_aszVisageTexture[kFaceNeutral][0])
      Warning(("face: object %s really should have a neutral face.\n",
               ObjName(Obj)));
#endif // ~SHIP

   for (int iVisage = 0; iVisage < kFaceVisageCount; ++iVisage) {
      // Is there a texture name for this visage?
      if (!pNames->m_aszVisageTexture[iVisage][0]) {
         for (int iMouthPos = 0; iMouthPos < kMouthPosCount; ++iMouthPos)
            pNames->m_apBitmap[iVisage][iMouthPos] = 0;
      } else {
         for (int iMouthPos = 0; iMouthPos < kMouthPosCount; ++iMouthPos) {
            char aszName[16];
            sprintf(aszName, "%s%d", pNames->m_aszVisageTexture[iVisage],
                    iMouthPos + 1);

            pNames->m_apBitmap[iVisage][iMouthPos] = MeshTexGetBitmap(aszName);
            if (!pNames->m_apBitmap[iVisage][iMouthPos]) {
               break;
            }
         }
      }
   }

   pNames->m_bSet = TRUE;
}


// Two independent things could affect our choice of bitmap.  The
// first is whether this object is talking right now, and the second
// is what visage it's set to use.
static inline grs_bitmap *FindBitmap(ObjID Obj, sFaceNames *pNames)
{
   sFaceState *pState;
   g_pFaceStateProperty->Get(Obj, &pState);
   Assert_(pState);

   int iVisage = kFaceNeutral;
   ulong nTime = GetSimTime();

   // Is a visage set for this object?
   if (pState->m_nPriority)
      if (pState->m_nEndTime > nTime)
         pState->m_nPriority = 0;
      else
         iVisage = pState->m_Visage;

   if (pState->m_bTalking) {
      // Does this visage have any talking animations?
      if (!pNames->m_apBitmap[iVisage][1]
       && pNames->m_apBitmap[kFaceNeutral][1])
         iVisage = kFaceNeutral;

      int iTimeIntoSample = (int) nTime - (int) pState->m_nTalkStartTime;
      int iPos = (iTimeIntoSample * kFaceSamplesPerSec) / 1000;

      if (pState->m_pszFacePosString) {
         if (iPos < 0) {
            // If the sample was delayed we could actually be ahead of
            // it.
            return pNames->m_apBitmap[iVisage][kMouthClosed];
         } else if (iPos < pState->m_iFacePosStringLength) {
            // Here's the usual case: we're inside the sample.
            int iFrame = pState->m_pszFacePosString[iPos] - '0';
            if (pNames->m_apBitmap[iVisage][iFrame])
               return pNames->m_apBitmap[iVisage][iFrame];
         } else {
            // Maybe the sample is really over.
            ConfigSpew("FaceSpew", ("face: End of string for %s.\n",
                                    ObjName(Obj)));
            pState->m_bTalking = FALSE;
            if (pState->m_pszFacePosString)
               pState->m_pszFacePosString = 0;
            return pNames->m_apBitmap[iVisage][kMouthClosed];
         }
      }

      // If there's no animation string we change frames in a fixed
      // sequence.
      static int aTimedAnim[8] = { 1, 2, 2, 3, 2, 2, 1, 1 };
      int iTimeBasedFrame = aTimedAnim[iPos & 7];
      grs_bitmap *pFrame = pNames->m_apBitmap[iVisage][iTimeBasedFrame];
      if (pFrame)
         return pFrame;
   }

   if (!pNames->m_apBitmap[iVisage][0])
      iVisage = kFaceNeutral;
   return pNames->m_apBitmap[iVisage][kMouthClosed];
}


/* --<<= -+-/-/-/=/=/\/- <<< ((( ((( / \ ))) ))) >>> -\/\=\=\-\-\-+- =>>-- *\

   exposed functions

\* --<<= -+-\-\-\=\=\/\- <<< ((( ((( \ / ))) ))) >>> -/\/=/=/-/-/-+- =>>-- */

BOOL FaceSetVisage(ObjID Obj, eFaceVisage Visage, uint nPriority,
                   int iDuration)
{
   Assert_(nPriority >= kFacePriorityMin && nPriority <= kFacePriorityMax);

   sFaceState *pState;
   g_pFaceStateProperty->Get(Obj, &pState);
   if (!pState) {
      Warning(("face: Object %s has no face state property.\n", ObjName(Obj)));
      return FALSE;
   }

   if (nPriority <= pState->m_nPriority && pState->m_nEndTime <= GetSimTime())
      return FALSE;

   pState->m_Visage = Visage;
   pState->m_nPriority = nPriority;
   pState->m_nEndTime = iDuration + GetSimTime();

   return TRUE;
}


void FaceInit()
{
   g_pObjSys = AppGetObj(IObjectSystem);
   g_pTraitMan = AppGetObj(ITraitManager);
   g_pResMan = AppGetObj(IResMan);

   g_pFaceProperty = new cFaceProperty(&g_FaceDesc);
   g_pFaceStateProperty = new cFaceStateProperty(&g_FaceStateDesc);
   g_pFacePosProperty = CreateBoolProperty(&g_FacePosDesc, kPropertyImplHash);

   g_pFaceTalkRes=g_pResMan->Bind(kFaceFilename, RESTYPE_STRING, 0, "strings");
   if (!g_pFaceTalkRes)
      return;
   Verify(SUCCEEDED(g_pFaceTalkRes->QueryInterface(IID_IStringRes,
                                                   (void**) &g_pFaceTalk)));
}


void FaceTerm()
{
   if (g_pFaceTalk)
      g_pFaceTalk->Release();
   if (g_pFaceTalkRes)
      g_pFaceTalkRes->Release();
}


void FaceReset()
{
}


void FacePrerender(ObjID Obj, mms_model *pModel)
{
   sFaceNames *pNames;

   // Does this object have our property?
   if (!g_pFaceProperty->Get(Obj, &pNames)) {
      g_pMaterial = 0;
      return;
   }

   if (!pNames->m_bSet)
      SetUpFaceNames(Obj, pModel, pNames);

   grs_bitmap *pBitmap = FindBitmap(Obj, pNames);
   g_pMaterial = mm_smatr_list(pModel) + pNames->m_iMaterialIndex;
   g_nDefaultMaterial = g_pMaterial->handle;
   if (pBitmap)
      g_pMaterial->handle = (ulong) pBitmap;
}


void FacePostrender()
{
   if (g_pMaterial)
      g_pMaterial->handle = g_nDefaultMaterial;
}
