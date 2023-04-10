/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchglue.h,v 1.3 2000/01/31 10:02:50 adurant Exp $
#pragma once

/* <<--- /-/-/-/-/-/-/-/ <<< (( / (( /\ )) \ )) >>> \-\-\-\-\-\-\-\ --->> *\
   spchglue.h

   The schema file parsing is in C, and the tag database system is in
   C++.  So here's where we reconcile them, by exposing parts of
   speech.cpp which we otherwise would not see.

   SpeechDatabaseNotify, SpeechInit and SpeechTerm also appear here
   since they're called from schloop.c.

\* <<--- \-\-\-\-\-\-\-\ <<< (( \ (( \/ )) / )) >>> /-/-/-/-/-/-/-/ --->> */


#ifndef _SPCHGLUE_H_
#define _SPCHGLUE_H_


#include <objtype.h>
#include <dispatch.h>


// base objects for hierarchy
#define BASE_VOICE_OBJ "Voice"

// app-level flow control
EXTERN void SpeechDatabaseNotify(DispatchData *msg);
EXTERN void SpeechInit(void);
EXTERN void SpeechTerm(void);

EXTERN void SpeechAddConcept(const Label *pConceptName, int iPriority);

EXTERN void SpeechAddTag(const Label *pTagName);
EXTERN void SpeechAddTagValue(const Label *pValueName);

EXTERN void SpeechAddIntTag(const Label *pTagName);

// This should be called after the appropriate archetype has been
// created.  It adds the VoiceIndex property to the object.
EXTERN void SpeechVoiceNew(ObjID VoiceObject);

// These three functions amount to an iterator.
EXTERN void SpeechSchemaNewStart(const Label *pSchemaName,
                                 ObjID SchemaObjID,
                                 const Label *pVoiceName,
                                 int iWeight, const Label *pConceptName);
EXTERN void SpeechSchemaNewAddEnumTag(const Label *pTagName,
                                      const Label *pEnum);
EXTERN void SpeechSchemaNewAddIntTag(const Label *pTagName,
                                     int iMin, int iMax);
EXTERN void SpeechSchemaNewFinish(void);

#endif // ~_SPCHGLUE_H_
