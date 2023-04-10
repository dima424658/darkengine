/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/schyacc.h,v 1.2 2000/01/31 10:01:37 adurant Exp $
#pragma once

#ifndef __SCHYACC_H
#define __SCHYACC_H

EXTERN void SchemaYaccParse(char *schemaFile);
EXTERN void SchemaYaccCount(char *schemaFile, int *schemaCount, 
                            int *sampleCount, int *sampleCharCount);
#endif
