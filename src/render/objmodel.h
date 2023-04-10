/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objmodel.h,v 1.14 1998/11/02 04:44:47 dc Exp $

#pragma once

#ifndef __OBJMODEL_H
#define __OBJMODEL_H

#include <r3ds.h>
#include <objtype.h> // sadly, i have failed in keeping Obj's out of this, i suck - dc

#define MAX_OBJMODELS 1024 // was 512, changed for e3 98 - was 2048 changed 10/3 dc

// ref counting for model loading
EXTERN void  objmodelIncRef(int idx);
EXTERN void  objmodelDecRef(int idx);

// set up models
EXTERN void  objmodelInit(void);
EXTERN void  objmodelShutdown(void);
EXTERN void  objmodelReset(void);

// for command system
#ifdef EDITOR
EXTERN void  objmodel_command(char *objName);
EXTERN void  objmodel_set_debug_tex(int type);
#endif

#if defined(REPORT) || !defined(SHIP)
EXTERN void objmodelGetSize(int idx, int *model_mem, int *text_mem);
EXTERN void objmodelListMemory(char **rep_str, BOOL loud);
#endif

// load and free an object
EXTERN int   objmodelLoadModel(char *objName);
// really, you should only call this one if you really know what you are doing
EXTERN int   objmodelLoadModelIntoHandle(char *objName, int handle);
// freeing models
EXTERN void  objmodelFreeAllModels(void);


#define MIDX_INVALID -1

// get an obj idx for a name
EXTERN int   objmodelGetIdx(char *objName);
EXTERN char *objmodelGetName(int idx);

// get maximum model index
EXTERN int   objmodelMaxModel(void);

// returns a pointer to the model.  user needs to know how to cast it
EXTERN void *objmodelGetModel(int idx);

// XXX put these #defines in a different header

#define OM_TYPE_DEFAULT    0
#define OM_TYPE_MD         1
#define OM_TYPE_MESH       2
#define OM_TYPE_HEAT       3
#define OM_TYPE_SPARK      4
#define OM_TYPE_PARTICLES  5
#define OM_TYPE_BITMAP     6
#define OM_NUM_TYPES       7

#define OM_TYPE_INVALID (OM_NUM_TYPES)

// get model rendering type
// always returns a valid type
EXTERN int   objmodelGetModelType(int idx);
// these, sadly, need Obj now, since it holds the replace properties
EXTERN void  objmodelSetupMdTextures(ObjID obj, void *model, int idx);
EXTERN void  objmodelSetupMeshTextures(ObjID obj, void *model, int idx);

// You should Release when you are done with the texture from Setup:
EXTERN r3s_texture objmodelSetupBitmapTexture(int idx, int frame);
EXTERN void  objmodelReleaseBitmapTexture(int idx, int frame);
EXTERN int   objmodelBitmapNumFrames(int idx);

// detail level control
EXTERN BOOL g_highpoly_characters; // so we can tell/look in options
EXTERN void objmodelDetailSetCommand(BOOL new_mode);
EXTERN void objmodelSwitchHighPolyDetail(BOOL target_high, BOOL mesh_only);

#endif   // __OBJMODEL_H
