/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/spchbase.h,v 1.3 2000/01/31 10:02:48 adurant Exp $
#pragma once

#ifndef __SPCHBASE_H
#define __SPCHBASE_H

// speech flags
#define SPEECH_SPEAKING 0x0001 // AI is currently speaking

// Attached to an AI, indicates current or last speech schema status
typedef struct sSpeech
{
   int flags;   
   long time;       // time at which schema started/stopped
   ObjID schemaID;  // schema we're speaking or last spoke
   int m_iConceptIndex; // within domain
   int m_iPriority; // of our current concept
   int hSchemaPlay; // handle to schema playing
} sSpeech;

#endif
