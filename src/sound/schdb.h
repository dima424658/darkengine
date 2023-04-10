/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schdb.h,v 1.5 2000/01/29 13:41:48 adurant Exp $
#pragma once

#ifndef __SCHDB_H
#define __SCHDB_H

#include <iobjsys.h>
#include <dispatch.h>

#define BASE_SCHEMA_OBJ "Schema"

// load all speech, archetype and schema files
EXTERN void SchemaFilesLoadAll(void);
// load all schema files matching "what" string
EXTERN void SchemaFilesLoad(const char *what);
EXTERN ObjID SchemaCreate(Label *pSchemaLabel, ObjID arch);
EXTERN void SchemaDatabaseNotify(DispatchData *msg);
EXTERN void SchemasDestroy(void);
EXTERN BOOL ObjIsSchema(ObjID objID);

#endif

