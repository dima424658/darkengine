/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/editor/linkdraw.h,v 1.4 2000/01/29 13:12:23 adurant Exp $
//
// Link-drawing system
//
#pragma once

#ifndef __LINKDRAW_H
#define __LINKDRAW_H

#include <objtype.h>
#include <linktype.h>

EXTERN void InitDrawnRelations();
EXTERN void TermDrawnRelations();
EXTERN void ResetDrawnRelations();

EXTERN void AddDrawnRelation(const char *name, ubyte r, ubyte g, ubyte b);
EXTERN void AddDrawnRelationID(RelationID relationID, ubyte r, ubyte g, ubyte b);
EXTERN void RemoveDrawnRelation(const char *name);
EXTERN void RemoveDrawnRelationID(RelationID relationID);

EXTERN void DrawRelations();
EXTERN BOOL ObjHasDrawnLinks(ObjID objID);

#endif // __LINKDRAW_H
