/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schbase.h,v 1.25 2000/03/22 17:56:59 patmac Exp $
// Schema internal type definitions
#pragma once

#ifndef SCHBASE__H
#define SCHBASE__H

#include <objtype.h>
#include <label.h>
#include <matrixs.h>

#define SCHEMAS_MAX     100
#define SAMPLES_MAX     200
#define AVG_SAMPLE_NAME_LEN     10

#define SCH_PLAY_RETRIGGER (1<<0) // Does schema restart when multi-plays?
#define SCH_PAN_POS        (1<<1) // Fixed pan position described
#define SCH_PAN_RANGE      (1<<2) // Fixed pan within range described
#define SCH_NO_REPEAT      (1<<3) // Don't repeat last sample (if possible)

#define SCH_NO_CACHE       (1<<4) // dont cache this, we are unlikely to want it again
#define SCH_STREAM         (1<<5) // stream this off, dont load it all at once
#define SCH_PLAY_ONCE      (1<<6) // This is intended to be played only once.
#define SCH_NO_COMBAT      (1<<7) // This is not intended to be played in combat
#define SCH_NET_AMBIENT    (1<<8) // This ambient should be networked
#define SCH_LOC_SPATIAL    (1<<9) // This spatial sound should be local-only

#define SCH_CLASS_SHIFT    (16)
#define SCH_CLASS_MASK     (0x0F << SCH_CLASS_SHIFT)

typedef struct sSchemaPlayParams
{
   uint flags;
   int volume;        // nominal volume (-10k to -1)
   int pan;           // pan position or range if flag set (-10k to 10k)
   int initialDelay;  // before we start sample
   int fade;          // fade in/out in msec
} sSchemaPlayParams;  

#define SCHEMA_LOOP_POLY 0x01        // polyphonous
#define SCHEMA_LOOP_COUNT 0x02       // only loop n times

typedef struct sSchemaLoopParams
{
   uchar flags;
   uchar maxSamples;
   ushort count;        // number of times to loop
   ushort intervalMin;
   ushort intervalMax;
} sSchemaLoopParams;

typedef void (*SchemaCallback)(int hSchema, ObjID schemaID, void *pData);

// which fields to pay attention to
#define SCH_SET_VOLUME    0x0001 // exlusive with add, overrides it if both
#define SCH_ADD_VOLUME    0x0002
#define SCH_SCALE_VOLUME  0x0004
#define SCH_RADIUS_VOLUME 0x0008
#define SCH_SET_SAMPLE    0x0010
#define SCH_SET_OBJ       0x0020  // exclusive with loc, overrides it if both
#define SCH_SET_LOC       0x0040
#define SCH_SET_CALLBACK  0x0080
#define SCH_SET_MSG_DATA  0x0100
#define SCH_SET_MSG       0x0200
#define SCH_SET_LOOP_CALLBACK 0x0400
#define SCH_SHARP_ATTEN   0x0800
#define SCH_NETWORK       0x1000  // broadcast this ambient sound
#define SCH_NO_NETWORK    0x2000  // don't broadcast this spatial sound
#define SCH_FORCE_PLAY    0x4000

// Parameters provided to schema play functions
typedef struct sSchemaCallParams
{
   int flags;           // which fields are set
   int sampleNum;       // particular sample to play
   ObjID sourceID; 
   mxs_vector *pSourceLoc; 
   int volume;          // override/add to schema volume
   SchemaCallback callback; // callback when schema ends
   SchemaCallback loop_callback;
   void *pData;         // user data for callback
   Label *pMsg;         
   long msgData[3];     // data attached to schema message
} sSchemaCallParams;

// for when you only want to modify one call parameter
extern sSchemaCallParams g_sDefaultSchemaCallParams;

#define SCHEMA_MSG_DATA_SIZE 3 // big enough for an mxs_vector

// schema -> AI messages
typedef struct sSchemaMsg
{
   Label msgType;
   int volume;
   ObjID sourceID; 
   mxs_vector sourceLoc;
   long data[SCHEMA_MSG_DATA_SIZE]; 
} sSchemaMsg;


// these sound schema types must be in the same order as those in schema.y

typedef enum {
   kSchemaTypeNone,           // Originial Thief classes.
   kSchemaTypeNoise,
   kSchemaTypeSpeech,
   kSchemaTypeAmbient,
   kSchemaTypeMusic,
   kSchemaTypeMetaUI,
   kSchemaTypePlayerFeet,     // Additional Shock classes.
   kSchemaTypeOtherFeet,
   kSchemaTypeCollisions,
   kSchemaTypeWeapons,
   kSchemaTypeMonsters
} eSchemaType;

#define kNumSchemaTypes 16

#endif


