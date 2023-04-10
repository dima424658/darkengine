/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCFROB_H
#define __DPCFROB_H

#ifndef PROPFACE_H
#include <propface.h>
#endif // !PROPFACE_H

// Frob sounds
#define PROP_FROB_SOUND "FrobSound"
EXTERN BOOL FrobSoundIsRelevant(ObjID objID);
EXTERN Label *FrobSoundGet(ObjID objID);

// audio log sound
#define PROP_AUDIO_LOG "AudioLog"

// audio log sound
#define PROP_APPAR_ID "ApparID"

// failure sound (for hack lock)
#define PROP_FAIL_SOUND "FailSound"

// hack lock & key
#define PROP_HACK_LOCK "HackLock"
#define PROP_HACK_KEY "HackKey"
EXTERN BOOL HackLockIsRelevant(ObjID objID);
EXTERN BOOL HackKeyIsRelevant(ObjID objID);
EXTERN int HackLockGet(ObjID objID);
EXTERN int HackKeyGet(ObjID objID);

// audio log sound
#define PROP_BOOK_DATA "BookData"

// Keypad code
#define PROP_KEYPAD_CODE "KeypadCode"
EXTERN IIntProperty *g_pPropKeypadCode;

EXTERN void DPCFrobPropertiesInit(void);

#endif  // __DPCFROB_H