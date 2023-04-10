/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/tag.h,v 1.3 2000/01/31 09:48:43 adurant Exp $
#pragma once

#ifndef __TAG_H
#define __TAG_H

#include <objtype.h>
#include <osystype.h>
#include <tagfile.h>
#include <tagtype.h>

// Get a pointer to a tag if it exists
EXTERN BOOL TagGet(char *pName, sTag **ppTag);

// Define a tag - return TRUE if already defined or success
// set pointer to new tag if success
EXTERN BOOL TagDefine(eTagType type, char *name, sTag **ppTag);
// Set a tag given a pointer
EXTERN void TagIntSet(sTag *pTag, int value);
EXTERN void TagFloatSet(sTag *pTag, float value);

// Delete all tags
EXTERN void TagsClear();

// Add a tag constraint
EXTERN BOOL TagConstraintAdd(ObjID schemaID, eTagType type, const char *pName, 
                             eTagOp op, long value);
// Remove all tag constraints for a particular schema
EXTERN void TagConstraintsRemove(ObjID schemaID);
// remove all tag constraints from all objects
EXTERN void TagConstraintsClear();

// Evaluate all tag constraints for a schema, return fraction true
EXTERN float TagConstraintsEvaluate(ObjID schemaID);

EXTERN void TagConstraintsRead(ITagFile *pFile);
EXTERN void TagConstraintsWrite(ITagFile *pFile);

#ifndef SHIP
EXTERN void SetupTagCommands();
#endif

#endif
