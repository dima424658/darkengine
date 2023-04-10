/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schsamps.h,v 1.5 2000/01/29 13:41:56 adurant Exp $
// Schema sample name storage
#pragma once

#ifndef SCHSAMPS_H
#define SCHSAMPS_H

#include <objtype.h>
#include <tagfile.h>

#define SAMPLE_DEFAULT_FREQ 1

// Create the samples for a schema
EXTERN BOOL SchemaSamplesCreate(ObjID objID, int num, 
                                const char **pSampleName, 
                                const uchar *freq);
// Get the nth sample for a schema
EXTERN const char *SchemaSampleGet(ObjID objID, int num);
// Get the index of the sample that matches this string
EXTERN int SchemaSampleGetNamed(ObjID objID, const char *name);
// Get array of frequencies
EXTERN const uchar *SchemaFreqsGet(ObjID objID);
// Get number of samples
EXTERN int SchemaSamplesNum(ObjID objID);

// Destroy samples for a particular schema
EXTERN void SchemaSamplesDestroy(ObjID objID);

EXTERN void SchemaSamplesShutdown();
EXTERN void SchemaSamplesInit();
// Destroy all schema samples stored
EXTERN void SchemaSamplesClear();

EXTERN void SchemaSamplesRead(ITagFile *pFile);
EXTERN void SchemaSamplesWrite(ITagFile *pFile);

#endif




