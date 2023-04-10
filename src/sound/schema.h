/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schema.h,v 1.11 2000/01/29 13:41:49 adurant Exp $
#pragma once

#ifndef SCHEMA_H
#define SCHEMA_H

#include <schtype.h>
#include <osystype.h>
#include <matrixs.h>
#include <objpos.h>

// Initialize the schema system
EXTERN void SchemaInit(void);
EXTERN void SchemaShutdown(void);


// In C++, the pData parameters in the playing functions are optional.
#ifdef __cplusplus
#define OPT_DATA(d) = d
#else // __cplusplus
#define OPT_DATA(d)
#endif // __cplusplus


// Play a named schema return handle to schema playing
EXTERN int SchemaPlay(const Label *schemaName, sSchemaCallParams *pParams,
                      void *pData OPT_DATA(NULL));
// Play on an object (dynamic 3d)
EXTERN int SchemaPlayObj(const Label *schemaName, ObjID objID,
                         void *pData OPT_DATA(NULL));
// Play at a location (static 3d)
EXTERN int SchemaPlayLoc(const Label *schemaName, mxs_vector *pLoc,
                         void *pData OPT_DATA(NULL));

// Play at an object's location at time of call (static 3d)
#define SCHEMA_PLAY_OBJ_LOC(pSchemaName, objID) \
   SchemaPlayLoc(pSchemaName, &(ObjPosGet(objID)->loc.vec), NULL)

// Play a schema by ID
EXTERN int SchemaIDPlay(ObjID schemaID, sSchemaCallParams *pParams,
                        void *pData OPT_DATA(NULL));
EXTERN int SchemaIDPlayObj(ObjID schemaID, ObjID objID,
                           void *pData OPT_DATA(NULL));
EXTERN int SchemaIDPlayLoc(ObjID schemaID, mxs_vector *pLoc,
                           void *pData OPT_DATA(NULL));

#define SCHEMA_ID_PLAY_OBJ_LOC(schemaID, objID) \
   SchemaIDPlayLoc(schemaID, &(ObjPosGet(objID)->loc.vec), NULL)

EXTERN void SchemaPlayHalt(int hSchemaPlay);
EXTERN void SchemaHaltAll(void);

// Get a schema ID from the schema handle
EXTERN ObjID SchemaGetIDFromHandle(int hSchemaPlay);

// Get an appsfx handle from the schema handle
EXTERN int SchemaGetSFXFromHandle(int hSchemaPlay, int iWhich);

EXTERN void SchemaFrame(void);

// why is this here? where should it be?
EXTERN double SchemaRand(void);

#endif

