/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchprop.h,v 1.7 2000/01/31 10:02:51 adurant Exp $
#pragma once

#ifndef __SPCHPROP_H
#define __SPCHPROP_H

#include <propface.h>

#include <spchtype.h>

#undef INTERFACE
#define INTERFACE ISpeechProperty
DECLARE_PROPERTY_INTERFACE(ISpeechProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(sSpeech*); 
}; 


#define PROP_SPEECH "Speech"

EXTERN sSpeech *SpeechPropCreateAndGet(ObjID objID);
EXTERN void SpeechPropInit();

// Last usage property
#define PROP_SPEECH_NEXT_PLAY "SpchNextPlay"
EXTERN IIntProperty *g_pPropSpeechNextPlay;
EXTERN void SpeechNextPlayPropInit(void);
EXTERN int SpeechNextPlayGet(ObjID objID);
#define SPEECH_NEXT_PLAY_SET(objID, time) PROPERTY_SET(g_pPropSpeechNextPlay, (objID), (time))

// Voice name property
#define PROP_SPEECH_VOICE "SpchVoice"
EXTERN ILabelProperty *g_pPropSpeechVoice;
EXTERN void SpeechVoicePropInit(void);
EXTERN Label *ObjGetSpeechVoice(ObjID objID);
#define OBJ_SET_SPEECH_VOICE(objID, voiceID) PROPERTY_SET(g_pPropSpeechVoice, (objID), (voiceID))

// voice index property
#define PROP_VOICE_INDEX_NAME "VoiceIdx"
EXTERN IIntProperty *g_pPropSpeechVoiceIndex;
EXTERN BOOL ObjGetSpeechVoiceIndex(ObjID obj, int* iIndex);
EXTERN void ObjSetSpeechVoiceIndex(ObjID obj, int iIndex);

// Speech pause properties
#define PROP_SPEECH_PAUSE_MIN "MinSpchPause"
EXTERN IIntProperty *g_pPropSpeechPauseMin;
EXTERN void SpeechPauseMinPropInit(void);
EXTERN int SpeechPauseMinGet(ObjID objID);
#define SPEECH_PAUSE_MIN_SET(objID, time) \
   PROPERTY_SET(g_pPropSpeechPauseMin, (objID), (time))
#define PROP_SPEECH_PAUSE_MAX "MaxSpchPause"
EXTERN IIntProperty *g_pPropSpeechPauseMax;
EXTERN void SpeechPauseMaxPropInit(void);
EXTERN int SpeechPauseMaxGet(ObjID objID);
#define SPEECH_PAUSE_MAX_SET(objID, time) \
   PROPERTY_SET(g_pPropSpeechPauseMax, (objID), (time))

// Initialize all speech properties
EXTERN void SpeechPropsInit(void);

#endif



