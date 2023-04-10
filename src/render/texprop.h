/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/texprop.h,v 1.7 2000/01/31 09:53:37 adurant Exp $
// silly texture property config file craziness for EEE
// this is _not_ shipping code
#pragma once

#ifndef __TEXPROP_H
#define __TEXPROP_H

////////////////////////////////
// PLEASE SOMEONE DEAR GOD REWRITE THIS USING PROPERTIES!!!
////////////////////////////////

#include <texmem.h>

// load generic numbered terrain properties
EXTERN void terrainprop_load(void);

// for talking to texture properties and terrain schemas
typedef struct {
   char foot_terr;       // player footstep on terrain
   char coll_terr;       // collision of object with terrain
   char land_terr;       // sound of landing from jump on terrain
   char monster_terr[5]; // ai guard footstep on terrain
} texture_properties;

EXTERN texture_properties txtprop_list[TEXMEM_MAX];

#define MAX_TERRPROP     64
// we know these are really ObjIDs for now, but as ints they are find generic handles
EXTERN int terr_schema_ids[MAX_TERRPROP];  // swizzled schema values for playback

// for actually getting the data
#define TerrGetCollisionSchema(terr_type)   terr_schema_ids[txtprop_list[terr_type].coll_terr]
#define TerrGetFootStepSchema(terr_type)    terr_schema_ids[txtprop_list[terr_type].foot_terr]
#define TerrGetLandingSchema(terr_type)     terr_schema_ids[txtprop_list[terr_type].land_terr]

#define TerrGetAIGuardStepSchema(terr_type)      terr_schema_ids[txtprop_list[terr_type].monster_terr[0]]
#define TerrGetBurrickStepSchema(terr_type)      terr_schema_ids[txtprop_list[terr_type].monster_terr[1]]
#define TerrGetHauntStepSchema(terr_type)        terr_schema_ids[txtprop_list[terr_type].monster_terr[2]]
#define TerrGetMonsterNumStepSchema(terr_type,n) terr_schema_ids[txtprop_list[terr_type].monster_terr[n]]

// silly self lit hack for now
EXTERN BOOL terr_self_lit[TEXMEM_MAX];

#endif
