/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrep.h,v 1.10 2000/01/31 09:59:01 adurant Exp $
#pragma once

#ifndef __SHKREP_H
#define __SHKREP_H

extern "C"
{
#include <event.h>
}
#include <shklabel.h>
#include <propface.h>
#include <objsys.h>

EXTERN void ShockRepInit(int which);
EXTERN void ShockRepTerm(void);
EXTERN bool ShockRepHandleMouse(Point p);
EXTERN bool ShockReplicate(Point pt);
EXTERN void ShockRepDraw(void);
EXTERN void ShockRepStateChange(int which);
EXTERN void ShockRepSetup(ObjID rep);
EXTERN ObjID g_repID;

// Replicator Contents Property

#define NUM_REPLICATOR_CONTENTS  6

// The actual structure
typedef struct sRepContents
{
   char m_objs[NUM_REPLICATOR_CONTENTS][64];
   int m_costs[NUM_REPLICATOR_CONTENTS];
} sRepContents;

// Property stuff

F_DECLARE_INTERFACE(IRepContentsProperty);

#undef INTERFACE
#define INTERFACE IRepContentsProperty

DECLARE_PROPERTY_INTERFACE(IRepContentsProperty)
{
   DECLARE_UNKNOWN_PURE();                // IUnknown methods 
   DECLARE_PROPERTY_PURE();               // IProperty methods
   DECLARE_PROPERTY_ACCESSORS(sRepContents*);  // Type-specific accessors, by reference
};

#define PROP_REPCONTENTS "RepContents"
#define PROP_REPCONTENTSHACK "RepHacked"

EXTERN IRepContentsProperty *g_RepContentsProperty;
EXTERN IRepContentsProperty *g_RepContentsHackedProperty;

EXTERN void RepContentsPropertyInit();
EXTERN BOOL RepContentsGet(ObjID objID, sRepContents **ppRepContents);

EXTERN bool RepContentsGetObj(ObjID objID, int slot, char *name);
EXTERN int RepContentsGetCost(ObjID objID, int slot);

#endif