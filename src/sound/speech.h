/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/speech.h,v 1.12 2000/01/31 10:02:57 adurant Exp $
#pragma once

#ifndef __SPEECH_H
#define __SPEECH_H

#include <objtype.h>
#include <osystype.h>
#include <dispatch.h>
#include <tagdbt.h>


// ai-voice relation
#define VOICE_DATA "VoiceData"
#define VOICE_RELATION "VoiceLink"

// This tells the speech system that we have finished reading in a new
// database from our text files.
extern void SpeechNotifyTextParseEnd();

// We export these structures as handles because it saves us
// some swizzling.  The name used for the voice index here is the
// sym name.
extern int SpeechVoiceIndexFromName(Label *pVoiceName);

// choose and say a piece of speech corresponding to concept
// choose a voice if not already chosen

// This version is for when you have a request handy which contains
// all the tags you need.
extern int SpeechSpeak(ObjID SpeakerObjID, const Label *pConcept,
                       cTagDBInput *pInput, void *pData);

extern BOOL SpeechIsSpeaking(ObjID SpeakerObjID);

extern void SpeechHalt(ObjID SpeakerObjID);
      
// set up our initial object archetypes
EXTERN void SpeechBuildDefault(void);

// destroy all voices and concept and tag databases
EXTERN void SpeechDestroy(void);

// swizzling tokens in our databases
EXTERN void SpeechLocalToGlobal(cTagDBKey *pKey);
EXTERN void SpeechGlobalToLocal(cTagDBKey *pKey);

typedef void (*SpeechCallbackFn)(ObjID speakerID, int hSchema, ObjID schemaID);
EXTERN void SpeechInstallStartCallback(ObjID speakerID, SpeechCallbackFn callback);
EXTERN void SpeechUninstallStartCallback(ObjID speakerID, SpeechCallbackFn callback);
EXTERN void SpeechInstallEndCallback(ObjID speakerID, SpeechCallbackFn callback);
EXTERN void SpeechUninstallEndCallback(ObjID speakerID, SpeechCallbackFn callback);

EXTERN BOOL g_bSpeechDatabaseLoaded;

#endif // ~__SPEECH_H
