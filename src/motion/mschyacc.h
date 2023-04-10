/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/mschyacc.h,v 1.2 2000/01/29 13:22:19 adurant Exp $
#pragma once

#ifndef __MSCHYACC_H
#define __MSCHYACC_H

EXTERN void MotSchemaYaccParse(char *schemaFile);
EXTERN void MotSchemaYaccCount(char *schemaFile, int *schemaCount, 
                            int *sampleCount, int *sampleCharCount);
#endif


