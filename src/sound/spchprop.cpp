/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchprop.cpp,v 1.10 1998/11/21 14:21:40 MROWLEY Exp $

#include <timer.h>

#include <propbase.h>
#include <propface.h>
#include <propert_.h>
#include <dataops_.h>
#include <prophash.h>

#include <sdesbase.h>
#include <sdesc.h>

#include <spchprop.h>
#include <spchbase.h>

// Must be last header
#include <dbmem.h>

// Speech property

// structure descriptors
static sFieldDesc SpeechFields[] = 
{
   {"flags", kFieldTypeInt, FieldLocation(sSpeech, flags),
    kFieldFlagNotEdit}, 
   {"time", kFieldTypeInt, FieldLocation(sSpeech, time),
    kFieldFlagNotEdit}, 
   {"schemaID", kFieldTypeInt, FieldLocation(sSpeech, schemaID),
    kFieldFlagNotEdit}, 
   {"concept", kFieldTypeInt, FieldLocation(sSpeech, m_iConceptIndex),
    kFieldFlagNotEdit}, 
   {"schema handle", kFieldTypeInt, FieldLocation(sSpeech, hSchemaPlay),
    kFieldFlagNotEdit}, 
};

static sStructDesc SpeechStructDesc = StructDescBuild(sSpeech, kStructFlagNone, SpeechFields);

static ISpeechProperty *pSpeechProperty;

struct sSpeechProp : public sSpeech
{
   sSpeechProp()
   {
      memset(this,0,sizeof(*this));
      time = tm_get_millisec_unrecorded(); 
      m_iConceptIndex = -1;
      hSchemaPlay = -1;
   }

   sSpeechProp(const sSpeechProp& other)
   {
      *this = other; 
   }
};


class cSpeechOps : public cClassDataOps<sSpeechProp>
{
   
}; 

class cSpeechStore : public cHashPropertyStore<cSpeechOps>
{
   
}; 

class cSpeechProperty: public cSpecificProperty<ISpeechProperty, &IID_ISpeechProperty, sSpeech*, cSpeechStore>
{
   typedef cSpecificProperty<ISpeechProperty, &IID_ISpeechProperty, sSpeech*, cSpeechStore> cParent; 

public:

   cSpeechProperty(const sPropertyDesc* desc)
      : cParent(desc)
   {
      
   }

   STANDARD_DESCRIBE_TYPE(sSpeech); 

   
   

};

static sPropertyDesc sSpeechDesc = 
{
   PROP_SPEECH, kPropertyTransient, NULL,
   0, 0, { "Speech", "Current Speech" },
   kPropertyChangeLocally, // net_flags
};


sSpeech *SpeechPropCreateAndGet(ObjID objID)
{
   sSpeech *pSpeech; 

   pSpeechProperty->Create(objID);
   if (!pSpeechProperty->Get(objID, &pSpeech))
   {
      Warning(("SpeechPropCreateAndGet: can't create speech property\n"));
      return NULL;
   }
   return pSpeech;
}

void SpeechPropInit()
{
   StructDescRegister(&SpeechStructDesc);

   pSpeechProperty = new cSpeechProperty(&sSpeechDesc); 
}

// Speech last played property
IIntProperty *g_pPropSpeechNextPlay;

static sPropertyDesc SpeechNextPlayDesc = 
{
   PROP_SPEECH_NEXT_PLAY, kPropertyTransient, NULL,
   0, 0, { "Speech", "Last Played" },
   kPropertyChangeLocally, // net_flags
};

void SpeechNextPlayPropInit(void)
{
   g_pPropSpeechNextPlay = CreateIntProperty(&SpeechNextPlayDesc, kPropertyImplDense);
}

// get last time speech played, -1 if never played
int SpeechNextPlayGet(ObjID objID)
{
   int NextPlay = -1;

   g_pPropSpeechNextPlay->Get(objID, &NextPlay);
   return NextPlay;
}

// Voice property
ILabelProperty *g_pPropSpeechVoice;

static sPropertyDesc SpeechVoiceDesc = 
{
   PROP_SPEECH_VOICE, 0, NULL,
   0, 0, { "Speech", "Voice" },
};

void SpeechVoicePropInit(void)
{
   g_pPropSpeechVoice = CreateLabelProperty(&SpeechVoiceDesc, kPropertyImplDense);
}

// get voice
Label *ObjGetSpeechVoice(ObjID objID)
{
   Label *voiceLabel = NULL;

   g_pPropSpeechVoice->Get(objID, &voiceLabel);
   return voiceLabel;
}

// Speech pause properties
IIntProperty *g_pPropSpeechPauseMin;

static sPropertyDesc speechPauseMinDesc = 
{
   PROP_SPEECH_PAUSE_MIN, 0, NULL,
   0, 0, { "Speech", "Pause Min" },
};

void SpeechPauseMinPropInit(void)
{
   g_pPropSpeechPauseMin = CreateIntProperty(&speechPauseMinDesc, kPropertyImplDense);
}

// get last time speech played, -1 if never played
int SpeechPauseMinGet(ObjID objID)
{
   int PauseMin = -1;

   g_pPropSpeechPauseMin->Get(objID, &PauseMin);
   return PauseMin;
}

IIntProperty *g_pPropSpeechPauseMax;

static sPropertyDesc speechPauseMaxDesc = 
{
   PROP_SPEECH_PAUSE_MAX, 0, NULL,
   0, 0, { "Speech", "Pause Max" },
};

void SpeechPauseMaxPropInit(void)
{
   g_pPropSpeechPauseMax = CreateIntProperty(&speechPauseMaxDesc, kPropertyImplDense);
}

// get last time speech played, -1 if never played
int SpeechPauseMaxGet(ObjID objID)
{
   int PauseMax = -1;

   g_pPropSpeechPauseMax->Get(objID, &PauseMax);
   return PauseMax;
}


static sPropertyDesc VoiceIndexDesc = 
{
   PROP_VOICE_INDEX_NAME, 0, NULL,
   0, 0, { "Speech", "Voice Index" },
};

IIntProperty* g_pPropSpeechVoiceIndex = NULL;
#define VOICE_INDEX_PROP_IMPL kPropertyImplSparseHash

static void VoiceIndexPropInit(void)
{
   g_pPropSpeechVoiceIndex = CreateIntProperty(&VoiceIndexDesc,
                                               VOICE_INDEX_PROP_IMPL);
}

extern "C" void ObjSetSpeechVoiceIndex(ObjID obj, int iIndex)
{
   g_pPropSpeechVoiceIndex->Set(obj, iIndex);
}

extern "C" BOOL ObjGetSpeechVoiceIndex(ObjID obj, int *iIndex)
{
   if (g_pPropSpeechVoiceIndex->Get(obj, iIndex))
      return TRUE;
   return FALSE;
}


void SpeechPropsInit(void)
{
   SpeechPropInit();
   SpeechNextPlayPropInit();
   SpeechVoicePropInit();
   SpeechPauseMinPropInit();
   SpeechPauseMaxPropInit();
   VoiceIndexPropInit();
}

